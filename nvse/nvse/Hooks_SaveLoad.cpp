#include "SafeWrite.h"
#include "Hooks_SaveLoad.h"
#include "PluginAPI.h"
#include "PluginManager.h"
#include "Serialization.h"
#include "utilities.h"
#include "Core_Serialization.h"

#if RUNTIME
#include "EventManager.h"

bool g_gameLoaded = false;
bool g_gameStarted = false;	// remains true as long as a game is loaded. TBD: Should be cleared when exiting to MainMenu.
static const char* LoadGameMessage = "---Finished loading game: %s";
static const char* LoadingGameMessage = "***Loading game: %s (%s)";
static const char* SaveGameMessage = "---Finished saving game: %s";

static const UInt32 kLoadGamePatchAddr =		0x00848C3C;
static const UInt32 kLoadGameRetnAddr =			0x00848C41;

static const UInt32 kSaveGamePatchAddr =		0x00847C45;		// push SaveGameMessage
static const UInt32 kSaveGameRetnAddr =			0x00847C4A;

static const UInt32 kNewGamePatchAddr =			0x007D33CE;		// overwrite call to doesNothing 7 call above reference to aQuestsUnableTo
static const UInt32 kDeleteGamePatchAddr =		0x00850398;		// DeleteFile() call	// third call to DeleteFileA, like FO3
static const UInt32 kRenameGamePatchAddr =		0x0085762B;		// call to rename()		//

static const UInt32 kPreLoadGamePatchAddr =		0x00847ECC;
static const UInt32 kPreLoadGameRetnAddr =		0x00847ED1;
static const UInt32 kPostLoadGamePatchAddr =	0x00848C83;
static const UInt32 kPostLoadGameRetnAddr =		0x00848C89;
static const UInt32 kPostLoadGameFinishedAddr = 0x00848C91;	// exit point for TESSaveLoadGame::LoadGame()

/*
	When loading a saved game, scripts attached to references are executed as soon as the refr
	is loaded. This can cause CTD in mods which use token scripts as ref vars may point to unloaded references.
	It can also cause errors with array/string variable access as the variable data has not yet been loaded
	From obse 0018 through 0019 beta 2, script execution was disabled during Load to prevent these problems,
	but some mods rely on execution during load in order to function properly.
	0019 beta 3 deferred execution of refr scripts until after Load had completed, but this leaves a chance that array
	or string variables are not initialized when the script runs, or that scripts don't run in the order in which they
	were invoked.

	0019 beta 4 changes LoadGame hooks as follows:
	-PreLoad: Load array and string vars from co-save, keep a backup of the previous vars. Script execution not disabled.
		^ vanilla CTD bug with token scripts returns, worth addressing separately
	-FinishLoad: If LoadGame() returns false (failed to load game), restore the backup of the previous array/string var maps.
		Otherwise erase them. Dispatch message to plugins to indicate whether or not LoadGame() succeeded
	NVSE is following 0019 beta 4 logic.
*/

static const char* s_saveFilePath = NULL;

static void __stdcall DoPreLoadGameHook(const char* saveFilePath)
{
	g_gameStarted = false;
	s_saveFilePath = (const char *)saveFilePath;
	_MESSAGE("NVSE DLL DoPreLoadGameHook: %s", saveFilePath);
	Serialization::HandlePreLoadGame(saveFilePath);
}

static __declspec(naked) void PreLoadGameHook(void)
{
	__asm
	{
		pushad
		push		edx				// filepath for savegame
		call		DoPreLoadGameHook
		popad

		// overwritten code
		push		offset	LoadingGameMessage
		jmp			[kPreLoadGameRetnAddr]
	}
}

static void __stdcall DispatchLoadGameEventToScripts(const char* saveFilePath)
{
	_MESSAGE("NVSE DLL DoPostLoadGameHook: %s", saveFilePath);
	if (saveFilePath)
		EventManager::HandleNVSEMessage(NVSEMessagingInterface::kMessage_LoadGame, (void*)saveFilePath);
}

