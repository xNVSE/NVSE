#include <stdarg.h>
#include "EventManager.h"
#include "ArrayVar.h"
#include "PluginAPI.h"
#include "GameAPI.h"
#include "Utilities.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "FunctionScripts.h"
#include "GameObjects.h"
#include "ThreadLocal.h"
#include "common/ICriticalSection.h"
#include "Hooks_Gameplay.h"
#include "GameOSDepend.h"
//#include "InventoryReference.h"
#include "GameData.h"
#include "GameRTTI.h"

namespace EventManager {

static ICriticalSection s_criticalSection;

//////////////////////
// Event definitions
/////////////////////

// Hook routines need to be forward declared so they can be used in EventInfo structs.
static void  InstallHook();
static void  InstallActivateHook();
static void	 InstallOnActorEquipHook();

enum {
	kEventMask_OnActivate		= 0x01000000,		// special case as OnActivate has no event mask
};

typedef void (* EventHookInstaller)();

typedef LinkedList<EventCallback>	CallbackList;

UnorderedMap<const char*, UInt32> s_eventNameToID(0x40);

UInt32 EventIDForString(const char* eventStr)
{
	UInt32 *idPtr = s_eventNameToID.GetPtr(eventStr);
	return idPtr ? *idPtr : kEventID_INVALID;
}

struct EventInfo
{
	EventInfo (const char *name_, UInt8* params_, UInt8 nParams_, UInt32 eventMask_, EventHookInstaller* installer_)
		: evName(name_), paramTypes(params_), numParams(nParams_), eventMask(eventMask_), installHook(installer_)
	{
		UInt32 eventID = s_eventNameToID.Size(), *idPtr;
		if (s_eventNameToID.Insert(evName, &idPtr))
			*idPtr = eventID;
	}
	EventInfo (const char *name_, UInt8 * params_, UInt8 numParams_) : evName(name_), paramTypes(params_), numParams(numParams_), eventMask(0), installHook(NULL)
		{ }
	EventInfo () : evName(""), paramTypes(NULL), numParams(0), eventMask(0), installHook(NULL)
		{ ; }
	EventInfo (const EventInfo& other) :
		evName(other.evName), 
		paramTypes(other.paramTypes),
		numParams(other.numParams), 
		callbacks(other.callbacks),
		eventMask(other.eventMask), 
		installHook(other.installHook)
		{ 
		}
	EventInfo& operator=(const EventInfo& other) {
		evName = other.evName;
		paramTypes = other.paramTypes;
		numParams = other.numParams;
		callbacks = other.callbacks;
		eventMask = other.eventMask;
		installHook = other.installHook;
		return *this;
	};

	const char			*evName;			// must be lowercase
	UInt8				*paramTypes;
	UInt8				numParams;
	UInt32				eventMask;
	CallbackList		callbacks;
	EventHookInstaller	*installHook;	// if a hook is needed for this event type, this will be non-null. 
										// install it once and then set *installHook to NULL. Allows multiple events
										// to use the same hook, installing it only once.
	
};

// hook installers
static EventHookInstaller s_MainEventHook = InstallHook;
static EventHookInstaller s_ActivateHook = InstallActivateHook;
static EventHookInstaller s_ActorEquipHook = InstallOnActorEquipHook;

// event handler param lists
static UInt8 kEventParams_GameEvent[2] =
{
	Script::eVarType_Ref, Script::eVarType_Ref
};

static UInt8 kEventParams_OneRef[1] =
{
	Script::eVarType_Ref,
};

static UInt8 kEventParams_OneString[1] =
{
	Script::eVarType_String
};

static UInt8 kEventParams_OneInteger[1] =
{
	Script::eVarType_Integer
};

static UInt8 kEventParams_TwoIntegers[2] =
{
	Script::eVarType_Integer, Script::eVarType_Integer
};

static UInt8 kEventParams_OneFloat_OneRef[2] =
{
	 Script::eVarType_Float, Script::eVarType_Ref
};

static UInt8 kEventParams_OneRef_OneInt[2] =
{
	Script::eVarType_Ref, Script::eVarType_Integer
};

static UInt8 kEventParams_OneArray[1] =
{
	Script::eVarType_Array
};

///////////////////////////
// internal functions
//////////////////////////

void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1);
void __stdcall HandleGameEvent(UInt32 eventMask, TESObjectREFR* source, TESForm* object);

