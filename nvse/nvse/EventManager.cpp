#include <stdarg.h>
#include "EventManager.h"

#include "ArrayVar.h"
#include "GameAPI.h"
#include "Utilities.h"
#include "SafeWrite.h"
#include "GameObjects.h"
#include "ThreadLocal.h"
#include "common/ICriticalSection.h"
#include "GameOSDepend.h"
#include "InventoryReference.h"

#include "GameData.h"
#include "GameRTTI.h"
#include "GameScript.h"
#include "EventParams.h"

namespace EventManager {

ICriticalSection s_criticalSection;

std::list<EventInfo> s_eventInfos;

EventInfoMap s_eventInfoMap(0x40);

InternalEventVec s_internalEventInfos(kEventID_InternalMAX);

//////////////////////
// Event definitions
/////////////////////

// Hook routines need to be forward declared so they can be used in EventInfo structs.
static void  InstallHook();
static void  InstallActivateHook();
static void	 InstallOnActorEquipHook();
namespace OnSell
{
	static void InstallOnSellHook();
}

enum {
	kEventMask_OnActivate		= 0x01000000,		// special case as OnActivate has no event mask
};

// hook installers
static EventHookInstaller s_MainEventHook = InstallHook;
static EventHookInstaller s_ActivateHook = InstallActivateHook;
static EventHookInstaller s_ActorEquipHook = InstallOnActorEquipHook;
static EventHookInstaller s_SellHook = OnSell::InstallOnSellHook;


///////////////////////////
// internal functions
//////////////////////////

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
static TESObjectREFR* s_lastObj = nullptr;
static TESForm* s_lastTarget = nullptr;
static UInt32 s_lastEvent = NULL;

// OnHitWith often gets marked twice per event. If weapon enchanted, may see:
//  OnHitWith >> OnMGEFHit >> ... >> OnHitWith. 
// Prevent OnHitWith being reported more than once by recording OnHitWith events processed
static TESObjectREFR* s_lastOnHitWithActor = nullptr;
static TESForm* s_lastOnHitWithWeapon = nullptr;

// And it turns out OnHit is annoyingly marked several times per frame for spells/enchanted weapons
static TESObjectREFR* s_lastOnHitVictim = nullptr;
static TESForm* s_lastOnHitAttacker = nullptr;

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
		s_MainEventHook = nullptr;
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

namespace OnSell
{
	static ExtraContainerChanges::EntryData* g_soldItemData{};
	static TESObjectREFR* g_sellerRef{};

	// If non-null, use this instead of soldItemData.
	static TESObjectREFR* g_soldItemInvRef{};

	TESObjectREFR* GetSoldItemInvRef()
	{
		if (g_soldItemInvRef)
			return g_soldItemInvRef;
		if (!g_soldItemData || !g_sellerRef)
			return nullptr;

		auto const entry = g_soldItemData;
		auto const cont = g_sellerRef->GetContainer();
		
		// create the invRef (copied code from GetInvRefsForItem)
		SInt32 xCount = entry->countDelta;
		auto baseCount = cont->GetCountForForm(entry->type);
		if (baseCount)
		{
			if (entry->HasExtraLeveledItem())
				baseCount = xCount;
			else baseCount += xCount;
		}
		else baseCount = xCount;

		if (baseCount > 0 && entry->extendData)
		{
			// find first valid extraDataList
			for (auto xdlIter = entry->extendData->Begin(); !xdlIter.End(); ++xdlIter)
			{
				ExtraDataList* xData = xdlIter.Get();
				xCount = GetCountForExtraDataList(xData);
				if (xCount < 1) continue;
				if (xCount > baseCount)
					xCount = baseCount;
				g_soldItemInvRef = CreateInventoryRefEntry(g_sellerRef, entry->type, xCount, xData);
				break;
			}

			if (!g_soldItemInvRef)
				g_soldItemInvRef = CreateInventoryRefEntry(g_sellerRef, entry->type, xCount, nullptr);
		}
		return g_soldItemInvRef;
	}

	static void PostDispatchCleanup()
	{
		g_soldItemData = nullptr;
		g_sellerRef = nullptr;
		g_soldItemInvRef = nullptr;
	}

	template <bool IsPlayerOwner>
	static SInt32 __fastcall OnSellHook(ExtraContainerChanges::EntryData* contChangesEntry)
	{
		if ((s_eventsInUse & ScriptEventList::kEvent_OnSell) != 0)
		{
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress());
			auto* barterMenu = *reinterpret_cast<Menu**>(ebp - 0x6C);
			TESForm* item = contChangesEntry->type;
			TESObjectREFR* seller = IsPlayerOwner ? *g_thePlayer : *reinterpret_cast<TESObjectREFR**>((char*)barterMenu + 0x80);

			if (seller->GetIsReference() && item)
			{
				// store the data for an Inventory Ref for a frame, so it can be queried via function during the event.
				g_soldItemData = contChangesEntry;
				g_sellerRef = seller;

				// Handle the event directly, since we must have the item as first arg, but HandleGameEvent wants that to be a reference.
				HandleEvent(kEventID_OnSell, item, seller, PostDispatchCleanup);
			}
		}
		return contChangesEntry->countDelta;
	}

