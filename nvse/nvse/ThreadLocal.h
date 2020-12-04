#pragma once

#include "ScriptTokenCache.h"
#include "common/ICriticalSection.h"

class ExpressionEvaluator;
class UserFunctionManager;
class LoopManager;

/* added v0020 to clean up the way we handle scripts executing in parallel */

struct ThreadLocalData
{
	ExpressionEvaluator* expressionEvaluator;	// evaluator at top of expression stack
	UserFunctionManager* userFunctionManager;	// per-thread singleton
	LoopManager* loopManager;			// per-thread singleton

	ThreadLocalData();

	// get data for current thread, creating if it doesn't exist yet
	static ThreadLocalData& Get();

	// get for current thread if data exists
	static ThreadLocalData* TryGet();

	static void Init();
	static void DeInit();

private:
	static DWORD	s_tlsIndex;
};
