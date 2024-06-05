#pragma once

#ifdef RUNTIME

#include "GameScript.h"

namespace StackVariables
{
	using Index_t = int;
	using Value_t = double;

	struct LocalStackFrame {
		std::vector<Value_t> vars{};

		Value_t& get(Index_t idx) {
			if (idx >= vars.size()) {
				vars.resize(idx + 10, 0);
			}

			return vars[idx];
		}

		void set(Index_t idx, Value_t val) {
			if (idx >= vars.size()) {
				vars.resize(idx + 10, 0);
			}

			vars[idx] = val;
		}
	};

	inline thread_local std::vector<LocalStackFrame> g_localStackVars;
	inline thread_local Index_t g_localStackPtr{ -1 };

	void SetLocalStackVarVal(Index_t idx, Value_t val);
	void SetLocalStackVarFormID(Index_t idx, UInt32 formID);
	Value_t& GetLocalStackVarVal(Index_t idx);

	void PushLocalStack();
	void PopLocalStack();
}

// Utility struct
struct Variable {
	union
	{
		ScriptLocal* local{};
		StackVariables::Index_t stackVarIdx;
	} var{};
	bool isStackVar = false;
	Script::VariableType type = Script::eVarType_Invalid;

	[[nodiscard]] bool IsValid() const {
		return var.local != nullptr;
	}

	[[nodiscard]] Script::VariableType GetType() const {
		return type;
	}

	// Could check isStackVar, but we assume that was already checked.
	[[nodiscard]] ScriptLocal* GetScriptLocal() const
	{
		return var.local;
	}
	[[nodiscard]] StackVariables::Index_t GetStackVarIdx() const
	{
		return var.stackVarIdx;
	}

	Variable() = default;
	Variable(ScriptLocal* _local, Script::VariableType _varType) : isStackVar(false), type(_varType)
	{
		var.local = _local;
	}
	Variable(StackVariables::Index_t _stackVarIdx, Script::VariableType _varType) : isStackVar(true), type(_varType)
	{
		var.stackVarIdx = _stackVarIdx;
	}
};
#endif