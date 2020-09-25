#include "ThreadLocal.h"
#include "nvse/SafeWrite.h"
#include "FunctionScripts.h"
#include "Loops.h"

static const UInt32 kBackgroundLoaderThreadHookAddr = 0x0047CF3E;

void __stdcall HandleThreadExit()
{
	// when thread exits, free singleton objects in thread local storage

	ThreadLocalData* data = ThreadLocalData::TryGet();
	if (data) 
	{
		if (data->expressionEvaluator) 
			_WARNING("Game thread exiting with non-empty ExpressionEvaluator stack");
		
		delete data->userFunctionManager;
		delete data->loopManager;


		// free memory allocated for this thread's data
		delete data;
	}
}

static __declspec(naked) void BackgroundLoaderThreadHook(void)
{
	__asm {
		pushad
		call HandleThreadExit
		popad

		// overwritten code
		retn 4
	}
}

ThreadLocalData::ThreadLocalData(): expressionEvaluator(NULL), userFunctionManager(NULL), loopManager(NULL)
{
	//
}

ThreadLocalData& ThreadLocalData::Get()
{
	ThreadLocalData* data = TryGet();
	if (!data)
	{
		data = new ThreadLocalData();
		const auto result = TlsSetValue(s_tlsIndex, data);
		ASSERT_STR(result, "TlsSetValue() failed in ThreadLocalData::Get()");
	}

	return *data;
}

ThreadLocalData* ThreadLocalData::TryGet()
{
	return static_cast<ThreadLocalData*>(TlsGetValue(s_tlsIndex));
}

void ThreadLocalData::Init()
{
	s_tlsIndex = TlsAlloc();
	ASSERT_STR(s_tlsIndex != 0xFFFFFFFF, "TlsAlloc() failed in ThreadLocalData::Init()");

	// hook BackgroundLoaderThread threadProc retn
//	WriteRelJump(kBackgroundLoaderThreadHookAddr, (UInt32)&BackgroundLoaderThreadHook);
}

void ThreadLocalData::DeInit()
{
	if (s_tlsIndex != 0xFFFFFFFF) {
		TlsFree(s_tlsIndex);
		s_tlsIndex = 0xFFFFFFFF;
	}
}

DWORD ThreadLocalData::s_tlsIndex = 0xFFFFFFFF;
