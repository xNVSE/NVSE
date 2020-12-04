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
#include "PluginManager.h"
#include "GameOSDepend.h"
#include "InventoryReference.h"
#include "EventManager.h"
#include "Hooks_Other.h"

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

// boolean, used by ExtraDataList::IsExtraDefaultForContainer() to determine if ExtraOwnership should be treated
// as 'non-default' for an inventory object. Is 0 in vanilla, set to 1 to make ownership NOT treated as default
// Might be those addresses, used to decide if can be copied
static const UInt32 kExtraOwnershipDefaultSetting  = 0x00411F78;	//	0040A654 in Fallout3 1.7
// Byte array at the end of the sub who is the 4th call in ExtraDataList__RemoveAllCopyableExtra
//don't see a second array.. static const UInt32 kExtraOwnershipDefaultSetting2 = 0x0041FE0D;	//

DWORD g_mainThreadID = 0;

#if SINGLE_THREAD_SCRIPTS
Script* __fastcall CopyScript(Script *source)
{
	Script *script = (Script*)GameHeapAlloc(sizeof(Script));
	memcpy(script, source, 0x44);
	script->text = NULL;
	script->refList.var = NULL;
	script->refList.next = NULL;
	script->varList.data = NULL;
	script->varList.next = NULL;
	script->data = GameHeapAlloc(source->info.dataLength);
	memcpy(script->data, source->data, source->info.dataLength);
	CdeclCall(0x5AB7F0, &source->refList, &script->refList, script);
	CdeclCall(0x5AB930, &source->varList, &script->varList, script);
	return script;
}

QueuedScript::QueuedScript(Script* _script, TESObjectREFR* _thisObj, ScriptEventList* _eventList, TESObjectREFR* _containingObj, UInt8 _arg5, UInt8 _arg6, UInt8 _arg7, UInt32 _arg8) :
	eventList(_eventList), arg5(_arg5), arg6(_arg6), arg7(_arg7), arg8(_arg8)
{
	script = (_script->flags & 0x4000) ? CopyScript(_script) : _script;
	thisObj = _thisObj ? _thisObj->refID : 0;
	containingObj = _containingObj ? _containingObj->refID : 0;
}

void QueuedScript::Execute()
{
	TESForm *_thisObj = NULL;
	if (thisObj && !(_thisObj = LookupFormByID(thisObj)))
	{
		_MESSAGE("QueuedScript::Execute() attempting to execute %08X on invalid thisObj (%08X).", script->refID, thisObj);
		goto done;
	}
	TESForm *_containingObj = NULL;
	if (containingObj && !(_containingObj = LookupFormByID(containingObj)))
	{
		_MESSAGE("QueuedScript::Execute() attempting to execute %08X on invalid containingObj (%08X).", script->refID, containingObj);
		goto done;
	}
	ThisStdCall<bool>(0x5E2590, (void*)0x11CAED0, script, _thisObj, eventList, _containingObj, arg5, arg6, arg7, arg8);
done:
	if (script->flags & 0x4000)
	{
		ThisStdCall(0x5AA2A0, script);
		GameHeapFree(script);
	}
}

Vector<QueuedScript> s_queuedScripts(0x40);

bool __stdcall AddQueuedScript(Script *script, TESObjectREFR *thisObj, ScriptEventList *eventList, TESObjectREFR *containingObj, UInt8 arg5, UInt8 arg6, UInt8 arg7, UInt32 arg8)
{
	s_queuedScripts.Append(script, thisObj, eventList, containingObj, arg5, arg6, arg7, arg8);
	return true;
}

__declspec(naked) bool __fastcall Hook_ScriptRunner_Run(void *srQueue, int EDX, Script *script, TESObjectREFR *thisObj, ScriptEventList *eventList, TESObjectREFR *containingObj, UInt8 arg5, UInt8 arg6, UInt8 arg7, UInt32 arg8)
{
	__asm
	{
		push	ecx
		call	GetCurrentThreadId
		pop		ecx
		cmp		g_mainThreadID, eax
		jnz		addQueue
		mov		eax, 0x5E2590
		jmp		eax
	addQueue:
		jmp		AddQueuedScript
	}
}

