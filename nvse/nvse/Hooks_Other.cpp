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

#if RUNTIME
#include "InventoryReference.h"
namespace OtherHooks
{
	const static auto ClearCachedTileMap = &MemoizedMap<const char*, Tile::Value*>::Clear;
	thread_local FastStack<CurrentScriptContext> g_currentScriptContext;
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
		// Prevent leakage of variables that's ScriptEventList gets deleted (e.g. in Effect scripts, UDF)
		auto* scriptVars = g_nvseVarGarbageCollectionMap.GetPtr(eventList);
		if (!scriptVars)
			return;
		ScopedLock lock(g_gcCriticalSection);
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
					//g_ArrayMap.MarkTemporary(static_cast<int>(node->var->data), true);
					g_ArrayMap.RemoveReference(&var->data, eventList->m_script->GetModIndex());
					break;
				default:
					break;
				}
			}
		}
		g_nvseVarGarbageCollectionMap.Erase(eventList);
	}

	void DeleteEventList(ScriptEventList* eventList)
	{
		
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

	// Saves last thisObj in effect/object scripts before they get assigned to something else with dot syntax
	void __fastcall SaveLastScriptOwnerRef(UInt8* ebp, int spot)
	{
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
	}

	void __fastcall PostScriptExecute(Script* script)
	{
		if (script)
			g_currentScriptContext.Pop();
	}

	__declspec (naked) void PostScriptExecuteHook1()
	{
		__asm
		{
			push eax
			mov ecx, [ebp+0x8]
			call PostScriptExecute
			pop eax
			mov esp, ebp
			pop ebp
			ret 0x20
		}
	}

	__declspec (naked) void PostScriptExecuteHook2()
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

	__declspec(naked) void SaveScriptOwnerRefHook()
	{
		const static auto hookedCall = 0x702FC0;
		const static auto retnAddr = 0x5E0D56;
		__asm
		{
			lea ecx, [ebp]
			mov edx, 1
			call SaveLastScriptOwnerRef
			call hookedCall
			jmp retnAddr
		}
	}

	__declspec(naked) void SaveScriptOwnerRefHook2()
	{
		const static auto hookedCall = 0x4013E0;
		const static auto retnAddr = 0x5E119F;
		__asm
		{
			push ecx
			mov edx, 2
			lea ecx, [ebp]
			call SaveLastScriptOwnerRef
			pop ecx
			call hookedCall
			jmp retnAddr
		}
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
		void* __fastcall OnUnlock_5CC222(void* a1, void* unused, char a2) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			auto ref = *reinterpret_cast<TESObjectREFR**>(ebp + 0x10);

			EventManager::DispatchEvent("onunlock", nullptr, ref, nullptr, 0);

			return ThisStdCall<void*>(0x430A90, a1, a2);
		}

		// Lockpick menu
		void* __fastcall OnUnlock_78F8E5(TESObjectREFR* doorRef) {
			EventManager::DispatchEvent("onunlock", nullptr, doorRef, *g_thePlayer, 1);

			return ThisStdCall<void*>(0x5692A0, doorRef);
		}

		// Unlock via interact
		void* __fastcall OnUnlock_518B00(void* a1) {
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress(), false);
			auto unlocker = *reinterpret_cast<TESObjectREFR**>(ebp - 0x18);
			auto doorRef = *reinterpret_cast<TESObjectREFR**>(ebp + 0x8);

			if (unlocker == *reinterpret_cast<TESObjectREFR**>(g_thePlayer)) {
				EventManager::DispatchEvent("onunlock", nullptr, doorRef, *g_thePlayer, 1);
			}
			else {
				ExtraLock::Data* lockData = *reinterpret_cast<ExtraLock::Data**>(ebp - 0x20);
				char* v58 = *reinterpret_cast<char**>(ebp - 0x1C);
				bool hasKey = ThisStdCall<char>(0x891DB0, unlocker, lockData->key, 0, 1, 0, v58);
				if (!hasKey) {
					EventManager::DispatchEvent("onunlock", nullptr, doorRef, unlocker, 1);
				}
				else {
					EventManager::DispatchEvent("onunlock", nullptr, doorRef, unlocker, 2);
				}
			}

			return ThisStdCall<void*>(0x517360, a1);
		}

		void WriteHooks() {
			WriteRelCall(0x790461, reinterpret_cast<UInt32>(OnLockBroken));
			WriteRelCall(0x78F8E5, reinterpret_cast<UInt32>(OnLockPickSuccess)); 
			WriteRelCall(0x78FE24, reinterpret_cast<UInt32>(OnLockPickBroken));
			WriteRelCall(0x5CC222, reinterpret_cast<UInt32>(OnUnlock_5CC222));
			WriteRelCall(0x78F8E5, reinterpret_cast<UInt32>(OnUnlock_78F8E5));
			WriteRelCall(0x518B00, reinterpret_cast<UInt32>(OnUnlock_518B00));
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

	void Hooks_Other_Init()
	{
		WriteRelJump(0x9FF5FB, UInt32(TilesDestroyedHook));
		WriteRelJump(0x709910, UInt32(TilesCreatedHook));
		WriteRelJump(0x41AF70, UInt32(ScriptEventListsDestroyedHook));
		
		WriteRelJump(0x5E0D51, UInt32(SaveScriptOwnerRefHook));
		WriteRelJump(0x5E119A, UInt32(SaveScriptOwnerRefHook2));

		WriteRelJump(0x5E1137, UInt32(PostScriptExecuteHook1));
		WriteRelJump(0x5E1392, UInt32(PostScriptExecuteHook2));

		WriteRelCall(0x5AA206, ScriptDestructorHook);

		*(UInt32*)0x0126FDF4 = 1; // locale fix

		IMod::WriteHooks();
		Locks::WriteHooks();
		Terminal::WriteHooks();
		Repair::WriteHooks();
		DisEnable::WriteHooks();
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