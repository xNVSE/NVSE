#include "Hooks_CreatedObject.h"
#include "Core_Serialization.h"
#include "Utilities.h"
#include "GameAPI.h"
#include "GameForms.h"

static std::string sDirectory;
static std::string sName;

void SplitSavePath(const char* SavePath)
{

	std::string aTmp = std::string(SavePath);
	// truncate at last slash
	std::string::size_type	lastSlash = aTmp.rfind('\\');
	if(lastSlash != std::string::npos)	// if we don't find a slash something is VERY WRONG
	{
		sDirectory = aTmp.substr(0, lastSlash +1);
		sName = aTmp.substr(lastSlash + 1);
	}
	else
		{
			sDirectory.clear();
			sName.clear();
		}
}

static FakeModInfo* g_FakeModInfo = NULL;
static ModInfo** s_refModInfo = NULL;

FakeModInfo::FakeModInfo(NVSESerializationInterface* _intfc)
{
	memset(this, 0, sizeof(FakeModInfo));
	intfc = _intfc;
	modIndex = 0xFF;

	DataHandler* dataHand = DataHandler::Get();

	// refModInfo is normally used to fix up formIDs as saved in GECK when referencing forms from other mods
	// for created objects, formIDs represent run-time formIDs, must be fixed up if load order changes

	//	func at 0x00471870 for the Runtime gore, from refModNames

	if (s_refModInfo) {
		FormHeap_Free(refModInfo);
		s_refModInfo = NULL;
	}
	if (s_numPreloadMods) {
		numRefMods = s_numPreloadMods;
		size_t arraySize = numRefMods * sizeof(ModInfo*);
		s_refModInfo = (ModInfo**)FormHeap_Allocate(arraySize);
		if (s_refModInfo) {
			memset(s_refModInfo, 0, arraySize);
			for (SInt8 i=0; i<s_numPreloadMods; i++) {
				s_refModInfo[i] = dataHand->modList.modInfoList.GetNthItem(s_preloadModRefIDs[i]);
			}
		}
	}
	refModInfo = s_refModInfo;

	// name, filepath, fileoffset, dataoffset
	SplitSavePath(intfc->GetSavePath());
	strcpy_s(name, sName.c_str());
	strcpy_s(filepath, sDirectory.c_str());
}

FakeModInfo::~FakeModInfo()
{
	if (s_refModInfo) {
		FormHeap_Free(refModInfo);
		s_refModInfo = NULL;
	}
}

FakeModInfo* FakeModInfo::Get() {
	if (!g_FakeModInfo)
		g_FakeModInfo = new FakeModInfo(&g_NVSESerializationInterface);
	return g_FakeModInfo;
}

UInt32 FakeModInfo::SetStaticFieldsAndGetFormTypeEnum(UInt32 aChunkType) {
	if (*s_ModInfo_CurrentChunkTypeCode == aChunkType)
		return *s_ModInfo_CurrentFormTypeEnum;
	for (UInt32 i = 0; i < 0x79; i++) {
		if (s_ModInfo_ChunkAndFormTypes[i].chunkType == aChunkType) {
			*s_ModInfo_CurrentChunkTypeCode = aChunkType;
			*s_ModInfo_CurrentFormTypeEnum = i;
			return i;
		}
	}
	return 0;
};

void SaveCreatedObjects(NVSESerializationInterface* nvse)
{
	TESSaveLoadGame* game = TESSaveLoadGame::Get();
	for (TESSaveLoadGame::CreatedObject* crobj = &game->createdObjectList; crobj; crobj = crobj->next)
	{
		if (crobj && crobj->refID)
			if ((crobj->refID & 0xFF000000) == 0xFF000000)
			{
				TESForm* form = LookupFormByID(crobj->refID);
				if (!form)
				{
					_MESSAGE("SAVE: Unkown Object %08x found in created base object list", crobj->refID);
					continue;
				}
				else
				{
					form->SaveForm();
					nvse->OpenRecord('CROB', 0);
					nvse->WriteRecordData(*g_CreatedObjectData, *g_CreatedObjectSize);
				}
			}
			else
				_MESSAGE("Save: Non-created object or garbage refID %08x found in created base object list.", crobj->refID);
	}
}

