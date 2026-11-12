#include "Serialization.h"
#include "Core_Serialization.h"
#include "GameData.h"
#include "Hooks_CreatedObject.h"
#include <string>
#include "StringVar.h"
#include "ArrayVar.h"
#include "Commands_Script.h"
#include "EventManager.h"
#include "ScriptTokens.h"
#include "InventoryReference.h"

std::vector<const class ModInfo*> g_saveModList[ModListType::kCount];
std::vector<std::string> g_saveModListNames[ModListType::kCount];

/*************************
Save file format:
	Header
		MODS				// stored in order of modIndex when game was saved
			UInt8 numMods
				UInt16	nameLen
				char	modName[nameLen]
		CROB				// a created base object
			void * objectData		// recorded by TESForm::SaveForm() in mod file format


**************************/					

constexpr UInt32 kNewPluginTypesVersion = 1;

namespace {

	inline void __fastcall SaveMod(NVSESerializationInterface* intfc, const ModInfo* mod) noexcept {
		const UInt16 nameLen = strlen(mod->name);
		intfc->WriteRecordData(&nameLen, sizeof(nameLen));
		intfc->WriteRecordData(mod->name, nameLen);
	}

	inline void __fastcall LoadMod(NVSESerializationInterface* intfc, char* name) noexcept {
		UInt16 nameLen = 0;
		intfc->ReadRecordData(&nameLen, sizeof(nameLen));
		intfc->ReadRecordData(name, nameLen);
		name[nameLen] = 0;
	}

	template<ModListType type>
	inline bool __fastcall IsValidMod(const ModInfo* mod) noexcept {
		if (mod->IsLoaded()) [[likely]] {
			switch (type) {
				case ModListType::kNormal:
					return DataHandler::HasNewFileTypeSupport() ? !mod->IsSpecial() : true;
				case ModListType::kSmall:
					return mod->IsSmall();
				case ModListType::kMedium:
					return mod->IsMedium();
				case ModListType::kOverlay:
					return mod->IsOverlay();
				default:
					__assume(false);
			}
		}
		return false;
	}

	template<ModListType type>
	inline const ModInfo* __fastcall FindLoadedMod(const char* modName) noexcept {
		const ModInfo* mod = DataHandler::Get()->GetModByName(modName);
		if (mod && IsValidMod<type>(mod)) [[likely]]
			return mod;
		return nullptr;
	}

	template<ModListType type>
	inline bool __fastcall ResolveMod(const char* modName, UInt32 index) noexcept {
		const ModInfo* mod = FindLoadedMod<type>(modName);
		g_saveModList[type][index] = mod;
		g_saveModListNames[type][index] = modName;
		return mod != nullptr;
	}

	template<ModListType type>
	inline void __fastcall SetModArraysSize(UInt32 size) noexcept {
		g_saveModList[type].resize(size);
		g_saveModListNames[type].resize(size);
	}

	bool __fastcall ReadModListFromCoSave(NVSESerializationInterface* intfc, UInt32 version) noexcept {
		_MESSAGE("Reading mod list from co-save");

		char name[MAX_PATH] = {};
		UInt8 normalModCount = 0;
		intfc->ReadRecordData(&normalModCount, sizeof(normalModCount));
		SetModArraysSize<ModListType::kNormal>(normalModCount);
		for (UInt32 i = 0; i < normalModCount; i++) {
			LoadMod(intfc, name);

			if (!ResolveMod<ModListType::kNormal>(name, i))
				_MESSAGE("PRELOAD: Mod %s not found in mod list", name);
		}

		if (version >= kNewPluginTypesVersion) {
			UInt16 smallModCount = 0;
			intfc->ReadRecordData(&smallModCount, sizeof(smallModCount));
			SetModArraysSize<ModListType::kSmall>(smallModCount);
			for (UInt32 i = 0; i < smallModCount; i++) {
				LoadMod(intfc, name);

				if (!ResolveMod<ModListType::kSmall>(name, i))
					_MESSAGE("PRELOAD: Small mod %s not found in mod list", name);
			}

			UInt8 mediumModCount = 0;
			intfc->ReadRecordData(&mediumModCount, sizeof(mediumModCount));
			SetModArraysSize<ModListType::kMedium>(mediumModCount);
			for (UInt32 i = 0; i < mediumModCount; i++) {
				LoadMod(intfc, name);

				if (!ResolveMod<ModListType::kMedium>(name, i))
					_MESSAGE("PRELOAD: Medium mod %s not found in mod list", name);
			}

			UInt16 overlayModCount = 0;
			intfc->ReadRecordData(&overlayModCount, sizeof(overlayModCount));
			SetModArraysSize<ModListType::kOverlay>(overlayModCount);
			for (UInt32 i = 0; i < overlayModCount; i++) {
				LoadMod(intfc, name);

				if (!ResolveMod<ModListType::kOverlay>(name, i))
					_MESSAGE("PRELOAD: Overlay mod %s not found in mod list", name);
			}
		}

		return true;
	};

