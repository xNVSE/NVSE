#pragma once
#include "PluginManager.h"
#include "ScriptUtils.h"
#include "StackVector.h"

#ifdef RUNTIME

#include <string>

#include "ArrayVar.h"
#include "LambdaManager.h"
#include "PluginAPI.h"
#include <variant>

#include "FunctionScripts.h"
#include "Hooks_Gameplay.h"
#include <stdexcept>
#include <array>

class Script;
class TESForm;
class TESObjectREFR;
class BGSListForm;
class Actor;
typedef void (*EventHookInstaller)();

// For dispatching events to scripts.
// Scripts can register an event handler for any of the supported events.
// Can optionally specify filters to match against the event arguments.
// Event handler is a function script which must take the expected number and types of arguments associated with the event.
// Supporting hooks only installed if at least one handler is registered for a particular event.

namespace EventManager
{
	extern Stack<const char *> s_eventStack;
	extern UInt32 s_eventsInUse;

	struct EventInfo;
	// Contains a stable list of references to EventInfos.
	// Assumption: no EventInfos will be removed from this list.
	extern std::list<EventInfo> s_eventInfos; 
	
	using EventInfoMap = UnorderedMap<const char*, EventInfo*>;
	// Links event names (and aliases) to their matching eventInfo.
	extern EventInfoMap s_eventInfoMap;

	EventInfo* TryGetEventInfoForName(const char* eventName);

	enum eEventID
	{
		// correspond to ScriptEventList event masks
		kEventID_OnAdd,
		kEventID_OnActorEquip,
		kEventID_OnDrop,
		kEventID_OnActorUnequip,
		kEventID_OnDeath,
		kEventID_OnMurder,
		kEventID_OnCombatEnd,
		kEventID_OnHit,
		kEventID_OnHitWith,
		kEventID_OnPackageChange,
		kEventID_OnPackageStart,
		kEventID_OnPackageDone,
		kEventID_OnLoad,
		kEventID_OnMagicEffectHit,
		kEventID_OnSell,
		kEventID_OnStartCombat,
		kEventID_SayToDone,
		kEventID_OnGrab,
		kEventID_OnOpen,
		kEventID_OnClose,
		kEventID_OnFire,
		kEventID_OnTrigger,
		kEventID_OnTriggerEnter,
		kEventID_OnTriggerLeave,
		kEventID_OnReset,

		kEventID_ScriptEventListMAX,

		// special-cased game events
		kEventID_OnActivate = kEventID_ScriptEventListMAX,
		kEventID_OnDropItem,

		kEventID_GameEventMAX,

		// NVSE internal events, correspond to NVSEMessagingInterface messages
		kEventID_ExitGame = kEventID_GameEventMAX,
		kEventID_ExitToMainMenu,
		kEventID_LoadGame,
		kEventID_SaveGame,
		kEventID_QQQ,
		kEventID_PostLoadGame,
		kEventID_RuntimeScriptError,
		kEventID_DeleteGame,
		kEventID_RenameGame,
		kEventID_RenameNewGame,
		kEventID_NewGame,
		kEventID_DeleteGameName,
		kEventID_RenameGameName,
		kEventID_RenameNewGameName,
		kEventID_DebugEvent, // for unit tests

		kEventID_InternalMAX,
		kEventID_ExternalEvent = kEventID_InternalMAX, // could be user or plugin-defined.

		// User/plugin-defined events don't get an ID assigned

		kEventID_INVALID = 0xFFFFFFFF
	};

	// Gives fast access to EventInfos for internal events, whose existence we know in advance.
	// Each index in the vector represents an eventID.
	using InternalEventVec = Vector<EventInfo*>;
	extern InternalEventVec s_internalEventInfos;

	using NativeEventHandler = NVSEEventManagerInterface::NativeEventHandler;
	struct NativeEventHandlerInfo
	{
		NativeEventHandler m_func{};
		const char* m_pluginName = "[UNKNOWN PLUGIN]";
		const char* m_handlerName = "[NO NAME]";

		NativeEventHandlerInfo() = default;
		NativeEventHandlerInfo(NativeEventHandler func) : m_func(func) {}
		[[nodiscard]] bool InitWithPluginInfo(NativeEventHandler func, PluginHandle pluginHandle, const char* handlerName);
		[[nodiscard]] std::string GetStringRepresentation() const;
		[[nodiscard]] ArrayVar* GetArrayRepresentation(UInt8 modIndex) const;

		bool operator==(const NativeEventHandlerInfo& rhs) const { return m_func == rhs.m_func; }
		operator bool() const { return m_func != nullptr; }
	};

	using EventArgType = NVSEEventManagerInterface::ParamType;
	using EventFlags = NVSEEventManagerInterface::EventFlags;

	inline bool IsFormParam(EventArgType pType)
	{
		return NVSEEventManagerInterface::IsFormParam(pType);
	}
	inline bool IsPtrParam(EventArgType pType)
	{
		return NVSEEventManagerInterface::IsPtrParam(pType);
	}
	inline EventArgType GetNonPtrParamType(EventArgType pType)
	{
		return NVSEEventManagerInterface::GetNonPtrParamType(pType);
	}
	Script::VariableType ParamTypeToVarType(EventArgType pType);
	EventArgType VarTypeToParamType(Script::VariableType varType);
	DataType ParamTypeToDataType(EventArgType pType);

	bool ParamTypeMatches(EventArgType from, EventArgType to);