static const UInt32 kVtbl_PlayerCharacter = 0x0108AA3C;
static const UInt32 kVtbl_Character = 0x01086A6C;
static const UInt32 kVtbl_Creature = 0x010870AC;
static const UInt32 kVtbl_ArrowProjectile = 0x01085954;
static const UInt32 kVtbl_MagicBallProjectile = 0x0107A554;
static const UInt32 kVtbl_MagicBoltProjectile = 0x0107A8F4;
static const UInt32 kVtbl_MagicFogProjectile = 0x0107AD84;
static const UInt32 kVtbl_MagicSprayProjectile = 0x0107B8C4;
static const UInt32 kVtbl_TESObjectREFR = 0x0102F55C;

static const UInt32 kMarkEvent_HookAddr = 0x005AC750;
static const UInt32 kMarkEvent_RetnAddr = 0x005AC756;

static const UInt32 kActivate_HookAddr = 0x0057318E;
static const UInt32 kActivate_RetnAddr = 0x00573194;

static const UInt32 kDestroyCIOS_HookAddr = 0x008232B5;
static const UInt32 kDestroyCIOS_RetnAddr = 0x008232EF;

// cheap check to prevent duplicate events being processed in immediate succession
// (e.g. game marks OnHitWith twice per event, this way we only process it once)
static TESObjectREFR* s_lastObj = NULL;
static TESForm* s_lastTarget = NULL;
static UInt32 s_lastEvent = NULL;

// OnHitWith often gets marked twice per event. If weapon enchanted, may see:
//  OnHitWith >> OnMGEFHit >> ... >> OnHitWith. 
// Prevent OnHitWith being reported more than once by recording OnHitWith events processed
static TESObjectREFR* s_lastOnHitWithActor = NULL;
static TESForm* s_lastOnHitWithWeapon = NULL;

// And it turns out OnHit is annoyingly marked several times per frame for spells/enchanted weapons
static TESObjectREFR* s_lastOnHitVictim = NULL;
static TESForm* s_lastOnHitAttacker = NULL;

//////////////////////////////////
// Hooks
/////////////////////////////////

UInt32 s_eventsInUse = 0;

static __declspec(naked) void MarkEventHook(void)
{
	__asm
	{
		// overwritten code
		push	ebp
		mov		ebp, esp
		sub		esp, 8

		// grab args
		mov		eax, [ebp+0xC]			// ExtraDataList*
		test	eax, eax
		jz		skipHandle

		mov		edx, [ebp+0x10]
		test	s_eventsInUse, edx
		jz		skipHandle

		push	dword ptr [ebp+8]		// target
		sub		eax, 0x44				// TESObjectREFR* thisObj
		push	eax
		push	edx						// event mask
		call	HandleGameEvent

	skipHandle:
		jmp		kMarkEvent_RetnAddr
	}
}		

void InstallHook()
{
	WriteRelJump(kMarkEvent_HookAddr, (UInt32)&MarkEventHook);
}

static __declspec(naked) void DestroyCIOSHook(void)
{
	__asm
	{
		test	ecx, ecx
		jz		doHook
		mov		edx, [ecx]
		ret
	doHook:
		fldz
		jmp		kDestroyCIOS_RetnAddr
	}
}

static void InstallDestroyCIOSHook()
{
	WriteRelCall(kDestroyCIOS_HookAddr, (UInt32)&DestroyCIOSHook);
}

static __declspec(naked) void OnActorEquipHook(void)
{
	__asm
	{
		test	s_eventsInUse, 2
		jz		skipHandle

		push	dword ptr [ebp+8]
		push	dword ptr [ebp+0x10]
		push	2
		call	HandleGameEvent
		mov		ecx, [ebp-0xA8]

	skipHandle:
		mov		eax, 0x4BF0E0
		jmp		eax
	}
}

