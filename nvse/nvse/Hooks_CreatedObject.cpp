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

#if 0
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
				s_refModInfo[i] = dataHand->modInfoList.GetNthItem(s_preloadModRefIDs[i]);
			}
		}
	}
	refModInfo = s_refModInfo;
#endif
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