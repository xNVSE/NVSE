#include <list>
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

void __stdcall HandleEventForCallingObject(UInt32 id, TESObjectREFR* callingObj, void* arg0, void* arg1);

static ICriticalSection s_criticalSection;

//////////////////////
// Event definitions
/////////////////////

// Hook routines need to be forward declared so they can be used in EventInfo structs.
static void  InstallHook();
static void  InstallActivateHook();
static void	 InstallOnActorEquipHook();

enum {
	kEventMask_OnActivate		= 0xFFFFFFFF,		// special case as OnActivate has no event mask
};

typedef void (* EventHookInstaller)();

typedef std::list<EventCallback>	CallbackList;

struct EventInfo
{
	EventInfo (std::string const& name_, UInt8* params_, UInt8 nParams_, bool defer_, EventHookInstaller* installer_)
		: name(name_), paramTypes(params_), numParams(nParams_), isDeferred(defer_), callbacks(NULL), installHook(installer_)
		{ MakeLower (name); }
	EventInfo (std::string const& name_, UInt8 * params_, UInt8 numParams_) : name(name_), paramTypes(params_), numParams(numParams_), isDeferred(false), callbacks(NULL), installHook(NULL)
		{ MakeLower (name); }
	EventInfo () : name(""), paramTypes(NULL), numParams(0), isDeferred(false), callbacks(NULL), installHook(NULL)
		{ ; }
	EventInfo (const EventInfo& other) :
		name(other.name), 
		paramTypes(other.paramTypes),
		numParams(other.numParams), 
		callbacks(other.callbacks ? new CallbackList(*(other.callbacks)) : NULL),
		isDeferred(other.isDeferred), 
		installHook(other.installHook)
		{ 
		}
	~EventInfo();
	EventInfo& operator=(const EventInfo& other) {
		name = other.name;
		paramTypes = other.paramTypes;
		numParams = other.numParams;
		callbacks = other.callbacks ? new CallbackList(*(other.callbacks)) : NULL;
		isDeferred = other.isDeferred;
		installHook = other.installHook;
		return *this;
	};