	// For classic event handlers, 2 params is the max.
	static constexpr UInt8 kMaxArgsForClassicEvents = 2;
	// For newer event handlers, max # of params is tied to how many args can be dispatched via UDF.
	static constexpr auto kNumMaxFilters = kMaxUdfParams;

	using RawArgStack = StackVector<void*, kMaxUdfParams>;
	using ClassicRawArgStack = StackVector<void*, kMaxArgsForClassicEvents>;

	using ArgTypeStack = StackVector<EventArgType, kMaxUdfParams>;
	using ClassicArgTypeStack = StackVector<EventArgType, kMaxArgsForClassicEvents>;


	// Represents an event handler registered for an event.
	class EventCallback
	{
		void TrySaveLambdaContext();
		bool ValidateFirstOrSecondFilter(bool isFirst, const EventInfo& parent, std::string& outErrorMsg) const;
		bool ValidateFirstAndSecondFilter(const EventInfo& parent, std::string& outErrorMsg) const;

	public:
		EventCallback() = default;
		~EventCallback() = default;
		EventCallback(Script *funcScript, TESForm *sourceFilter = nullptr, TESForm *objectFilter = nullptr)
			: toCall(funcScript), source(sourceFilter), object(objectFilter) {}

		EventCallback(const NativeEventHandlerInfo &funcInfo) : toCall(funcInfo) {}

		EventCallback(const EventCallback &other) = delete;
		EventCallback &operator=(const EventCallback &other) = delete;

		EventCallback(EventCallback &&other) noexcept;
		EventCallback &operator=(EventCallback &&other) noexcept;

		[[nodiscard]] bool operator==(const EventCallback& rhs) const;

		// If variant is Maybe_Lambda, must try to capture lambda context once the EventCallback is confirmed to stay.
		using CallbackFunc = std::variant<LambdaManager::Maybe_Lambda, NativeEventHandlerInfo>;
		CallbackFunc toCall{};

		TESForm *source{}; // first arg to handler (reference or base form or form list)
		TESForm *object{}; // second arg to handler
		// ^ If one is null, then it is unfiltered.

		bool removed{};
		bool pendingRemove{};
		bool flushOnLoad = false;

		using Index = UInt32;
		using Filter = SelfOwningArrayElement;

		// Indexes for filters must respect the max amount of BaseFilters for the base event definition.
		// If no filter is at an index = it is unfiltered for the nth BaseFilter.
		// Using a map to avoid adding duplicate indexes.
		std::map<Index, Filter> filters;

		[[nodiscard]] bool ValidateFilters(std::string& outErrorMsg, const EventInfo& parent) const;

		[[nodiscard]] std::string GetFiltersAsStr() const;
		[[nodiscard]] ArrayVar* GetFiltersAsArray(const Script* scriptObj) const;
		[[nodiscard]] std::string GetCallbackFuncAsStr() const;

		// [0] = callbackFunc (script for udf, or int for func ptr), [1] = filters string map.
		[[nodiscard]] ArrayVar* GetAsArray(const Script* scriptObj) const;

		[[nodiscard]] bool IsRemoved() const { return removed; }
		void SetRemoved(bool bSet) { removed = bSet; }
		[[nodiscard]] bool FlushesOnLoad() const { return flushOnLoad; }

		// True if registered by SetEventHandlerAlt.
		[[nodiscard]] bool IsAltRegistered() const { return flushOnLoad; }

		[[nodiscard]] Script* TryGetScript() const;
		[[nodiscard]] bool HasCallbackFunc() const;

		[[nodiscard]] bool DoDeprecatedFiltersMatch(const RawArgStack& args, const ArgTypeStack* argTypes,
		                                            const EventInfo& eventInfo, ExpressionEvaluator* eval = nullptr) const;

		// 1. Must have the same # of Args and ArgTypes.
		// 2. A filter for User-Defined Events will automatically fail to match if that filter's index is > # of args dispatched.
		// ^ While we could assume that a 0 was dispatched for that arg, we don't know the type of that null value.
		// ^^ We would encounter an ambiguity while comparing this non-typed null value to an array filter containing 0;
		//	  is that null value an int, meaning it should match since the array contains 0?
		//	  Or is it a null array, meaning it should not match with the valid (non-null) array?
		// ^^^ Thus, to avoid this situation, I opted to simply not allow filtering values that aren't dispatched.
		// -Demorome
		template<bool ExtractIntTypeAsFloat, UInt8 NumMaxArgs>
		bool DoNewFiltersMatch(
			TESObjectREFR* thisObj,
			const StackVector<void*, NumMaxArgs>& args,
			const StackVector<EventArgType, NumMaxArgs>& argTypes,
			const EventInfo& eventInfo,
			ExpressionEvaluator* eval) const;

		template<bool ExtractIntTypeAsFloat>
		bool DoesParamMatchFiltersInArray(const Filter& arrayFilter,
			EventArgType paramType, void* param, int index) const;

		// If "this" would run anytime toCheck would run or more, return true (toCheck should be removed; "this" has more or equally generic filters).
		// The above rule is used to remove redundant callbacks in one fell swoop.
		// Assumes both have the same callbacks.
		// Eval is passed to report errors.
		[[nodiscard]] bool ShouldRemoveCallback(const EventCallback& toCheck, const EventInfo& evInfo, ExpressionEvaluator* eval = {}) const;

		std::unique_ptr<ScriptToken> Invoke(EventInfo &eventInfo, const ClassicArgTypeStack& argTypes, void *arg0, void *arg1);