void fakeModInfo_GetNextChunk(void) {
	FakeModInfo* fakeModInfo = FakeModInfo::Get();
	ChunkHeader* current = (ChunkHeader*)((UInt32)(fakeModInfo->dataBuf)+fakeModInfo->dataOffset);
	while (current && ('XXXX' == current->type)) {
		// Skip this SubRecord
		fakeModInfo->dataOffset += (current->size+sizeof(ChunkHeader));
		if (fakeModInfo->dataOffset >= fakeModInfo->formInfo.dataSize)
			current = NULL;
		else
			current = (ChunkHeader*)((UInt32)(fakeModInfo->dataBuf)+fakeModInfo->dataOffset);
	};
	if (current) {
		fakeModInfo->subRecordHeader.type = current->type;
		fakeModInfo->subRecordHeader.size = current->size;
	}
};

void __declspec( naked ) Hook_fakeModInfo_GetNextChunk(void) {
	__asm {
		pusha
		call fakeModInfo_GetNextChunk
		popa
	}
};

bool LoadCreatedObject(NVSESerializationInterface* nvse)
{
	//stub
	FakeModInfo* fakeModInfo = FakeModInfo::Get();
	DataHandler* dataHandler =DataHandler::Get();

	nvse->PeekRecordData(&(fakeModInfo->formInfo), sizeof(fakeModInfo->formInfo));
	if (IsBigEndian()) {
		MACRO_SWAP32(fakeModInfo->formInfo.recordType);
		MACRO_SWAP32(fakeModInfo->formInfo.dataSize);
		MACRO_SWAP32(fakeModInfo->formInfo.formFlags);
		MACRO_SWAP32(fakeModInfo->formInfo.formID);
		MACRO_SWAP32(fakeModInfo->formInfo.unk10);
		MACRO_SWAP16(fakeModInfo->formInfo.formVersion);
		MACRO_SWAP16(fakeModInfo->formInfo.unk16);
	}
	fakeModInfo->SetStaticFieldsAndGetFormTypeEnum(fakeModInfo->formInfo.recordType);

	bool result = false;
	UInt32 RecordSize = fakeModInfo->formInfo.dataSize + sizeof(fakeModInfo->formInfo);
	void* formData = FormHeap_Allocate(RecordSize);
	if (formData)
		__try {
			if (nvse->ReadRecordData(formData, RecordSize) == RecordSize)
				__try {
					fakeModInfo->dataBuf = (UInt8*)formData + sizeof(fakeModInfo->formInfo);
					fakeModInfo->dataBufSize = fakeModInfo->formInfo.dataSize;
					fakeModInfo->dataOffset = 0;
					fakeModInfo->fileOffset = 0;
					void** formDataAddr = &(fakeModInfo->dataBuf);
					g_CreatedObjectSize = &(fakeModInfo->dataBufSize);
					g_CreatedObjectData = (UInt8**)formDataAddr;

					TESForm* form = CreateFormInstance(*s_ModInfo_CurrentFormTypeEnum);
					if (!form) {
						_MESSAGE("LOAD: Unkown Object type %08x found in created objects cosave", s_ModInfo_CurrentFormTypeEnum);
					}
					else {
						UInt32 bWasCompressed = fakeModInfo->formInfo.formFlags & TESForm::kFormFlags_Compressed;
						UInt8 bSaveUnk61A = dataHandler->unk61A & 1;
						bool formReady = false;
						__try {
							dataHandler->unk61A |= 1;
							fakeModInfo->formInfo.formFlags |= TESForm::kFormFlags_Compressed;	// avoids calling TESFile.read in GetNextChunk...
							form->LoadForm(fakeModInfo);
							form->InitItem();
							formReady = true;
						} __finally {
							if (!bWasCompressed)
								fakeModInfo->formInfo.formFlags &= !TESForm::kFormFlags_Compressed;	// restores
							if (!bSaveUnk61A) 
								dataHandler->unk61A &= 0xFFFFFFFE;
						}
						if (formReady)
							form->DoAddForm(form, true, false);
					}
			} __finally {
					fakeModInfo->dataBuf = NULL;
					fakeModInfo->dataOffset = 0;
					g_CreatedObjectSize = NULL;
					g_CreatedObjectData = NULL;
					result = true;
			}
		} __finally {
			FormHeap_Free(formData);
		}

	return result;
}