	static void InstallOnSellHook()
	{
		// Main hook does not run reliably for this event, plus we don't handle the data that gets passed properly.
		// So we make a unique hook.

		// Replace this_1 calls.
		WriteRelCall(0x72FE3E, (UInt32)&OnSellHook<false>);
		WriteRelCall(0x72FF20, (UInt32)&OnSellHook<true>);
	}
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

eEventID EventIDForMask(UInt32 eventMask)
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

eEventID EventIDForMessage(UInt32 msgID)
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

// Prevent filters of the wrong type from being added to an Event Handler instance.
// Only needs to be called for SetEventHandlerAlt, to filter out most user weirdness.
bool IsPotentialFilterValid(EventArgType const expectedParamType, std::string& outErrorMsg,
	const EventCallback::Filter &potentialFilter, size_t const filterNum)
{
	if (expectedParamType == EventArgType::eParamType_Anything)
		return true;

	auto const dataType = potentialFilter.DataType();
	if (dataType == kDataType_Array)
	{
		// Could be an array of filters, or just a filter array.
		ArrayID arrID;
		potentialFilter.GetAsArray(&arrID);
		auto const arrPtr = g_ArrayMap.Get(arrID);
		if (!arrPtr) [[unlikely]]
		{
			outErrorMsg = FormatString("Invalid/uninitialized array filter was passed for filter #%u (array id %u).", filterNum, arrID);
			return false;
		}
		// If array of filters, assume that every element in the array is of the expected type.
		// If it's a (String)Map, then its keys will simply be ignored.
		return true;
	}

	auto const expectedDataType = ParamTypeToDataType(expectedParamType);
	if (dataType != expectedDataType) [[unlikely]]
	{
		outErrorMsg = FormatString("Invalid type for filter #%u: expected %s, got %s.", filterNum, 
			DataTypeToString(expectedDataType), DataTypeToString(dataType));
		return false;
	}

	if (dataType == kDataType_Form)
	{
		TESForm* form;
		// Allow null forms to go through, in case that would be a desired filter.
		if (UInt32 refID; potentialFilter.GetAsFormID(&refID) && (form = LookupFormByID(refID)))
		{
			if (expectedParamType == EventArgType::eParamType_BaseForm
				&& form->GetIsReference()) [[unlikely]]
			{
				//Prefer not to sneakily convert the user's reference to its baseform, lessons must be learned.
				outErrorMsg = FormatString("Expected BaseForm-type filter for filter #%u, got Reference.", filterNum);
				return false;
			}
			// Allow passing a baseform to a Reference-type filter.
			// When the event is dispatched, it will check if the passed reference belongs to the baseform.
			// (The above assumes the baseform is not a formlist. If it is, then it'll repeat the above check for each form in the list until a match is found).
			// (We are not checking the validity of each form in a formlist filter for performance concerns).
		}
	}

	return true;
}

bool EventCallback::ValidateFirstOrSecondFilter(bool isFirst, const EventInfo& parent, std::string &outErrorMsg) const
{
	const TESForm* filter = isFirst ? source : object;
	if (!filter) // unfiltered
		return true;
	auto const filterName = isFirst ? R"("first"/"source")" : R"("second"/"object")";

	if (isFirst )
	{
		if (!parent.numParams) [[unlikely]]
		{
			outErrorMsg = R"(Cannot use "first"/"source" filter; event has 0 args.)";
			return false;
		}
	}
	else if (parent.numParams < 2) [[unlikely]]
	{
		outErrorMsg = FormatString(R"(Cannot use "second"/"object" filter; event has %u args.)", parent.numParams);
		return false;
	}
	auto const expectedType = GetNonPtrParamType(parent.paramTypes[isFirst ? 0 : 1]);

	if (!IsFormParam(expectedType)) [[unlikely]]
	{
		outErrorMsg = FormatString("Cannot set a non-Form type filter for %s.", filterName);
		return false;
	}

	if (expectedType == EventArgType::eParamType_BaseForm 
		&& filter->GetIsReference()) [[unlikely]]
	{
		//Prefer not to sneakily convert the user's reference to its baseform, lessons must be learned.
		outErrorMsg = FormatString("Expected BaseForm-type filter for filter %s, got Reference.", filterName);
		return false;
	}

	return true;
}

bool EventCallback::ValidateFirstAndSecondFilter(const EventInfo& parent, std::string& outErrorMsg) const
{
	return ValidateFirstOrSecondFilter(true, parent, outErrorMsg)
		&& ValidateFirstOrSecondFilter(false, parent, outErrorMsg);
}

bool EventCallback::ValidateFilters(std::string& outErrorMsg, const EventInfo& parent) const
{
	if (parent.HasUnknownArgTypes())
		return true;

	if (!ValidateFirstAndSecondFilter(parent, outErrorMsg))
		return false;

	for (auto &[index, filter] : filters)
	{
		if (index > parent.numParams) [[unlikely]]
		{
			outErrorMsg = FormatString("Index %d exceeds max number of args for event (%u)", index, parent.numParams);
			return false;
		}

		// Index #0 is reserved for callingReference filter.
		bool const isCallingRefFilter = index == 0;
		auto const filterType = isCallingRefFilter ? EventArgType::eParamType_Reference
			: GetNonPtrParamType(parent.paramTypes[index - 1]);

		if (!IsPotentialFilterValid(filterType, outErrorMsg, filter, index)) [[unlikely]]
			return false;
	}

	return true;
}

std::string EventCallback::GetFiltersAsStr() const
{
	std::string result;

	if (source)
	{
		result += std::string{ R"("first"::)" } + source->GetStringRepresentation();
	}

	if (object)
	{
		if (!result.empty())
			result += ", ";
		result += R"("second"::)" + object->GetStringRepresentation();
	}

	for (auto const &[index, filter] : filters)
	{
		if (!result.empty())
			result += ", ";
		result += std::to_string(index) + "::" + filter.GetStringRepresentation();
	}

	if (result.empty())
	{
		result = "(None)";
	}

	return result;
}

ArrayVar* EventCallback::GetFiltersAsArray(const Script* scriptObj) const
{
	ArrayVar* arr = g_ArrayMap.Create(kDataType_String, false, scriptObj->GetModIndex());

	if (source)
	{
		arr->SetElementFormID("first", source->refID);
	}
	if (object)
	{
		arr->SetElementFormID("second", object->refID);
	}
	for (auto const& [index, filter] : filters)
	{
		arr->SetElement(std::to_string(index).c_str(), &filter);
	}

	return arr;
}


std::string EventCallback::GetCallbackFuncAsStr() const
{
	return std::visit(overloaded
		{
			[](const LambdaManager::Maybe_Lambda& script) -> std::string
			{
				return script.Get()->GetStringRepresentation();
			},
			[](const NativeEventHandlerInfo& handlerInfo) -> std::string
			{
				return handlerInfo.GetStringRepresentation();
			}
		}, this->toCall);
}

ArrayVar* EventCallback::GetAsArray(const Script* scriptObj) const
{
	ArrayVar* handlerArr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());

	std::visit(overloaded
		{
			[=](const LambdaManager::Maybe_Lambda& maybeLambda)
			{
				handlerArr->SetElementFormID(0.0, maybeLambda.Get()->refID);
			},
			[=](const NativeEventHandlerInfo& handler)
			{
				handlerArr->SetElementArray(0.0, handler.GetArrayRepresentation(scriptObj->GetModIndex())->ID());
			}
		}, toCall);

	handlerArr->SetElementArray(1.0, GetFiltersAsArray(scriptObj)->ID());
	return handlerArr;
}

bool EventCallback::operator==(const EventCallback& rhs) const
{
	return (toCall == rhs.toCall) &&
		(object == rhs.object) && (source == rhs.source) && (filters == rhs.filters);
}

Script* EventCallback::TryGetScript() const
{
	if (auto const maybe_lambda = std::get_if<LambdaManager::Maybe_Lambda>(&toCall))
		return maybe_lambda->Get();
	return nullptr;
}

bool EventCallback::HasCallbackFunc() const
{
	return std::visit([](auto const& vis) -> bool { return vis; }, toCall);
}

void EventCallback::TrySaveLambdaContext()
{
	if (auto const maybe_lambda = std::get_if<LambdaManager::Maybe_Lambda>(&toCall))
		maybe_lambda->TrySaveContext();
}

EventCallback::EventCallback(EventCallback&& other) noexcept: toCall(std::move(other.toCall)),
                                                              source(other.source),
                                                              object(other.object),
                                                              removed(other.removed),
                                                              pendingRemove(other.pendingRemove),
															  flushOnLoad(other.flushOnLoad),
                                                              filters(std::move(other.filters))
{}

EventCallback& EventCallback::operator=(EventCallback&& other) noexcept
{
	if (this == &other)
		return *this;
	toCall = std::move(other.toCall);
	source = other.source;
	object = other.object;
	removed = other.removed;
	pendingRemove = other.pendingRemove;
	flushOnLoad = other.flushOnLoad;
	filters = std::move(other.filters);
	return *this;
}

void EventCallback::Confirm()
{
	TrySaveLambdaContext();
}

class ClassicEventHandlerCaller : public InternalFunctionCaller
{
public:
	ClassicEventHandlerCaller(Script* script, const ClassicArgTypeStack &argTypes, void* arg0, void* arg1)
	 : InternalFunctionCaller(script, nullptr), m_argTypes(argTypes)
	{
		UInt8 numArgs = 2;
		if (!arg1)
			numArgs = 1;
		if (!arg0)
			numArgs = 0;

		SetArgs(numArgs, arg0, arg1);
	}

	bool ValidateParam(UserFunctionParam* param, UInt8 paramIndex) override
	{
		const auto paramType = VarTypeToParamType(param->varType);
		return ParamTypeMatches(paramType, m_argTypes->at(paramIndex));
	}

	bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) override
	{
		// make sure we've got the same # of args as expected by event handler
		const DynamicParamInfo& dParams = info->ParamInfo();

		if (dParams.NumParams() > 2 || dParams.NumParams() != m_argTypes->size())
		{
			ShowRuntimeError(m_script, "Number of arguments to function script does not match those expected for event");
			return false;
		}

		return InternalFunctionCaller::PopulateArgs(eventList, info);
	}

private:
	// Contains the types expected for event.
	ClassicArgTypeStack m_argTypes;
};


std::unique_ptr<ScriptToken> EventCallback::Invoke(EventInfo &eventInfo, const ClassicArgTypeStack& argTypes, void* arg0, void* arg1)
{
	return std::visit(overloaded
		{
			[=, &eventInfo](const LambdaManager::Maybe_Lambda& script)
			{
				ScopedLock lock(s_criticalSection);	//for event stack

				// handle immediately
				s_eventStack.Push(eventInfo.evName);
				auto ret = UserFunctionManager::Call(ClassicEventHandlerCaller(script.Get(), argTypes, arg0, arg1));
				s_eventStack.Pop();
				return ret;
			},
			[=](const NativeEventHandlerInfo& handlerInfo) -> std::unique_ptr<ScriptToken>
			{
				// native plugin event handlers
				void* params[] = { arg0, arg1 };
				handlerInfo.m_func(nullptr, params);
				return nullptr;
			}
		}, this->toCall);
}