		// If the EventCallback is confirmed to stay, then call this to wrap up loose ends, e.g save lambda var context.
		void Confirm();
	};

	using enum NVSEEventManagerInterface::SpecialHandlerPriorities;

	// Ordered by priority; a single priority can have multiple associated callbacks.
	// Greatest priority = will run first.
	using CallbackMap = std::multimap<int, EventCallback, std::greater<>>;

	struct EventInfo
	{
		EventInfo(const char *name_, EventArgType *params_, UInt8 nParams_, UInt32 eventMask_, EventHookInstaller *installer_,
				  EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), paramTypes(params_), numParams(nParams_), eventMask(eventMask_), installHook(installer_), flags(flags)
		{
			Init();
		}
		// ctor w/ alias
		EventInfo(const char* name_, const char* alias_, EventArgType* params_, UInt8 nParams_, UInt32 eventMask_, EventHookInstaller* installer_,
			EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), alias(alias_), paramTypes(params_), numParams(nParams_), eventMask(eventMask_), installHook(installer_), flags(flags)
		{
			Init();
		}

		EventInfo(const char *name_, EventArgType *params_, UInt8 numParams_, EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), paramTypes(params_), numParams(numParams_), flags(flags)
		{
			Init();
		}
		// ctor w/ alias
		EventInfo(const char* name_, const char* alias_, EventArgType* params_, UInt8 numParams_, EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), alias(alias_), paramTypes(params_), numParams(numParams_), flags(flags)
		{
			Init();
		}

		EventInfo(const EventInfo &other) = delete;
		EventInfo& operator=(const EventInfo& other) = delete;

		EventInfo(EventInfo&& other) noexcept :
			evName(other.evName), alias(other.alias), paramTypes(other.paramTypes), numParams(other.numParams),
			eventMask(other.eventMask), callbacks(std::move(other.callbacks)), installHook(other.installHook), flags(other.flags)
		{}

		const char *evName; //should never be nullptr
		const char *alias{}; //could be nullptr (unused)
		EventArgType *paramTypes;
		UInt8 numParams = 0;
		UInt32 eventMask = 0;
		CallbackMap callbacks;
		EventHookInstaller *installHook{}; // if a hook is needed for this event type, this will be non-null.
										   // install it once and then set *installHook to NULL. Allows multiple events
										   // to use the same hook, installing it only once.
		EventFlags flags = EventFlags::kFlags_None;

		[[nodiscard]] bool FlushesOnLoad() const {
			return flags & EventFlags::kFlag_FlushOnLoad;
		}
		[[nodiscard]] bool HasUnknownArgTypes() const {
			return flags & EventFlags::kFlag_HasUnknownArgTypes;
		}
		[[nodiscard]] bool IsUserDefined() const {
			return flags & EventFlags::kFlag_IsUserDefined;
		}
		[[nodiscard]] bool AllowsScriptDispatch() const {
			return flags & EventFlags::kFlag_AllowScriptDispatch;
		}
		[[nodiscard]] bool ReportErrorIfNoResultGiven() const {
			return flags & EventFlags::kFlag_ReportErrorIfNoResultGiven;
		}
		// n is 0-based
		// Assumes that the index was already checked as valid (i.e numParams was checked).
		[[nodiscard]] EventArgType TryGetNthParamType(size_t n) const
		{
			return !HasUnknownArgTypes() ? GetNonPtrParamType(paramTypes[n]) : EventArgType::eParamType_Anything;
		}
		[[nodiscard]] ArgTypeStack GetArgTypesAsStackVector() const;
		[[nodiscard]] ClassicArgTypeStack GetClassicArgTypesAsStackVector() const;

		// Useful to double-check the args dispatched via script for an internally-defined event.
		// Can also verify args to check which event callbacks would fire with hypothetical args (for GetEventHandlers_Execute).
		// Only useful for Plugin-Defined Events.
		[[nodiscard]] bool ValidateDispatchedArgTypes(const ArgTypeStack& argTypes, ExpressionEvaluator* eval = nullptr) const;

		// If any ptr-type values are passed, then this will dereference them.
		[[nodiscard]] RawArgStack GetEffectiveArgs(RawArgStack &passedArgs);

	private:
		// Only used for plugin-defined events with known types.
		// Set up once during event registration.
		bool hasPtrArg = false;

		void Init() {
			// Assume numParams == 0 if no paramTypes are passed.
			for (decltype(numParams) i = 0; i < numParams; i++) {
				if (IsPtrParam(paramTypes[i])) {
					hasPtrArg = true;
					break;
				}
			}
		}
	};

	struct DeferredRemoveCallback
	{
		EventInfo* eventInfo;
		CallbackMap::iterator	iterator;

		DeferredRemoveCallback(EventInfo* pEventInfo, CallbackMap::iterator iter)
			: eventInfo(pEventInfo), iterator(std::move(iter))
		{}
		~DeferredRemoveCallback();
	};
	typedef Stack<DeferredRemoveCallback> DeferredRemoveList;
	extern DeferredRemoveList s_deferredRemoveList;

	template <bool IsInternalHandler>
	bool SetHandler(const char *eventName, EventCallback &toSet, int priority, ExpressionEvaluator* eval = nullptr);

	// removes handler only if all filters match
	bool RemoveHandler(const char *eventName, const EventCallback& toRemove, int priority, ExpressionEvaluator* eval = nullptr);

	// handle an NVSEMessagingInterface message
	void HandleNVSEMessage(UInt32 msgID, void *data);