QueuedScript2::QueuedScript2(Script *_script, TESObjectREFR *_thisObj, ScriptEventList *_eventList) : eventList(_eventList)
{
	script = (_script->flags & 0x4000) ? CopyScript(_script) : _script;
	thisObj = _thisObj ? _thisObj->refID : 0;
}

void QueuedScript2::Execute()
{
	TESForm *_thisObj = NULL;
	if (thisObj && !(_thisObj = LookupFormByID(thisObj)))
	{
		_MESSAGE("QueuedScript2::Execute() attempting to execute %08X on invalid thisObj (%08X).", script->refID, thisObj);
		goto done;
	}
	ThisStdCall<bool>(0x5E2730, (void*)0x11CAED0, script, _thisObj, eventList);
done:
	if (script->flags & 0x4000)
	{
		ThisStdCall(0x5AA2A0, script);
		GameHeapFree(script);
	}
}

Vector<QueuedScript2> s_queuedScripts2(0x40);

bool __stdcall AddQueuedScript2(Script *script, TESObjectREFR *thisObj, ScriptEventList *eventList)
{
	s_queuedScripts2.Append(script, thisObj, eventList);
	return true;
}

__declspec(naked) bool __fastcall Hook_ScriptRunner_Run2(void *srQueue, int EDX, Script *script, TESObjectREFR *thisObj, ScriptEventList *eventList)
{
	__asm
	{
		push	ecx
		call	GetCurrentThreadId
		pop		ecx
		cmp		g_mainThreadID, eax
		jnz		addQueue
		mov		eax, 0x5E2730
		jmp		eax
	addQueue:
		jmp		AddQueuedScript2
	}
}
#endif

static void HandleMainLoopHook(void)
{
	static bool s_recordedMainThreadID = false;
	if (!s_recordedMainThreadID)
	{
		s_recordedMainThreadID = true;
		g_mainThreadID = GetCurrentThreadId();
		
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_DeferredInit, NULL, 0, NULL);

#if SINGLE_THREAD_SCRIPTS
		WriteRelCall(0x5AC295, (UInt32)Hook_ScriptRunner_Run);
		WriteRelCall(0x5AC368, (UInt32)Hook_ScriptRunner_Run);
		WriteRelCall(0x5AC3A8, (UInt32)Hook_ScriptRunner_Run);
		WriteRelCall(0x5AC3E9, (UInt32)Hook_ScriptRunner_Run);

		WriteRelCall(0x5AA05A, (UInt32)Hook_ScriptRunner_Run2);
		WriteRelCall(0x5AC1CF, (UInt32)Hook_ScriptRunner_Run2);

		for (UInt32 addr : {0x5AA053, 0x5AC1C8, 0x5AC28E, 0x5AC361, 0x5AC3A1, 0x5AC3E2})
			SafeWriteBuf(addr, "\xB8\xD0\xAE\x1C\x01", 5);
	}
	else
	{
		if (!s_queuedScripts.Empty())
		{
			//InterlockedIncrement((UInt32*)0x11CAEDC);
			for (auto iter = s_queuedScripts.Begin(); iter; ++iter)
				iter().Execute();
			//InterlockedDecrement((UInt32*)0x11CAEDC);
			s_queuedScripts.Clear();
		}
		if (!s_queuedScripts2.Empty())
		{
			for (auto iter = s_queuedScripts2.Begin(); iter; ++iter)
				iter().Execute();
			s_queuedScripts2.Clear();
		}
#endif
	}

	OtherHooks::s_eventListDestructionQueue.Clear();

	// if any temporary references to inventory objects exist, clean them up
	if (!s_invRefMap.Empty())
		s_invRefMap.Clear();

	// Tick event manager
	EventManager::Tick();

	// clean up any temp arrays/strings (moved after deffered processing because of array parameter to User Defined Events)
	g_ArrayMap.Clean();
	g_StringMap.Clean();
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