bool IsValidReference(void* refr)
{
	bool bIsRefr = false;
	__try
	{
		if ((*static_cast<UInt8*>(refr) & 4) && ((static_cast<UInt16*>(refr)[1] == 0x108) || (*static_cast<UInt32*>(refr) == 0x102F55C)))
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

// Some events are best deferred until Tick() invoked rather than being handled immediately.
// This stores info about such an event.
// Only used in the deprecated HandleEvent.
struct DeferredDeprecatedCallback
{
	EventCallback		&callback;	//assume this contains a Script* CallbackFunc.
	void				*arg0;
	void				*arg1;
	EventInfo			&eventInfo;
	ClassicArgTypeStack	argTypes;
	void				(*cleanupCallback)();

	DeferredDeprecatedCallback(EventCallback &pCallback, void *_arg0, void *_arg1, EventInfo &_eventInfo, 
		const ClassicArgTypeStack& _argTypes, void (*cleanupCallback)() = {})
	: callback(pCallback), arg0(_arg0), arg1(_arg1), eventInfo(_eventInfo), argTypes(_argTypes), cleanupCallback(cleanupCallback) {}

	~DeferredDeprecatedCallback()
	{
		if (callback.removed)
			return;

		// assume callback is owned by a global; prevent data race.
		ScopedLock lock(s_criticalSection);

		callback.Invoke(eventInfo, argTypes, arg0, arg1);
		if (cleanupCallback)
			cleanupCallback();
	}
};

typedef Stack<DeferredDeprecatedCallback> DeferredCallbackList;
DeferredCallbackList s_deferredDeprecatedCallbacks;

DeferredRemoveList s_deferredRemoveList;

enum class RefState {NotSet, Invalid, Valid};


// Deprecated
void HandleEvent(EventInfo& eventInfo, void* arg0, void* arg1, ClassicArgTypeStack const &argTypes,
	ExpressionEvaluator* eval = nullptr, void (*cleanupCallback)() = nullptr)
{
	// For filtering via new filters
	ClassicRawArgStack args{};

	//Ensure Args and ArgTypes are equal size.
	if (!argTypes->empty())
	{
		args->push_back(arg0);
		if (argTypes->size() >= 2)
			args->push_back(arg1);
	}

	auto isArg0Valid = RefState::NotSet;
	for (auto& iter : eventInfo.callbacks)
	{
		EventCallback& callback = iter.second;

		if (callback.IsRemoved())
			continue;

		// Check filters
		if (callback.source && arg0 != callback.source)
		{
			if (isArg0Valid == RefState::NotSet)
				isArg0Valid = IsValidReference(arg0) ? RefState::Valid : RefState::Invalid;
			if (isArg0Valid == RefState::Invalid || GetPermanentBaseForm(static_cast<TESObjectREFR*>(arg0)) != callback.source)
				continue;
		}

		if (callback.object && (callback.object != arg1))
			continue;

		if (!callback.DoNewFiltersMatch<false>(nullptr, args, argTypes, eventInfo, eval))
			continue;

		if (GetCurrentThreadId() != g_mainThreadID)
		{
			// avoid potential issues with invoking handlers outside of main thread by deferring event handling
			s_deferredDeprecatedCallbacks.Push(callback, arg0, arg1, eventInfo, argTypes, cleanupCallback);
		}
		else
		{
			callback.Invoke(eventInfo, argTypes, arg0, arg1);
			if (cleanupCallback)
				cleanupCallback();
		}
	}
}

void HandleInternalEvent(EventInfo& eventInfo, void* arg0, void* arg1, void (*cleanupCallback)())
{
	HandleEvent(eventInfo, arg0, arg1, eventInfo.GetClassicArgTypesAsStackVector(), nullptr, cleanupCallback);
}

void HandleUserDefinedEvent(EventInfo& eventInfo, void* arg0, void* arg1, ExpressionEvaluator *eval)
{
	ClassicArgTypeStack argTypes;
	argTypes->push_back(EventArgType::eParamType_Array);
	HandleEvent(eventInfo, arg0, arg1, argTypes, eval);
}

// Deprecated in favor of EventManager::DispatchEvent
void __stdcall HandleEvent(eEventID id, void* arg0, void* arg1, void (*cleanupCallback)())
{
	ScopedLock lock(s_criticalSection);
	EventInfo* eventInfo = s_internalEventInfos[id]; //assume ID is valid
	if (eventInfo->callbacks.empty()) 
		return;
	HandleInternalEvent(*eventInfo, arg0, arg1, cleanupCallback);
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

// Avoid constantly looking up the eventName for potentially recursive calls.
// Assume priority is valid.	
bool DoSetHandler(EventInfo &info, EventCallback& toSet, int priority)
{
	if (auto const script = toSet.TryGetScript())
	{
		// Use recursion over formlists to Set the handler for each form.
		// Only kept for legacy purposes.
		UInt32 setted = 0;

		// trying to use a FormList to specify the source filter
		if (toSet.source && toSet.source->GetTypeID() == 0x055 && recursiveLevel < 100)
		{
			const auto formList = static_cast<BGSListForm*>(toSet.source);
			for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
			{
				EventCallback listHandler(script, iter.Get(), toSet.object);
				recursiveLevel++;
				if (DoSetHandler(info, listHandler, priority)) setted++;
				recursiveLevel--;
			}
			return setted > 0;
		}

		// trying to use a FormList to specify the object filter
		if (toSet.object && toSet.object->GetTypeID() == 0x055 && recursiveLevel < 100)
		{
			const auto formList = static_cast<BGSListForm*>(toSet.object);
			for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
			{
				EventCallback listHandler(script, toSet.source, iter.Get());
				recursiveLevel++;
				if (DoSetHandler(info, listHandler, priority)) setted++;
				recursiveLevel--;
			}
			return setted > 0;
		}
	}

	ScopedLock lock(s_criticalSection);

	// is hook installed for this event type?
	if (info.installHook)
	{
		if (*info.installHook)
		{
			// if this hook is used by more than one event type, prevent it being installed a second time
			(*info.installHook)();
		}
		// mark event as having had its hook installed
		info.installHook = nullptr;
	}

	if (!info.callbacks.empty())
	{
		// if an existing handler matches this one exactly, don't duplicate it
		auto const priorityRange = info.callbacks.equal_range(priority);
		for (auto i = priorityRange.first; i != priorityRange.second; ++i)
		{
			auto& existingCallback = i->second;
			if (existingCallback == toSet)
			{
				// may be re-adding a previously removed handler, so clear the Removed flag
				if (existingCallback.IsRemoved())
				{
					existingCallback.SetRemoved(false);
					return true; 
				}
				return false; //attempting to add a duplicate handler.
			}
		}
	}

	toSet.Confirm();
	info.callbacks.emplace(priority, std::move(toSet));

	s_eventsInUse |= info.eventMask;
	return true;
}

ArgTypeStack EventInfo::GetArgTypesAsStackVector() const
{
	ArgTypeStack argTypes;
	for (decltype(numParams) i = 0; i < numParams; i++)
	{
		argTypes->push_back(GetNonPtrParamType(paramTypes[i]));
	}
	return argTypes;
}

ClassicArgTypeStack EventInfo::GetClassicArgTypesAsStackVector() const
{
	if (numParams > 2)
		throw std::logic_error("Event has more than 2 params, expected classic-style event.");

	ClassicArgTypeStack argTypes;
	for (decltype(numParams) i = 0; i < numParams; i++)
	{
		argTypes->push_back(GetNonPtrParamType(paramTypes[i]));
	}
	return argTypes;
}

bool EventInfo::ValidateDispatchedArgTypes(const ArgTypeStack& argTypes, ExpressionEvaluator* eval) const
{
	if (this->HasUnknownArgTypes())
		return true;  // We have insufficient information to validate.

	if (argTypes->size() != this->numParams)
	{
		ShowRuntimeScriptError(nullptr, eval, "While validating (pseudo?)-dispatched args for event %s, # of expected args (%u) != # of args passed (%u)",
			this->evName, this->numParams, argTypes->size());
		return false;
	}

	auto const ReportTypeMismatch = [eval, this](EventArgType expected, EventArgType received)
	{
		ShowRuntimeScriptError(nullptr, eval, "While validating (pseudo?)-dispatched args for event %s, encountered invalid ArgType #%u (expected %s).",
			this->evName, received, DataTypeToString(ParamTypeToDataType(expected)));
	};

	for (UInt8 i = 0; auto const argType : *argTypes)
	{
		// Check if the basic argType (extracted from script function call) respects the expected type.
		switch (auto const expected = GetNonPtrParamType(this->paramTypes[i]))
		{
		case EventArgType::eParamType_Reference:
		case EventArgType::eParamType_AnyForm:
		case EventArgType::eParamType_BaseForm:
			if (argType != EventArgType::eParamType_AnyForm)
			{
				ReportTypeMismatch(expected, argType);
				return false;
			}
			break;

		case EventArgType::eParamType_Array:
			if (argType != EventArgType::eParamType_Array)
			{
				ReportTypeMismatch(expected, argType);
				return false;
			}
			break;

		case EventArgType::eParamType_Int:
		case EventArgType::eParamType_Float:
			if (argType != EventArgType::eParamType_Float)
			{
				ReportTypeMismatch(expected, argType);
				return false;
			}
			break;

		case EventArgType::eParamType_String:
			if (argType != EventArgType::eParamType_String)
			{
				ReportTypeMismatch(expected, argType);
				return false;
			}
			break;

		case EventArgType::eParamType_Invalid:
		default:
			ShowRuntimeScriptError(nullptr, eval, "While validating (pseudo?)-dispatched args for event %s, encountered invalid ArgType.",
				this->evName);
			return false;
		}
		i++;
	}
	return true;
}

RawArgStack EventInfo::GetEffectiveArgs(RawArgStack& passedArgs)
{
	if (!hasPtrArg)
		return passedArgs;

	RawArgStack effectiveArgs{};
	for (decltype(numParams) i = 0; i < numParams; i++) 
	{
		if (IsPtrParam(paramTypes[i]))
			effectiveArgs->emplace_back(*static_cast<void**>(passedArgs->at(i)));  //get the pointer's value
		else
			effectiveArgs->emplace_back(passedArgs->at(i));
	}
	return effectiveArgs;
}

DeferredRemoveCallback::~DeferredRemoveCallback()
{
	if (iterator->second.removed)
	{
		eventInfo->callbacks.erase(iterator);
		if (eventInfo->callbacks.empty() && eventInfo->eventMask)
			s_eventsInUse &= ~eventInfo->eventMask;
	}
	else iterator->second.pendingRemove = false;
}

// Avoid constantly looking up the eventName for potentially recursive calls.
bool DoRemoveHandler(EventInfo& info, const EventCallback& toRemove, int priority, ExpressionEvaluator* eval)
{
	// Only kept for legacy purposes.
	if (auto const script = toRemove.TryGetScript())
	{
		UInt32 removed = 0;

		// trying to use a FormList to specify the source filter
		if (toRemove.source && toRemove.source->GetTypeID() == 0x055 && recursiveLevel < 100)
		{
			const auto formList = static_cast<BGSListForm*>(toRemove.source);
			for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
			{
				EventCallback listHandler(script, iter.Get(), toRemove.object);
				recursiveLevel++;
				if (DoRemoveHandler(info, listHandler, priority, eval)) removed++;
				recursiveLevel--;
			}
			return removed > 0;
		}

		// trying to use a FormList to specify the object filter
		if (toRemove.object && toRemove.object->GetTypeID() == 0x055 && recursiveLevel < 100)
		{
			const auto formList = static_cast<BGSListForm*>(toRemove.object);
			for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter)
			{
				EventCallback listHandler(script, toRemove.source, iter.Get());
				recursiveLevel++;
				if (DoRemoveHandler(info, listHandler, priority, eval)) removed++;
				recursiveLevel--;
			}
			return removed > 0;
		}
	}
	
	ScopedLock lock(s_criticalSection);
	bool bRemovedAtLeastOne = false;

	if (!info.callbacks.empty())
	{
		auto const TryRemoveNthHandler = [&](CallbackMap::iterator i)
		{
			auto& callback = i->second;
			if (!toRemove.ShouldRemoveCallback(callback, info, eval))
				return;

			if (!callback.IsRemoved())
			{
				callback.SetRemoved(true);
				bRemovedAtLeastOne = true;
			}

			if (!callback.pendingRemove)
			{
				callback.pendingRemove = true;
				s_deferredRemoveList.Push(&info, i);
			}
		};

		if (priority != kHandlerPriority_Invalid)
		{
			auto const priorityRange = info.callbacks.equal_range(priority);
			for (auto i = priorityRange.first; i != priorityRange.second; ++i)
			{
				TryRemoveNthHandler(i);
			}
		}
		else // unfiltered by priority
		{
			for (auto i = info.callbacks.begin(); i != info.callbacks.end(); ++i)
			{
				TryRemoveNthHandler(i);
			}
		}
	}

	return bRemovedAtLeastOne;
}

// If the passed Callback is more or equally generic filter-wise than some already-set events, will remove those events.
// Ex: Callback with "SunnyREF" filter is already set.
// Calling this with a Callback with no filters will lead to the "SunnyREF"-filtered callback being removed.	
bool RemoveHandler(const char* eventName, const EventCallback& toRemove, int priority, ExpressionEvaluator* eval)
{
	if (!toRemove.HasCallbackFunc()) [[unlikely]]
		return false;

	if (priority > kHandlerPriority_Max) [[unlikely]]
	{
		ShowRuntimeScriptError(toRemove.TryGetScript(), eval, "Can't remove event handler with priority above %u.", kHandlerPriority_Max);
		return false;
	}
	if (priority < kHandlerPriority_Min) [[unlikely]]
	{
		ShowRuntimeScriptError(toRemove.TryGetScript(), eval, "Can't remove event handler with priority below %u.", kHandlerPriority_Min);
		return false;
	}

	EventInfo** infoPtr = s_eventInfoMap.GetPtr(eventName);
	bool bRemovedAtLeastOne = false;
	if (infoPtr) [[likely]]
	{
		EventInfo &info = **infoPtr;

		std::string errMsg;
		if (!toRemove.ValidateFilters(errMsg, info)) [[unlikely]]
		{
			if (eval)
				eval->Error(errMsg.c_str());
			return false;
		}

		bRemovedAtLeastOne = DoRemoveHandler(info, toRemove, priority, eval);
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

	const eEventID eventID = EventIDForMask(eventMask);
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
	const eEventID eventID = EventIDForMessage(msgID);
	if (eventID != kEventID_INVALID)
		HandleEvent(eventID, data, nullptr);
}

void ClearFlushOnLoadEventHandlers()
{
	s_deferredRemoveList.Clear();

	for (auto& info : s_eventInfos)
	{
		if (info.FlushesOnLoad())
		{
			info.callbacks.clear(); // WARNING: may invalidate iterators in DeferredRemoveCallbacks.
			// Thus, ensure that list is cleared before this code is reached.
			if (info.eventMask)
			{
				s_eventsInUse &= ~info.eventMask;
			}
		}
		else
		{
			// Remove individual callbacks.
			for (auto iter = info.callbacks.begin(); iter != info.callbacks.end(); )
			{
				auto& callback = iter->second;
				if (callback.FlushesOnLoad())
				{
					iter = info.callbacks.erase(iter);
				}
				else
					++iter;
			}

			if (info.callbacks.empty() && info.eventMask)
				s_eventsInUse &= ~info.eventMask;
		}
	}
}

bool DoesFormMatchFilter(TESForm* inputForm, TESForm* filterForm, bool expectReference, const UInt32 recursionLevel)
{
	if (filterForm == inputForm)	//filter and form could both be null, and that's okay.
		return true;
	if (!filterForm || !inputForm)
		return false;
	if (recursionLevel > 100) [[unlikely]]
		return false;
	if (IS_ID(filterForm, BGSListForm))
	{
		const auto* filterList = static_cast<BGSListForm*>(filterForm);
		if (IS_ID(inputForm, BGSListForm))
		{
			const auto* inputList = static_cast<BGSListForm*>(inputForm);

			// Compare the contents of two lists (which could be recursive).
			// The order of the contents does not matter.
			// Everything in the inputList must be matched with a form in the filterList.
			for (auto* inputListForm : inputList->list)
			{
				bool foundMatch = false;
				for (auto* filterListForm : filterList->list)
				{
					if (DoesFormMatchFilter(inputListForm, filterListForm, expectReference, recursionLevel + 1))
					{
						foundMatch = true;
						break;
					}
				}
				if (!foundMatch)
					return false;
			}
			return true;
		}
		//inputForm is not a formlist.

		// try matching the inputForm with a Form from the filter list
		for (auto* filterListForm : filterList->list)
		{
			if (DoesFormMatchFilter(inputForm, filterListForm, expectReference, recursionLevel + 1))
				return true;
		}
	}
	else //filterForm is not a formlist.
	{
		// Allow matching a single form with a formlist if the formlist effectively only contains that form.
		if (IS_ID(inputForm, BGSListForm))
		{
			auto const inputList = static_cast<BGSListForm*>(inputForm);
			for (auto const inputListForm : inputList->list)
			{
				if (!DoesFormMatchFilter(inputListForm, filterForm, expectReference))
					return false;
			}
			return true;
		}
		//inputForm is not a formlist.

		// If input form is a reference, then try matching its baseForm to the filter.
		if (expectReference && inputForm->GetIsReference())
		{
			if (filterForm == GetPermanentBaseForm(static_cast<TESObjectREFR*>(inputForm)))
				return true;
		}
	}
	return false;
}

// ArgTypes should only be null for Plugin-Defined Events, since filters are already checked during SetEventHandler.
// If not null, then check if filters are valid (for User-Defined Events).
bool EventCallback::DoDeprecatedFiltersMatch(const RawArgStack& args, const ArgTypeStack* argTypes, const EventInfo& eventInfo, ExpressionEvaluator* eval) const
{
	auto const numParams = args->size();
	if (argTypes && numParams != (*argTypes)->size())
	{
		ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if first/second filters match for event %s, there was a different # of ArgTypes vs. Args", 
			eventInfo.evName);
		return false;
	}

	// old filter system
	if (source)
	{
		if (argTypes)
		{
			// Check if filters are valid, given what we now know about what args are dispatched for this event.
			if (!numParams)
			{
				ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if first/second filters match for User-Defined event %s, saw a filter for an event that did not dispatch anything.", 
					eventInfo.evName);
				return false;
			}
			if (!IsFormParam((*argTypes)->at(0)))  // Shouldn't encounter param type "Anything" here.
			{
				ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if first/second filters match for User-Defined event %s, saw a form filter for 'first' arg when a non-form was dispatched.",
					eventInfo.evName);
				return false;
			}
		}

		if (!DoesFormMatchFilter(static_cast<TESForm*>(args->at(0)), source, true))
			return false;
	}

	if (object)
	{
		if (argTypes)
		{
			// Check if filters are valid, given what we now know about what args are dispatched for this event.
			if (numParams < 2)
			{
				ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if first/second filters match for User-Defined event %s, saw a filter for 'second' arg when no 2nd arg was dispatched.",
					eventInfo.evName);
				return false;
			}
			if (!IsFormParam((*argTypes)->at(0)))  // Shouldn't encounter param type "Anything" here.
			{
				ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if first/second filters match for User-Defined event %s, saw a form filter for 'second' arg when a non-form was dispatched.",
					eventInfo.evName);
				return false;
			}
		}

		if (!DoesFormMatchFilter(static_cast<TESForm*>(args->at(1)), object, true))
			return false;
	}
	return true;
}

