#include "Hooks_Other.h"
#include "GameAPI.h"
#include "Commands_UI.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "Commands_UI.h"

namespace OtherHooks
{
	void __stdcall PrintCurrentThreadID()
	{
		_MESSAGE("Thread ID: %X", GetCurrentThreadId());
	}

	__declspec(naked) void SaveRunLineScriptHook()
	{
		static const UInt32 hookedCall = 0x594D40;
		static const UInt32 returnAddress = 0x593E10;
		__asm
		{
			call hookedCall
			mov ecx, [ebp+0x8]
			mov g_lastScriptData, ecx
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

	__declspec(naked) void ScriptEventListsDestroyedHook()
	{
		static UInt32 hookedCall = 0x5A8EA0;
		static UInt32 returnAddress = 0x5AA0A9;
		__asm
		{
			call hookedCall
			mov g_scriptEventListsDestroyed, 1
			jmp returnAddress
		}
	}

	void Hooks_Other_Init()
	{
		WriteRelJump(0x593E0B, UInt32(SaveRunLineScriptHook));
		WriteRelJump(0x9FF5FB, UInt32(TilesDestroyedHook));
		WriteRelJump(0x5AA0A4, UInt32(ScriptEventListsDestroyedHook));
	}
}