static void InstallOnActorEquipHook()
{
	static const UInt32 kOnActorEquipHookAddr = 0x004C0032;
	if (s_MainEventHook) {
		// OnActorEquip event also (unreliably) passes through main hook, so install that
		s_MainEventHook();
		// since it's installed, prevent it from being installed again
		s_MainEventHook = NULL;
	}

	// additional hook to overcome game's failure to consistently mark this event type

	// OBSE: The issue is that our Console_Print routine interacts poorly with the game's debug text (turned on with TDT console command)
	// OBSE: when called from a background thread.
	// OBSE: So if the handler associated with this event calls Print, PrintC, etc, there is a chance it will crash.
	//
	// Fix:
	// Added s_InsideOnActorEquipHook to neutralize Print during OnEquip events, with an optional s_CheckInsideOnActorEquipHook to bypass it for testing.
	WriteRelCall(kOnActorEquipHookAddr, (UInt32)&OnActorEquipHook);
}

static __declspec(naked) void TESObjectREFR_ActivateHook(void)
{
	__asm
	{
		test	s_eventsInUse, kEventMask_OnActivate
		jz		skipHandle

		push	dword ptr [ebp+8]	// activating refr
		push	ecx					// this
		push	kEventMask_OnActivate
		call	HandleGameEvent
		mov		ecx, [ebp-0x12C]

	skipHandle:
		jmp		kActivate_RetnAddr
	}
}

void InstallActivateHook()
{
	WriteRelJump(kActivate_HookAddr, (UInt32)&TESObjectREFR_ActivateHook);
}

UInt32 EventIDForMask(UInt32 eventMask)
{
	switch (eventMask) {
		case ScriptEventList::kEvent_OnAdd:
			return kEventID_OnAdd;
		case ScriptEventList::kEvent_OnActorEquip:
			return kEventID_OnActorEquip;
		case ScriptEventList::kEvent_OnDrop:
			return kEventID_OnDrop;
		case ScriptEventList::kEvent_OnActorUnequip:
			return kEventID_OnActorUnequip;
		case ScriptEventList::kEvent_OnDeath:
			return kEventID_OnDeath;
		case ScriptEventList::kEvent_OnMurder:
			return kEventID_OnMurder;
		case ScriptEventList::kEvent_OnCombatEnd:
			return kEventID_OnCombatEnd;
		case ScriptEventList::kEvent_OnHit:
			return kEventID_OnHit;
		case ScriptEventList::kEvent_OnHitWith:
			return kEventID_OnHitWith;
		case ScriptEventList::kEvent_OnPackageStart:
			return kEventID_OnPackageStart;
		case ScriptEventList::kEvent_OnPackageDone:
			return kEventID_OnPackageDone;
		case ScriptEventList::kEvent_OnPackageChange:
			return kEventID_OnPackageChange;
		case ScriptEventList::kEvent_OnLoad:
			return kEventID_OnLoad;
		case ScriptEventList::kEvent_OnMagicEffectHit:
			return kEventID_OnMagicEffectHit;
		case ScriptEventList::kEvent_OnSell:
			return kEventID_OnSell;
		case ScriptEventList::kEvent_OnStartCombat:
			return kEventID_OnStartCombat;
		case ScriptEventList::kEvent_SayToDone:
			return kEventID_SayToDone;
		case ScriptEventList::kEvent_OnFire:
			return kEventID_OnFire;
		case ScriptEventList::kEvent_OnOpen:
			return kEventID_OnOpen;
		case ScriptEventList::kEvent_OnClose:
			return kEventID_OnClose;
		case ScriptEventList::kEvent_OnGrab:
			return kEventID_OnGrab;
		case ScriptEventList::kEvent_OnTrigger:
			return kEventID_OnTrigger;
		case ScriptEventList::kEvent_OnTriggerEnter:
			return kEventID_OnTriggerEnter;
		case ScriptEventList::kEvent_OnTriggerLeave:
			return kEventID_OnTriggerLeave;
		case ScriptEventList::kEvent_OnReset:
			return kEventID_OnReset;
		case kEventMask_OnActivate:
			return kEventID_OnActivate;
		default:
			return kEventID_INVALID;
	}
}