	std::string			name;			// must be lowercase
	UInt8				* paramTypes;
	UInt8				numParams;
	bool				isDeferred;		// dispatch event in Tick() instead of immediately - currently unused
	CallbackList		* callbacks;
	EventHookInstaller	* installHook;	// if a hook is needed for this event type, this will be non-null. 
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

static const UInt32 kActivate_HookAddr = 0x00573183;
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

static __declspec(naked) void MarkEventHook(void)
{
	__asm {
		// overwritten code
		push    ebp
		mov     ebp, esp
		sub     esp, 8

		// grab args
		pushad
		mov	eax, [ebp+0x0C]			// ExtraDataList*
		test eax, eax
		jz	XDataListIsNull

	//XDataListIsNotNull:
		sub eax, 0x44				// TESObjectREFR* thisObj
		mov ecx, [ebp+0x10]			// event mask
		mov edx, [ebp+0x08]			// target

		push edx
		push eax
		push ecx
		call HandleGameEvent

	XDataListIsNull:
		popad
		jmp [kMarkEvent_RetnAddr]
	}
}		

void InstallHook()
{
	WriteRelJump(kMarkEvent_HookAddr, (UInt32)&MarkEventHook);
}

static __declspec(naked) void DestroyCIOSHook(void)
{
	__asm {
		pushad
		cmp ecx, 0
		jz doHook
		popad
		mov edx, [ecx]
		ret

doHook:
		popad
		fldz
		jmp	[kDestroyCIOS_RetnAddr]
	}
}

static void InstallDestroyCIOSHook()
{
	WriteRelCall(kDestroyCIOS_HookAddr, (UInt32)&DestroyCIOSHook);
}

static __declspec(naked) void OnActorEquipHook(void)
{
	static const UInt32 s_callAddr = 0x004BF0E0;
	static const UInt32 kEventMask = ScriptEventList::kEvent_OnActorEquip;
	__asm {
		pushad
		push [ebp+0x08]
		push [ebp+0x10]
		push [kEventMask]
		mov [s_InsideOnActorEquipHook], 0x0FFFFFFFF
		call HandleGameEvent
		mov [s_InsideOnActorEquipHook], 0x0
		popad

		jmp	[s_callAddr]
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
	__asm {
		pushad
		push [ebp+0x08]			// activating refr
		push ecx				// this
		mov eax, kEventMask_OnActivate
		push eax
		call HandleGameEvent
		popad

		// overwritten code
		push esi
		mov	[ebp-0x012C], ecx
		mov	byte ptr [ebp-1], 0

		jmp	[kActivate_RetnAddr]
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
		case ScriptEventList::kEvent_0x00400000:
			return kEventID_0x00400000;
		case ScriptEventList::kEvent_OnOpen:
			return kEventID_OnOpen;
		case ScriptEventList::kEvent_OnClose:
			return kEventID_OnClose;
		case ScriptEventList::kEvent_OnOpen2:
			return kEventID_OnOpen;
		case ScriptEventList::kEvent_OnClose2:
			return kEventID_OnClose;
		case ScriptEventList::kEvent_0x00080000:
			return kEventID_0x00080000;
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
		case NVSEMessagingInterface::kMessage_DeferredInit:
			return kEventID_DeferredInit;
		default:
			return kEventID_INVALID;
	}
}

typedef std::vector<EventInfo> EventInfoList;
static EventInfoList s_eventInfos;

UInt32 EventIDForString(const char* eventStr)
{
	std::string name(eventStr);
	MakeLower(name);
	eventStr = name.c_str();
	UInt32 numEventInfos = s_eventInfos.size ();
	for (UInt32 i = 0; i < numEventInfos; i++) {
		if (!strcmp(eventStr, s_eventInfos[i].name.c_str())) {
			return i;
		}
	}

	return kEventID_INVALID;
}

bool EventCallback::Equals(const EventCallback& rhs) const
{
	return (script == rhs.script &&
		object == rhs.object &&
		source == rhs.source &&
		callingObj == rhs.callingObj);
}

class EventHandlerCaller : public InternalFunctionCaller
{
public:
	EventHandlerCaller(Script* script, EventInfo* eventInfo, void* arg0, void* arg1, TESObjectREFR* callingObj = NULL)
		: InternalFunctionCaller(script, callingObj), m_eventInfo(eventInfo)
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

bool TryGetReference(TESObjectREFR* refr)
{
	// ### HACK HACK HACK
	// MarkEventList() may have been called for a BaseExtraList not associated with a TESObjectREFR
	bool bIsRefr = false;
	__try 
	{
		switch (*((UInt32*)refr)) {
			case kVtbl_PlayerCharacter:
			case kVtbl_Character:
			case kVtbl_Creature:
			case kVtbl_ArrowProjectile:
			case kVtbl_MagicBallProjectile:
			case kVtbl_MagicBoltProjectile:
			case kVtbl_MagicFogProjectile:
			case kVtbl_MagicSprayProjectile:
			case kVtbl_TESObjectREFR:
				bIsRefr = true;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
		bIsRefr = false;
	}

	return bIsRefr;
}

// stack of event names pushed when handler invoked, popped when handler returns
// used by GetCurrentEventName
std::stack<std::string> s_eventStack;

// some events are best deferred until Tick() invoked rather than being handled immediately
// this stores info about such an event. Currently unused.
struct DeferredCallback
{
	DeferredCallback(CallbackList::iterator& _iter, TESObjectREFR* _callingObj, void* _arg0, void* _arg1, EventInfo* _eventInfo)
		: iterator(_iter), callingObj(_callingObj), arg0(_arg0), arg1(_arg1), eventInfo(_eventInfo) { }

	CallbackList::iterator	iterator;
	TESObjectREFR			* callingObj;
	void					* arg0;
	void					* arg1;
	EventInfo				* eventInfo;
};

typedef std::list<DeferredCallback> DeferredCallbackList;
DeferredCallbackList s_deferredCallbacks;

void __stdcall HandleEventForCallingObject(UInt32 id, TESObjectREFR* callingObj, void* arg0, void* arg1)
{
	ScopedLock lock(s_criticalSection);

	EventInfo* eventInfo = &s_eventInfos[id];
	if (eventInfo->callbacks) {
		for (CallbackList::iterator iter = eventInfo->callbacks->begin(); iter != eventInfo->callbacks->end(); ) {
			if (iter->IsRemoved()) {
				if (!iter->IsInUse()) {
					iter = eventInfo->callbacks->erase(iter);
				}
				else {
					++iter;
				}

				continue;
			}

			// Check filters
			if (iter->source) {
				bool match = ((TESForm*)arg0 == iter->source);
				if (!match && TryGetReference((TESObjectREFR*)arg0))
				{
					TESObjectREFR* source = (TESObjectREFR*)arg0;
					if (source->baseForm == iter->source)
						match = true;
				}
				if (!match)
				{
					++iter;
					continue;
				}
			}

			if (iter->callingObj && !(callingObj == iter->callingObj)) {
				++iter;
				continue;
			}

			if (iter->object) {
				if (!(iter->object == (TESForm*)arg1)) {
					++iter;
					continue;
				}
			}

			if (eventInfo->isDeferred || GetCurrentThreadId() != g_mainThreadID) {
				// avoid potential issues with invoking handlers outside of main thread by deferring event handling
				if (!iter->IsRemoved()) {
					iter->SetInUse(true);
					s_deferredCallbacks.push_back(DeferredCallback(iter, callingObj, arg0, arg1, eventInfo));
					++iter;
				}
			}
			else {
				// handle immediately
				bool bWasInUse = iter->IsInUse();
				iter->SetInUse(true);
				s_eventStack.push(eventInfo->name);
				ScriptToken* result = UserFunctionManager::Call(EventHandlerCaller(iter->script, eventInfo, arg0, arg1, callingObj));
				s_eventStack.pop();
				iter->SetInUse(bWasInUse);

				// it's possible the handler decided to remove itself, so take care of that, being careful
				// not to remove a callback that is needed for deferred invocation
				if (!bWasInUse && iter->IsRemoved()) {
					iter = eventInfo->callbacks->erase(iter);
				}
				else {
					++iter;
				}

				// result is unused
				delete result;
			}
		}
	}
}

void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1)
{
	// initial implementation didn't support a calling object; pass through to impl which does
	HandleEventForCallingObject(id, NULL, arg0, arg1);
}

////////////////
// public API
///////////////

std::string GetCurrentEventName()
{
	ScopedLock lock(s_criticalSection);

	return s_eventStack.size() ? s_eventStack.top() : "";
}

static UInt32 recursiveLevel = 0;

bool SetHandler(const char* eventName, EventCallback& handler)
{
	UInt32 setted = 0;

	// trying to use a FormList to specify the source filter
	if (handler.source && handler.source->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.source;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, iter.Get(), handler.object, handler.callingObj);
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
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
			recursiveLevel++;
			if (SetHandler(eventName, listHandler)) setted++;
			recursiveLevel--;
		}
		return setted>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 id = EventIDForString (eventName);
	if (kEventID_INVALID == id)
	{
		// have to assume registering for a user-defined event which has not been used before this point
		id = s_eventInfos.size();
		s_eventInfos.push_back (EventInfo (eventName, kEventParams_OneArray, 1));
	}

	if (id < s_eventInfos.size()) {
		EventInfo* info = &s_eventInfos[id];
		// is hook installed for this event type?
		if (info->installHook) {
			if (*(info->installHook)) {
				// if this hook is used by more than one event type, prevent it being installed a second time
				(*info->installHook)();
				*(info->installHook) = NULL;
			}
			// mark event as having had its hook installed
			info->installHook = NULL;
		}

		if (!info->callbacks) {
			info->callbacks = new CallbackList();
		}
		else {
			// if an existing handler matches this one exactly, don't duplicate it
			for (CallbackList::iterator iter = info->callbacks->begin(); iter != info->callbacks->end(); ++iter) {
				if (iter->Equals(handler)) {
					// may be re-adding a previously removed handler, so clear the Removed flag
					iter->SetRemoved(false);
					return false;
				}
			}
		}

		info->callbacks->push_back(handler);
		return true;
	}
	else {
		return false; 
	}
}

bool RemoveHandler(const char* id, EventCallback& handler)
{
	UInt32 removed = 0;

	// trying to use a FormList to specify the filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
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
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
			recursiveLevel++;
			if (RemoveHandler(id, listHandler)) removed++;
			recursiveLevel--;
		}
		return removed>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 eventType = EventIDForString(id);
	bool bRemovedAtLeastOne = false;
	if (eventType < s_eventInfos.size() && s_eventInfos[eventType].callbacks) {
		CallbackList* callbacks = s_eventInfos[eventType].callbacks;
		for (CallbackList::iterator iter = callbacks->begin(); iter != callbacks->end(); ) {
			if (iter->script == handler.script) {
				bool bMatches = true;
				/* if ((eventType == kEventID_OnHealthDamage && handler.callingObj && handler.callingObj != iter->object) ||	// OnHealthDamage special-casing
					handler.object && handler.object != iter->object) {
					bMatches = false;
				}
				else */
				if (handler.source && handler.source != iter->source) {
					bMatches = false;
				}

				if (bMatches) {
					if (iter->IsInUse()) {
						// this handler is currently active, flag it for later removal
						iter->SetRemoved(true);
						++iter;
					}
					else {
						iter = callbacks->erase(iter);
					}

					bRemovedAtLeastOne = true;
				}
				else {
					++iter;
				}
			}
			else {
				++iter;
			}
		}
	}
	
	return bRemovedAtLeastOne;
}

void __stdcall HandleGameEvent(UInt32 eventMask, TESObjectREFR* source, TESForm* object)
{
	if (!TryGetReference(source)) {
		return;
	}

	ScopedLock lock(s_criticalSection);

	// ScriptEventList can be marked more than once per event, cheap check to prevent sending duplicate events to scripts
	if (source != s_lastObj || object != s_lastTarget || eventMask != s_lastEvent) {
		s_lastObj = source;
		s_lastEvent = eventMask;
		s_lastTarget = object;
	}
	else {
		// duplicate event, ignore it
		return;
	}

	UInt32 eventID = EventIDForMask(eventMask);
	if (eventID != kEventID_INVALID) {
		if (eventID == kEventID_OnHitWith) {
			// special check for OnHitWith, since it gets called redundantly
			if (source != s_lastOnHitWithActor || object != s_lastOnHitWithWeapon) {
				s_lastOnHitWithActor = source;
				s_lastOnHitWithWeapon = object;
				HandleEvent(eventID, source, object);
			}
		}
		else if (eventID == kEventID_OnHit) {
			if (source != s_lastOnHitVictim || object != s_lastOnHitAttacker) {
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
	if (argsArrayId == 0)
		argsArrayId = g_ArrayMap.Create (kDataType_String, false, sender->GetModIndex ());
	else if (!g_ArrayMap.Exists (argsArrayId) || g_ArrayMap.GetKeyType (argsArrayId) != kDataType_String)
		return false;

	// populate args array
	g_ArrayMap.SetElementString (argsArrayId, "eventName", eventName);
	if (NULL == senderName)
		if (sender)
			senderName = DataHandler::Get()->GetNthModName (sender->GetModIndex ());
		else
			senderName = "NVSE";

	g_ArrayMap.SetElementString (argsArrayId, "eventSender", senderName);

	// dispatch
	HandleEvent (eventID, (void*)argsArrayId, NULL);
	return true;
}

EventInfo::~EventInfo()
{
	if (callbacks) {
		delete callbacks;
		callbacks = NULL;
	}
}

typedef std::list< DeferredCallbackList::iterator > DeferredCallbackListIteratorList;

void Tick()
{
	ScopedLock lock(s_criticalSection);

	// handle deferred events
	if (s_deferredCallbacks.size()) {
		DeferredCallbackListIteratorList s_removedCallbacks;

		DeferredCallbackList::iterator iter = s_deferredCallbacks.begin();
		while (iter != s_deferredCallbacks.end()) {
			if (!iter->iterator->IsRemoved()) {
				s_eventStack.push(iter->eventInfo->name);
				ScriptToken* result = UserFunctionManager::Call(
					EventHandlerCaller(iter->iterator->script, iter->eventInfo, iter->arg0, iter->arg1, iter->callingObj));
				s_eventStack.pop();

				if (iter->iterator->IsRemoved()) {
					s_removedCallbacks.push_back(iter);
					++iter;
				}
				else {
					iter = s_deferredCallbacks.erase(iter);
				}

				// result is unused
				delete result;
			}
		}

		// get rid of any handlers removed during processing above
		while (s_removedCallbacks.size()) {
			(*s_removedCallbacks.begin())->eventInfo->callbacks->erase(iter->iterator);
			s_removedCallbacks.pop_front();
		}

		s_deferredCallbacks.clear();
	}

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
#define EVENT_INFO(name, params, hookInstaller, deferred) s_eventInfos.push_back (EventInfo (name, params, params ? sizeof(params) : 0, deferred, hookInstaller));


	EVENT_INFO("onadd", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onactorequip", kEventParams_GameEvent, &s_ActorEquipHook, false)
	EVENT_INFO("ondrop", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onactorunequip", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ondeath", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onmurder", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("oncombatend", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onhit", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onhitwith", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagechange", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagestart", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagedone", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onload", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onmagiceffecthit", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onsell", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onstartcombat", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("saytodone", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("on0x0080000", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onopen", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onclose", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("on0x00400000", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontrigger", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontriggerenter", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontriggerleave", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onreset", kEventParams_GameEvent, &s_MainEventHook, false)

	EVENT_INFO("onactivate", kEventParams_GameEvent, &s_ActivateHook, false)
	EVENT_INFO("ondropitem", kEventParams_GameEvent, &s_MainEventHook, false)

	EVENT_INFO("exitgame", NULL, NULL, false)
	EVENT_INFO("exittomainmenu", NULL, NULL, false)
	EVENT_INFO("loadgame", kEventParams_OneString, NULL, false)
	EVENT_INFO("savegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("qqq", NULL, NULL, false)
	EVENT_INFO("postloadgame", kEventParams_OneInteger, NULL, false)
	EVENT_INFO("runtimescripterror", kEventParams_OneString, NULL, false)
	EVENT_INFO("deletegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamenewgame", kEventParams_OneString, NULL, false)
	EVENT_INFO("newgame", NULL, NULL, false)
	EVENT_INFO("deletegamename", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamegamename", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamenewgamename", kEventParams_OneString, NULL, false)

	ASSERT (kEventID_InternalMAX == s_eventInfos.size());

#undef EVENT_INFO

	InstallDestroyCIOSHook();	// handle a missing parameter value check.

}


};	// namespace