	// Deprecated in favor of EventManager::DispatchEvent options
	void HandleInternalEvent(EventInfo& eventInfo, void* arg0, void* arg1, void (*cleanupCallback)() = nullptr);

	// Used for the (deprecrated) DispatchEvent_Execute
	void HandleUserDefinedEvent(EventInfo& eventInfo, void* arg0, void* arg1, ExpressionEvaluator *eval);

	// handle an eventID directly
	// Deprecated in favor of EventManager::DispatchEvent options
	void __stdcall HandleEvent(eEventID id, void *arg0, void *arg1, void (*cleanupCallback)() = nullptr);

	void ClearFlushOnLoadEventHandlers();

	// name of whatever event is currently being handled, empty string if none
	const char *GetCurrentEventName();

	// called each frame to update internal state
	void Tick();

	void Init();

	bool RegisterEventEx(const char *name, const char* alias, bool isInternal, UInt8 numParams, EventArgType *paramTypes,
	                     UInt32 eventMask = 0, EventHookInstaller *hookInstaller = nullptr,
	                     EventFlags flags = EventFlags::kFlags_None);

	// Exported
	bool RegisterEvent(const char *name, UInt8 numParams, EventArgType *paramTypes, NVSEEventManagerInterface::EventFlags flags);

	// Exported
	bool RegisterEventWithAlias(const char* name, const char* alias, UInt8 numParams, EventArgType* paramTypes,
		NVSEEventManagerInterface::EventFlags flags);

	bool SetNativeEventHandler(const char *eventName, NativeEventHandler func);
	bool RemoveNativeEventHandler(const char *eventName, NativeEventHandler func);

