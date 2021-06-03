#include <set>

#include "Hooks_Gameplay.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "SafeWrite.h"
#include "Serialization.h"
#include "GameAPI.h"
#include <share.h>
#include <set>
#include "StringVar.h"
#include "ArrayVar.h"
#include "Commands_Script.h"
#include "PluginManager.h"
#include "GameOSDepend.h"
#include "InventoryReference.h"
#include "EventManager.h"
#include "FunctionScripts.h"
#include "Hooks_Other.h"
#include "ScriptTokenCache.h"

static void HandleMainLoopHook(void);

static const UInt32 kMainLoopHookPatchAddr	= 0x0086B386;	// 7th call BEFORE first call to Sleep in oldWinMain	// 006EEC15 looks best for FO3
static const UInt32 kMainLoopHookRetnAddr	= 0x0086B38B;

__declspec(naked) void MainLoopHook()
{
	__asm
	{
		call	HandleMainLoopHook
		mov	eax, ds:[0x11F4748]
		jmp	kMainLoopHookRetnAddr
	}
}

void ToggleUIMessages(bool bEnable)
{
	// Disable: write an immediate return at function entry
	// Enable: restore the push instruction at function entry
	SafeWrite8((UInt32)QueueUIMessage, bEnable ? 0x55 : 0xC3);
}

bool RunCommand_NS(COMMAND_ARGS, Cmd_Execute cmd)
{
	ToggleUIMessages(false);
	bool cmdResult = cmd(PASS_COMMAND_ARGS);
	ToggleUIMessages(true);

	return cmdResult;
}

extern std::vector<DelayedCallInfo> g_callAfterScripts;
extern ICriticalSection g_callAfterScriptsCS;
float g_gameSecondsPassed = 0;

// xNVSE 6.1
void HandleDelayedCall()
{
	if (g_callAfterScripts.empty())
		return; // avoid lock overhead
	
	ScopedLock lock(g_callAfterScriptsCS);

	auto iter = g_callAfterScripts.begin();
	while (iter != g_callAfterScripts.end())
	{
		if (g_gameSecondsPassed >= iter->time)
		{
			InternalFunctionCaller caller(iter->script, iter->thisObj);
			delete UserFunctionManager::Call(std::move(caller));
			iter = g_callAfterScripts.erase(iter); // yes, this is valid: https://stackoverflow.com/a/3901380/6741772
		}
		else
		{
			++iter;
		}
	}
}

extern std::vector<CallWhileInfo> g_callWhileInfos;
extern ICriticalSection g_callWhileInfosCS;

void HandleCallWhileScripts()
{
	if (g_callWhileInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callWhileInfosCS);

	auto iter = g_callWhileInfos.begin();
	while (iter != g_callWhileInfos.end())
	{
		InternalFunctionCaller conditionCaller(iter->condition);
		if (auto conditionResult = std::unique_ptr<ScriptToken>(UserFunctionManager::Call(std::move(conditionCaller))); conditionResult && conditionResult->GetBool())
		{
			InternalFunctionCaller scriptCaller(iter->callFunction, iter->thisObj);
			delete UserFunctionManager::Call(std::move(scriptCaller));
			++iter;
		}
		else
		{
			iter = g_callWhileInfos.erase(iter);
		}
	}
}

extern std::vector<DelayedCallInfo> g_callForInfos;
extern ICriticalSection g_callForInfosCS;

void HandleCallForScripts()
{
	if (g_callForInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callForInfosCS);

	auto iter = g_callForInfos.begin();
	while (iter != g_callForInfos.end())
	{
		if (g_gameSecondsPassed < iter->time)
		{
			InternalFunctionCaller caller(iter->script, iter->thisObj);
			delete UserFunctionManager::Call(std::move(caller));
			++iter;
		}
		else
		{
			iter = g_callForInfos.erase(iter);
		}
	}
}

// boolean, used by ExtraDataList::IsExtraDefaultForContainer() to determine if ExtraOwnership should be treated
// as 'non-default' for an inventory object. Is 0 in vanilla, set to 1 to make ownership NOT treated as default
// Might be those addresses, used to decide if can be copied
static const UInt32 kExtraOwnershipDefaultSetting  = 0x00411F78;	//	0040A654 in Fallout3 1.7
// Byte array at the end of the sub who is the 4th call in ExtraDataList__RemoveAllCopyableExtra
//don't see a second array.. static const UInt32 kExtraOwnershipDefaultSetting2 = 0x0041FE0D;	//

