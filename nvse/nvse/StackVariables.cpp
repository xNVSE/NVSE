#include "StackVariables.h"

#if RUNTIME
#include "ScriptTokens.h"

namespace StackVariables
{
	void SetLocalStackVarVal(Index_t idx, Value_t val, bool toNextStack)
	{
		if (toNextStack) {
			PushLocalStack();
		}

		g_localStackVars[g_localStackPtr].set(idx - 1, val);

		if (toNextStack) {
			PopLocalStack(false);
		}
	}

	void SetLocalStackVarFormID(Index_t idx, UInt32 formID, bool toNextStack)
	{
		auto* const outRefID = reinterpret_cast<UInt64*>(&(StackVariables::GetLocalStackVarVal(idx, toNextStack)));
		*outRefID = formID;
	}

	double& GetLocalStackVarVal(Index_t idx, bool fromNextStack)
	{
		if (fromNextStack) {
			PushLocalStack();
		}

		auto& result = g_localStackVars[g_localStackPtr].get(idx - 1);

		if (fromNextStack) {
			PopLocalStack(false);
		}
		return result;
	}

	void PushLocalStack()
	{
		if (g_localStackVars.size() <= g_localStackPtr + 1) {
			g_localStackVars.push_back(LocalStackFrame{});
		}
		g_localStackPtr++;
		_DMESSAGE("LOCAL VAR STACK PUSH : Index %d", g_localStackPtr);
	}

	void PopLocalStack(bool destroy)
	{
		// Reset all stack variable values to 0.
		// Prevents potentially seeing invalid values for stack vars that are used before assignment.
		if (destroy)
		{
			auto& currentVars = g_localStackVars[g_localStackPtr].vars;
			std::fill(currentVars.begin(), currentVars.end(), 0);
		}

		g_localStackPtr--;
		if (destroy) {
			_DMESSAGE("LOCAL VAR STACK DESTROY : Index %d", g_localStackPtr);
		}
		else {
			_DMESSAGE("LOCAL VAR STACK DESCEND : Index %d", g_localStackPtr);
		}
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

bool VariableStorage::AssignToArray(UInt32 arrID, ScriptEventList* eventList, bool toNextStack, ScriptLocal* resolvedLocal)
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
		StackVariables::SetLocalStackVarVal(m_varIdx, arrID, toNextStack);
		return true;
	}
	return false;
}

bool VariableStorage::AssignToString(const char* str, ScriptEventList* eventList, bool tempForLocal, 
	bool toNextStack, ScriptLocal* resolvedLocal)
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
		auto const strID = g_StringMap.Add(eventList->m_script->GetModIndex(), str, true);
		StackVariables::SetLocalStackVarVal(m_varIdx, strID, toNextStack);
		return true;
	}
	return false;
}

double* VariableStorage::GetValuePtr(ScriptEventList* eventList, bool fromNextStack, ScriptLocal* resolvedLocal) {
	if (!m_isStackVar)
	{
		ScriptLocal* var = resolvedLocal ? resolvedLocal : eventList->GetVariable(m_varIdx);
		if (!var) [[unlikely]] {
			return nullptr;
		}
		return &var->data;
	}
	else if (m_varIdx) {
		return &StackVariables::GetLocalStackVarVal(m_varIdx, fromNextStack);
	}
	return nullptr;
}

std::string VariableStorage::GetVariableName(ScriptEventList* eventList, bool fromNextStack, ScriptLocal* resolvedLocal)
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
