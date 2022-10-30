#include <set>

#include "Hooks_Gameplay.h"

#include <filesystem>

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
#include "Core_Serialization.h"
#include "PluginManager.h"
#include "GameOSDepend.h"
#include "InventoryReference.h"
#include "EventManager.h"
#include "FunctionScripts.h"
#include "Hooks_Other.h"
#include "ScriptTokenCache.h"
#include <chrono>
#include <fstream>

#include "GameData.h"
#include "UnitTests.h"

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

float g_gameSecondsPassed = 0;

// xNVSE 6.1
void HandleDelayedCall(float timeDelta, bool isMenuMode)
{
	if (g_callAfterInfos.empty())
		return; // avoid lock overhead

	ScopedLock lock(g_callAfterInfosCS);

	auto iter = g_callAfterInfos.begin();
	while (iter != g_callAfterInfos.end())
	{
		if (!iter->RunInMenuMode() && isMenuMode)
		{
			iter->time += timeDelta;
		}
		if (g_gameSecondsPassed >= iter->time)
		{
			ArrayElementArgFunctionCaller caller(iter->script, iter->args, iter->thisObj);
			UserFunctionManager::Call(std::move(caller));
			iter = g_callAfterInfos.erase(iter); // yes, this is valid: https://stackoverflow.com/a/3901380/6741772
		}
		else
		{
			++iter;
		}
	}
}

void HandleCallAfterFramesScripts(bool isMenuMode)
{
	if (g_callAfterFramesInfos.empty())
		return; // avoid lock overhead

	ScopedLock lock(g_callAfterFramesInfosCS);

	auto iter = g_callAfterFramesInfos.begin();
	while (iter != g_callAfterFramesInfos.end())
	{
		auto& framesLeft = iter->time; //alias for clarification
		if (!iter->RunInMenuMode() && isMenuMode)
		{
			++iter;
			continue;
		}

		if (--framesLeft <= 0)
		{
			ArrayElementArgFunctionCaller caller(iter->script, iter->args, iter->thisObj);
			UserFunctionManager::Call(std::move(caller));
			iter = g_callAfterFramesInfos.erase(iter); // yes, this is valid: https://stackoverflow.com/a/3901380/6741772
		}
		else
		{
			++iter;
		}
	}
}

void HandleCallWhileScripts(bool isMenuMode)
{
	if (g_callWhileInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callWhileInfosCS);

	auto iter = g_callWhileInfos.begin();
	while (iter != g_callWhileInfos.end())
	{
		if ((isMenuMode && !iter->RunInMenuMode())
			|| (!isMenuMode && !iter->RunInGameMode()))
		{
			++iter;
			continue;
		}

		ArrayElementArgFunctionCaller<SelfOwningArrayElement> conditionCaller(iter->condition);
		if (iter->PassArgsToCondFunc())
		{
			conditionCaller.SetArgs(iter->args);
		}

		if (auto const conditionResult = UserFunctionManager::Call(std::move(conditionCaller)); 
			conditionResult && conditionResult->GetBool())
		{
			ArrayElementArgFunctionCaller<SelfOwningArrayElement> scriptCaller(iter->callFunction, iter->thisObj);
			if (iter->PassArgsToCallFunc())
				scriptCaller.SetArgs(iter->args);
			UserFunctionManager::Call(std::move(scriptCaller));
			++iter;
		}
		else
		{
			iter = g_callWhileInfos.erase(iter);
		}
	}
}