UInt32 EventIDForMessage(UInt32 msgID)
{
	switch (msgID) {
		case NVSEMessagingInterface::kMessage_ExitGame:
			return kEventID_ExitGame;
		case NVSEMessagingInterface::kMessage_ExitToMainMenu:
			return kEventID_ExitToMainMenu;
		case NVSEMessagingInterface::kMessage_LoadGame:
			return kEventID_LoadGame;
		case NVSEMessagingInterface::kMessage_SaveGame:
			return kEventID_SaveGame;
		case NVSEMessagingInterface::kMessage_ExitGame_Console:
			return kEventID_QQQ;
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			return kEventID_PostLoadGame;
		case NVSEMessagingInterface::kMessage_RuntimeScriptError:
			return kEventID_RuntimeScriptError;
		case NVSEMessagingInterface::kMessage_DeleteGame:
			return kEventID_DeleteGame;
		case NVSEMessagingInterface::kMessage_RenameGame:
			return kEventID_RenameGame;
		case NVSEMessagingInterface::kMessage_RenameNewGame:
			return kEventID_RenameNewGame;
		case NVSEMessagingInterface::kMessage_NewGame:
			return kEventID_NewGame;
		case NVSEMessagingInterface::kMessage_DeleteGameName:
			return kEventID_DeleteGameName;
		case NVSEMessagingInterface::kMessage_RenameGameName:
			return kEventID_RenameGameName;
		case NVSEMessagingInterface::kMessage_RenameNewGameName:
			return kEventID_RenameNewGameName;
		default:
			return kEventID_INVALID;
	}
}

typedef Vector<EventInfo> EventInfoList;
static EventInfoList s_eventInfos(0x30);

bool EventCallback::Equals(const EventCallback& rhs) const
{
	return (script == rhs.script) && (object == rhs.object) && (source == rhs.source);
}

class EventHandlerCaller : public InternalFunctionCaller
{
public:
	EventHandlerCaller(Script* script, EventInfo* eventInfo, void* arg0, void* arg1) : InternalFunctionCaller(script, NULL), m_eventInfo(eventInfo)
	{
		UInt8 numArgs = 2;
		if (!arg1)
			numArgs = 1;
		if (!arg0)
			numArgs = 0;

		SetArgs(numArgs, arg0, arg1);
	}

	virtual bool ValidateParam(UserFunctionParam* param, UInt8 paramIndex)
	{
		return param->varType == m_eventInfo->paramTypes[paramIndex];
	}

	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) {
		// make sure we've got the same # of args as expected by event handler
		DynamicParamInfo& dParams = info->ParamInfo();
		if (dParams.NumParams() != m_eventInfo->numParams || dParams.NumParams() > 2) {
			ShowRuntimeError(m_script, "Number of arguments to function script does not match those expected for event");
			return false;
		}

		return InternalFunctionCaller::PopulateArgs(eventList, info);
	}

private:
	EventInfo		* m_eventInfo;
};

bool IsValidReference(void* refr)
{
	bool bIsRefr = false;
	__try
	{
		if ((*(UInt8*)refr & 4) && ((((UInt16*)refr)[1] == 0x108) || (*(UInt32*)refr == 0x102F55C)))
			bIsRefr = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bIsRefr = false;
	}

	return bIsRefr;
}

// stack of event names pushed when handler invoked, popped when handler returns
// used by GetCurrentEventName
Stack<const char*> s_eventStack;

// some events are best deferred until Tick() invoked rather than being handled immediately
// this stores info about such an event. Currently unused.
struct DeferredCallback
{
	EventCallback		*callback;
	void				*arg0;
	void				*arg1;
	EventInfo			*eventInfo;

	DeferredCallback(EventCallback *pCallback, void *_arg0, void *_arg1, EventInfo *_eventInfo) : callback(pCallback), arg0(_arg0), arg1(_arg1), eventInfo(_eventInfo) {}

	~DeferredCallback()
	{
		if (callback->removed) return;

		s_eventStack.Push(eventInfo->evName);
		ScriptToken *result = UserFunctionManager::Call(EventHandlerCaller(callback->script, eventInfo, arg0, arg1));
		s_eventStack.Pop();

		// result is unused
		if (result) delete result;
	}
};

