#include "Hooks_Other.h"
#include "GameAPI.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "Hooks_Gameplay.h"
#include "LambdaManager.h"
#include "PluginManager.h"
#include "Commands_UI.h"
#include "FastStack.h"
#include "GameTiles.h"
#include "MemoizedMap.h"
#include "StackVariables.h"

#if RUNTIME
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

		// Clear all thread caches for this event list
		for (int i = 0; i < g_nextThreadID; i++) {
			auto vc = g_scriptVarCache[i];
			if (i != g_threadID) {
				vc->mt.lock();
			}
			vc->varCache[eventList] = {};
			if (i != g_threadID) {
				vc->mt.unlock();
			}
		}

		DeleteEventList(eventList);
		return eventList;
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

			// Do other stuff
			StackVariables::PushLocalStack();

			if (g_threadID == -1) {
				g_threadID = g_nextThreadID++;
				g_scriptVarCache[g_threadID] = new VarCache{ {}, {} };
			}

			if (g_currentScriptContext.Size() == 1) {
				g_scriptVarCache[g_threadID]->mt.lock();
			}
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

	namespace PostScriptExecute
	{
		void __fastcall PostScriptExecute(Script* script)
		{
			if (script) {
				g_currentScriptContext.Pop();
				StackVariables::PopLocalStack();

				if (g_currentScriptContext.Size() == 0) {
					g_scriptVarCache[g_threadID]->varCache.clear();
					g_scriptVarCache[g_threadID]->mt.unlock();
				}
			}
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

	void __fastcall ScriptDestructorHook(Script* script)
	{
		// hooked call
		ThisStdCall(0x483630, script);
		LambdaManager::MarkScriptAsDeleted(script);
	}

	void Hooks_Other_Init()
	{
		WriteRelJump(0x9FF5FB, UInt32(TilesDestroyedHook));
		WriteRelJump(0x709910, UInt32(TilesCreatedHook));
		WriteRelJump(0x41AF70, UInt32(ScriptEventListsDestroyedHook));

		PreScriptExecute::WriteHooks();
		PostScriptExecute::WriteHooks();

		WriteRelCall(0x5AA206, ScriptDestructorHook);

		*(UInt32*)0x0126FDF4 = 1; // locale fix
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