EventInfo* TryGetEventInfoForName(const char* eventName)
{
	if (EventInfo** infoPtr = s_eventInfoMap.GetPtr(eventName))
		return *infoPtr;
	return nullptr;
}

bool NativeEventHandlerInfo::InitWithPluginInfo(NativeEventHandler func, PluginHandle pluginHandle, const char* handlerName)
{
	if (!func)
		return false;
	m_func = func;

	if (const auto* pluginInfo = g_pluginManager.GetInfoFromHandle(pluginHandle))
		m_pluginName = pluginInfo->name;
	else
		return false;

	if (handlerName)
		m_handlerName = handlerName;

	return true;
}

std::string NativeEventHandlerInfo::GetStringRepresentation() const
{
	return FormatString("Internal handler %s (plugin %s)", m_handlerName, m_pluginName);
}

ArrayVar* NativeEventHandlerInfo::GetArrayRepresentation(UInt8 modIndex) const
{
	auto* result = g_ArrayMap.Create(kDataType_String, false, modIndex);
	result->SetElementString("Plugin", m_pluginName);
	result->SetElementString("Handler", m_handlerName);
	return result;
}

// Meant for use to validate param types, not much else.
Script::VariableType ParamTypeToVarType(EventArgType pType)
{
	switch (pType)
	{
	case EventArgType::eParamType_Int: // Int filter type is only different from Float when Dispatching (number is floored).
	case EventArgType::eParamType_Float: return Script::VariableType::eVarType_Float;
	case EventArgType::eParamType_String: return Script::VariableType::eVarType_String;
	case EventArgType::eParamType_Array: return Script::VariableType::eVarType_Array;
	case EventArgType::eParamType_RefVar:
	case EventArgType::eParamType_Reference:
	case EventArgType::eParamType_BaseForm:
		return Script::VariableType::eVarType_Ref;
	case NVSEEventManagerInterface::eParamType_Invalid:
	case NVSEEventManagerInterface::eParamType_Anything:
		return Script::VariableType::eVarType_Invalid;
	}
	return Script::VariableType::eVarType_Invalid;
}