typedef Stack<DeferredCallback> DeferredCallbackList;
DeferredCallbackList s_deferredCallbacks;

struct DeferredRemoveCallback
{
	EventInfo				*eventInfo;
	CallbackList::Iterator	iterator;

	DeferredRemoveCallback(EventInfo *pEventInfo, const CallbackList::Iterator &iter) : eventInfo(pEventInfo), iterator(iter) {}

	~DeferredRemoveCallback()
	{
		if (iterator.Get().removed)
		{
			eventInfo->callbacks.Remove(iterator);
			if (eventInfo->callbacks.Empty() && eventInfo->eventMask)
				s_eventsInUse &= ~eventInfo->eventMask;
		}
		else iterator.Get().pendingRemove = false;
	}
};

typedef Stack<DeferredRemoveCallback> DeferredRemoveList;
DeferredRemoveList s_deferredRemoveList;

void __stdcall HandleEvent(UInt32 id, void* arg0, void* arg1)
{
	ScopedLock lock(s_criticalSection);

	EventInfo* eventInfo = &s_eventInfos[id];
	if (eventInfo->callbacks.Empty()) return;

	for (auto iter = eventInfo->callbacks.Begin(); !iter.End(); ++iter)
	{
		EventCallback &callback = iter.Get();

		if (callback.IsRemoved())
			continue;

		// Check filters
		if (callback.source && (arg0 != callback.source))
		{
			if (!IsValidReference(arg0) || (((TESObjectREFR*)arg0)->baseForm != callback.source))
				continue;
		}

		if (callback.object && (callback.object != arg1))
			continue;

		if (GetCurrentThreadId() != g_mainThreadID)
		{
			// avoid potential issues with invoking handlers outside of main thread by deferring event handling
			s_deferredCallbacks.Push(&callback, arg0, arg1, eventInfo);
		}
		else
		{
			// handle immediately
			s_eventStack.Push(eventInfo->evName);
			ScriptToken* result = UserFunctionManager::Call(EventHandlerCaller(callback.script, eventInfo, arg0, arg1));
			s_eventStack.Pop();

			// result is unused
			if (result)	delete result;
		}
	}
}

////////////////
// public API
///////////////

const char* GetCurrentEventName()
{
	ScopedLock lock(s_criticalSection);

	return s_eventStack.Empty() ? "" : s_eventStack.Top();
}

static UInt32 recursiveLevel = 0;

bool SetHandler(const char* eventName, EventCallback& handler)
{
	UInt32 setted = 0;

	// trying to use a FormList to specify the source filter
	if (handler.source && handler.source->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.source;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
		{
			EventCallback listHandler(handler.script, iter.Get(), handler.object);
			recursiveLevel++;
			if (SetHandler(eventName, listHandler)) setted++;
			recursiveLevel--;
		}
		return setted>0;
	}

	// trying to use a FormList to specify the object filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
		{
			EventCallback listHandler(handler.script, handler.source, iter.Get());
			recursiveLevel++;
			if (SetHandler(eventName, listHandler)) setted++;
			recursiveLevel--;
		}
		return setted>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 *idPtr;
	if (s_eventNameToID.Insert(eventName, &idPtr))
	{
		// have to assume registering for a user-defined event which has not been used before this point
		*idPtr = s_eventInfos.Size();
		char *nameCopy = CopyString(eventName);
		StrToLower(nameCopy);
		s_eventInfos.Append(nameCopy, kEventParams_OneArray, 1);
	}

	UInt32 id = *idPtr;

	if (id < s_eventInfos.Size())
	{
		EventInfo* info = &s_eventInfos[id];
		// is hook installed for this event type?
		if (info->installHook)
		{
			if (*(info->installHook))
			{
				// if this hook is used by more than one event type, prevent it being installed a second time
				(*info->installHook)();
				*(info->installHook) = NULL;
			}
			// mark event as having had its hook installed
			info->installHook = NULL;
		}

		if (!info->callbacks.Empty())
		{
			// if an existing handler matches this one exactly, don't duplicate it
			for (auto iter = info->callbacks.Begin(); !iter.End(); ++iter)
			{
				if (iter.Get().Equals(handler))
				{
					// may be re-adding a previously removed handler, so clear the Removed flag
					iter.Get().SetRemoved(false);
					return false;
				}
			}
		}

		info->callbacks.Append(handler);

		s_eventsInUse |= info->eventMask;

		return true;
	}

	return false; 
}

