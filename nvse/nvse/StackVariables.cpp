#include "StackVariables.h"

#if RUNTIME
namespace StackVariables
{
	void SetLocalStackVarVal(Index_t idx, Value_t val)
	{
		g_localStackVars[g_localStackPtr].set(idx - 1, val);
	}

	void SetLocalStackVarFormID(Index_t idx, UInt32 formID)
	{
		auto* const outRefID = reinterpret_cast<UInt64*>(&(StackVariables::GetLocalStackVarVal(idx)));
		*outRefID = formID;
	}

	double& GetLocalStackVarVal(Index_t idx)
	{
		return g_localStackVars[g_localStackPtr].get(idx - 1);
	}

	void PushLocalStack()
	{
		if (g_localStackVars.size() <= g_localStackPtr + 1) {
			g_localStackVars.push_back(LocalStackFrame{});
		}
		g_localStackPtr++;
		_DMESSAGE("LOCAL VAR STACK CREATE : Index %d", g_localStackPtr);
	}

	void PopLocalStack()
	{
		// Reset all stack variable values to 0.
		// Prevents potentially seeing invalid values for stack vars that are used before assignment.
		{
			auto& currentVars = g_localStackVars[g_localStackPtr].vars;
			std::fill(currentVars.begin(), currentVars.end(), 0);
		}

		g_localStackPtr--;
		_DMESSAGE("LOCAL VAR STACK DESTROY : Index %d", g_localStackPtr);
	}
}
#endif