EventArgType VarTypeToParamType(Script::VariableType varType)
{
	switch (varType) {
	case Script::eVarType_Float: return EventArgType::eParamType_Float;
	case Script::eVarType_Integer:	return EventArgType::eParamType_Int;
	case Script::eVarType_String: return EventArgType::eParamType_String;
	case Script::eVarType_Array: return EventArgType::eParamType_Array;
	case Script::eVarType_Ref: return EventArgType::eParamType_AnyForm;
	case Script::eVarType_Invalid:
		return EventArgType::eParamType_Invalid;
	}
	return EventArgType::eParamType_Invalid;
}

DataType ParamTypeToDataType(EventArgType pType)
{
	switch (pType)
	{
	case EventArgType::eParamType_Int:
	case EventArgType::eParamType_Float: return kDataType_Numeric;
	case EventArgType::eParamType_String: return kDataType_String;
	case EventArgType::eParamType_Array: return kDataType_Array;
	case EventArgType::eParamType_RefVar:
	case EventArgType::eParamType_Reference:
	case EventArgType::eParamType_BaseForm:
		return kDataType_Form;
	case NVSEEventManagerInterface::eParamType_Invalid:
	case NVSEEventManagerInterface::eParamType_Anything:
		return kDataType_Invalid;
	}
	return kDataType_Invalid;
}

bool ParamTypeMatches(EventArgType from, EventArgType to)
{
	if (from == to)
		return true;
	if (from == EventArgType::eParamType_AnyForm)
	{
		switch (to) {
		case NVSEEventManagerInterface::eParamType_AnyForm:
		case NVSEEventManagerInterface::eParamType_Reference:
		case NVSEEventManagerInterface::eParamType_BaseForm: return true;
		default: break;
		}
	}
	if (to == NVSEEventManagerInterface::eParamType_Anything)
		return true;
	return false;
}


