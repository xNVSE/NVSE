#pragma once

#include <mutex>
#include <stack>

#include "GameAPI.h"
#include "GameScript.h"

#ifdef RUNTIME

struct VarCache {
	std::mutex mt;
	std::unordered_map<ScriptEventList*, std::unordered_map<UInt32, ScriptLocal*>> varCache;

	void put(ScriptEventList* eventList, ScriptLocal* local) {
		varCache[eventList][local->id] = local;
	}

	ScriptLocal *get(ScriptEventList* eventList, UInt32 id) {
		auto entry = varCache[eventList];

		if (const auto find = entry.find(id); find != entry.end()) {
			return find->second;
		}

		return nullptr;
	}

	void clear(ScriptEventList* eventList) {
		varCache[eventList] = {};
	}
};

extern VarCache* g_scriptVarCache[];
extern thread_local int g_threadID;
extern std::atomic<int> g_nextThreadID;

// Utility struct
struct Variable {
	ScriptLocal* var{};
	Script::VariableType type = Script::eVarType_Invalid;

	[[nodiscard]] bool IsValid() const {
		return var != nullptr;
	}

	[[nodiscard]] Script::VariableType GetType() const {
		return type;
	}

	// Could check isStackVar, but we assume that was already checked.
	[[nodiscard]] ScriptLocal* GetScriptLocal() const
	{
		return var;
	}

	Variable() = default;
	Variable(ScriptLocal* _local, Script::VariableType _varType) : type(_varType)
	{
		var = _local;
	}
};
#endif

// Utility struct
// More suited for long storage, as ScriptLocal pointers can become invalid when loading, according to Kormakur.
struct VariableStorage {
	int m_varIdx = -1;
	Script::VariableType m_type = Script::eVarType_Invalid;

	[[nodiscard]] bool IsValid() const { return (m_varIdx != -1) && (m_type != Script::eVarType_Invalid); }
	[[nodiscard]] Script::VariableType GetType() const { return m_type; }

	// Useful so that the slow eventList->GetVariable doesn't have to be called again.
	// Returns false for an invalid variable, or if non-stack-variable couldn't be resolved.
	[[nodiscard]] bool ResolveVariable(ScriptEventList* eventList, ScriptLocal*& out_resolvedLocal) const;

	bool AssignToArray(UInt32 arrID, ScriptEventList* eventList, ScriptLocal* resolvedLocal = nullptr);
	bool AssignToString(const char* str, ScriptEventList* eventList, bool tempForLocal, ScriptLocal* resolvedLocal = nullptr);
	double* GetValuePtr(ScriptEventList* eventList, ScriptLocal* resolvedLocal = nullptr);
	std::string GetVariableName(ScriptEventList* eventList, ScriptLocal* resolvedLocal = nullptr);

	VariableStorage() = default;
	VariableStorage(UInt32 _varIdx, Script::VariableType _varType)
		: m_varIdx(_varIdx), m_type(_varType)
	{}
};

using UserFunctionParam = VariableStorage;