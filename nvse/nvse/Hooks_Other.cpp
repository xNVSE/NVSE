#include "Hooks_Other.h"
#include "GameAPI.h"
#include "Commands_UI.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "Commands_UI.h"
#include "Hooks_Gameplay.h"

#if RUNTIME
namespace OtherHooks
{
	// set ... to and if ... statements pass a different scriptData value from the stack, hook is used to save last position
	// function call is needed as g_lastScriptData is thread_local, compiler generates TlsSetValue from statement
	void __fastcall SaveOpcodePosition(UInt8* currentOpcodePos)
	{
		g_lastScriptData = currentOpcodePos;
	}

	__declspec(naked) void SaveRunLineScriptHook()
	{
		static const UInt32 hookedCall = 0x594D40;
		static const UInt32 returnAddress = 0x593E10;
		__asm
		{
			call hookedCall
			mov ecx, [ebp+0x8]
			call SaveOpcodePosition
			jmp returnAddress
		}
	}

	__declspec(naked) void TilesDestroyedHook()
	{
		__asm
		{
			mov g_tilesDestroyed, 1
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
			mov g_tilesDestroyed, 1
			pop ecx
			mov esp, ebp
			pop ebp
			ret
		}
	}

	void CleanUpNVSEVars(const ScriptEventList* eventList)
	{
		// Prevent leakage of variables that's ScriptEventList gets deleted (e.g. in Effect scripts)
		if (!eventList->m_script)
			return;
		auto* scriptVars = g_nvseVarGarbageCollectionMap.GetPtr(eventList->m_script->refID);
		if (!scriptVars)
			return;
		auto* node = eventList->m_vars;
		while (node)
		{
			if (node->var)
			{
				const auto id = node->var->id;
				const auto type = scriptVars->Get(id);
				if (type)
				{
					switch (type)
					{
					case kTokenType_StringVar:
						g_StringMap.MarkTemporary(node->var->data, true);
						break;
					case kTokenType_ArrayVar:
						g_ArrayMap.MarkTemporary(node->var->data, true);
						break;
					default:
						break;
					}
				}
			}
			node = node->next;
		}
	}

	void DeleteEventList(ScriptEventList* eventList)
	{
		CleanUpNVSEVars(eventList);
		ThisStdCall(0x5A8BC0, eventList);
		GameHeapFree(eventList);
	}
	
	ScriptEventList* __fastcall ScriptEventListsDestroyedHook(ScriptEventList *eventList, int EDX, bool doFree)
	{
		DeleteEventList(eventList);
		return eventList;
	}

	void Hooks_Other_Init()
	{
		WriteRelJump(0x593E0B, UInt32(SaveRunLineScriptHook));
		WriteRelJump(0x9FF5FB, UInt32(TilesDestroyedHook));
		WriteRelJump(0x709910, UInt32(TilesCreatedHook));
		WriteRelJump(0x41AF70, UInt32(ScriptEventListsDestroyedHook));
	}
}
#endif