	bool __fastcall WriteModListToCoSave(NVSESerializationInterface* intfc, UInt32 version) noexcept {
		const DataHandler* dhand = DataHandler::Get();
		const UInt8 normalModCount = dhand->GetNormalModCount();

		intfc->WriteRecordData(&normalModCount, sizeof(normalModCount));
		for (UInt32 i = 0; i < normalModCount; i++) {
			SaveMod(intfc, dhand->GetNormalMod(i));
		}

		if (version >= kNewPluginTypesVersion) {
			const UInt16 smallModCount = dhand->GetSmallModCount();
			intfc->WriteRecordData(&smallModCount, sizeof(smallModCount));
			for (UInt32 i = 0; i < smallModCount; i++) {
				SaveMod(intfc, dhand->GetSmallMod(i));
			}

			const UInt8 mediumModCount = dhand->GetMediumModCount();
			intfc->WriteRecordData(&mediumModCount, sizeof(mediumModCount));
			for (UInt32 i = 0; i < mediumModCount; i++) {
				SaveMod(intfc, dhand->GetMediumMod(i));
			}

			const UInt16 overlayModCount = dhand->GetOverlayModCount();
			intfc->WriteRecordData(&overlayModCount, sizeof(overlayModCount));
			for (UInt32 i = 0; i < overlayModCount; i++) {
				SaveMod(intfc, dhand->GetOverlayMod(i));
			}
		}

		return true;
	};

}

bool __fastcall LoadModList(NVSESerializationInterface* intfc, UInt32 version) {
	return ReadModListFromCoSave(intfc, version);
}

bool __fastcall SaveModList(NVSESerializationInterface* intfc) {
	UInt32 version = kNewPluginTypesVersion;
	intfc->OpenRecord('MODS', version);
	return WriteModListToCoSave(intfc, version);
}

/*******************************
*	Callbacks
*******************************/
void Core_SaveCallback(void * reserved)
{
	NVSESerializationInterface* intfc = &g_NVSESerializationInterface;
	SaveModList(intfc);

	// prevent arrays from being saved that were supposed to be deleted but are held due to potential future callbacks
	LambdaManager::ClearSavedDeletedEventLists();

	g_ArrayMap.Save(intfc);
	g_StringMap.Save(intfc);

}

void Core_LoadCallback(void * reserved)
{
	NVSESerializationInterface* intfc = &g_NVSESerializationInterface;
	UInt32 type, version, length;

	while (intfc->GetNextRecordInfo(&type, &version, &length))
	{
		Serialization::ignoreNextChunk = false;
		switch (type)
		{
		case 'STVS':
		case 'STVR':
		case 'STVE':
		case 'ARVS':
		case 'ARVR':
		case 'ARVE':
		case 'CROB':
		case 'MODS':
			Serialization::ignoreNextChunk = true;
			break;		// processed during preload
		default:
			_MESSAGE("LOAD: Unhandled chunk type in LoadCallback: %08x", type);
			continue;
		}
	}
}

void Core_NewGameCallback(void * reserved)
{
	DataHandler* dhand = DataHandler::Get();
	UInt8 modCount = dhand->GetNormalModCount();

	g_ArrayMap.Clean();
	g_StringMap.Clean();
	LambdaManager::ClearSavedDeletedEventLists();
}

void Core_PreLoadCallback(void * reserved)
{
	// this is invoked only if at least one other plugin registers a preload callback
	
	// reset refID fixup table. if save made prior to 0019, this will remain empty
	for (auto& list : g_saveModList)
		list.clear();

	for (auto& list : g_saveModListNames)
		list.clear();

	NVSESerializationInterface* intfc = &g_NVSESerializationInterface;

	g_gcCriticalSection.Enter();
	g_nvseVarGarbageCollectionMap.Clear();
	g_gcCriticalSection.Leave();

	ClearDelayedCalls();
	EventManager::ClearFlushOnLoadEventHandlers();
	TogglePlayerControlsAlt::ResetOnLoad();
	
	// if any temporary references to inventory objects exist, clean them up
	{
		ScopedLock lock(s_invRefMapCS);
		if (!s_invRefMap.Empty()) {
			s_invRefMap.Clear();
		}
	}

	g_ArrayMap.Reset();
	g_StringMap.Reset();

	UInt32 type, version, length;

	while (intfc->GetNextRecordInfo(&type, &version, &length)) {
		switch (type) {
			case 'MODS':
				if (!LoadModList(intfc, version))
					_MESSAGE("PRELOAD: Error occurred while loading mod list");
				break;
			case 'STVS':
				g_StringMap.Load(intfc);
				break;
			case 'ARVS':
				g_ArrayMap.Load(intfc);
				break;
			default:
				break;
		}
	}
	LambdaManager::ClearSavedDeletedEventLists();
}

void Init_CoreSerialization_Callbacks()
{
	Serialization::InternalSetSaveCallback(0, Core_SaveCallback);
	Serialization::InternalSetLoadCallback(0, Core_LoadCallback);
	Serialization::InternalSetNewGameCallback(0, Core_NewGameCallback);
	Serialization::InternalSetPreLoadCallback(0, Core_PreLoadCallback);
}