	using DispatchReturn = NVSEEventManagerInterface::DispatchReturn;
	using DispatchCallback = NVSEEventManagerInterface::DispatchCallback;
	using PostDispatchCallback = NVSEEventManagerInterface::PostDispatchCallback;

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& args,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback);

	// Eval can be passed for better error reporting.
	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRawWithTypes(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& args, ArgTypeStack& argTypes,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, 
		PostDispatchCallback postCallback, ExpressionEvaluator* eval = nullptr);

	bool DispatchEvent(const char* eventName, TESObjectREFR* thisObj, ...);
	DispatchReturn DispatchEventAlt(const char *eventName, DispatchCallback resultCallback, 
		void *anyData, TESObjectREFR *thisObj, ...);

	bool DispatchEventThreadSafe(const char* eventName, PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...);
	DispatchReturn DispatchEventAltThreadSafe(const char* eventName, DispatchCallback resultCallback, void* anyData, 
		PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...);

	// dispatch a user-defined event from a script (for Cmd_DispatchEvent)
	// Cmd_DispatchEventAlt provides more flexibility with how args are passed.
	bool DispatchUserDefinedEvent(const char *eventName, Script *sender, UInt32 argsArrayId, const char *senderName, 
		ExpressionEvaluator* eval = nullptr);

	void SetNativeHandlerFunctionValue(NVSEArrayVarInterface::Element& value);

	bool SetNativeEventHandlerWithPriority(const char* eventName, NativeEventHandler func,
		PluginHandle pluginHandle, const char* handlerName, int priority);
	bool RemoveNativeEventHandlerWithPriority(const char* eventName, NativeEventHandler func, int priority);

	struct ScriptHandlerFilters
	{
		TESForm* scriptsToIgnore{}; ArrayVar* pluginsToIgnore{}; ArrayVar* pluginHandlersToIgnore{};

		// 'scriptsToIgnore' can either be nullptr, a script, or a formlist of scripts.
		// If non-null, 'pluginsToIgnore' and 'internalHandlersToIgnore' must contain string-type name filters.
		[[nodiscard]] bool ShouldIgnore(const EventCallback& toFilter) const;
	};

	struct PluginHandlerFilters
	{
		TESForm** scriptsToIgnore{}; UInt32 numScriptsToIgnore{};
		const char** pluginsToIgnore{}; UInt32 numPluginsToIgnore{};
		const char** pluginHandlersToIgnore{}; UInt32 numPluginHandlersToIgnore{};

		[[nodiscard]] bool ShouldIgnore(const EventCallback& toFilter) const;
	};

	// A quick way to check for a handler priority conflict, i.e. if a handler is expected to run first/last. 
	/* If any non - excluded handlers are found above / below 'startPriority', returns false.
	 * 'startPriority' is assumed to NOT be 0.
	*/
	// Returns false if 'func' is not found at 'startPriority'. It can appear elsewhere and will be ignored.
	/* Returns false upon finding any non-excluded handlers with the same priority as a handler with the 'func' callback.
	 * This is to report time bombs before they happen, as relying on insertion order within a single priority leads to hard-to-debug conflicts
	*/
	template <bool CheckRunFirst, class Filters_t>
	bool IsEventHandlerFirstOrLast(const char* eventName, EventCallback::CallbackFunc func, 
		int startPriority, Filters_t filters);

	// Used after determining that the handler will not run first/last as desired, to list the conflict culprits.
	template <bool CheckHigherPriority, class Filters_t>
	ArrayVar* GetHigherOrLowerPriorityEventHandlers(const char* eventName, EventCallback::CallbackFunc func, 
		int startPriority, Filters_t filters, Script* scriptObj = nullptr);

	namespace ExportedToPlugins
	{
		bool IsEventHandlerFirst(const char* eventName, NativeEventHandler func, int priority,
			TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
			const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
			const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore);

		bool IsEventHandlerLast(const char* eventName, NativeEventHandler func, int priority,
			TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
			const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
			const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore);

		NVSEArrayVarInterface::Array* GetHigherPriorityEventHandlers(const char* eventName, NativeEventHandler func, int priority,
			TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
			const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
			const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore);

		NVSEArrayVarInterface::Array* GetLowerPriorityEventHandlers(const char* eventName, NativeEventHandler func, int priority,
			TESForm** scriptsToIgnore, UInt32 numScriptsToIgnore,
			const char** pluginsToIgnore, UInt32 numPluginsToIgnore,
			const char** pluginHandlersToIgnore, UInt32 numPluginHandlersToIgnore);
	}


	// == Template definitions

	bool DoesFormMatchFilter(TESForm* inputForm, TESForm* filterForm, bool expectReference, const UInt32 recursionLevel = 0);

	// eParamType_Anything is treated as "use default param type" (usually for a User-Defined Event).
	template<bool ExtractIntTypeAsFloat>
	bool DoesFilterMatch(const ArrayElement& sourceFilter, void* arg, EventArgType argType)
	{
		switch (sourceFilter.DataType()) {
		case kDataType_Numeric:
		{
			double filterNumber{};
			sourceFilter.GetAsNumber(&filterNumber);
			// This filter could be inside an array, so we can't be sure if the number is floored.
			if (argType == EventArgType::eParamType_Int)
				filterNumber = floor(filterNumber);

			float inputNumber;
			if constexpr (ExtractIntTypeAsFloat)
			{
				// this function could be being called by a function, where even ints are passed as floats.
				// Alternatively, it could be called by some internal function that got the param from an ArrayElement 
				inputNumber = *reinterpret_cast<float*>(&arg);
				if (argType == EventArgType::eParamType_Int)  // mostly for if IsParamExistingFilter
					inputNumber = floor(inputNumber);
			}
			else  
			{
				// this function is being called internally, via a va_arg-using function, so expect ints to be packed like ints.
				inputNumber = (argType == EventArgType::eParamType_Int)
					? static_cast<float>(*reinterpret_cast<UInt32*>(&arg))
					: *reinterpret_cast<float*>(&arg);
			}
			
			if (!FloatEqual(inputNumber, static_cast<float>(filterNumber)))
				return false;
			break;
		}
		case kDataType_Form:
		{
			UInt32 filterFormId{};
			sourceFilter.GetAsFormID(&filterFormId);

			// Allow matching a null form filter with a null input.
			auto* inputForm = static_cast<TESForm*>(arg);
			auto* filterForm = LookupFormByID(filterFormId);
			bool const expectReference = argType != EventArgType::eParamType_BaseForm;

			if (!DoesFormMatchFilter(inputForm, filterForm, expectReference))
				return false;
			break;
		}
		case kDataType_String:
		{
			const char* filterStr{};
			sourceFilter.GetAsString(&filterStr);
			const auto inputStr = static_cast<const char*>(arg);
			if (inputStr == filterStr)
				return true;
			if (!filterStr || !inputStr || StrCompare(filterStr, inputStr) != 0)
				return false;
			break;
		}
		case kDataType_Array:
		{
			ArrayID filterArrayId{};
			sourceFilter.GetAsArray(&filterArrayId);
			const auto inputArrayId = *reinterpret_cast<ArrayID*>(&arg);
			if (!inputArrayId)
				return false;
			const auto inputArray = g_ArrayMap.Get(inputArrayId);
			const auto filterArray = g_ArrayMap.Get(filterArrayId);
			if (!inputArray || !filterArray)
				return false;
			if (!inputArray->Equals(filterArray))
				return false;
			break;
		}
		case kDataType_Invalid:
			break;
		}
		return true;
	}

	template<bool ExtractIntTypeAsFloat>
	bool EventCallback::DoesParamMatchFiltersInArray(const Filter& arrayFilter, EventArgType paramType, void* param, int index) const
	{
		ArrayID arrayFiltersId{};
		arrayFilter.GetAsArray(&arrayFiltersId);
		auto* arrayFilters = g_ArrayMap.Get(arrayFiltersId);
		if (!arrayFilters)
		{
			ShowRuntimeError(TryGetScript(), "While checking event filters in array at index %d, the array was invalid/uninitialized (array id: %u).", index, arrayFiltersId);
			return false;
		}
		// If array of filters is (string)map, then ignore the keys.
		for (auto iter = arrayFilters->Begin(); !iter.End(); ++iter)
		{
			auto const& elem = *iter.second();
			if (ParamTypeToVarType(paramType) != DataTypeToVarType(elem.DataType()) 
				&& paramType != EventArgType::eParamType_Anything) [[unlikely]]
			{
				continue;
			}
			if (DoesFilterMatch<ExtractIntTypeAsFloat>(elem, param, paramType))
				return true;
		}
		return false;
	}

	template<bool ExtractIntTypeAsFloat, UInt8 NumMaxArgs>
	bool EventCallback::DoNewFiltersMatch(
		TESObjectREFR* thisObj, 
		const StackVector<void*, NumMaxArgs>& args, 
		const StackVector<EventArgType, NumMaxArgs>& argTypes,
	    const EventInfo& eventInfo, 
		ExpressionEvaluator* eval) const
	{
		auto const numParams = argTypes->size();
		if (args->size() != numParams) [[unlikely]]  // only possible for internally-defined events, where ArgTypes are known in advance.
		{
			ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if new filters match for event %s, # of expected args (%u) != # of args passed (%u)",
				eventInfo.evName, numParams, args->size());
			return false;
		}

