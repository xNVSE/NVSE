#include "Hooks_Other.h"
#include "GameAPI.h"
#include "SafeWrite.h"


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
			mov esp, ebp
			pop ebp
			ret 0x4
		}
	}

	void Hooks_Other_Init()
	{
		WriteRelJump(0x593E0B, UInt32(SaveRunLineScriptHook));
		WriteRelJump(0x7099A6, UInt32(TilesDestroyedHook));
	}
}
