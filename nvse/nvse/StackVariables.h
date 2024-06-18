#pragma once

#include <mutex>
#include <stack>

#include "GameScript.h"

#ifdef RUNTIME

namespace StackVariables
{
	using Index_t = int;
	using Value_t = double;

	struct LocalStackFrame {
		std::vector<Value_t> vars{};
		std::vector<std::string> names{};

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

	void SetLocalStackVarVal(Index_t idx, Value_t val, bool toNextStack = false);
	void SetLocalStackVarFormID(Index_t idx, UInt32 formID, bool toNextStack = false);
	Value_t& GetLocalStackVarVal(Index_t idx, bool fromNextStack = false);

	void PushLocalStack();
	void PopLocalStack(bool destroy = true);

	std::vector<std::string> ParseDumpStackVars(UInt8* data);
}

struct VarCache {
	std::mutex mt;
	std::unordered_map<ScriptEventList*, std::unordered_map<UInt32, ScriptLocal*>> varCache;
};

extern VarCache* g_scriptVarCache[];
extern thread_local int g_threadID;
extern std::atomic<int> g_nextThreadID;

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

// Utility struct
// More suited for long storage, as ScriptLocal pointers can become invalid when loading, according to Kormakur.
struct VariableStorage {
	int m_varIdx = -1;
	bool m_isStackVar = false;
	Script::VariableType m_type = Script::eVarType_Invalid;

	[[nodiscard]] bool IsValid() const { return (m_varIdx != -1) && (m_type != Script::eVarType_Invalid); }
	[[nodiscard]] Script::VariableType GetType() const { return m_type; }

	// Useful so that the slow eventList->GetVariable doesn't have to be called again.
	// Returns false for an invalid variable, or if non-stack-variable couldn't be resolved.
	[[nodiscard]] bool ResolveVariable(ScriptEventList* eventList, ScriptLocal*& out_resolvedLocal) const;

	bool AssignToArray(UInt32 arrID, ScriptEventList* eventList, bool toNextStack, ScriptLocal* resolvedLocal = nullptr);
	bool AssignToString(const char* str, ScriptEventList* eventList, bool tempForLocal, bool toNextStack, ScriptLocal* resolvedLocal = nullptr);
	double* GetValuePtr(ScriptEventList* eventList, bool fromNextStack, ScriptLocal* resolvedLocal = nullptr);
	std::string GetVariableName(ScriptEventList* eventList, bool fromNextStack, ScriptLocal* resolvedLocal = nullptr);

	VariableStorage() = default;
	VariableStorage(UInt32 _varIdx, bool _isStackVar, Script::VariableType _varType)
		: m_varIdx(_varIdx), m_isStackVar(_isStackVar), m_type(_varType)
	{}
};

using UserFunctionParam = VariableStorage;