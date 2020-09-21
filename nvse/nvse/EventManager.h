#pragma once
#include <string>

class Script;
class TESForm;
class TESObjectREFR;
class BGSListForm;
class Actor;

// For dispatching events to scripts.
// Scripts can register an event handler for any of the supported events.
// Can optionally specify filters to match against the event arguments.
// Event handler is a function script which must take the expected number and types of arguments associated with the event.
// Supporting hooks only installed if at least one handler is registered for a particular event.
// ###TODO: at present only supports 0-2 arguments per event. All we need for now, but would be nice to support arbitrary # of args.

namespace EventManager
{
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
	struct EventCallback
	{
		EventCallback() : script(NULL), source(NULL), object(NULL), removed(false), pendingRemove(false) {}
		EventCallback(Script* funcScript, TESForm* sourceFilter = NULL, TESForm* objectFilter = NULL)
			: script(funcScript), source(sourceFilter), object(objectFilter), removed(false), pendingRemove(false) {}
		EventCallback& operator=(const EventCallback& other)
		{
			script = other.script;
			source = other.source;
			object = other.object;
			removed = other.removed;
			pendingRemove = other.pendingRemove;
			return *this;
		};

		Script			*script;
		TESForm			*source;				// first arg to handler (reference or base form or form list)
		TESForm			*object;				// second arg to handler
		bool			removed;
		bool			pendingRemove;

		bool IsRemoved() const { return removed; }
		void SetRemoved(bool bSet) { removed = bSet; }
		bool Equals(const EventCallback& rhs) const;	// compare, return true if the two handlers are identical
	};

	bool SetHandler(const char* eventName, EventCallback& handler);

	// removes handler only if all filters match
	bool RemoveHandler(const char* id, EventCallback& handler);

	// handle an NVSEMessagingInterface message
	void HandleNVSEMessage(UInt32 msgID, void* data);

	// handle an eventID directly
	void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1);

	// name of whatever event is currently being handled, empty string if none
	const char* GetCurrentEventName();

	// called each frame to update internal state
	void Tick();

	void Init();

	// dispatch a user-defined event from a script
	bool DispatchUserDefinedEvent (const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName);
};