bool RemoveHandler(const char* id, EventCallback& handler)
{
	UInt32 removed = 0;

	// trying to use a FormList to specify the source filter
	if (handler.source && handler.source->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.source;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
		{
			EventCallback listHandler(handler.script, iter.Get(), handler.object);
			recursiveLevel++;
			if (RemoveHandler(id, listHandler)) removed++;
			recursiveLevel--;
		}
		return removed>0;
	}

	// trying to use a FormList to specify the object filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
		{
			EventCallback listHandler(handler.script, handler.source, iter.Get());
			recursiveLevel++;
			if (RemoveHandler(id, listHandler)) removed++;
			recursiveLevel--;
		}
		return removed>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 eventType = EventIDForString(id);
	bool bRemovedAtLeastOne = false;
	if (eventType < s_eventInfos.Size())
	{
		EventInfo *eventInfo = &s_eventInfos[eventType];
		if (!eventInfo->callbacks.Empty())
		{
			for (auto iter = eventInfo->callbacks.Begin(); !iter.End(); ++iter)
			{
				EventCallback &callback = iter.Get();

				if (handler.script != callback.script)
					continue;

				if (handler.source && (handler.source != callback.source))
					continue;

				if (handler.object && (handler.object != callback.object))
					continue;

				callback.SetRemoved(true);

				if (!callback.pendingRemove)
				{
					callback.pendingRemove = true;
					s_deferredRemoveList.Push(eventInfo, iter);
				}

				bRemovedAtLeastOne = true;
			}
		}
	}
	
	return bRemovedAtLeastOne;
}

void __stdcall HandleGameEvent(UInt32 eventMask, TESObjectREFR* source, TESForm* object)
{
	if (!IsValidReference(source)) {
		return;
	}

	ScopedLock lock(s_criticalSection);

	// ScriptEventList can be marked more than once per event, cheap check to prevent sending duplicate events to scripts
	if (source != s_lastObj || object != s_lastTarget || eventMask != s_lastEvent)
	{
		s_lastObj = source;
		s_lastEvent = eventMask;
		s_lastTarget = object;
	}
	else {
		// duplicate event, ignore it
		return;
	}

	UInt32 eventID = EventIDForMask(eventMask);
	if (eventID != kEventID_INVALID)
	{
		if (eventID == kEventID_OnHitWith)
		{
			// special check for OnHitWith, since it gets called redundantly
			if (source != s_lastOnHitWithActor || object != s_lastOnHitWithWeapon)
			{
				s_lastOnHitWithActor = source;
				s_lastOnHitWithWeapon = object;
				HandleEvent(eventID, source, object);
			}
		}
		else if (eventID == kEventID_OnHit)
		{
			if (source != s_lastOnHitVictim || object != s_lastOnHitAttacker)
			{
				s_lastOnHitVictim = source;
				s_lastOnHitAttacker = object;
				HandleEvent(eventID, source, object);
			}
		}
		else
			HandleEvent(eventID, source, object);
	}
}

void HandleNVSEMessage(UInt32 msgID, void* data)
{
	UInt32 eventID = EventIDForMessage(msgID);
	if (eventID != kEventID_INVALID)
		HandleEvent(eventID, data, NULL);
}

bool DispatchUserDefinedEvent (const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName)
{
	ScopedLock lock(s_criticalSection);

	// does an EventInfo entry already exist for this event?
	UInt32 eventID = EventIDForString (eventName);
	if (kEventID_INVALID == eventID)
		return true;

	// get or create args array
	ArrayVar *arr;
	if (argsArrayId)
	{
		arr = g_ArrayMap.Get(argsArrayId);
		if (!arr || (arr->KeyType() != kDataType_String))
			return false;
	}
	else
	{
		arr = g_ArrayMap.Create (kDataType_String, false, sender->GetModIndex ());
		argsArrayId = arr->ID();
	}

	// populate args array
	arr->SetElementString("eventName", eventName);
	if (senderName == NULL)
	{
		if (sender)
			senderName = DataHandler::Get()->GetNthModName (sender->GetModIndex ());
		else
			senderName = "NVSE";
	}

	arr->SetElementString("eventSender", senderName);

	// dispatch
	HandleEvent(eventID, (void*)argsArrayId, NULL);
	return true;
}