#if 0	// Commented out to report errors further down with having a filter for an arg that isn't explicitly dispatched.
		if (!numParams) //no args to filter
			return true;
#endif

		for (auto& [index, filter] : filters)
		{
			bool const isCallingRefFilter = index == 0;

			if (index > numParams)
			{
				// insufficient params to match that filter (see comment #2 at the function declaration).
				ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if new filters match for event %s, saw filter at invalid index #%u (# of args: %u)",
					eventInfo.evName, index, numParams);
				return false; 
			}

			void* arg = isCallingRefFilter ? thisObj : args->at(index - 1);
			auto const argType = isCallingRefFilter ? EventArgType::eParamType_Reference : argTypes->at(index - 1);
			const auto filterVarType = DataTypeToVarType(filter.DataType());
			const auto argVarType = ParamTypeToVarType(argType);

			if (filterVarType != argVarType) 
			{
				// Check if it's an array-of-filters containing non-arrays.
				if (filterVarType != Script::eVarType_Array)
				{
					// Should only happen if argTypes weren't checked against known types, or for User-Defined Events, user error.
					ShowRuntimeScriptError(this->TryGetScript(), eval, "While checking if new filters match for event %s, filter #%u's type (%s) did not match the arg's (%s)",
						eventInfo.evName, index, VariableTypeToName(filterVarType), VariableTypeToName(argVarType));
					return false;
				}

				// elements of array are filters
				if (!DoesParamMatchFiltersInArray<ExtractIntTypeAsFloat>(filter, argType, arg, index))
					return false;
				continue;
			}

			// Try directly comparing them.
			if (DoesFilterMatch<ExtractIntTypeAsFloat>(filter, arg, argType))
				continue;

			// If both are arrays, then maybe the filter array contains multiple arrays to match.
			if (filterVarType == Script::eVarType_Array)
			{
				if (DoesParamMatchFiltersInArray<ExtractIntTypeAsFloat>(filter, argType, arg, index))
					continue;
			}

			// If no matches are found
			return false;
		}
		return true;
	}

	extern ICriticalSection s_criticalSection;

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& passedArgs, ArgTypeStack& argTypes,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback,
		ExpressionEvaluator* eval = nullptr);

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRawWithTypes(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& args, ArgTypeStack& argTypes,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread,
		PostDispatchCallback postCallback, ExpressionEvaluator* eval)
	{
		// For internally-defined events with known argTypes, scripter could be passing the wrong argTypes (or wrong # of args)
		if (!eventInfo.ValidateDispatchedArgTypes(argTypes, eval)) [[unlikely]]
		{
			ShowRuntimeScriptError(nullptr, eval, "Caught attempt to dispatch event %s with invalid args.", eventInfo.evName);
			return DispatchReturn::kRetn_GenericError;
		}
		return DispatchEventRaw<ExtractIntTypeAsFloat>(eventInfo, thisObj, args, argTypes, 
			resultCallback, anyData, deferIfOutsideMainThread, postCallback, eval);
	}

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& args, 
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback)
	{
		if (eventInfo.HasUnknownArgTypes()) [[unlikely]]
			throw std::logic_error("DispatchEventRaw was called on an event that has unknown types.");

		ArgTypeStack argTypes = eventInfo.GetArgTypesAsStackVector();
		return DispatchEventRaw<ExtractIntTypeAsFloat>(eventInfo, thisObj, args, argTypes,
			resultCallback, anyData, deferIfOutsideMainThread, postCallback);
	}

	// For events that are best deferred until Tick(), usually to avoid multithreading issues.
	template <bool ExtractIntTypeAsFloat>
	struct DeferredCallback
	{
		EventInfo& eventInfo;
		TESObjectREFR* thisObj;
		RawArgStack args;
		ArgTypeStack argTypes;
		DispatchCallback resultCallback;
		void* callbackData;
		PostDispatchCallback postCallback;

		DeferredCallback(EventInfo& eventInfo, TESObjectREFR* thisObj, const RawArgStack& args, 
			const ArgTypeStack &argTypes, DispatchCallback resultCallback, void* callbackData, PostDispatchCallback postCallback)
			:	eventInfo(eventInfo), thisObj(thisObj), args(args), argTypes(argTypes),
				resultCallback(resultCallback), callbackData(callbackData), postCallback(postCallback)
		{}

		~DeferredCallback()
		{
			DispatchEventRaw<ExtractIntTypeAsFloat>(eventInfo, thisObj, args, argTypes, resultCallback, callbackData,
				false, postCallback);
		}
	};
	extern std::deque<DeferredCallback<false>> s_deferredCallbacksDefault;
	extern std::deque<DeferredCallback<true>> s_deferredCallbacksWithIntsPackedAsFloats;

	extern NVSEArrayVarInterface::Element *g_NativeHandlerResult;

	template <bool CheckRunsFirst, class Filters_t>
	bool IsEventHandlerFirstOrLast(const char* eventName, EventCallback::CallbackFunc func, 
		int startPriority, Filters_t filters)
	{
		const auto eventPtr = TryGetEventInfoForName(eventName);
		if (!eventPtr)
			return false;
		const EventInfo& eventInfo = *eventPtr;
		if (eventInfo.callbacks.empty())
			return false;

		// Parameter callback must be found at least once AT THE STARTING PRIORITY, else return false.
		bool foundParameterCallbackAtStart = false;

		auto i = eventInfo.callbacks.find(startPriority);

		if constexpr (CheckRunsFirst)
		{
			// See if any handlers are beating it at HIGHER priorities, or are at the same priority.

			if (i == eventInfo.callbacks.end())
				return false; //else, there must be at least one element in our priority range

			// Move all the way to the end of the priority range, so we can loop backwards starting from the end.
			i = eventInfo.callbacks.equal_range(i->first).second;  // first iterator past the priority range
			auto j = std::make_reverse_iterator(i); // reverse iterator will point back to the last element in the range.

			// Decrement i to go higher in priority; highest priorities are first in the container.
			// Thus, do opposite for reverse iterator.
			do
			{
				auto const priority = j->first;
				auto const& callback = j->second;

				if (filters.ShouldIgnore(callback))
					continue;

				if (callback.toCall != func)
					return false;

				if (priority == startPriority)
					foundParameterCallbackAtStart = true;
			} while (++j != eventInfo.callbacks.rend());
		}
		else
		{
			// See if any handlers are beating it at LOWER priorities, or are at the same priority.

			// Increment i to go lower in priority, since higher priorities are ordered first.
			for (; i != eventInfo.callbacks.end(); ++i)
			{
				auto const priority = i->first;
				auto const& callback = i->second;

				if (filters.ShouldIgnore(callback))
					continue;

				if (callback.toCall != func)
					return false;

				if (priority == startPriority)
					foundParameterCallbackAtStart = true;
			}
		}
		
		return foundParameterCallbackAtStart;
	}

	template <bool CheckHigherPriority, class Filters_t>
	ArrayVar* GetHigherOrLowerPriorityEventHandlers(const char* eventName, EventCallback::CallbackFunc func,
		int startPriority, Filters_t filters, Script* scriptObj)
	{
		auto* result = g_ArrayMap.Create(kDataType_Numeric, false, scriptObj ? scriptObj->GetModIndex() : 0xFF);

		const auto eventPtr = TryGetEventInfoForName(eventName);
		if (!eventPtr)
			return result;
		const EventInfo& eventInfo = *eventPtr;
		if (eventInfo.callbacks.empty())
			return result;

		// Parameter callback must be found at least once AT THE STARTING PRIORITY, else return nullptr (invalid array).
		bool foundParameterCallbackAtStart = false;

		auto i = eventInfo.callbacks.find(startPriority);

		if constexpr (CheckHigherPriority)
		{
			// See if any handlers are beating it at HIGHER priorities, or are at the same priority.

			if (i == eventInfo.callbacks.end())
				return nullptr; //else, there must be at least one element in our priority range

			// Move all the way to the end of the priority range, so we can loop backwards starting from the end.
			i = eventInfo.callbacks.equal_range(i->first).second;  // first iterator past the priority range
			auto j = std::make_reverse_iterator(i); // reverse iterator will point back to the last element in the range.

			// Decrement i to go higher in priority; highest priorities are first in the container.
			// Thus, do opposite for reverse iterator.
			do
			{
				auto const priority = j->first;
				auto const& callback = j->second;

				if (filters.ShouldIgnore(callback))
					continue;

				if (callback.toCall != func)
				{
					result->SetElementArray(static_cast<double>(priority), callback.GetAsArray(scriptObj)->ID());
				}
				else if (priority == startPriority)
					foundParameterCallbackAtStart = true;
			} while (++j != eventInfo.callbacks.rend());
		}
		else
		{
			// See if any handlers are beating it at LOWER priorities, or are at the same priority.

			// Increment i to go lower in priority, since higher priorities are ordered first.
			for (; i != eventInfo.callbacks.end(); ++i)
			{
				auto const priority = i->first;
				auto const& callback = i->second;

				if (filters.ShouldIgnore(callback))
					continue;

				if (callback.toCall != func)
				{
					result->SetElementArray(static_cast<double>(priority), callback.GetAsArray(scriptObj)->ID());
				}
				else if (priority == startPriority)
					foundParameterCallbackAtStart = true;
			}
		}

		return foundParameterCallbackAtStart ? result : nullptr;
	}

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(EventInfo& eventInfo, TESObjectREFR* thisObj, RawArgStack& passedArgs, ArgTypeStack &argTypes,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback,
		ExpressionEvaluator* eval)
	{
		if (deferIfOutsideMainThread && GetCurrentThreadId() != g_mainThreadID)
		{
			ScopedLock lock(s_criticalSection);
			if constexpr (ExtractIntTypeAsFloat)
			{
				s_deferredCallbacksWithIntsPackedAsFloats.emplace_back(eventInfo, thisObj, passedArgs, argTypes, resultCallback, anyData, postCallback);
			}
			else
			{
				s_deferredCallbacksDefault.emplace_back(eventInfo, thisObj, passedArgs, argTypes, resultCallback, anyData, postCallback);
			}
			return DispatchReturn::kRetn_Deferred;
		}

		DispatchReturn result = DispatchReturn::kRetn_Normal;
		bool const reportErrorIfNoResultGiven = eventInfo.ReportErrorIfNoResultGiven();

		ScopedLock lock(s_criticalSection);	//for event stack and NativeHandlerResult
		//TODO: optimize if needed

		// handle immediately
		s_eventStack.Push(eventInfo.evName);

		for (auto& [priority, callback] : eventInfo.callbacks)
		{
			if (callback.IsRemoved())
				continue;

			auto args = eventInfo.GetEffectiveArgs(passedArgs);

			if (!callback.DoDeprecatedFiltersMatch(args, eventInfo.HasUnknownArgTypes() ? &argTypes : nullptr, eventInfo, eval))
				continue;
			if (!callback.DoNewFiltersMatch<ExtractIntTypeAsFloat>(thisObj, args, argTypes, eventInfo, eval))
				continue;

			result = std::visit(overloaded{
				[=, &args](const LambdaManager::Maybe_Lambda& script) -> DispatchReturn
				{
					using FunctionCaller = std::conditional_t<ExtractIntTypeAsFloat, InternalFunctionCallerAlt, InternalFunctionCaller>;

					FunctionCaller caller(script.Get(), thisObj, nullptr, true); // don't report errors if passing more args to a UDF than it can absorb.
					caller.SetArgsRaw(args->size(), args->data());
					auto const res = UserFunctionManager::Call(std::move(caller));
					if (resultCallback)
					{
						NVSEArrayVarInterface::Element elem;
						if (PluginAPI::BasicTokenToPluginElem(res.get(), elem, reportErrorIfNoResultGiven ? script.Get() : nullptr)
							|| !reportErrorIfNoResultGiven)
						{
							// Will pass an invalid elem if there is no result and reportErrorIfNoResultGiven is false.
							return resultCallback(elem, anyData) ? DispatchReturn::kRetn_Normal : DispatchReturn::kRetn_EarlyBreak;
						}
						return DispatchReturn::kRetn_GenericError;
					}
					return DispatchReturn::kRetn_Normal;
				},
				[=, &args](NativeEventHandlerInfo const handlerInfo) -> DispatchReturn
				{
					g_NativeHandlerResult = nullptr;
					handlerInfo.m_func(thisObj, args->data());  // g_NativeHandlerResult may change, needs a lock

					if (resultCallback)
					{
						if (!g_NativeHandlerResult)
						{
							if (!reportErrorIfNoResultGiven)
							{
								NVSEArrayVarInterface::Element elem{};
								return resultCallback(elem, anyData) ? DispatchReturn::kRetn_Normal : DispatchReturn::kRetn_EarlyBreak;
							}

							ShowRuntimeError(nullptr, "Internal (native) handler (%s) called from plugin %s failed to return a value when one was expected.",
								handlerInfo.m_handlerName, handlerInfo.m_pluginName);
							return DispatchReturn::kRetn_GenericError;
						}
						return resultCallback(*g_NativeHandlerResult, anyData) ? DispatchReturn::kRetn_Normal : DispatchReturn::kRetn_EarlyBreak;
					}
					return DispatchReturn::kRetn_Normal;
				},
				}, callback.toCall);

			if (result != DispatchReturn::kRetn_Normal)
				break;
		}

		if (postCallback)
			postCallback(anyData, result);

		s_eventStack.Pop();

		return result;
	}

	bool DoSetHandler(EventInfo& info, EventCallback& toSet, int priority);

	template <bool IsInternalHandler>
	bool SetHandler(const char* eventName, EventCallback& toSet, int priority, ExpressionEvaluator* eval)
	{
		if (!toSet.HasCallbackFunc()) [[unlikely]]
			return false;

		if (priority == kHandlerPriority_Invalid) [[unlikely]]
		{
			ShowRuntimeScriptError(toSet.TryGetScript(), eval, "Can't set event handler with invalid priority 0 (below 0 is allowed). Default priority = %u.", kHandlerPriority_Default);
			return false;
		}
		if (priority > kHandlerPriority_Max) [[unlikely]]
		{
			ShowRuntimeScriptError(toSet.TryGetScript(), eval, "Can't set event handler with priority above %u.", kHandlerPriority_Max);
			return false;
		}
		if (priority < kHandlerPriority_Min) [[unlikely]]
		{
			ShowRuntimeScriptError(toSet.TryGetScript(), eval, "Can't set event handler with priority below %u.", kHandlerPriority_Min);
			return false;
		}

		EventInfo** eventInfoPtr = nullptr;
		{
			if constexpr (IsInternalHandler)
			{
				// Only use pre-defined events.
				if (eventInfoPtr = s_eventInfoMap.GetPtr(eventName); 
					!eventInfoPtr)
				{
					ShowRuntimeScriptError(nullptr, eval, "Trying to set an internal handler for unknown event %s. It must be defined before setting handlers.", eventName);
					return false;
				}
			}
			else
			{
				ScopedLock lock(s_criticalSection);
				if (s_eventInfoMap.Insert(eventName, &eventInfoPtr))
				{
					// have to assume registering for a user-defined event (for DispatchEvent) which has not been used before this point
					char* nameCopy = CopyString(eventName);
					StrToLower(nameCopy);
					*eventInfoPtr = &s_eventInfos.emplace_back(nameCopy, nullptr, 0, EventFlags::kFlag_IsUserDefined);
				}
			}
		}
		if (!eventInfoPtr) [[unlikely]]
			return false;
		//else, assume ptr is valid
		EventInfo& info = **eventInfoPtr;

		{ // nameless scope
			std::string errMsg;
			if (!toSet.ValidateFilters(errMsg, info)) [[unlikely]]
			{
				ShowRuntimeScriptError(toSet.TryGetScript(), eval, errMsg.c_str());
				return false;
			}
		}

		return DoSetHandler(info, toSet, priority);
	}


	//== Stuff for specific events.

	namespace OnSell
	{
		TESObjectREFR* GetSoldItemInvRef();
	}
};

#endif