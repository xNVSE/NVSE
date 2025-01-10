#include "Hooks_Other.h"
#include "GameAPI.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "Hooks_Gameplay.h"
#include "LambdaManager.h"
#include "PluginManager.h"
#include "Commands_UI.h"
#include "EventManager.h"
#include "FastStack.h"
#include "GameTiles.h"
#include "GameUI.h"
#include "MemoizedMap.h"
#include "StackVariables.h"

#if RUNTIME
#include "InventoryReference.h"
namespace OtherHooks
{
	const static auto ClearCachedTileMap = &MemoizedMap<const char*, Tile::Value*>::Clear;
	thread_local FastStack<CurrentScriptContext> g_currentScriptContext;
	thread_local std::vector<std::function<void()>> g_onExitScripts;

	__declspec(naked) void TilesDestroyedHook()
	{
		__asm
		{
			call ClearCachedTileMap // static function
			// original asm
			pop ecx
			mov esp, ebp
			pop ebp
			ret
		}
	}

	__declspec(naked) void TilesCreatedHook()
	{
		// Eddoursol reported a problem where stagnant deleted tiles got cached
		__asm
		{
			call ClearCachedTileMap // static function
			pop ecx
			mov esp, ebp
			pop ebp
			ret
		}
	}

	void CleanUpNVSEVars(ScriptEventList* eventList)
	{
		ScopedLock lock(g_gcCriticalSection);
		// Prevent leakage of variables that's ScriptEventList gets deleted (e.g. in Effect scripts, UDF)
		auto* scriptVars = g_nvseVarGarbageCollectionMap.GetPtr(eventList);
		if (!scriptVars)
			return;
		for (auto* var : *eventList->m_vars)
		{
			if (var)
			{
				const auto type = scriptVars->Get(var);
				switch (type)
				{
				case NVSEVarType::kVarType_String:
					g_StringMap.MarkTemporary(static_cast<int>(var->data), true);
					break;
				case NVSEVarType::kVarType_Array:
					g_ArrayMap.RemoveReference(&var->data, eventList->m_script->GetModIndex());
					break;
				default:
					break;
				}
			}
		}
		g_nvseVarGarbageCollectionMap.Erase(eventList);
	}

	void CleanUpNVSEVar(ScriptEventList* eventList, ScriptLocal* local) 
	{
		ScopedLock lock(g_gcCriticalSection);
		auto* scriptVars = g_nvseVarGarbageCollectionMap.GetPtr(eventList);
		if (!scriptVars)
			return;
		const auto type = scriptVars->Get(local);
		
		if (type != NVSEVarType::kVarType_Null) 
		{
			switch (type)
			{
			case NVSEVarType::kVarType_String:
				g_StringMap.MarkTemporary(static_cast<int>(local->data), true);
				break;
			case NVSEVarType::kVarType_Array:
				g_ArrayMap.RemoveReference(&local->data, eventList->m_script->GetModIndex());
				break;
			default:
				break;
			}
			scriptVars->Erase(local);
		}
	}