bool EventCallback::ShouldRemoveCallback(const EventCallback& toCheck, const EventInfo& evInfo, ExpressionEvaluator* eval) const
{
	if (toCall != toCheck.toCall)
		return false;

	// Would be nice if it would try matching reference's baseforms to the filter-to-remove.
	// Can't do that now though, due to backwards compatibility.
	if (source && (source != toCheck.source))
		return false;

	if (object && (object != toCheck.object))
		return false;

	if (filters.empty())
		return true;

	if (filters.size() > toCheck.filters.size())
		return false; // "this" is more specific; it won't cover all the cases where toCheck would run.

	auto const TryGetArray = [this, eval](const Filter& filter, size_t filterIndex) -> ArrayVar*
	{
		ArrayID id;
		filter.GetAsArray(&id);
		auto* arr = g_ArrayMap.Get(id);
		if (!arr)
		{
			std::string const err = FormatString("While checking event filters in array at index %d, the array was invalid/uninitialized (array id: %u).",
				filterIndex, id);
			if (eval) eval->Error(err.c_str());
			else ShowRuntimeError(this->TryGetScript(), err.c_str());
		}
		return arr;
	};

	for (auto const& [index, toRemoveFilter] : filters)
	{
		if (auto const search = toCheck.filters.find(index);
			search != toCheck.filters.end())
		{
			auto const& existingFilter = search->second;

			EventArgType const paramType = (index == 0) ? EventArgType::eParamType_Reference
				: evInfo.TryGetNthParamType(index - 1);

			if (toRemoveFilter.DataType() == existingFilter.DataType())
			{
				if (toRemoveFilter.DataType() == kDataType_Array)
				{
					// Cases:
					// 1) Both are array filters
					// 2) Both are array-of-filters filters
					// 3) ToRemoveFilter is an array filter, ExistingFilter is an array-of-filters filter containing array(s).
					// 4) Opposite of the above.

					// TODO: multidimensional array filters currently aren't supported

					// if false, skip to case #2
					if (paramType == EventArgType::eParamType_Anything || paramType == EventArgType::eParamType_Array)
					{
						// Check if arrays are exactly equal.
						// Covers case #1
						if (DoesFilterMatch<true>(toRemoveFilter, existingFilter.GetAsVoidArg(), paramType))
							continue;
					}

					// Cover case #2
					// Loop over existingFilter as params, call DoesParamMatchFiltersInArray on them.
					auto const existingFilters = TryGetArray(existingFilter, index);
					if (!existingFilters)
						return false;

					bool filtersMatch = true;
					for (auto iter = existingFilters->Begin(); !iter.End(); ++iter)
					{
						if (!DoesParamMatchFiltersInArray<true>(toRemoveFilter, paramType, 
							iter.second()->GetAsVoidArg(), index))
						{
							filtersMatch = false;
							break;
						}
					}
					if (filtersMatch)
						continue;

					// Cases 3-4 are only possible if the filter type is array.
					if (paramType != EventArgType::eParamType_Anything && paramType != EventArgType::eParamType_Array)
						return false;

					// Cover case #3
					// ToRemoveFilter array must match with all of the arrays in existingFilters.
					filtersMatch = true;
					for (auto iter = existingFilters->Begin(); !iter.End(); ++iter)
					{
						if (!DoesFilterMatch<true>(toRemoveFilter, iter.second()->GetAsVoidArg(), paramType))
						{
							filtersMatch = false;
							break;
						}
					}
					if (filtersMatch)
						continue;

					// Cover case #4
					// ToRemoveFilter array could hold many array filters, so try matching existingFilter array to any of those arrays.
					if (DoesParamMatchFiltersInArray<true>(toRemoveFilter, paramType, existingFilter.GetAsVoidArg(), index))
						continue;

					return false;
				}
				// else, must be a simple filter
				if (!DoesFilterMatch<true>(toRemoveFilter, existingFilter.GetAsVoidArg(), paramType))
				{
					return false;
				}
			}
			// Cases left:
			// 1) toRemoveFilter is a filter-of-arrays filter, and existingFilter is basic type (non-array).
			// 2) Opposite of the above.
			// 3) Type mismatch (error) - should only be possible for User-Defined Event handlers.
			else if (toRemoveFilter.DataType() == kDataType_Array)
			{
				// Case #1: check if any of the elements in the array match existingFilter.
				if (!DoesParamMatchFiltersInArray<true>(toRemoveFilter, paramType, existingFilter.GetAsVoidArg(), index))
					return false;
			}
			else if (existingFilter.DataType() == kDataType_Array)
			{
				// Case #2: if existingFilter is an array with effectively just one filter (it can be repeated),
				// and that filter matches the basic toRemoveFilter, then toRemoveFilter is just as generic as existingFilter.

				auto const existingFilters = TryGetArray(existingFilter, index);
				if (!existingFilters)
					return false;

				// If array of filters is (string)map, then ignore the keys.
				for (auto iter = existingFilters->GetRawContainer()->begin();
					!iter.End(); ++iter)
				{
					if (!DoesFilterMatch<true>(toRemoveFilter, iter.second()->GetAsVoidArg(), paramType))
						return false;
				}
			}
			else
			{
				// Case #3: weird type mismatch
				std::string const err = FormatString("While comparing event filters at index %d, encountered a type mismatch (to-remove filter type: %s, existing filter type: %s).",
					index, DataTypeToString(toRemoveFilter.DataType()), DataTypeToString(existingFilter.DataType()));
				if (eval) eval->Error(err.c_str());
				else ShowRuntimeError(this->TryGetScript(), err.c_str());
				return false; 
			}
		}
		else // toCheck does not have a filter at this index, thus "this" has a more specific filter.
			return false;
	}
	return true;
}


DispatchReturn vDispatchEvent(const char* eventName, DispatchCallback resultCallback, void* anyData,
	TESObjectREFR* thisObj, va_list args, bool deferIfOutsideMainThread, PostDispatchCallback postCallback)
{
	const auto eventPtr = TryGetEventInfoForName(eventName);
	if (!eventPtr)
		return DispatchReturn::kRetn_UnknownEvent;
	EventInfo& eventInfo = *eventPtr;

#if _DEBUG //shouldn't need to be checked normally; RegisterEvent verifies numParams.
	if (eventInfo.numParams > kNumMaxFilters)
	{
		throw std::logic_error("NumParams is greater than kNumMaxFilters; how did we get here?.");
		//return DispatchReturn::kRetn_GenericError;
	}
#endif

	RawArgStack params;
	for (int i = 0; i < eventInfo.numParams; ++i)
		params->push_back(va_arg(args, void*));

	return DispatchEventRaw<false>(eventInfo, thisObj, params, resultCallback, anyData,
		deferIfOutsideMainThread, postCallback);
}

bool DispatchEvent(const char* eventName, TESObjectREFR* thisObj, ...)
{
	va_list paramList;
	va_start(paramList, thisObj);

	DispatchReturn const result = vDispatchEvent(eventName, nullptr, nullptr,
		thisObj, paramList, false, nullptr);

	va_end(paramList);
	return result > DispatchReturn::kRetn_GenericError;
}
bool DispatchEventThreadSafe(const char* eventName, PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...)
{
	va_list paramList;
	va_start(paramList, thisObj);

	DispatchReturn const result = vDispatchEvent(eventName, nullptr, nullptr,
		thisObj, paramList, true, postCallback);

	va_end(paramList);
	return result > DispatchReturn::kRetn_GenericError;
}

DispatchReturn DispatchEventAlt(const char* eventName, DispatchCallback resultCallback, void* anyData, TESObjectREFR* thisObj, ...)
{
	va_list paramList;
	va_start(paramList, thisObj);

	auto const result = vDispatchEvent(eventName, resultCallback, anyData,
		thisObj, paramList, false, nullptr);

	va_end(paramList);
	return result;
}
DispatchReturn DispatchEventAltThreadSafe(const char* eventName, DispatchCallback resultCallback, void* anyData, 
	PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...)
{
	va_list paramList;
	va_start(paramList, thisObj);

	auto const result = vDispatchEvent(eventName, resultCallback, anyData,
		thisObj, paramList, true, postCallback);

	va_end(paramList);
	return result;
}

