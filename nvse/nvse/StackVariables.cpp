#include "StackVariables.h"

#if RUNTIME
#include "ScriptTokens.h"

// Useful so that the slow eventList->GetVariable doesn't have to be called again.
// Returns false for an invalid variable, or if non-stack-variable couldn't be resolved.

[[nodiscard]]
bool VariableStorage::ResolveVariable(ScriptEventList* eventList, ScriptLocal*& out_resolvedLocal) const {
	if (!IsValid()) [[unlikely]] {
		return false;
	}
	if (m_varIdx == 0) [[unlikely]] {
		// stack variable index of 0 is invalid.
		return false;
	}

	out_resolvedLocal = eventList->GetVariable(m_varIdx);
	if (!out_resolvedLocal) [[unlikely]] {
		return false;
	}

	return true;
}

bool VariableStorage::AssignToArray(UInt32 arrID, ScriptEventList* eventList, ScriptLocal* resolvedLocal)
{
	ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
	if (!var) [[unlikely]] {
		return false;
		}
	g_ArrayMap.AddReference(&var->data, arrID, eventList->m_script->GetModIndex());
	AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_Array);
	return true;
}

bool VariableStorage::AssignToString(const char* str, ScriptEventList* eventList, bool tempForLocal, ScriptLocal* resolvedLocal)
{
	ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
	if (!var) [[unlikely]] {
		return false;
	}
	var->data = g_StringMap.Add(eventList->m_script->GetModIndex(), str, tempForLocal);
	AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_String);
	return true;
}

double* VariableStorage::GetValuePtr(ScriptEventList* eventList, ScriptLocal* resolvedLocal) {
	ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
	if (!var) [[unlikely]] {
		return nullptr;
	}
	return &var->data;
}

std::string VariableStorage::GetVariableName(ScriptEventList* eventList, ScriptLocal* resolvedLocal)
{
	ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
	if (!var) [[unlikely]] {
		return FormatString("<failed to get var name %d>", m_varIdx);
	}
	return std::string(::GetVariableName(var, eventList->m_script, eventList));
}

VarCache *g_scriptVarCache[16] {};
thread_local int g_threadID = -1;
std::atomic<int> g_nextThreadID = 0;
#endif
