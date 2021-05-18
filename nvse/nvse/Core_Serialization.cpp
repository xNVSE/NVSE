#include "Serialization.h"
#include "Core_Serialization.h"
#include "GameData.h"
#include "Hooks_CreatedObject.h"
#include <string>
#include "StringVar.h"
#include "ArrayVar.h"
#include "ScriptTokens.h"

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

static ModInfo** s_ModFixupTable = NULL;
bool LoadModList(NVSESerializationInterface* nvse);	// reads saved mod order, builds table mapping changed mod indexes

/*******************************
*	Callbacks
*******************************/
void Core_SaveCallback(void * reserved)
{
	NVSESerializationInterface* intfc = &g_NVSESerializationInterface;
	DataHandler* dhand = DataHandler::Get();
	ModInfo** mods = dhand->modList.loadedMods;
	UInt8 modCount = dhand->modList.loadedModCount;

	// save the mod list
	intfc->OpenRecord('MODS', 0);
	intfc->WriteRecordData(&modCount, sizeof(modCount));
	for (UInt32 i = 0; i < modCount; i++)
	{
		UInt16 nameLen = strlen(mods[i]->name);
		intfc->WriteRecordData(&nameLen, sizeof(nameLen));
		intfc->WriteRecordData(mods[i]->name, nameLen);
	}

#ifdef DEBUG
	SaveCreatedObjects(intfc);
#endif
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
	// reset mod indexes to match current load order
	if (s_ModFixupTable)
	{
		delete s_ModFixupTable;
		s_ModFixupTable = NULL;
	}

	DataHandler* dhand = DataHandler::Get();
	UInt8 modCount = dhand->modList.loadedModCount;
	ModInfo** mods = dhand->modList.loadedMods;

	s_ModFixupTable = new ModInfo*[modCount];
	for (UInt32 i = 0; i < modCount; i++)
		s_ModFixupTable[i] = mods[i];

	g_ArrayMap.Clean();
	g_StringMap.Clean();
}

void Core_PreLoadCallback(void * reserved)
{
	// this is invoked only if at least one other plugin registers a preload callback
	
	// reset refID fixup table. if save made prior to 0019, this will remain empty
	s_numPreloadMods = 0;	// no need to zero out table - unloaded mods will be set to 0xFF below

	NVSESerializationInterface* intfc = &g_NVSESerializationInterface;

	g_gcCriticalSection.Enter();
	g_nvseVarGarbageCollectionMap.Clear();
	g_gcCriticalSection.Leave();
	
	g_ArrayMap.Reset();
	g_StringMap.Reset();

	UInt32 type, version, length;

	while (intfc->GetNextRecordInfo(&type, &version, &length)) {
		switch (type) {
			case 'MODS':
				if (!LoadModList(intfc))
					_MESSAGE("PRELOAD: Error occurred while loading mod list");
				break;
#ifdef _DEBUG
			case 'CROB':
				LoadCreatedObject(intfc);	// if this fails it is handled internally so no need to check return val
				break;
#endif
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
}


void Init_CoreSerialization_Callbacks()
{
	Serialization::InternalSetSaveCallback(0, Core_SaveCallback);
	Serialization::InternalSetLoadCallback(0, Core_LoadCallback);
	Serialization::InternalSetNewGameCallback(0, Core_NewGameCallback);
	Serialization::InternalSetPreLoadCallback(0, Core_PreLoadCallback);
}

UInt8	s_preloadModRefIDs[0xFF];
UInt8	s_numPreloadMods = 0;

#if _DEBUG
std::vector<std::string> g_modsLoaded;
#endif
bool ReadModListFromCoSave(NVSESerializationInterface * intfc)
{
	_MESSAGE("Reading mod list from co-save");

	char name[0x104];
	UInt16 nameLen = 0;

	intfc->ReadRecordData(&s_numPreloadMods, sizeof(s_numPreloadMods));
	for (UInt32 i = 0; i < s_numPreloadMods; i++) {
		intfc->ReadRecordData(&nameLen, sizeof(nameLen));
		intfc->ReadRecordData(&name, nameLen);
		name[nameLen] = 0;
#if _DEBUG
		g_modsLoaded.emplace_back(name);
#endif
		s_preloadModRefIDs[i] = DataHandler::Get()->GetModIndex(name);
	}
	return true;
}

bool LoadModList(NVSESerializationInterface* intfc)
{
	// read the mod list
	return ReadModListFromCoSave(intfc);
}


UInt8 ResolveModIndexForPreload(UInt8 modIndexIn)
{
	return (modIndexIn < s_numPreloadMods) ? s_preloadModRefIDs[modIndexIn] : 0xFF;
}