void HandleCallWhenScripts(bool isMenuMode)
{
	if (g_callWhenInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callWhenInfosCS);

	auto iter = g_callWhenInfos.begin();
	while (iter != g_callWhenInfos.end())
	{
		if ((isMenuMode && !iter->RunInMenuMode())
			|| (!isMenuMode && !iter->RunInGameMode()))
		{
			++iter;
			continue;
		}

		ArrayElementArgFunctionCaller conditionCaller(iter->condition, iter->args);
		if (iter->PassArgsToCondFunc())
		{
			conditionCaller.SetArgs(iter->args);
		}

		if (auto const conditionResult = UserFunctionManager::Call(std::move(conditionCaller)); 
			conditionResult && conditionResult->GetBool())
		{
			ArrayElementArgFunctionCaller<SelfOwningArrayElement> scriptCaller(iter->callFunction, iter->thisObj);
			if (iter->PassArgsToCallFunc())
				scriptCaller.SetArgs(iter->args);
			UserFunctionManager::Call(std::move(scriptCaller));
			iter = g_callWhenInfos.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void HandleCallForScripts(float timeDelta, bool isMenuMode)
{
	if (g_callForInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callForInfosCS);

	auto iter = g_callForInfos.begin();
	while (iter != g_callForInfos.end())
	{
		if (!iter->RunInMenuMode() && isMenuMode)
		{
			iter->time += timeDelta;
		}
		if (g_gameSecondsPassed < iter->time)
		{
			ArrayElementArgFunctionCaller caller(iter->script, iter->args, iter->thisObj);
			UserFunctionManager::Call(std::move(caller));
			++iter;
		}
		else
		{
			iter = g_callForInfos.erase(iter);
		}
	}
}

void HandleCallWhilePerSecondsScripts(float timeDelta, bool isMenuMode)
{
	if (g_callWhilePerSecondsInfos.empty())
		return; // avoid lock overhead
	ScopedLock lock(g_callWhilePerSecondsInfosCS);

	auto iter = g_callWhilePerSecondsInfos.begin();
	while (iter != g_callWhilePerSecondsInfos.end())
	{
		if ((isMenuMode && !iter->RunInMenuMode())
			|| (!isMenuMode && !iter->RunInGameMode()))
		{
			iter->oldTime += timeDelta;
			++iter;
			continue;
		}

		if (g_gameSecondsPassed - iter->oldTime >= iter->interval)
		{
			ArrayElementArgFunctionCaller<SelfOwningArrayElement> conditionCaller(iter->condition);
			if (iter->PassArgsToCondFunc())
			{
				conditionCaller.SetArgs(iter->args);
			}

			if (auto const conditionResult = UserFunctionManager::Call(std::move(conditionCaller));
				conditionResult && conditionResult->GetBool())
			{
				ArrayElementArgFunctionCaller<SelfOwningArrayElement> scriptCaller(iter->callFunction, iter->thisObj);
				if (iter->PassArgsToCallFunc())
					scriptCaller.SetArgs(iter->args);
				UserFunctionManager::Call(std::move(scriptCaller));
				iter->oldTime = g_gameSecondsPassed;
			}
			else
			{
				iter = g_callWhilePerSecondsInfos.erase(iter);
				continue;
			}
		}
		++iter;
	}
}

// boolean, used by ExtraDataList::IsExtraDefaultForContainer() to determine if ExtraOwnership should be treated
// as 'non-default' for an inventory object. Is 0 in vanilla, set to 1 to make ownership NOT treated as default
// Might be those addresses, used to decide if can be copied
static const UInt32 kExtraOwnershipDefaultSetting  = 0x00411F78;	//	0040A654 in Fallout3 1.7
// Byte array at the end of the sub who is the 4th call in ExtraDataList__RemoveAllCopyableExtra
//don't see a second array.. static const UInt32 kExtraOwnershipDefaultSetting2 = 0x0041FE0D;	//

void ApplyGECKEditorIDs()
{
	// fix inconsistency
	const auto set = [](UInt32 id, const char* str)
	{
		LookupFormByID(id)->SetEditorID(str);
	};
	set(0x14, "PlayerRef");
	set(0xA, "Lockpick");
	set(0x2D, "MaleAdult01Default");
	set(0x2E, "FemaleAdult01Default");
	set(0x33, "RadiationMarker");
	set(0x3D, "DefaultCombatStyle");
	set(0x147, "PipBoyLight");
	set(0x163, "HelpManual");
	set(0x1F5, "DefaultWaterExplosion");
	set(0x1F6, "GasTrapDummy");
}

DWORD g_mainThreadID = 0;
bool s_recordedMainThreadID = false;
std::unordered_set<UInt8> g_myMods;
void DetermineShowScriptErrors()
{
	UInt32 iniOpt;
	if (GetNVSEConfigOption_UInt32("RELEASE", "bWarnScriptErrors", &iniOpt))
	{
		g_warnScriptErrors = iniOpt;
		return;
	}

	try
	{
		const auto* myModsFileName = "MyGECKMods.txt";
		if (std::filesystem::exists(myModsFileName))
		{
			std::ifstream is(myModsFileName);
			std::string curMod;
			while (std::getline(is, curMod))
			{
				if (curMod.empty())
					continue;
				if (const auto idx = DataHandler::Get()->GetModIndex(curMod.c_str()); idx != -1)
				{
					g_warnScriptErrors = true;
					g_myMods.insert(idx);
				}
			}
		}
	}
	catch (...)
	{
		g_warnScriptErrors = false;
	}
#if _DEBUG
	g_warnScriptErrors = true;
#endif
}

static void HandleMainLoopHook(void)
{ 
	if (!s_recordedMainThreadID)
	{
		DetermineShowScriptErrors();
		ApplyGECKEditorIDs();
		s_recordedMainThreadID = true;
#if _DEBUG
#if ALPHA_MODE
		Console_Print("xNVSE (debug) %d.%d.%d Beta Build %s", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA, __TIME__);
#else
		Console_Print("xNVSE (debug) %d.%d.%d", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA);
#endif
#else //release print
#if ALPHA_MODE
		Console_Print("xNVSE %d.%d.%d Beta Build %s", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA, __TIME__);
#else
		Console_Print("xNVSE %d.%d.%d", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA);
#endif
#endif
		g_mainThreadID = GetCurrentThreadId();
		
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_DeferredInit, NULL, 0, NULL);

#if RUNTIME
		ExecuteRuntimeUnitTests();
#endif
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
	LambdaManager::EraseUnusedSavedVariableLists();
	
	const auto vatsTimeMult = ThisStdCall<double>(0x9C8CC0, reinterpret_cast<void*>(0x11F2250));
	const float timeDelta = g_timeGlobal->secondsPassed * static_cast<float>(vatsTimeMult);
	const auto isMenuMode = CdeclCall<bool>(0x702360);
	g_gameSecondsPassed += timeDelta;

	// handle calls from cmd CallWhile
	HandleCallWhileScripts(isMenuMode);
	HandleCallWhenScripts(isMenuMode);

	// handle calls from cmd CallAfterSeconds
	HandleDelayedCall(timeDelta, isMenuMode);

	// handle calls from cmd CallForSeconds
	HandleCallForScripts(timeDelta, isMenuMode);

	HandleCallWhilePerSecondsScripts(timeDelta, isMenuMode);

	HandleCallAfterFramesScripts(isMenuMode);
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