#if 0 // not currently recommended for use, since regularly defining and dispatching an event should be safer.
namespace DispatchWithArgTypes
{
	// If kFlag_HasUnknownArgTypes is set, then these function must be called so that ArgTypes are known during dispatch.

	DispatchReturn vDispatchEventWithTypes(const char* eventName, DispatchCallback resultCallback, void* anyData,
		UInt8 numParams, EventArgType* paramTypes, TESObjectREFR* thisObj, va_list args,
		bool deferIfOutsideMainThread, PostDispatchCallback postCallback)
	{
		const auto eventPtr = TryGetEventInfoForName(eventName);
		if (!eventPtr)
			return DispatchReturn::kRetn_UnknownEvent;
		EventInfo& eventInfo = *eventPtr;

		RawArgStack paramsStack;
		for (int i = 0; i < numParams; ++i)
			paramsStack->push_back(va_arg(args, void*));

		ArgTypeStack paramTypesStack;
		for (int i = 0; i < numParams; ++i)
			paramTypesStack->push_back(paramTypes[i]);

		return DispatchEventRawWithTypes<false>(eventInfo, thisObj, paramsStack, paramTypesStack,
			resultCallback, anyData, deferIfOutsideMainThread, postCallback);
	}

	bool DispatchEvent(const char* eventName, UInt8 numParams, EventArgType* paramTypes, TESObjectREFR* thisObj, ...)
	{
		va_list paramList;
		va_start(paramList, thisObj);

		DispatchReturn const result = vDispatchEventWithTypes(eventName, nullptr, nullptr,
			numParams, paramTypes, thisObj, paramList, false, nullptr);

		va_end(paramList);
		return result > DispatchReturn::kRetn_GenericError;
	}

	DispatchReturn DispatchEventAlt(const char* eventName, DispatchCallback resultCallback,
		void* anyData, UInt8 numParams, EventArgType* paramTypes, TESObjectREFR* thisObj, ...)
	{
		va_list paramList;
		va_start(paramList, thisObj);

		DispatchReturn const result = vDispatchEventWithTypes(eventName, resultCallback, anyData,
			numParams, paramTypes, thisObj, paramList, false, nullptr);

		va_end(paramList);
		return result;
	}

	bool DispatchEventThreadSafe(const char* eventName, PostDispatchCallback postCallback,
		UInt8 numParams, EventArgType* paramTypes, TESObjectREFR* thisObj, ...)
	{
		va_list paramList;
		va_start(paramList, thisObj);

		DispatchReturn const result = vDispatchEventWithTypes(eventName, nullptr, nullptr,
			numParams, paramTypes, thisObj, paramList, true, postCallback);

		va_end(paramList);
		return result > DispatchReturn::kRetn_GenericError;
	}

	DispatchReturn DispatchEventAltThreadSafe(const char* eventName, DispatchCallback resultCallback, void* anyData,
		PostDispatchCallback postCallback, UInt8 numParams, EventArgType* paramTypes, TESObjectREFR* thisObj, ...)
	{
		va_list paramList;
		va_start(paramList, thisObj);

		auto const result = vDispatchEventWithTypes(eventName, resultCallback, anyData,
			numParams, paramTypes, thisObj, paramList, true, postCallback);

		va_end(paramList);
		return result;
	}
}
#endif

bool DispatchUserDefinedEvent(const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName, 
	ExpressionEvaluator* eval)
{
	ScopedLock lock(s_criticalSection);

	// does an EventInfo entry already exist for this event?
	const auto eventPtr = TryGetEventInfoForName(eventName);
	if (!eventPtr)
		return true; //don't signal an error, it just means no handlers are registered for this event yet.

	EventInfo& eventInfo = *eventPtr;
	if (!eventInfo.IsUserDefined())
		return false;

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
		arr = g_ArrayMap.Create(kDataType_String, false, sender->GetModIndex());
		argsArrayId = arr->ID();
	}

	// populate args array
	arr->SetElementString("eventName", eventName);
	if (senderName == nullptr)
	{
		if (sender)
			senderName = DataHandler::Get()->GetNthModName(sender->GetModIndex());
		else
			senderName = "NVSE";
	}

	arr->SetElementString("eventSender", senderName);

	// dispatch
	HandleUserDefinedEvent(eventInfo, (void*)argsArrayId, nullptr, eval);
	return true;
}

NVSEArrayVarInterface::Element* g_NativeHandlerResult = nullptr;

void SetNativeHandlerFunctionValue(NVSEArrayVarInterface::Element& value)
{
	g_NativeHandlerResult = &value;
}

bool PluginHandlerFilters::ShouldIgnore(const EventCallback& toFilter) const
{
	return toFilter.IsRemoved() || std::visit(overloaded{
		[=, this](const LambdaManager::Maybe_Lambda& maybe_lambda)
		{
			for (UInt32 i = 0; i < numScriptsToIgnore; i++)
			{
				if (scriptsToIgnore[i] == maybe_lambda.Get())
					return true;
			}
			return false;
		},
		[=, this](const NativeEventHandlerInfo& handlerInfo)
		{
			for (UInt32 i = 0; i < numPluginsToIgnore; i++)
			{
				if (!StrCompare(handlerInfo.m_pluginName, pluginsToIgnore[i]))
					return true;
			}
			for (UInt32 i = 0; i < numPluginHandlersToIgnore; i++)
			{
				if (!StrCompare(handlerInfo.m_handlerName, pluginHandlersToIgnore[i]))
					return true;
			}
			return false;
		}
	}, toFilter.toCall);
}

bool ScriptHandlerFilters::ShouldIgnore(const EventCallback &toFilter) const
{
	return toFilter.IsRemoved() || std::visit(overloaded{
		[=, this](const LambdaManager::Maybe_Lambda& maybe_lambda)
		{
			return scriptsToIgnore && scriptsToIgnore->FormMatches(maybe_lambda.Get());
		},
		[=, this](const NativeEventHandlerInfo& handlerInfo)
		{
			if (auto pluginsToIgnoreArr = g_ArrayMap.Get(reinterpret_cast<ArrayID>(pluginsToIgnore)))
			{
				for (const auto* elem : *pluginsToIgnoreArr)
				{
					if (!StrCompare(handlerInfo.m_pluginName, elem->m_data.GetStr()))
						return true;
				}
			}
			if (auto pluginHandlersToIgnoreArr = g_ArrayMap.Get(reinterpret_cast<ArrayID>(pluginHandlersToIgnore)))
			{
				for (const auto* elem : *pluginHandlersToIgnoreArr)
				{
					if (!StrCompare(handlerInfo.m_handlerName, elem->m_data.GetStr()))
						return true;
				}
			}
			return false;
		}
	}, toFilter.toCall);
}

bool SetNativeEventHandlerWithPriority(const char* eventName, NativeEventHandler func,
                                       PluginHandle pluginHandle, const char* handlerName, int priority)
{
	NativeEventHandlerInfo internalInfo;
	if (!internalInfo.InitWithPluginInfo(func, pluginHandle, handlerName))
		return false;
	EventCallback callback(internalInfo);
	return SetHandler<true>(eventName, callback, priority);
}	

bool RemoveNativeEventHandlerWithPriority(const char* eventName, NativeEventHandler func, int priority)
{
	const EventCallback callback(NativeEventHandlerInfo{ func });
	return RemoveHandler(eventName, callback, priority, nullptr);
}

namespace ExportedToPlugins
{
	bool IsEventHandlerFirst(const char* eventName, NativeEventHandler func, int priority,
		TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
		const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
		const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore)
	{
		return IsEventHandlerFirstOrLast<true>(eventName, NativeEventHandlerInfo{ func }, priority,
			PluginHandlerFilters{ scriptsToIgnore, numScriptsToIgnore,
				pluginsToIgnore, numPluginsToIgnore,
				pluginHandlersToIgnore, numPluginHandlersToIgnore }
		);
	}

