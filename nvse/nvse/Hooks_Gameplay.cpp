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
#include "GameUI.h"
#include "CachedScripts.h"

static void HandleMainLoopHook(void);

static const UInt32 kMainLoopHookPatchAddr	= 0x0086B386;	// 7th call BEFORE first call to Sleep in oldWinMain	// 006EEC15 looks best for FO3
static const UInt32 kMainLoopHookRetnAddr	= 0x0086B38B;

static constexpr UInt32 kConsoleOpenGlobalAddr = 0x11DEA2E;
static constexpr UInt32 kIsInPauseFadeGlobalAddr = 0x11DEA2D;

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

bool IsGamePaused()
{
	bool isMainOrPauseMenuOpen = *(Menu**)0x11DAAC0; // g_startMenu, credits to lStewieAl
	auto* console = ConsoleManager::GetSingleton();

	return isMainOrPauseMenuOpen|| console->IsConsoleOpen();
}

// xNVSE 6.1
void HandleDelayedCall(float timeDelta, bool isMenuMode)
{
	if (g_callAfterInfos.empty())
		return; // avoid lock overhead

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callAfterInfosCS);

	auto iter = g_callAfterInfos.begin();
	while (iter != g_callAfterInfos.end())
	{
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			iter->time += timeDelta;
			// intentional fallthrough in case the delay was 0
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

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callAfterFramesInfosCS);

	auto iter = g_callAfterFramesInfos.begin();
	while (iter != g_callAfterFramesInfos.end())
	{
		auto& framesLeft = iter->time; //alias for clarification
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			++iter;
			continue; // ignore the possibility of callback setup to wait 0 frames (just use regular Call)
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

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callWhileInfosCS);

	auto iter = g_callWhileInfos.begin();
	while (iter != g_callWhileInfos.end())
	{
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			++iter;
			continue;
		}

		ArrayElementArgFunctionCaller<SelfOwningArrayElement> conditionCaller(iter->condition, iter->thisObj);
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

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callWhenInfosCS);

	auto iter = g_callWhenInfos.begin();
	while (iter != g_callWhenInfos.end())
	{
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			++iter;
			continue;
		}

		ArrayElementArgFunctionCaller<SelfOwningArrayElement> conditionCaller(iter->condition, iter->thisObj);
		if (iter->PassArgsToCondFunc())
			conditionCaller.SetArgs(iter->args);

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

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callForInfosCS);

	auto iter = g_callForInfos.begin();
	while (iter != g_callForInfos.end())
	{
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			iter->time += timeDelta;
			// intentional fallthrough in case the delay was 0
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

	const bool isGamePaused = IsGamePaused();
	ScopedLock lock(g_callWhilePerSecondsInfosCS);

	auto iter = g_callWhilePerSecondsInfos.begin();
	while (iter != g_callWhilePerSecondsInfos.end())
	{
		if (!iter->ShouldRun(isMenuMode, isGamePaused))
		{
			iter->oldTime += timeDelta;
			++iter;
			continue;
		}

		if (g_gameSecondsPassed - iter->oldTime >= iter->interval)
		{
			ArrayElementArgFunctionCaller<SelfOwningArrayElement> conditionCaller(iter->condition, iter->thisObj);
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

namespace DisablePlayerControlsAlt
{
	std::map<ModID, flags_t> g_disabledFlagsPerMod;
	flags_t g_disabledControls = 0;

	flags_t CondenseVanillaFlagArgs(UInt32 movementFlag, UInt32 pipboyFlag, UInt32 fightingFlag, 
		UInt32 POVFlag, UInt32 lookingFlag, UInt32 rolloverTextFlag, UInt32 sneakingFlag)
	{
		// Copy the order that flags are arranged from vanilla (doesn't 100% match the order of args passed to the func).
		return static_cast<flags_t>(
			  (movementFlag << 0)
			| (lookingFlag << 1)
			| (pipboyFlag << 2)
			| (fightingFlag << 3)
			| (POVFlag << 4)
			| (rolloverTextFlag << 5)
			| (sneakingFlag << 6)
			);
	}

	void ApplyImmediateDisablingEffects(flags_t changedFlagsForMod)
	{
		auto* player = PlayerCharacter::GetSingleton();

		// Check if the changed (re-enabled controls) flags are actually changing current flags.
		// Thus, remove the control-disabling bits that are already on.
		flags_t realFlagChanges = changedFlagsForMod & ~(g_disabledControls | static_cast<flags_t>(player->disabledControlFlags));
		if (!realFlagChanges)
			return;

		// Copy code at 0x95F590 to force out of sneak etc
		if ((realFlagChanges & kFlag_Movement) != 0)
		{
			HUDMainMenu::UpdateVisibilityState(HUDMainMenu::kHUDState_PlayerDisabledControls);
		}
		else
		{
			// important for updating RolloverText and such at 0x771972
			HUDMainMenu::UpdateVisibilityState(HUDMainMenu::kHUDState_RECALCULATE);
		}

		if ((realFlagChanges & kFlag_Fighting) != 0)
			player->SetWantsWeaponOut(0);

		if ((realFlagChanges & kFlag_POV) != 0)
		{
			player->bThirdPerson = false;
			if (player->playerNode)
				player->UpdateCamera(0, 0);
			else
			{
				float& g_fThirdPersonZoomHeight = *reinterpret_cast<float*>(0x11E0768);
				g_fThirdPersonZoomHeight = 0.0;
			}
		}

		if ((realFlagChanges & kFlag_Sneaking) != 0)
		{
			// force out of sneak by removing sneak movement flag
			ThisStdCall(0x9EA3E0, PlayerCharacter::GetSingleton()->actorMover, (player->GetMovementFlags() & ~0x400));
		}
	}

	void ApplyImmediateEnablingEffects(flags_t changedFlagsForMod)
	{
		// changedFlagsForMod = the control-disabling flags that will be removed, aka controls being re-enabled.
		// Thus, only keep a control-disabling bit if the bit was on.
		flags_t realFlagChanges = changedFlagsForMod & g_disabledControls;

		// Vanilla disabled control flags won't be overwritten, so if vanilla still has it disabled it'll stay that way.
		realFlagChanges &= ~static_cast<flags_t>(PlayerCharacter::GetSingleton()->disabledControlFlags);

		// If re-enabling movement control...
		if ((realFlagChanges & kFlag_Movement) != 0)
			HUDMainMenu::UpdateVisibilityState(HUDMainMenu::kHUDState_RECALCULATE);
	}

	void ResetOnLoad()
	{
		g_disabledFlagsPerMod.clear();
		g_disabledControls = 0;
		ApplyImmediateEnablingEffects(kAllFlags);
	}

	// Logical OR the vanilla flags with ours
	__HOOK ModifyPlayerControlFlags()
	{
		static const UInt32 ContinueFuncAddr = 0x5A0401;
		_asm
		{
			mov     eax, [ebp - 4]
			movzx   ecx, byte ptr [eax + 0x680]
			movzx   edx, g_disabledControls
			AND		edx, kVanillaFlags // prevent our custom flags from being added to vanilla pcControlFlags here.
			OR		ecx, edx
			jmp		ContinueFuncAddr
		}
	}

	CallDetour g_PreventAttackDetour;
	bool __fastcall MaybePreventPlayerAttacking(Actor* player, void* edx, UInt32 animGroupId)
	{
		if ((g_disabledControls & kFlag_Attacking) != 0)
			return false;

		// actually fire the weapon
		return ThisStdCall<bool>(g_PreventAttackDetour.GetOverwrittenAddr(), player, animGroupId);
	}

	CallDetour g_PreventVATSDetour;
	signed int __fastcall GetControlState_VATSHook(OSInputGlobals* self, void* edx, int key, int state)
	{
		auto defaultResult = ThisStdCall<signed int>(g_PreventVATSDetour.GetOverwrittenAddr(), self, key, state);
		if ((g_disabledControls & kFlag_EnterVATS) != 0)
			return 0;
		return defaultResult;
	}

	CallDetour g_PreventJumpingDetour;
	signed int __fastcall GetControlState_JumpingHook(OSInputGlobals* self, void* edx, int key, int state)
	{
		auto defaultResult = ThisStdCall<signed int>(g_PreventJumpingDetour.GetOverwrittenAddr(), self, key, state);
		if ((g_disabledControls & kFlag_Jumping) != 0)
			return 0;
		return defaultResult;
	}

	// To prevent aiming/blocking, need 2 detours to handle 2 function calls:
	// ...one for OSInputGlobals::GetControlState -> IsPressed, another for isHeld.
	CallDetour g_PreventAimingOrBlockingDetour_IsPressed;
	signed int __fastcall GetControlState_Aiming_IsPressedHook(OSInputGlobals* self, void* edx, int key, int state)
	{
		auto defaultResult = ThisStdCall<signed int>(g_PreventAimingOrBlockingDetour_IsPressed.GetOverwrittenAddr(), self, key, state);
		if ((g_disabledControls & kFlag_AimingOrBlocking) != 0)
			return 0;
		return defaultResult;
	}

	CallDetour g_PreventAimingOrBlockingDetour_IsHeld;
	signed int __fastcall GetControlState_Aiming_IsHeldHook(OSInputGlobals* self, void* edx, int key, int state)
	{
		auto defaultResult = ThisStdCall<signed int>(g_PreventAimingOrBlockingDetour_IsHeld.GetOverwrittenAddr(), self, key, state);
		if ((g_disabledControls & kFlag_AimingOrBlocking) != 0)
			return 0;
		return defaultResult;
	}

	__HOOK MaybePreventRunningForControllers()
	{
		static const UInt32 NormalRetnAddr = 0x941708,
			PreventRunningAddr = 0x941792;
		_asm
		{
			movzx	eax, g_disabledControls
			AND		eax, kFlag_Running
			test	eax, eax
			jz		DoRegular
			// prevent activation
			jmp		PreventRunningAddr
		DoRegular :
			// go back to the code we jumped from and slightly overwrote
			mov     edx, [ebp - 0x18]
			mov     eax, [edx + 0x68]
			jmp		NormalRetnAddr
		}
	}

	CallDetour g_PreventRunningForNonController;
	double __fastcall MaybePreventRunningForNonController(Actor* self, void* edx)
	{
		auto* _ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
		auto& moveFlags = *reinterpret_cast<UInt16*>(_ebp - 0x6C);

		if ((g_disabledControls & kFlag_Running) != 0)
			moveFlags &= ~0x200; // remove kMoveFlag_Running

		auto result = ThisStdCall<double>(g_PreventRunningForNonController.GetOverwrittenAddr(), self);
		return result;
	}

	void WriteHooks()
	{
		WriteRelJump(0x5A03F7, (UInt32)ModifyPlayerControlFlags);
		g_PreventAttackDetour.WriteRelCall(0x949CF1, (UInt32)MaybePreventPlayerAttacking);

		// Use detour since "GetControlState" funcs could be popular hook spots
		g_PreventVATSDetour.WriteRelCall(0x942884, (UInt32)GetControlState_VATSHook);
		g_PreventJumpingDetour.WriteRelCall(0x94215F, (UInt32)GetControlState_JumpingHook);
		g_PreventAimingOrBlockingDetour_IsPressed.WriteRelCall(0x941F4F, (UInt32)GetControlState_Aiming_IsPressedHook);
		g_PreventAimingOrBlockingDetour_IsHeld.WriteRelCall(0x941F5F, (UInt32)GetControlState_Aiming_IsHeldHook);

		WriteRelJump(0x941702, (UInt32)MaybePreventRunningForControllers);

		// hooks Actor::GetMovementSpeed() call
		g_PreventRunningForNonController.WriteRelCall(0x941B60, (UInt32)MaybePreventRunningForNonController);

		// todo: maybe add hook+flag to disable grabbing @ 0x95F6DE
		// todo: maybe add hook+flag to disable AmmoSwap at 0x94098B
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
		CacheAllScriptsInPath(ScriptFilesPath);
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

	DisablePlayerControlsAlt::WriteHooks();
}

void SetRetainExtraOwnership(bool bRetain)
{
	UInt8 retain = bRetain ? 1 : 0;
	SafeWrite8(kExtraOwnershipDefaultSetting, retain);
	//SafeWrite8(kExtraOwnershipDefaultSetting2, retain);	Not seen (yet?) in Fallout
}
