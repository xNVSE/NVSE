#include "StackVariables.h"

#if RUNTIME
#include "ScriptTokens.h"

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


// Useful so that the slow eventList->GetVariable doesn't have to be called again.
// Returns false for an invalid variable, or if non-stack-variable couldn't be resolved.

[[nodiscard]]
bool VariableStorage::ResolveVariable(ScriptEventList* eventList, ScriptLocal*& out_resolvedLocal) const {
	if (!IsValid()) [[unlikely]] {
		return false;
	}
	if (!m_isStackVar) {
		out_resolvedLocal = eventList->GetVariable(m_varIdx);
		if (!out_resolvedLocal) [[unlikely]] {
			return false;
		}
	}
	else if (m_varIdx == 0) [[unlikely]] {
		// stack variable index of 0 is invalid.
		return false;
	}
	return true;
}

bool VariableStorage::AssignToArray(UInt32 arrID, ScriptEventList* eventList, ScriptLocal* resolvedLocal)
{
	if (!m_isStackVar)
	{
		ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
		if (!var) [[unlikely]] {
			return false;
			}
		g_ArrayMap.AddReference(&var->data, arrID, eventList->m_script->GetModIndex());
		AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_Array);
		return true;
	}
	else if (m_varIdx) {
		StackVariables::SetLocalStackVarVal(m_varIdx, arrID);
		return true;
	}
	return false;
}

bool VariableStorage::AssignToString(const char* str, ScriptEventList* eventList, 
	bool tempForLocal, ScriptLocal* resolvedLocal)
{
	if (!m_isStackVar)
	{
		ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
		if (!var) [[unlikely]] {
			return false;
			}
		var->data = g_StringMap.Add(eventList->m_script->GetModIndex(), str, tempForLocal);
		AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_String);
		return true;
	}
	else if (m_varIdx) {
		StackVariables::SetLocalStackVarVal(m_varIdx, g_StringMap.Add(eventList->m_script->GetModIndex(), str, true));
		return true;
	}
	return false;
}

double* VariableStorage::GetValuePtr(ScriptEventList* eventList, ScriptLocal* resolvedLocal) {
	if (!m_isStackVar)
	{
		ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
		if (!var) [[unlikely]] {
			return nullptr;
			}
		return &var->data;
	}
	else if (m_varIdx) {
		return &StackVariables::GetLocalStackVarVal(m_varIdx);
	}
	return nullptr;
}
std::string VariableStorage::GetVariableName(ScriptEventList* eventList, ScriptLocal* resolvedLocal)
{
	if (!m_isStackVar)
	{
		ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
		if (!var) [[unlikely]] {
			return FormatString("<failed to get var name %d>", m_varIdx);
		}
		return std::string(::GetVariableName(var, eventList->m_script, eventList));
	}
	else {
		// TODO: do something better for stack variables.
		return FormatString("<stack var %d>", m_varIdx);
	}
}
#endif
