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
		kEventID_0x00080000,
		kEventID_OnOpen,
		kEventID_OnClose,
		kEventID_0x00400000,
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
		kEventID_DeferredInit,

		kEventID_InternalMAX,

		// user-defined
		kEventID_UserDefinedMIN = kEventID_InternalMAX,

		kEventID_INVALID = 0xFFFFFFFF
	};

	// Represents an event handler registered for an event.
	struct EventCallback
	{
		EventCallback(Script* funcScript, TESObjectREFR* sourceFilter = NULL, TESForm* objectFilter = NULL, TESObjectREFR* thisObj = NULL)
			: script(funcScript), source((TESForm*)sourceFilter), object(objectFilter), callingObj(thisObj), flags(0) { }
		EventCallback(Script* funcScript, BGSListForm* sourceFilter, TESForm* objectFilter = NULL, TESObjectREFR* thisObj = NULL)
			: script(funcScript), source((TESForm*)sourceFilter), object(objectFilter), callingObj(thisObj), flags(0) { }
		EventCallback(Script* funcScript, TESForm* sourceFilter, TESForm* objectFilter = NULL, TESObjectREFR* thisObj = NULL)
			: script(funcScript), source(sourceFilter), object(objectFilter), callingObj(thisObj), flags(0) { }
		~EventCallback() { }
		EventCallback& operator=(const EventCallback& other) {
			script = other.script;
			source = other.source;
			object = other.object;
			callingObj = other.callingObj;
			flags = other.flags;
			return *this;
		};

		
		enum {
			kFlag_Removed		= 1 << 0,		// set when RemoveEventHandler called while handler is in use
			kFlag_InUse			= 1 << 1,		// is in use by HandleEvent (so removing it would invalidate an iterator)
		};

		Script			* script;
		TESForm			* source;				// first arg to handler (reference or base form or form list)
		TESForm			* object;				// second arg to handler
		TESObjectREFR	* callingObj;			// invoking object for handler
		UInt8			flags;

		bool IsInUse() const { return flags & kFlag_InUse ? true : false; }
		bool IsRemoved() const { return flags & kFlag_Removed ? true : false; }
		void SetInUse(bool bSet) { flags = bSet ? flags | kFlag_InUse : flags & ~kFlag_InUse; }
		void SetRemoved(bool bSet) { flags = bSet ? flags | kFlag_Removed : flags & ~kFlag_Removed; }
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
	std::string GetCurrentEventName();

	// called each frame to update internal state
	void Tick();

	void Init();

	// dispatch a user-defined event from a script
	bool DispatchUserDefinedEvent (const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName);
};