static __declspec(naked) void PostLoadGameHook(void)
{
	__asm {
		pushad

		mov		eax, [s_saveFilePath]
		push	eax
		call	DispatchLoadGameEventToScripts

		popad
		
		pop ecx
		mov     ecx, [ebp - 0x01C]
		xor     ecx, ebp

		jmp		[kPostLoadGameRetnAddr]
	}
}

static void __stdcall DoFinishLoadGame(bool bLoadedSuccessfully)
{
	DEBUG_MESSAGE("NVSE DLL DoFinishLoadGame() %s", bLoadedSuccessfully ? "succeeded" : "failed");
	Serialization::HandlePostLoadGame(bLoadedSuccessfully);
//	handled by Dispatch_Message EventManager::HandleNVSEMessage(NVSEMessagingInterface::kMessage_PostLoadGame, (void*)bLoadedSuccessfully);
}

static __declspec(naked) void FinishLoadGameHook(void)
{
	__asm {
		pushad

		movzx eax, al
		push eax		// bool retn value from LoadGame()
		call DoFinishLoadGame

		popad
		retn 8
	}
}

extern UnorderedSet<UInt32> s_gameLoadedInformedScripts;

static void __stdcall DoLoadGameHook(const char* saveFilePath)
{
	g_gameLoaded = true;
	g_gameStarted = true;
	s_gameLoadedInformedScripts.Clear();

	_MESSAGE("NVSE DLL DoLoadGameHook: %s", saveFilePath);
	Serialization::HandleLoadGame(saveFilePath);
}

static __declspec(naked) void LoadGameHook(void)
{
	__asm
	{
		pushad
		push		ecx				// filepath for savegame
		call		DoLoadGameHook
		popad

		// overwritten code
		push		offset	LoadGameMessage
		jmp			[kLoadGameRetnAddr]
	}
}

static void __stdcall DoSaveGameHook(const char* saveFilePath)
{
	_MESSAGE("NVSE DLL DoSaveGameHook: %s", saveFilePath);
	Serialization::HandleSaveGame(saveFilePath);
}

static __declspec(naked) void SaveGameHook(void)
{
	__asm
	{
		pushad
		push		edx				// filepath for savegame
		call		DoSaveGameHook
		popad

		// overwritten code
		push		offset	SaveGameMessage
		jmp			[kSaveGameRetnAddr]
	}
}

static void NewGameHook(void)
{
	_MESSAGE("NewGameHook");

	g_gameLoaded = true;
	g_gameStarted = true;
	s_gameLoadedInformedScripts.Clear();
	Serialization::HandleNewGame();
}

static void __stdcall DeleteGameHook(const char * path)
{
	_MESSAGE("DeleteGameHook: %s", path);

	Serialization::HandleDeleteGame(path);

	DeleteFile(path);
}

static void RenameGameHook(const char * oldPath, const char * newPath)
{
	_MESSAGE("RenameGameHook: %s -> %s", oldPath, newPath);

	Serialization::HandleRenameGame(oldPath, newPath);

	rename(oldPath, newPath);
}

void Hook_SaveLoad_Init(void)
{
	WriteRelJump(kLoadGamePatchAddr, (UInt32)&LoadGameHook);
	WriteRelJump(kSaveGamePatchAddr, (UInt32)&SaveGameHook);
	WriteRelCall(kNewGamePatchAddr, (UInt32)&NewGameHook);
	WriteRelCall(kDeleteGamePatchAddr, (UInt32)&DeleteGameHook);
	SafeWrite8(kDeleteGamePatchAddr + 5, 0x90);		// nop out leftover byte from original instruction
	WriteRelCall(kRenameGamePatchAddr, (UInt32)&RenameGameHook);
	WriteRelJump(kPreLoadGamePatchAddr, (UInt32)&PreLoadGameHook);
	WriteRelJump(kPostLoadGamePatchAddr, (UInt32)&PostLoadGameHook);
	WriteRelJump(kPostLoadGameFinishedAddr, (UInt32)&FinishLoadGameHook);
	Init_CoreSerialization_Callbacks();
}
#endif