DWORD g_mainThreadID = 0;
bool s_recordedMainThreadID = false;
static void HandleMainLoopHook(void)
{ 
	if (!s_recordedMainThreadID)
	{
		s_recordedMainThreadID = true;
		g_mainThreadID = GetCurrentThreadId();
		
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_DeferredInit, NULL, 0, NULL);
		Console_Print("xNVSE %d.%d.%d", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA);
	}
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_MainGameLoop, nullptr, 0, nullptr);

	// if any temporary references to inventory objects exist, clean them up
	if (!s_invRefMap.Empty())
		s_invRefMap.Clear();

	// Tick event manager
	EventManager::Tick();

	// clean up any temp arrays/strings (moved after deffered processing because of array parameter to User Defined Events)
	g_ArrayMap.Clean();
	g_StringMap.Clean();

	g_gameSecondsPassed += *g_globalTimeMult * g_timeGlobal->secondsPassed;
	// handle calls from cmd CallAfter
	HandleDelayedCall();

	// handle calls from cmd CallWhile
	HandleCallWhileScripts();

	// handle calls from cmd CallFor
	HandleCallForScripts();
}

#define DEBUG_PRINT_CHANNEL(idx)								\
																\
static UInt32 __stdcall DebugPrint##idx(const char * str)		\
{																\
	static FILE	* dst = NULL;									\
	if(!dst) dst = _fsopen("nvse_debugprint" #idx ".log", "w", _SH_DENYWR);	\
	if(dst) fputs(str, dst);									\
	return 0;													\
}

DEBUG_PRINT_CHANNEL(0)	// used to exit
DEBUG_PRINT_CHANNEL(1)	// ignored
DEBUG_PRINT_CHANNEL(2)	// ignored
// 3 - program flow
DEBUG_PRINT_CHANNEL(4)	// ignored
// 5 - stack trace?
DEBUG_PRINT_CHANNEL(6)	// ignored
// 7 - ingame
// 8 - ingame

// these are all ignored in-game
static void Hook_DebugPrint(void)
{
	const UInt32	kMessageHandlerVtblBase = 0x010C14A8;

	SafeWrite32(kMessageHandlerVtblBase + (0 * 4), (UInt32)DebugPrint0);
	SafeWrite32(kMessageHandlerVtblBase + (1 * 4), (UInt32)DebugPrint1);
	SafeWrite32(kMessageHandlerVtblBase + (2 * 4), (UInt32)DebugPrint2);
	SafeWrite32(kMessageHandlerVtblBase + (4 * 4), (UInt32)DebugPrint4);
	SafeWrite32(kMessageHandlerVtblBase + (6 * 4), (UInt32)DebugPrint6);
}

void ToggleConsoleOutput(bool enable)
{
	static bool s_bEnabled = true;
	if (enable != s_bEnabled) {
		s_bEnabled = enable;
		if (enable) {
			// original code: 'push ebp ; mov esp, ebp
			SafeWrite8(s_Console__Print+0, 0x55);
			SafeWrite8(s_Console__Print+1, 0x88);
			SafeWrite8(s_Console__Print+2, 0xEC);
		}
		else {
			// 'retn 8'
			SafeWrite8(s_Console__Print, 0xC2);
			SafeWrite8(s_Console__Print+1, 0x08);
			SafeWrite8(s_Console__Print+2, 0x00);
		}
	}
}

static const UInt32 kCreateReferenceCallAddr		= 0x004C37D0;
static const UInt32 kCreateDroppedReferenceHookAddr = 0x0057515A;
static const UInt32 kCreateDroppedReferenceRetnAddr = 0x00575162;

void __stdcall HandleDroppedItem(TESObjectREFR *dropperRef, TESForm *itemBase, TESObjectREFR *droppedRef)
{
	if (droppedRef)
		EventManager::HandleEvent(EventManager::kEventID_OnDrop, dropperRef, droppedRef);
	if (itemBase)
		EventManager::HandleEvent(EventManager::kEventID_OnDropItem, dropperRef, itemBase);
}

__declspec(naked) void DroppedItemHook(void)
{
	__asm
	{
		call	kCreateReferenceCallAddr
		mov	[ebp-4], eax
		push	eax
		push	dword ptr [ebp+8]
		push	dword ptr [ebp-0x20]
		call	HandleDroppedItem
		jmp	kCreateDroppedReferenceRetnAddr
	}
}

static const UInt32 kMainMenuFromIngameMenuPatchAddr = 0x007D0B17;	// 3rd call above first reference to aDataMusicSpecialMaintitle_mp3, call following mov ecx, g_osGlobals
static const UInt32 kMainMenuFromIngameMenuRetnAddr	 = 0x007D0B90;	// original call

static const UInt32 kExitGameViaQQQPatchAddr		 = 0x005B6CA6;	// Inside Cmd_QuitGame_Execute, after mov ecx, g_osGlobals
static const UInt32 kExitGameViaQQQRetnAddr			 = 0x005B6CB0;	// original call

static const UInt32 kExitGameFromMenuPatchAddr       = 0x007D0C3E;	// 2nd call to kExitGameViaQQQRetnAddr

enum QuitGameMessage
{
	kQuit_ToMainMenu,
	kQuit_ToWindows,
	kQuit_QQQ,
};

void __stdcall SendQuitGameMessage(QuitGameMessage msg)
{
	UInt32 msgToSend = NVSEMessagingInterface::kMessage_ExitGame;
	if (msg == kQuit_ToMainMenu)
		msgToSend = NVSEMessagingInterface::kMessage_ExitToMainMenu;
	else if (msg == kQuit_QQQ)
		msgToSend = NVSEMessagingInterface::kMessage_ExitGame_Console;

	PluginManager::Dispatch_Message(0, msgToSend, NULL, 0, NULL);
//	handled by Dispatch_Message EventManager::HandleNVSEMessage(msgToSend, NULL);
}

__declspec(naked) void ExitGameFromMenuHook()
{
	__asm
	{
		mov	byte ptr [ecx+1], 1
		push	kQuit_ToWindows
		call	SendQuitGameMessage
		mov	esp, ebp
		pop	ebp
		retn
	}
}

__declspec(naked) void ExitGameViaQQQHook()
{
	__asm
	{
		mov	byte ptr [ecx+1], 1
		push	kQuit_QQQ
		call	SendQuitGameMessage
		mov	al, 1
		pop	ebp
		retn
	}
}

__declspec(naked) void MainMenuFromIngameMenuHook()
{
	__asm
	{
		mov	byte ptr [ecx+2], 1
		push	kQuit_ToMainMenu
		call	SendQuitGameMessage
		retn
	}
}

void Hook_Gameplay_Init(void)
{
	// game main loop
	// this address was chosen because it is only run when new vegas is in the foreground
	WriteRelJump(kMainLoopHookPatchAddr, (UInt32)&MainLoopHook);

	// hook code that creates a new reference to an item dropped by the player
	WriteRelJump(kCreateDroppedReferenceHookAddr, (UInt32)&DroppedItemHook);

	// hook exit to main menu or to windows
	WriteRelCall(kMainMenuFromIngameMenuPatchAddr, (UInt32)&MainMenuFromIngameMenuHook);
	WriteRelJump(kExitGameViaQQQPatchAddr, (UInt32)&ExitGameViaQQQHook);
	WriteRelJump(kExitGameFromMenuPatchAddr, (UInt32)&ExitGameFromMenuHook);

	// this seems stable and helps in debugging, but it makes large files during gameplay
#if _DEBUG
	UInt32 print;
	if (GetNVSEConfigOption_UInt32("DEBUG", "Print", &print) && print)
		Hook_DebugPrint();
#endif
}

void SetRetainExtraOwnership(bool bRetain)
{
	UInt8 retain = bRetain ? 1 : 0;
	SafeWrite8(kExtraOwnershipDefaultSetting, retain);
	//SafeWrite8(kExtraOwnershipDefaultSetting2, retain);	Not seen (yet?) in Fallout
}
