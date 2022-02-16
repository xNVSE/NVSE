#pragma once
#include <string>

#include "ArrayVar.h"
#include "LambdaManager.h"
#include "PluginAPI.h"
#include <variant>

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
	static constexpr auto numMaxFilters = 0x20;

	using EventHandler = NVSEEventManagerInterface::EventHandler;

	enum eEventID {
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

		kEventID_InternalMAX,

		// user-defined
		kEventID_UserDefinedMIN = kEventID_InternalMAX,

		kEventID_INVALID = 0xFFFFFFFF
	};

	// Represents an event handler registered for an event.
	class EventCallback
	{
		void TrySaveLambdaContext();

	public:

		//If variant is Maybe_Lambda, must try to capture lambda context once the EventCallback is confirmed to stay. 
		using CallbackFunc = std::variant<LambdaManager::Maybe_Lambda, EventHandler>;

		EventCallback() = default;
		~EventCallback() = default;
		EventCallback(Script* funcScript, TESForm* sourceFilter = nullptr, TESForm* objectFilter = nullptr)
			: toCall(funcScript), source(sourceFilter), object(objectFilter) {}

		EventCallback(EventHandler func, TESForm* sourceFilter = nullptr, TESForm* objectFilter = nullptr)
			: toCall(func), source(sourceFilter), object(objectFilter) {}

		EventCallback(const EventCallback& other) = delete;
		EventCallback& operator=(const EventCallback& other) = delete;

		EventCallback(EventCallback&& other) noexcept;
		EventCallback& operator=(EventCallback&& other) noexcept;

		CallbackFunc	toCall{};
		TESForm			*source{};				// first arg to handler (reference or base form or form list)
		TESForm			*object{};				// second arg to handler
		bool			removed{};
		bool			pendingRemove{};

		using Index = UInt32;
		using Filter = ArrayElement;

		//Indexes for filters must respect the max amount of BaseFilters for the base event definition.
		//If no filter is at an index = it is unfiltered for the nth BaseFilter.
		//Using a map to avoid adding duplicate indexes.
		std::map<Index, Filter> filters;

		[[nodiscard]] bool IsRemoved() const { return removed; }
		void SetRemoved(bool bSet) { removed = bSet; }
		[[nodiscard]] bool Equals(const EventCallback& rhs) const;	// compare, return true if the two handlers are identical

		[[nodiscard]] Script* TryGetScript() const;
		[[nodiscard]] bool HasCallbackFunc() const;

		//If the EventCallback is confirmed to stay, then call this to wrap up loose ends, e.g save lambda var context.
		void Confirm();
	};

	bool SetHandler(const char* eventName, EventCallback& handler);

	// removes handler only if all filters match
	bool RemoveHandler(const char* id, const EventCallback& handler);

	// handle an NVSEMessagingInterface message
	void HandleNVSEMessage(UInt32 msgID, void* data);

	// handle an eventID directly
	void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1);

	// name of whatever event is currently being handled, empty string if none
	const char* GetCurrentEventName();

	// called each frame to update internal state
	void Tick();

	void Init();

	bool RegisterEventEx(const char* name, UInt8 numParams, UInt8* paramTypes, UInt32 eventMask, EventHookInstaller* hookInstaller);

	bool RegisterEvent(const char* name, UInt8 numParams, UInt8* paramTypes);
	bool SetNativeEventHandler(const char* eventName, EventHandler func);
	bool RemoveNativeEventHandler(const char* eventName, EventHandler func);

	bool DispatchEvent(const char* eventName, TESObjectREFR* thisObj, ...);

	// dispatch a user-defined event from a script
	bool DispatchUserDefinedEvent (const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName);
};
