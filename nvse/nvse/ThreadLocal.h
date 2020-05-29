#pragma once

#include "common/ICriticalSection.h"

class ExpressionEvaluator;
class UserFunctionManager;
class LoopManager;

/* added v0020 to clean up the way we handle scripts executing in parallel */

struct ThreadLocalData
{
	ExpressionEvaluator		* expressionEvaluator;	// evaluator at top of expression stack
	UserFunctionManager		* userFunctionManager;	// per-thread singleton
	LoopManager				* loopManager;			// per-thread singleton

	ThreadLocalData() : expressionEvaluator(NULL), userFunctionManager(NULL), loopManager(NULL) {
		//
	}

	// get data for current thread, creating if it doesn't exist yet
	static ThreadLocalData& Get() {
		ThreadLocalData* data = TryGet();
		if (!data) {
			data = new ThreadLocalData();
			int result = TlsSetValue(s_tlsIndex, data);
			ASSERT_STR(result, "TlsSetValue() failed in ThreadLocalData::Get()");
		}

		return *data;
	}

	// get for current thread if data exists
	static ThreadLocalData* TryGet() {
		return (ThreadLocalData*)TlsGetValue(s_tlsIndex);
	}

	static void Init();
	static void DeInit();

private:
	static DWORD	s_tlsIndex;
};

class ScopedLock
{
public:
	ScopedLock(ICriticalSection& critSection) : m_critSection(critSection) {
		m_critSection.Enter();
	}

	~ScopedLock() {
		m_critSection.Leave();
	}

private:
	ICriticalSection& m_critSection;
};