	bool IsEventHandlerLast(const char* eventName, NativeEventHandler func, int priority,
		TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
		const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
		const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore)
	{
		return IsEventHandlerFirstOrLast<false>(eventName, NativeEventHandlerInfo{ func }, priority,
			PluginHandlerFilters{ scriptsToIgnore, numScriptsToIgnore,
				pluginsToIgnore, numPluginsToIgnore,
				pluginHandlersToIgnore, numPluginHandlersToIgnore }
		);
	}

	NVSEArrayVarInterface::Array* GetHigherPriorityEventHandlers(const char* eventName, NativeEventHandler func, int priority,
		TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
		const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
		const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore)
	{
		if (auto const arr = GetHigherOrLowerPriorityEventHandlers<true>(eventName, NativeEventHandlerInfo{ func }, priority,
			PluginHandlerFilters{ scriptsToIgnore, numScriptsToIgnore,
				pluginsToIgnore, numPluginsToIgnore,
				pluginHandlersToIgnore, numPluginHandlersToIgnore }))
		{
			return reinterpret_cast<NVSEArrayVarInterface::Array*>(arr->ID());
		}
		return nullptr;
	}

	NVSEArrayVarInterface::Array* GetLowerPriorityEventHandlers(const char* eventName, NativeEventHandler func, int priority,
		TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
		const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
		const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore)
	{
		if (auto const arr = GetHigherOrLowerPriorityEventHandlers<false>(eventName, NativeEventHandlerInfo{ func }, priority,
			PluginHandlerFilters{ scriptsToIgnore, numScriptsToIgnore,
				pluginsToIgnore, numPluginsToIgnore,
				pluginHandlersToIgnore, numPluginHandlersToIgnore }))
		{
			return reinterpret_cast<NVSEArrayVarInterface::Array*>(arr->ID());
		}
		return nullptr;
	}
}

std::deque<DeferredCallback<false>> s_deferredCallbacksDefault;
std::deque<DeferredCallback<true>> s_deferredCallbacksWithIntsPackedAsFloats;

void Tick()
{
	ScopedLock lock(s_criticalSection);

	// handle deferred events
	s_deferredDeprecatedCallbacks.Clear();
	s_deferredCallbacksDefault.clear();
	s_deferredCallbacksWithIntsPackedAsFloats.clear();

	// Clear callbacks pending removal.
	s_deferredRemoveList.Clear();

	s_lastObj = nullptr;
	s_lastTarget = nullptr;
	s_lastEvent = NULL;
	s_lastOnHitWithActor = nullptr;
	s_lastOnHitWithWeapon = nullptr;
	s_lastOnHitVictim = nullptr;
	s_lastOnHitAttacker = nullptr;
}

void Init()
{
	// Registering internal events.
#define EVENT_INFO(name, params, hookInstaller, eventMask) \
	EventManager::RegisterEventEx(name, nullptr, true, (params ? sizeof(params) : 0), \
		params, eventMask, hookInstaller)

#define EVENT_INFO_FLAGS(name, params, hookInstaller, eventMask, flags) \
	EventManager::RegisterEventEx(name, nullptr, true, (params ? sizeof(params) : 0), \
		params, eventMask, hookInstaller, flags)

#define EVENT_INFO_WITH_ALIAS(name, alias, params, hookInstaller, eventMask) \
	EventManager::RegisterEventEx(name, alias, true, (params ? sizeof(params) : 0), \
		params, eventMask, hookInstaller)

	// Must define the events in the same order for their eEventID.
	EVENT_INFO("onadd", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnAdd);
	EVENT_INFO_WITH_ALIAS("onactorequip", "onequip", kEventParams_GameEvent, &s_ActorEquipHook, ScriptEventList::kEvent_OnEquip);
	EVENT_INFO("ondrop", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnDrop);
	EVENT_INFO_WITH_ALIAS("onactorunequip", "onunequip", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnUnequip);
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
	EVENT_INFO("onsell", kEventParams_GameEvent, &s_SellHook, ScriptEventList::kEvent_OnSell);
	EVENT_INFO("onstartcombat", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnStartCombat);
	EVENT_INFO("saytodone", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_SayToDone);
	EVENT_INFO_WITH_ALIAS("ongrab", "on0x0080000", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnGrab);
	EVENT_INFO("onopen", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnOpen);
	EVENT_INFO("onclose", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnClose);
	EVENT_INFO_WITH_ALIAS("onfire", "on0x00400000", kEventParams_GameEvent, &s_MainEventHook, ScriptEventList::kEvent_OnFire);
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
	EVENT_INFO("postloadgame", kEventParams_OneInt, nullptr, 0);
	EVENT_INFO("runtimescripterror", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("deletegame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamegame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamenewgame", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("newgame", nullptr, nullptr, 0);
	EVENT_INFO("deletegamename", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamegamename", kEventParams_OneString, nullptr, 0);
	EVENT_INFO("renamenewgamename", kEventParams_OneString, nullptr, 0);

	EVENT_INFO_FLAGS("nvsetestevent", kEventParams_OneInt_OneFloat_OneArray_OneString_OneForm_OneReference_OneBaseform,
		nullptr, 0, EventFlags::kFlag_AllowScriptDispatch); // dispatched via DispatchEventAlt, for unit tests

	ASSERT (kEventID_InternalMAX == s_eventInfos.size());


#undef EVENT_INFO
#undef EVENT_INFO_FLAGS
#undef EVENT_INFO_WITH_ALIAS

	InstallDestroyCIOSHook();	// handle a missing parameter value check.

}

bool RegisterEventEx(const char* name, const char* alias, bool isInternal, UInt8 numParams, EventArgType* paramTypes,
                     UInt32 eventMask, EventHookInstaller* hookInstaller, EventFlags flags)
{
	if (numParams > kNumMaxFilters) [[unlikely]]
		return false;
	if (!name) [[unlikely]]
		return false;

	EventInfo** infoPtr;
	if (!s_eventInfoMap.Insert(name, &infoPtr))
		return false; // event with this name already exists
	*infoPtr = &s_eventInfos.emplace_back(name, alias, paramTypes, numParams, eventMask, hookInstaller, flags);

	EventInfo& info = **infoPtr;
	if (alias && s_eventInfoMap.Insert(alias, &infoPtr))
	{
		*infoPtr = &info;
	}

	if (isInternal)
	{
		// provide fast access for internal events.
		s_internalEventInfos.Append(&info);
	}

	return true;
}

bool RegisterEvent(const char* name, UInt8 numParams, EventArgType* paramTypes, NVSEEventManagerInterface::EventFlags flags)
{
	return RegisterEventEx(name, nullptr, false, numParams, 
		paramTypes, 0, nullptr, flags);
}

bool RegisterEventWithAlias(const char* name, const char* alias, UInt8 numParams, EventArgType* paramTypes, 
	NVSEEventManagerInterface::EventFlags flags)
{
	return RegisterEventEx(name, alias, false, numParams, 
		paramTypes, 0, nullptr, flags);
}

bool SetNativeEventHandler(const char* eventName, NativeEventHandler func)
{
	EventCallback event(NativeEventHandlerInfo{ func });
	return SetHandler<true>(eventName, event, kHandlerPriority_Default);
}

bool RemoveNativeEventHandler(const char* eventName, NativeEventHandler func)
{
	const EventCallback event(NativeEventHandlerInfo { func });
	return RemoveHandler(eventName, event, kHandlerPriority_Invalid, nullptr);
}

bool EventHandlerExists(const char* ev, const EventCallback& handler, int priority = kHandlerPriority_Invalid)
{
	ScopedLock lock(s_criticalSection);
	if (EventInfo* infoPtr = TryGetEventInfoForName(ev))
	{
		CallbackMap& callbacks = infoPtr->callbacks;

		if (priority == kHandlerPriority_Invalid)
		{
			// Don't filter by priority.
			for (auto const &[priorityKey, nthHandler] : callbacks)
			{
				if (handler == nthHandler)
					return true;
			}
		}
		else
		{
			auto const priorityRange = callbacks.equal_range(priority);
			for (auto i = priorityRange.first; i != priorityRange.second; ++i)
			{
				if (i->second == handler)
					return true;
			}
		}
	}
	return false;
}


}