	void DeleteEventList(ScriptEventList* eventList)
	{
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_EventListDestroyed, eventList, sizeof (ScriptEventList*), nullptr);
		LambdaManager::MarkParentAsDeleted(eventList); // deletes if exists
		CleanUpNVSEVars(eventList);
		ThisStdCall(0x5A8BC0, eventList);
		FormHeap_Free(eventList);
	}
	
	ScriptEventList* __fastcall ScriptEventListsDestroyedHook(ScriptEventList *eventList, int EDX, bool doFree)
	{
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_EventListDestroyed, eventList, sizeof ScriptEventList, nullptr);

		DeleteEventList(eventList);
		return eventList;
	}

	void __fastcall ScriptEventListFreeVarsHook(ScriptEventList *eventList) {
		//Clear all thread caches for this event list
		for (int i = 0; i < g_nextThreadID; i++) {
			auto vc = g_scriptVarCache[i];
			if (i != g_threadID) {
				vc->mt.lock();
			}
			vc->clear(eventList);
			if (i != g_threadID) {
				vc->mt.unlock();
			}
		}

		// Original FreeVars
		ThisStdCall(0x5A8C90, eventList);
	}

	namespace PreScriptExecute
	{
		void __fastcall PreScriptExecute(UInt8* ebp, int spot)
		{
			// Saves last thisObj in effect/object scripts before they get assigned to something else with dot syntax
			auto& [script, scriptRunner, lineNumberPtr, scriptOwnerRef, command, curData] = *g_currentScriptContext.Push();
			command = nullptr; // set in ExtractArgsEx
			scriptOwnerRef = *reinterpret_cast<TESObjectREFR**>(ebp + 0xC);
			script = *reinterpret_cast<Script**>(ebp + 0x8);
			if (spot == 1)
			{
				scriptRunner = *reinterpret_cast<ScriptRunner**>(ebp - 0x774);
				lineNumberPtr = reinterpret_cast<UInt32*>(ebp - 0x40);
				curData = reinterpret_cast<UInt32*>(ebp - 0x3C);
			}
			else if (spot == 2)
			{
				// ScriptRunner::Run2
				scriptRunner = *reinterpret_cast<ScriptRunner**>(ebp - 0x744);
				lineNumberPtr = reinterpret_cast<UInt32*>(ebp - 0x28);
				curData = reinterpret_cast<UInt32*>(ebp - 0x24);
			}

			//Do other stuff
			// if (g_threadID == -1) {
			// 	g_threadID = g_nextThreadID++;
			// 	g_scriptVarCache[g_threadID] = new VarCache{ {}, {} };
			// }
			//
			// if (g_currentScriptContext.Size() == 1) {
			// 	g_scriptVarCache[g_threadID]->mt.lock();
			// }
		}

		__declspec(naked) void Hook1()
		{
			const static auto hookedCall = 0x702FC0;
			const static auto retnAddr = 0x5E0D56;
			__asm
			{
				lea ecx, [ebp]
				mov edx, 1
				call PreScriptExecute
				call hookedCall
				jmp retnAddr
			}
		}

		__declspec(naked) void Hook2()
		{
			const static auto hookedCall = 0x4013E0;
			const static auto retnAddr = 0x5E119F;
			__asm
			{
				push ecx
				mov edx, 2
				lea ecx, [ebp]
				call PreScriptExecute
				pop ecx
				call hookedCall
				jmp retnAddr
			}
		}

		void WriteHooks()
		{
			WriteRelJump(0x5E0D51, UInt32(Hook1));
			WriteRelJump(0x5E119A, UInt32(Hook2));
		}
	}

	namespace PostScriptExecute {
		void __fastcall PostScriptExecute(Script* script)
		{
			if (script)
				g_currentScriptContext.Pop();
			for (auto& func : g_onExitScripts)
				func();
			g_onExitScripts.clear();
		}

		__declspec (naked) void Hook1()
		{
			__asm
			{
				push eax
				mov ecx, [ebp + 0x8]
				call PostScriptExecute
				pop eax
				mov esp, ebp
				pop ebp
				ret 0x20
			}
		}

		__declspec (naked) void Hook2()
		{
			__asm
			{
				push eax
				mov ecx, [ebp + 0x8]
				call PostScriptExecute
				pop eax
				mov esp, ebp
				pop ebp
				ret 0xC
			}
		}

		void WriteHooks()
		{
			WriteRelJump(0x5E1137, UInt32(Hook1));
			WriteRelJump(0x5E1392, UInt32(Hook2));
		}
	}

	// Called via ResetQuest when it is reallocating the new script event list
	ScriptEventList* __fastcall ResetQuestReallocateEventListHook(const Script* script) {
		const auto res = ThisStdCall<ScriptEventList*>(0x5ABF60, script);

		// Not sure that this is even possible, better safe than sorry
		if (!g_currentScriptContext.Empty()) {
			const auto& currentContext = g_currentScriptContext.Top();

			// If ResetQuest is called from within the quest's script, we need to update the
			// ScriptRunner's event list otherwise it will keep using the freed one
			if (script == currentContext.script) {
				currentContext.scriptRunner->eventList = res;
			}
		}

		return res;
	}

	void __fastcall ScriptDestructorHook(Script* script)
	{
		// hooked call
		ThisStdCall(0x483630, script);
		LambdaManager::MarkScriptAsDeleted(script);
	}

	namespace IMod {
		void __fastcall ApplyIMOD(void** a1, void* unused, void* a2) {
			auto *ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			auto ref = *reinterpret_cast<TESForm**>(ebp + 0x8);

			EventManager::DispatchEvent("onapplyimod", nullptr, ref);

			ThisStdCall(0x633C90, a1, a2);
		}

		void __fastcall RemoveIMOD_1(void** a1, void* unused, void* a2) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			auto ref = *reinterpret_cast<TESForm**>(ebp + 0x8);
		
			EventManager::DispatchEvent("onremoveimod", nullptr, ref);
		
 			ThisStdCall(0x633C90, a1, a2);
		}

		// IMOD Instance removal
		void __fastcall RemoveIMOD_2(void** a1, void* unused, void* a2) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			uint8_t* refObj = *reinterpret_cast<uint8_t**>(ebp - 0x1C);
			TESObjectREFR* ref = *reinterpret_cast<TESObjectREFR**>(refObj + 0x1C);

			EventManager::DispatchEvent("onremoveimod", nullptr, ref);

			ThisStdCall(0x631620, a1, a2);
		}

		void WriteHooks() {
			WriteRelCall(0x529A37, reinterpret_cast<UInt32>(ApplyIMOD));
			WriteRelCall(0x529D37, reinterpret_cast<UInt32>(RemoveIMOD_1));
			WriteRelCall(0x86FE75, reinterpret_cast<UInt32>(RemoveIMOD_2));
		}
	}

	namespace Locks {
		void __fastcall OnLockBroken(TESObjectREFR* doorRef) {
			EventManager::DispatchEvent("onlockbroken", nullptr, doorRef);

			ThisStdCall(0x569160, doorRef);
		}

		void __fastcall OnLockPickSuccess(TESObjectREFR* doorRef) {
			EventManager::DispatchEvent("onlockpicksuccess", nullptr, doorRef);

			ThisStdCall(0x5692A0, doorRef);
		}

		void __fastcall OnLockPickBroken() {
			uint8_t* menu = *reinterpret_cast<uint8_t**>(g_lockpickMenu);
			TESObjectREFR* doorRef = *reinterpret_cast<TESObjectREFR**>(menu + 0x6C);

			EventManager::DispatchEvent("onlockpickbroken", nullptr, doorRef);

			// Dont need to call original hooked method since nullsub
		}

		// unlocked door/container, unlocker, unlock type (0 = script, 1 = lockpick, 2 = key)
		// Cmd_Unlock_Execute
		void* __fastcall OnUnlock_5CC186(const void* extraLockData) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			const auto unlockedRef = *reinterpret_cast<TESObjectREFR**>(ebp + 0x10);

			EventManager::DispatchEvent("onunlock", nullptr, unlockedRef, nullptr, 0);

			return ThisStdCall<void*>(0x430A90, extraLockData, 0);
		}

		// Lockpick menu
		void* __fastcall OnUnlock_78F8E5(TESObjectREFR* doorRef) {
			EventManager::DispatchEvent("onunlock", nullptr, doorRef, *g_thePlayer, 1);

			return ThisStdCall<void*>(0x5692A0, doorRef);
		}

		// Unlock via interact
		void* __fastcall OnUnlock_518B00(const ExtraLock* lock) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			const auto unlocker = *reinterpret_cast<Actor**>(ebp - 0x18);
			const auto doorRef = *reinterpret_cast<TESObjectREFR**>(ebp + 0x8);

			const auto lockData = lock->data;
			if (lockData->flags & 0x1) {
				if (unlocker) {
					// Player
					if (unlocker->refID == 0x14) {
						EventManager::DispatchEvent("onunlock", nullptr, doorRef, *g_thePlayer, 1);
					}
					else {
						// Actor::HasObjects
						uint32_t unused;
						const bool hasKey = ThisStdCall<bool>(0x891DB0, unlocker, lockData->key, 0, 1, 0, &unused);
						EventManager::DispatchEvent("onunlock", nullptr, doorRef, unlocker, hasKey ? 2 : 1);
					}
				}
				else {
					EventManager::DispatchEvent("onunlock", nullptr, doorRef, nullptr, 0);
				}
			}

			// ExtraLock::Unlock
			return ThisStdCall<void*>(0x517360, lock);
		}

		void WriteHooks() {
			WriteRelCall(0x790461, reinterpret_cast<UInt32>(OnLockBroken));
			WriteRelCall(0x78F8E5, reinterpret_cast<UInt32>(OnLockPickSuccess)); 
			WriteRelCall(0x78FE24, reinterpret_cast<UInt32>(OnLockPickBroken));

			// WriteRelCall(0x5CC186, reinterpret_cast<UInt32>(OnUnlock_5CC186));
			// WriteRelCall(0x78F8E5, reinterpret_cast<UInt32>(OnUnlock_78F8E5));
			// WriteRelCall(0x518B00, reinterpret_cast<UInt32>(OnUnlock_518B00));
		}
	}

	namespace Terminal {
		void __fastcall OnTerminalHacked(UInt8* menu, void* unused, int a2, int a3) {
			TESObjectREFR* unlockedRef = *reinterpret_cast<TESObjectREFR**>(menu + 0x198);

			EventManager::DispatchEvent("onterminalhacked", nullptr, unlockedRef);

			ThisStdCall(0x767EF0, menu, a2, a3);
		}

		void __fastcall OnTerminalHackFailed(UInt8 * menu, void* unused, int a2, int a3) {
			TESObjectREFR* lockedRef = *reinterpret_cast<TESObjectREFR**>(menu + 0x198);

			EventManager::DispatchEvent("onterminalhackfailed", nullptr, lockedRef);

			ThisStdCall(0x767EF0, menu, a2, a3);
		}

		void WriteHooks() {
			WriteRelCall(0x76717A, reinterpret_cast<UInt32>(OnTerminalHacked));
			WriteRelCall(0x7674B0, reinterpret_cast<UInt32>(OnTerminalHackFailed));
		}
	}

	namespace Repair {
		// Repaired inv ref, repairer, type (0 = standard, 1 = merchant)
		char __fastcall OnItemRepair_1(void* a1, void* unused, char a2) {
			ExtraContainerChanges::EntryData *data = *reinterpret_cast<ExtraContainerChanges::EntryData**>(0x11DA760);
			auto invRef = CreateInventoryRefEntry(*g_thePlayer, data->type, data->countDelta, data->extendData->GetFirstItem());
			EventManager::DispatchEvent("onrepair", nullptr, invRef, *g_thePlayer, 0);

			return ThisStdCall<char>(0xAD8830, a1, a2);
		}

		TESForm* __fastcall OnItemRepair_2(ExtraContainerChanges::EntryData* data) {
			auto invRef = CreateInventoryRefEntry(*g_thePlayer, data->type, data->countDelta, data->extendData->GetFirstItem());
			auto repairer = *reinterpret_cast<TESObjectREFR**>(0x11DA7F4);

			EventManager::DispatchEvent("onrepair", nullptr, invRef, repairer, 1);

			return ThisStdCall<TESForm*>(0x44DDC0, data);
		}

		void WriteHooks() {
			WriteRelCall(0x7B5CAC, reinterpret_cast<UInt32>(OnItemRepair_1));
			WriteRelCall(0x7B800E, reinterpret_cast<UInt32>(OnItemRepair_2));
		}
	}

	namespace DisEnable {
		void __cdecl OnDisable(TESObjectREFR* ref, char a2) {
			EventManager::DispatchEvent("ondisable", nullptr, ref);

			CdeclCall(0x5AA500, ref, a2);
		}

		void __cdecl OnEnable(TESObjectREFR* ref) {
			EventManager::DispatchEvent("onenable", nullptr, ref);

			CdeclCall(0x5AA580, ref);
		}

		void WriteHooks() {
			WriteRelCall(0x5C47B6, reinterpret_cast<UInt32>(OnDisable));
			WriteRelCall(0x5C453C, reinterpret_cast<UInt32>(OnEnable));
		}
	}

	namespace Misc {
		bool __fastcall OnCellLoadFromBuffer(const TESForm* thisPtr) {
			const auto ptr = *reinterpret_cast<TESObjectREFR**>(GetParentBasePtr(_AddressOfReturnAddress()) - 0x8);
			if (thisPtr && ptr && ptr->GetNiNode()) {
				EventManager::DispatchEvent("onloadalt", ptr, ptr);
			}

			return ThisStdCall<bool>(0x549580, thisPtr);
		}

		void* __fastcall OnCellLoad(TESObjectREFR* thisPtr) {
			EventManager::DispatchEvent("onloadalt", thisPtr, thisPtr);

			return ThisStdCall<void*>(0x5D43C0, thisPtr);
		}

		void WriteHooks() {
			//WriteRelCall(0x54BC65, &OnCellLoadFromBuffer);
			//WriteRelCall(0x54BE9A, &OnCellLoad);
		}
	}

	void Hooks_Other_Init()
	{
		WriteRelJump(0x9FF5FB, UInt32(TilesDestroyedHook));
		WriteRelJump(0x709910, UInt32(TilesCreatedHook));
		WriteRelJump(0x41AF70, UInt32(ScriptEventListsDestroyedHook));

		// WriteRelCall(0x5A8BD4, reinterpret_cast<UInt32>(ScriptEventListFreeVarsHook));
		// WriteRelCall(0x5A9D0C, reinterpret_cast<UInt32>(ScriptEventListFreeVarsHook));
		// WriteRelCall(0x5AA09C, reinterpret_cast<UInt32>(ScriptEventListFreeVarsHook));

		PreScriptExecute::WriteHooks();
		PostScriptExecute::WriteHooks();

		WriteRelCall(0x5AA206, ScriptDestructorHook);

		WriteRelCall(0x60D876, ResetQuestReallocateEventListHook);

		*(UInt32*)0x0126FDF4 = 1; // locale fix

		IMod::WriteHooks();
		Locks::WriteHooks();
		Terminal::WriteHooks();
		Repair::WriteHooks();
		DisEnable::WriteHooks();
		Misc::WriteHooks();
	}

	thread_local CurrentScriptContext emptyCtx{}; // not every command gets run through script runner

	CurrentScriptContext* GetExecutingScriptContext()
	{
		if (!g_currentScriptContext.Empty())
			return &g_currentScriptContext.Top();
		emptyCtx = {};
		return &emptyCtx;
	}

	void PushScriptContext(const CurrentScriptContext& ctx)
	{
		g_currentScriptContext.Push(ctx);
	}

	void PopScriptContext()
	{
		g_currentScriptContext.Pop();
	}
}
#endif