void Tick()
{
	ScopedLock lock(s_criticalSection);

	// handle deferred events
	s_deferredCallbacks.Clear();

	// Clear callbacks pending removal.
	s_deferredRemoveList.Clear();

	s_lastObj = NULL;
	s_lastTarget = NULL;
	s_lastEvent = NULL;
	s_lastOnHitWithActor = NULL;
	s_lastOnHitWithWeapon = NULL;
	s_lastOnHitVictim = NULL;
	s_lastOnHitAttacker = NULL;
}

void Init()
{
#define EVENT_INFO(name, params, hookInstaller, eventMask) s_eventInfos.Append(name, params, params ? sizeof(params) : 0, eventMask, hookInstaller)


	EVENT_INFO("onadd", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnAdd);
	EVENT_INFO("onactorequip", kEventParams_GameEvent, &s_ActorEquipHook, ScriptEventList::kEvent_OnEquip);
	EVENT_INFO("ondrop", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnDrop);
	EVENT_INFO("onactorunequip", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnUnequip);
	EVENT_INFO("ondeath", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnDeath);
	EVENT_INFO("onmurder", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnMurder);
	EVENT_INFO("oncombatend", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnCombatEnd);
	EVENT_INFO("onhit", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnHit);
	EVENT_INFO("onhitwith", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnHitWith);
	EVENT_INFO("onpackagechange", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnPackageChange);
	EVENT_INFO("onpackagestart", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnPackageStart);
	EVENT_INFO("onpackagedone", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnPackageDone);
	EVENT_INFO("onload", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnLoad);
	EVENT_INFO("onmagiceffecthit", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnMagicEffectHit);
	EVENT_INFO("onsell", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnSell);
	EVENT_INFO("onstartcombat", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnStartCombat);
	EVENT_INFO("saytodone", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_SayToDone);
	EVENT_INFO("ongrab", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnGrab);
	EVENT_INFO("onopen", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnOpen);
	EVENT_INFO("onclose", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnClose);
	EVENT_INFO("onfire", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnFire);
	EVENT_INFO("ontrigger", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnTrigger);
	EVENT_INFO("ontriggerenter", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnTriggerEnter);
	EVENT_INFO("ontriggerleave", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnTriggerLeave);
	EVENT_INFO("onreset", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnReset);

	EVENT_INFO("onactivate", kEventParams_GameEvent, &s_ActivateHook, kEventMask_OnActivate);
	EVENT_INFO("ondropitem", kEventParams_GameEvent, &s_MainEventHook, 0);

	EVENT_INFO("exitgame", nullptr, nullptr, 0);
	EVENT_INFO("exittomainmenu", nullptr, nullptr, 0);
	EVENT_INFO("loadgame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("savegame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("qqq", nullptr, nullptr, 0);
	EVENT_INFO("postloadgame", kEventParams_OneInteger, nullptr, 0);
	EVENT_INFO("runtimescripterror", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("deletegame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamegame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamenewgame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("newgame", nullptr, nullptr, 0);
	EVENT_INFO("deletegamename", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamegamename", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamenewgamename", kEventParams_OneString, nullptr, 0);

	s_eventNameToID["onequip"] = kEventID_OnActorEquip;
	s_eventNameToID["onunequip"] = kEventID_OnActorUnequip;
	s_eventNameToID["on0x0080000"] = kEventID_OnGrab;
	s_eventNameToID["on0x00400000"] = kEventID_OnFire;

	ASSERT (kEventID_InternalMAX == s_eventInfos.Size());

#undef EVENT_INFO

	InstallDestroyCIOSHook();	// handle a missing parameter value check.

}


};	// namespace

