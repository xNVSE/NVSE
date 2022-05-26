#pragma once
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

	using EventHandler = NVSEEventManagerInterface::EventHandler;
	using EventFilterType = NVSEEventManagerInterface::ParamType;
	using EventFlags = NVSEEventManagerInterface::EventFlags;

	inline bool IsParamForm(EventFilterType pType)
	{
		return NVSEEventManagerInterface::IsFormParam(pType);
	}
	Script::VariableType ParamTypeToVarType(EventFilterType pType);
	EventFilterType VarTypeToParamType(Script::VariableType varType);
	DataType ParamTypeToDataType(EventFilterType pType);

	bool ParamTypeMatches(EventFilterType from, EventFilterType to);

	// Represents an event handler registered for an event.
	class EventCallback
	{
		void TrySaveLambdaContext();
		bool ValidateFirstOrSecondFilter(bool isFirst, const EventInfo& parent, std::string& outErrorMsg) const;
		bool ValidateFirstAndSecondFilter(const EventInfo& parent, std::string& outErrorMsg) const;

	public:
		// If variant is Maybe_Lambda, must try to capture lambda context once the EventCallback is confirmed to stay.
		using CallbackFunc = std::variant<LambdaManager::Maybe_Lambda, EventHandler>;

		EventCallback() = default;
		~EventCallback() = default;
		EventCallback(Script *funcScript, TESForm *sourceFilter = nullptr, TESForm *objectFilter = nullptr)
			: toCall(funcScript), source(sourceFilter), object(objectFilter) {}

		EventCallback(EventHandler func, TESForm *sourceFilter = nullptr, TESForm *objectFilter = nullptr)
			: toCall(func), source(sourceFilter), object(objectFilter) {}

		EventCallback(const EventCallback &other) = delete;
		EventCallback &operator=(const EventCallback &other) = delete;

		EventCallback(EventCallback &&other) noexcept;
		EventCallback &operator=(EventCallback &&other) noexcept;

		CallbackFunc toCall{};
		TESForm *source{}; // first arg to handler (reference or base form or form list)
		TESForm *object{}; // second arg to handler
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

		[[nodiscard]] bool IsRemoved() const { return removed; }
		void SetRemoved(bool bSet) { removed = bSet; }
		[[nodiscard]] bool FlushesOnLoad() const { return flushOnLoad; }
		[[nodiscard]] bool EqualFilters(const EventCallback &rhs) const; // return true if the two handlers have matching filters.

		[[nodiscard]] Script *TryGetScript() const;
		[[nodiscard]] bool HasCallbackFunc() const;

		// If the EventCallback is confirmed to stay, then call this to wrap up loose ends, e.g save lambda var context.
		void Confirm();

		// If "this" would run anytime toCheck would run or more, return true (toCheck should be removed; "this" has more or equally generic filters).
		// The above rule is used to remove redundant callbacks in one fell swoop.
		// Assumes both have the same callbacks.
		// Eval is passed to report errors.
		[[nodiscard]] bool ShouldRemoveCallback(const EventCallback& toCheck, const EventInfo& evInfo, ExpressionEvaluator* eval = {}) const;

		std::unique_ptr<ScriptToken> Invoke(EventInfo &eventInfo, void *arg0, void *arg1);
	};

	//Does not attempt to store lambda info for Script*.
	using BasicCallbackFunc = std::variant<Script*, EventHandler>;

	BasicCallbackFunc GetBasicCallback(const EventCallback::CallbackFunc& func);

	//Each callback function can have multiple EventCallbacks.
	using CallbackMap = std::multimap<BasicCallbackFunc, EventCallback>;

	struct EventInfo
	{
		EventInfo(const char *name_, EventFilterType *params_, UInt8 nParams_, UInt32 eventMask_, EventHookInstaller *installer_,
				  EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), paramTypes(params_), numParams(nParams_), eventMask(eventMask_), installHook(installer_), flags(flags)
		{}
		// ctor w/ alias
		EventInfo(const char* name_, const char* alias_, EventFilterType* params_, UInt8 nParams_, UInt32 eventMask_, EventHookInstaller* installer_,
			EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), alias(alias_), paramTypes(params_), numParams(nParams_), eventMask(eventMask_), installHook(installer_), flags(flags)
		{}

		EventInfo(const char *name_, EventFilterType *params_, UInt8 numParams_, EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), paramTypes(params_), numParams(numParams_), flags(flags) {}
		// ctor w/ alias
		EventInfo(const char* name_, const char* alias_, EventFilterType* params_, UInt8 numParams_, EventFlags flags = EventFlags::kFlags_None)
			: evName(name_), alias(alias_), paramTypes(params_), numParams(numParams_), flags(flags) {}

		EventInfo(const EventInfo &other) = delete;
		EventInfo& operator=(const EventInfo& other) = delete;

		EventInfo(EventInfo&& other) noexcept :
			evName(other.evName), alias(other.alias), paramTypes(other.paramTypes), numParams(other.numParams),
			eventMask(other.eventMask), callbacks(std::move(other.callbacks)), installHook(other.installHook), flags(other.flags)
		{}

		const char *evName; //should never be nullptr
		const char *alias{}; //could be nullptr (unused)
		EventFilterType *paramTypes;
		UInt8 numParams = 0;
		UInt32 eventMask = 0;
		CallbackMap callbacks;
		EventHookInstaller *installHook{}; // if a hook is needed for this event type, this will be non-null.
										   // install it once and then set *installHook to NULL. Allows multiple events
										   // to use the same hook, installing it only once.
		EventFlags flags = EventFlags::kFlags_None;

		[[nodiscard]] bool FlushesOnLoad() const
		{
			return flags & EventFlags::kFlag_FlushOnLoad;
		}
		[[nodiscard]] bool IsUserDefined() const
		{
			return flags & EventFlags::kFlag_IsUserDefined;
		}
		// n is 0-based
		// Assumes that the index was already checked as valid (i.e numParams was checked).
		[[nodiscard]] EventFilterType TryGetNthParamType(size_t n) const
		{
			return !IsUserDefined() ? paramTypes[n] : EventFilterType::eParamType_Anything;
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

	using ArgStack = StackVector<void*, kMaxUdfParams>;
	static constexpr auto numMaxFilters = kMaxUdfParams;

	bool SetHandler(const char *eventName, EventCallback &toSet, ExpressionEvaluator* eval = nullptr);

	// removes handler only if all filters match
	bool RemoveHandler(const char *eventName, const EventCallback& toRemove, ExpressionEvaluator* eval = nullptr);

	// handle an NVSEMessagingInterface message
	void HandleNVSEMessage(UInt32 msgID, void *data);

	// Deprecated in favor of EventManager::DispatchEvent
	void HandleEvent(EventInfo& eventInfo, void* arg0, void* arg1);

	// handle an eventID directly
	// Deprecated in favor of EventManager::DispatchEvent
	void __stdcall HandleEvent(eEventID id, void *arg0, void *arg1);

	void ClearFlushOnLoadEvents();

	// name of whatever event is currently being handled, empty string if none
	const char *GetCurrentEventName();

	// called each frame to update internal state
	void Tick();

	void Init();

	bool RegisterEventEx(const char *name, const char* alias, bool isInternal, UInt8 numParams, EventFilterType *paramTypes,
	                     UInt32 eventMask = 0, EventHookInstaller *hookInstaller = nullptr,
	                     EventFlags flags = EventFlags::kFlags_None);

	// Exported
	bool RegisterEvent(const char *name, UInt8 numParams, EventFilterType *paramTypes,
					   EventFlags flags = EventFlags::kFlags_None);

	// Exported
	bool RegisterEventWithAlias(const char* name, const char* alias, UInt8 numParams, EventFilterType* paramTypes,
		EventFlags flags = EventFlags::kFlags_None);

	bool SetNativeEventHandler(const char *eventName, EventHandler func);
	bool RemoveNativeEventHandler(const char *eventName, EventHandler func);

	using DispatchReturn = NVSEEventManagerInterface::DispatchReturn;
	using DispatchCallback = NVSEEventManagerInterface::DispatchCallback;
	using PostDispatchCallback = NVSEEventManagerInterface::PostDispatchCallback;

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(TESObjectREFR* thisObj, EventInfo& eventInfo, ArgStack& params,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback);

	template <bool ExtractIntTypeAsFloat>
	bool DispatchEventRaw(TESObjectREFR* thisObj, EventInfo& eventInfo, ArgStack& params, 
		bool deferIfOutsideMainThread, PostDispatchCallback postCallback);

	bool DispatchEvent(const char* eventName, TESObjectREFR* thisObj, ...);
	DispatchReturn DispatchEventAlt(const char *eventName, DispatchCallback resultCallback, void *anyData, TESObjectREFR *thisObj, ...);

	bool DispatchEventThreadSafe(const char* eventName, PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...);
	DispatchReturn DispatchEventAltThreadSafe(const char* eventName, DispatchCallback resultCallback, void* anyData, 
		PostDispatchCallback postCallback, TESObjectREFR* thisObj, ...);

	// dispatch a user-defined event from a script (for Cmd_DispatchEvent)
	// Cmd_DispatchEventAlt provides more flexibility with how args are passed.
	bool DispatchUserDefinedEvent(const char *eventName, Script *sender, UInt32 argsArrayId, const char *senderName);




	// template definitions

	bool DoesFormMatchFilter(TESForm* inputForm, TESForm* filter, bool expectReference, const UInt32 recursionLevel = 0);
	bool DoDeprecatedFiltersMatch(const EventCallback& callback, const ArgStack& params);

	// eParamType_Anything is treated as "use default param type" (usually for a User-Defined Event).
	template<bool ExtractIntTypeAsFloat, bool IsParamExistingFilter>
	bool DoesFilterMatch(const ArrayElement& sourceFilter, void* param, EventFilterType filterType)
	{
		switch (sourceFilter.DataType()) {
		case kDataType_Numeric:
		{
			double filterNumber{};
			sourceFilter.GetAsNumber(&filterNumber);
			// This filter could be inside an array, so we can't be sure if the number is floored.
			if (filterType == EventFilterType::eParamType_Int)
				filterNumber = floor(filterNumber);

			float inputNumber;
			if constexpr (ExtractIntTypeAsFloat)
			{
				// this function could be being called by a function, where even ints are passed as floats.
				// Alternatively, it could be called by some internal function that got the param from an ArrayElement 
				inputNumber = *reinterpret_cast<float*>(&param);
				if (filterType == EventFilterType::eParamType_Int)  // mostly for if IsParamExistingFilter
					inputNumber = floor(inputNumber);
			}
			else  
			{
				// this function is being called internally, via a va_arg-using function, so expect ints to be packed like ints.
				inputNumber = (filterType == EventFilterType::eParamType_Int)
					? static_cast<float>(*reinterpret_cast<UInt32*>(&param))
					: *reinterpret_cast<float*>(&param);
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
			auto* inputForm = static_cast<TESForm*>(param);
			auto* filterForm = LookupFormByID(filterFormId);
			bool const expectReference = filterType != EventFilterType::eParamType_BaseForm;

			if constexpr (IsParamExistingFilter)
			{
				if (inputForm && IS_ID(inputForm, BGSListForm))
				{
					// Multiple form filters
					auto const existingFilters = static_cast<BGSListForm*>(inputForm);
					for (auto const nthExistingFilterForm : existingFilters->list)
					{
						if (!DoesFormMatchFilter(nthExistingFilterForm, filterForm, expectReference))
							return false;
					}
					return true;
				}
			}
			if (!DoesFormMatchFilter(inputForm, filterForm, expectReference))
			{
				return false;
			}
			break;
		}
		case kDataType_String:
		{
			const char* filterStr{};
			sourceFilter.GetAsString(&filterStr);
			const auto inputStr = static_cast<const char*>(param);
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
			const auto inputArrayId = *reinterpret_cast<ArrayID*>(&param);
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

	template<bool ExtractIntTypeAsFloat, bool IsParamExistingFilter>
	bool DoesParamMatchFiltersInArray(const EventCallback& callback, const EventCallback::Filter& arrayFilter, EventFilterType paramType, void* param, int index)
	{
		ArrayID arrayFiltersId{};
		arrayFilter.GetAsArray(&arrayFiltersId);
		auto* arrayFilters = g_ArrayMap.Get(arrayFiltersId);
		if (!arrayFilters)
		{
			ShowRuntimeError(callback.TryGetScript(), "While checking event filters in array at index %d, the array was invalid/uninitialized (array id: %u).", index, arrayFiltersId);
			return false;
		}
		// If array of filters is (string)map, then ignore the keys.
		for (auto iter = arrayFilters->Begin();
			!iter.End(); ++iter)
		{
			auto const& elem = *iter.second();
			if (ParamTypeToVarType(paramType) != DataTypeToVarType(elem.DataType()))
				continue;
			if (DoesFilterMatch<ExtractIntTypeAsFloat, IsParamExistingFilter>(elem, param, paramType))
				return true;
		}
		return false;
	}

	template<bool ExtractIntTypeAsFloat>
	bool DoFiltersMatch(TESObjectREFR* thisObj, const EventInfo& eventInfo, const EventCallback& callback, const ArgStack& params)
	{
		for (auto& [index, filter] : callback.filters)
		{
			bool const isCallingRefFilter = index == 0;

			if (index > params->size())
				return false; // insufficient params to match that filter.

			void* param = isCallingRefFilter ? thisObj : params->at(index - 1);

			if (eventInfo.IsUserDefined()) // Skip filter type checking.
			{
				if (!DoesFilterMatch<ExtractIntTypeAsFloat, false>(filter, param, EventFilterType::eParamType_Anything))
					return false;
				//TODO: add support for array of filters
			}
			else
			{
				auto const paramType = isCallingRefFilter ? EventFilterType::eParamType_Reference : eventInfo.paramTypes[index - 1];

				const auto filterDataType = filter.DataType();
				const auto filterVarType = DataTypeToVarType(filterDataType);
				const auto paramVarType = ParamTypeToVarType(paramType);

				if (filterVarType != paramVarType) //if true, can assume that the filterVar's type is Array (if it isn't, type mismatch should have been reported in SetEventHandler).
				{
					// assume elements of array are filters
					if (!DoesParamMatchFiltersInArray<ExtractIntTypeAsFloat, false>(callback, filter, paramType, param, index))
						return false;
					continue;
				}
				if (!DoesFilterMatch<ExtractIntTypeAsFloat, false>(filter, param, paramType))
					return false;
			}
		}
		return true;
	}

	// For events that are best deferred until Tick(), usually to avoid multithreading issues.
	template <bool ExtractIntTypeAsFloat>
	struct DeferredCallback
	{
		EventInfo& eventInfo;
		TESObjectREFR* thisObj;
		ArgStack params;
		DispatchCallback resultCallback;
		void* callbackData;
		PostDispatchCallback postCallback;

		DeferredCallback(TESObjectREFR* thisObj, EventInfo& eventInfo, ArgStack &&params,
			DispatchCallback resultCallback, void* callbackData, PostDispatchCallback postCallback) :
			eventInfo(eventInfo), thisObj(thisObj), params(params),
			resultCallback(resultCallback), callbackData(callbackData), postCallback(postCallback)
		{}

		~DeferredCallback()
		{
			DispatchEventRaw<ExtractIntTypeAsFloat>(thisObj, eventInfo, params, resultCallback, callbackData,
				false, postCallback);
		}
	};
	extern std::deque<DeferredCallback<false>> s_deferredCallbacksDefault;
	extern std::deque<DeferredCallback<true>> s_deferredCallbacksWithIntsPackedAsFloats;

	extern ICriticalSection s_criticalSection;

	template <bool ExtractIntTypeAsFloat>
	DispatchReturn DispatchEventRaw(TESObjectREFR* thisObj, EventInfo& eventInfo, ArgStack& params,
		DispatchCallback resultCallback, void* anyData, bool deferIfOutsideMainThread, PostDispatchCallback postCallback)
	{
		if (deferIfOutsideMainThread && GetCurrentThreadId() != g_mainThreadID)
		{
			ScopedLock lock(s_criticalSection);
			if constexpr (ExtractIntTypeAsFloat)
			{
				s_deferredCallbacksWithIntsPackedAsFloats.emplace_back(thisObj, eventInfo, std::move(params), resultCallback, anyData, postCallback);
			}
			else
			{
				s_deferredCallbacksDefault.emplace_back(thisObj, eventInfo, std::move(params), resultCallback, anyData, postCallback);
			}
			return DispatchReturn::kRetn_Deferred;
		}

		DispatchReturn result = DispatchReturn::kRetn_Normal;

		for (auto& [funcKey, callback] : eventInfo.callbacks)
		{
			if (callback.IsRemoved())
				continue;

			if (!DoDeprecatedFiltersMatch(callback, params))
				continue;
			if (!DoFiltersMatch<ExtractIntTypeAsFloat>(thisObj, eventInfo, callback, params))
				continue;

			result = std::visit(overloaded{
				[=, &params](const LambdaManager::Maybe_Lambda& script) -> DispatchReturn
				{
					using FunctionCaller = std::conditional_t<ExtractIntTypeAsFloat, InternalFunctionCallerAlt, InternalFunctionCaller>;

					FunctionCaller caller(script.Get(), thisObj, nullptr, true); // don't report errors if passing more args to a UDF than it can absorb.
					caller.SetArgsRaw(params->size(), params->data());
					auto const res = UserFunctionManager::Call(std::move(caller));
					if (resultCallback)
					{
						NVSEArrayVarInterface::Element elem;
						if (PluginAPI::BasicTokenToPluginElem(res.get(), elem, script.Get()))
						{
							return resultCallback(elem, anyData) ? DispatchReturn::kRetn_Normal : DispatchReturn::kRetn_EarlyBreak;
						}
						return DispatchReturn::kRetn_GenericError;
					}
					return DispatchReturn::kRetn_Normal;
				},
				[&params, thisObj](EventHandler const handler) -> DispatchReturn
				{
					handler(thisObj, params->data());
					return DispatchReturn::kRetn_Normal;
				},
				}, callback.toCall);

			if (result != DispatchReturn::kRetn_Normal)
				break;
		}

		if (postCallback)
			postCallback(anyData, result);

		return result;
	}

	template <bool ExtractIntTypeAsFloat>
	bool DispatchEventRaw(TESObjectREFR* thisObj, EventInfo& eventInfo, ArgStack& params, 
		bool deferIfOutsideMainThread, PostDispatchCallback postCallback)
	{
		return DispatchEventRaw<ExtractIntTypeAsFloat>(thisObj, eventInfo, params, nullptr, nullptr, deferIfOutsideMainThread, postCallback)
			> DispatchReturn::kRetn_GenericError;
	}
};

#endif