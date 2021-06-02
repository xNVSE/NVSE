#include "FunctionScripts.h"
#include "ScriptTokens.h"
#include "ThreadLocal.h"
#include "GameRTTI.h"
#include "LambdaManager.h"

/*******************************************
	UserFunctionManager
*******************************************/

UserFunctionManager::UserFunctionManager() : m_nestDepth(0)
{
	//
}

UserFunctionManager::~UserFunctionManager()
{	
	while (!m_functionStack.Empty())
	{
		FunctionContext** context = &m_functionStack.Top();
		delete* context;
	}
}

static SmallObjectsAllocator::LockBasedAllocator<FunctionContext, 5> g_functionContextAllocator;

void* FunctionContext::operator new(size_t size)
{
	return g_functionContextAllocator.Allocate();
}

void FunctionContext::operator delete(void* p)
{
	g_functionContextAllocator.Free(p);
}

UserFunctionManager* UserFunctionManager::GetSingleton()
{
	ThreadLocalData& data = ThreadLocalData::Get();
	if (!data.userFunctionManager) {
		data.userFunctionManager = new UserFunctionManager();
	}

	return data.userFunctionManager;
}

FunctionContext* UserFunctionManager::Top(Script* funcScript)
{
	if (m_functionStack.Size() && m_functionStack.Top()->Info()->GetScript() == funcScript)
		return m_functionStack.Top();

	return NULL;
}

bool UserFunctionManager::Pop(Script* funcScript)
{
	FunctionContext* context = Top(funcScript);
	if (context)
	{
		m_functionStack.Pop();
		delete context;
		return true;
	}

#ifdef DBG_EXPR_LEAKS
	if (FUNCTION_CONTEXT_COUNT != m_functionStack.Size()) {
		DEBUG_PRINT("UserFunctionManager::Pop() detects leak - %d FunctionContext exist, %d expected",
			FUNCTION_CONTEXT_COUNT,
			m_functionStack.Size());
	}
#endif

	return false;
}

class ScriptFunctionCaller : public FunctionCaller
{
public:
	ScriptFunctionCaller(ExpressionEvaluator & context) : m_eval(context), m_callerVersion(-1), m_funcScript(NULL)
		{ }
	virtual ~ScriptFunctionCaller() { }

	virtual UInt8 ReadCallerVersion() {
		m_callerVersion = m_eval.ReadByte();
		return m_callerVersion;
	}

	virtual Script * ReadScript() {
		if (m_funcScript)
			return m_funcScript;

		ScriptToken* scrToken = NULL;
		switch (m_callerVersion) {
			case 0: 
				scrToken = ScriptToken::Read(&m_eval);
				break;
			case 1:
				scrToken = m_eval.Evaluate();
				break;
			default:
				m_eval.Error("Unknown bytecode version %d encountered in Call statement", m_callerVersion);
				return NULL;
		}

		if (scrToken) {
			m_funcScript = DYNAMIC_CAST(scrToken->GetTESForm(), TESForm, Script);
			delete scrToken;
		}

		return m_funcScript;
	}

	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) {
		m_eval.SetParams(info->Params());
		if (!m_eval.ExtractArgs())
			return false;

		// populate event list variables
		for (UInt32 i = 0; i < m_eval.NumArgs(); i++)
		{
			ScriptToken* arg = m_eval.Arg(i);

			UserFunctionParam* param = info->GetParam(i);
			if (!param)
			{
				ShowRuntimeError(info->GetScript(), "Param index %02X out of bounds", i);
				return false;
			}

			ScriptEventList::Var* var = eventList->GetVariable(param->varIdx);
			if (!var)
			{
				ShowRuntimeError(info->GetScript(), "Param variable not found. Function definition may be out of sync with function call. Recomplie the scripts and try again.");
				return false;
			}

			switch (param->varType)
			{
			case Script::eVarType_Array:
				if (arg->CanConvertTo(kTokenType_Array))
					g_ArrayMap.AddReference(&var->data, arg->GetArray(), info->GetScript()->GetModIndex());
				break;
			case Script::eVarType_String:
				if (arg->CanConvertTo(kTokenType_String))
					var->data = g_StringMap.Add(info->GetScript()->GetModIndex(), arg->GetString(), true);
				break;
			case Script::eVarType_Ref:
				if (arg->CanConvertTo(kTokenType_Form))
					*((UInt32*)&var->data) = arg->GetFormID();
				break;
			case Script::eVarType_Integer:
			case Script::eVarType_Float:
				if (arg->CanConvertTo(kTokenType_Number))
					var->data = (param->varType == Script::eVarType_Integer) ? floor(arg->GetNumber()) : arg->GetNumber();
				break;
			default:
				ShowRuntimeError(info->GetScript(), "Unknown variable type %02X encountered for parameter %d", param->varType, i);
				return false;
			}
		}

		return true;
	}

	virtual TESObjectREFR* ThisObj() { return m_eval.ThisObj(); }
	virtual TESObjectREFR* ContainingObj() { return m_eval.ContainingObj(); }
	virtual Script* GetInvokingScript() { return m_eval.script; }
private:
	ExpressionEvaluator&	m_eval;
	UInt8					m_callerVersion;
	Script					* m_funcScript;
};

ScriptToken* UserFunctionManager::Call(ExpressionEvaluator* eval)
{
	ScriptFunctionCaller caller(*eval);
	return Call(std::move(caller));
}

ScriptToken* UserFunctionManager::Call(FunctionCaller&& caller)
{
	UserFunctionManager* funcMan = GetSingleton();

	if (funcMan->m_nestDepth >= kMaxNestDepth)
	{
		ShowRuntimeError(NULL, "Max nest depth %d exceeded in function call.", kMaxNestDepth);
		return NULL;
	}

	// extract version and script
	UInt8 callerVersion = caller.ReadCallerVersion();
	Script* funcScript = caller.ReadScript();

	if (!funcScript)
	{
		ShowRuntimeError(NULL, "Could not extract function script.");
		return NULL;
	}

	if (!funcScript->data)
		return nullptr;

	// get function info for script
	FunctionInfo* info = funcMan->GetFunctionInfo(funcScript);
	if (!info)
	{
		ShowRuntimeError(funcScript, "Could not parse function info for function script");
		return NULL;
	}

	// create a function context for execution
	FunctionContext* context = info->CreateContext(callerVersion, caller.GetInvokingScript());
	if (!context)
	{
		ShowRuntimeError(funcScript, "Could not create function context for function script");
		return NULL;
	}

	// push and execute on stack
	ScriptToken * funcResult = NULL;
	funcMan->Push(context);

	funcMan->m_nestDepth++;
	if (info->Execute(caller, context) && funcMan->Top(funcScript) && context->Result())
		funcResult = context->Result()->ToBasicToken();

	funcMan->m_nestDepth--;

	if (!funcMan->Pop(funcScript))
		ShowRuntimeError(funcScript, "Call stack is corrupted on return from call to function script");

	return funcResult;
}

bool UserFunctionManager::Enter(Script* funcScript)
{
	FunctionInfo* info = GetSingleton()->GetFunctionInfo(funcScript);
	return info ? info->IsGood() : false;
}

bool UserFunctionManager::Return(ExpressionEvaluator* eval)
{
	UserFunctionManager* funcMan = GetSingleton();
	FunctionContext* context = funcMan->Top(eval->script);
	if (!context)
		return false;

	return context->Return(eval);
}

FunctionInfo* UserFunctionManager::GetFunctionInfo(Script* funcScript)
{
	FunctionInfo* funcInfo = m_functionInfos.Emplace(funcScript, funcScript);
	return (funcInfo->IsGood()) ? funcInfo : NULL;
}
	
UInt32 UserFunctionManager::GetFunctionParamTypes(Script* fnScript, UInt8* typesOut)
{
	FunctionInfo* info = GetSingleton()->GetFunctionInfo(fnScript);
	UInt32 numParams = -1;
	if (info) {
		numParams = info->GetParamVarTypes(typesOut);
	}

	return numParams;
}

Script* UserFunctionManager::GetInvokingScript(Script* fnScript)
{
	FunctionContext* context = GetSingleton()->Top(fnScript);
	return context ? context->InvokingScript() : NULL;
}

/*****************************
	FunctionInfo
*****************************/

FunctionInfo::FunctionInfo(Script* script)
: m_script(script), m_destructibles(NULL), m_numDestructibles(0), m_functionVersion(-1), m_bad(0), m_instanceCount(0), m_eventList(NULL)
{
	if (!script || !script->data)
		return;

	//								0             4       6       8       A             E
	// scriptData should look like: 1D 00 00 00 | 10 00 | XX XX | 07 00 | XX XX XX XX | ....
	//							        scn       begin    len    opcode   block len     our data
	// if it doesn't, it's not a function script
	if (script->info.dataLength < 15)
		return;

	UInt8* data = (UInt8*)script->data;
	if (*(data + 8) != 0x0D)		// not a 'Begin Function' block
	{
		ShowRuntimeError(script, "Begin Function block not found in compiled script data");
		return;
	}

	data += 14;		// beginning of our data

	m_functionVersion = *data++;
	switch (m_functionVersion)
	{
	case 0:
	case 1:
		break;
	default:
		ShowRuntimeError(script, "Unknown function version %02X.", m_functionVersion);
		return;
	}

	// **************************
	// *** bytecode version 0 ***
	// **************************

	// read params and construct ParamInfo
	UInt8 numParams = *data++;
	std::vector<UserFunctionParam> params(numParams);

	for (UInt32 i = 0; i < numParams; i++)
	{
		UInt16 idx = *((UInt16*)data);
		data += 2;
		UInt8 type = *data++;
		params[i] = UserFunctionParam(idx, type);
	}

	m_dParamInfo = DynamicParamInfo(params);
	m_userFunctionParams = params;

	if (!LambdaManager::IsScriptLambda(m_script))
	{
		// read destructibles
		m_numDestructibles = *data++;
		if (m_numDestructibles)
		{
			m_destructibles = new UInt16[m_numDestructibles];
			for (UInt32 i = 0; i < m_numDestructibles; i++)
			{
				m_destructibles[i] = *((UInt16*)data);
				data += 2;
			}
		}

		// construct event list
		m_eventList = m_script->CreateEventList();
		if (!m_eventList)
			ShowRuntimeError(script, "Cannot create initial event script.");

		// successfully constructed
		m_bad = (NULL == m_eventList);
	}
	else
	{
		m_bad = false;
	}
}

FunctionInfo::~FunctionInfo()
{
	if (m_numDestructibles)
		delete[] m_destructibles;
	if (m_eventList)
		GameHeapFree(m_eventList);
}

FunctionContext* FunctionInfo::CreateContext(UInt8 version, Script* invokingScript)
{
	if (!IsGood())
		return NULL;

	FunctionContext* context = new FunctionContext(this, version, invokingScript);
	if (!context->IsGood())
	{
		delete context;
		return NULL;
	}

	return context;
}

UserFunctionParam* FunctionInfo::GetParam(UInt32 paramIndex)
{
	if (paramIndex >= m_userFunctionParams.size())
		return NULL;

	return &m_userFunctionParams[paramIndex];
}

UInt32 FunctionInfo::GetParamVarTypes(UInt8* out) const
{
	UInt32 count = m_userFunctionParams.size();
	if (count) {
		for (UInt32 i = 0; i < count; i++) {
			out[i] = m_userFunctionParams[i].varType;
		}
	}

	return count;
}

bool FunctionInfo::CleanEventList(ScriptEventList* eventList)
{
	for (UInt32 i = 0; i < m_numDestructibles; i++)
	{
		ScriptEventList::Var* var = eventList->GetVariable(m_destructibles[i]);
		if (!var)
			return false;

		g_ArrayMap.RemoveReference(&var->data, m_script->GetModIndex());
	}

	return true;
}

bool FunctionInfo::Execute(FunctionCaller& caller, FunctionContext* context)
{
	// this should never happen as max function call depth is capped at 30
	ASSERT(m_instanceCount < 0xFF);
	auto enteredLambdaLock = false;
	if (context->IsLambda() && LambdaManager::g_lambdasCleared)
	{
		enteredLambdaLock = true;
		LambdaManager::g_lambdaCs.Enter();
	}
	m_instanceCount++;
	bool bResult = context->Execute(caller);
	m_instanceCount--;
	if (enteredLambdaLock)
		LambdaManager::g_lambdaCs.Leave();
	return bResult;
}

/******************************
	FunctionContext
******************************/

bool CopyToEventList(ScriptEventList* to, ScriptEventList* from)
{
	// need to refactor m_vars to tList later
	auto* toVars = reinterpret_cast<tList<ScriptEventList::Var>*>(to->m_vars);
	auto* fromVars = reinterpret_cast<tList<ScriptEventList::Var>*>(from->m_vars);
	auto fromIter = fromVars->Begin();
	for (auto toIter = toVars->Begin(); !toIter.End(); ++toIter, ++fromIter)
	{
		if (fromIter.End())
		{
			return false;
		}
		if (toIter->id == fromIter->id)
		{
			toIter->data = fromIter->data;
		}
		else
		{
			return false;
		}
	}
	return true;
}

FunctionContext::FunctionContext(FunctionInfo* info, UInt8 version, Script* invokingScript) : m_info(info), m_eventList(NULL),
m_invokingScript(invokingScript), m_callerVersion(version), m_bad(true), m_result(NULL)
{
#ifdef DBG_EXPR_LEAKS
	FUNCTION_CONTEXT_COUNT++;
#endif

	switch (version)
	{
	case 0:
	case 1:
		break;
	default:
		ShowRuntimeError(info->GetScript(), "Unknown function version %02X", version);
		return;
	}
	if (auto* parentEventList = LambdaManager::GetParentEventList(info->GetScript()))
	{
		m_eventList = parentEventList;
		m_isLambda = true;
	}
	else
	{
		if (info->IsActive()) {
			m_eventList = info->GetScript()->CreateEventList();
		}
		else {
			m_eventList = info->GetEventList();
			if (!m_eventList) // Let's try again if it failed originally (though info would be null in that case, so very unlikely to be called ever).
				m_eventList = info->GetScript()->CreateEventList();
		}
	}
	if (!m_eventList)
	{
		if (info->IsActive())
			ShowRuntimeError(info->GetScript(), "Couldn't create eventlist");
		else
			ShowRuntimeError(info->GetScript(), "Couldn't recreate eventlist");
		return;
	}
#if 0
	if (auto* parentEventList = LambdaManager::GetParentEventList(info->GetScript()))
	{
		if (!CopyToEventList(m_eventList, parentEventList))
		{
			ShowRuntimeError(info->GetScript(), "Event list mismatch between lambda and outer script");
			return;
		}
	}
#endif

	// successfully constructed
	m_bad = false;
}

FunctionContext::~FunctionContext()
{
#ifdef DBG_EXPR_LEAKS
	FUNCTION_CONTEXT_COUNT--;
#endif

	if (m_eventList && !m_isLambda)
	{
		if (m_eventList != m_info->GetEventList()) {
			m_eventList->Destructor();
			FormHeap_Free(m_eventList);
		}
		else {
			m_eventList->ResetAllVariables();
		}
		LambdaManager::MarkParentAsDeleted(m_eventList); // If any lambdas refer to the event list, clear them away
	}

	delete m_result;
}

bool FunctionContext::Execute(FunctionCaller & caller)
{
	if (!IsGood())
		return false;

	// Extract arguments to function
	if (!caller.PopulateArgs(m_eventList, m_info))
		return false;

	
	// run the script
	CALL_MEMBER_FN(m_info->GetScript(), Execute)(caller.ThisObj(), m_eventList, caller.ContainingObj(), false);

	// clean up
	if (m_info->CleanEventList(m_eventList))
		return true;

	ShowRuntimeError(m_info->GetScript(), "Couldn't clean event list after function call.");
	return false;
}

bool FunctionContext::Return(ExpressionEvaluator* eval)
{
	delete m_result;

	if (!eval->ExtractArgs() || eval->NumArgs() != 1)
	{
		m_result = NULL;
		return false;
	}
	else
	{
		m_result = eval->Arg(0)->ToBasicToken();
		return true;
	}
}

/*******************************
	InternalFunctionCaller
*******************************/

bool InternalFunctionCaller::PopulateArgs(ScriptEventList* eventList, FunctionInfo* info)
{
	DynamicParamInfo& dParams = info->ParamInfo();
	if (dParams.NumParams() >= kMaxArgs) {
		return false;
	}

	// populate the args in the event list
	for (UInt32 i = 0; i < m_numArgs; i++) {
		UserFunctionParam* param = info->GetParam(i);
		if (!ValidateParam(param, i)) {
			ShowRuntimeError(m_script, "Failed to extract arg %d", i);
			return false;
		}

		ScriptEventList::Var* var = eventList->GetVariable(param->varIdx);
		if (!var) {
			ShowRuntimeError(m_script, "Could not look up argument variable for function script");
			return false;
		}

		switch (param->varType) {
			case Script::eVarType_Integer:
				var->data = (SInt32)m_args[i];
				break;
			case Script::eVarType_Float:
				var->data = *((float*)&m_args[i]);
				break;
			case Script::eVarType_Ref:
				{
					TESForm* form = (TESForm*)m_args[i];
					*((UInt32*)&var->data) = form ? form->refID : 0;
				}
				break;
			case Script::eVarType_String:
				var->data = g_StringMap.Add(info->GetScript()->GetModIndex(), (const char*)m_args[i], true);
				break;
			case Script::eVarType_Array:
				{
					ArrayID arrID = (ArrayID)m_args[i];
					if (g_ArrayMap.Get(arrID))
						g_ArrayMap.AddReference (&var->data, arrID, info->GetScript()->GetModIndex());
					else
						var->data = 0;
				}
				break;
			default:
				// wtf?
				ShowRuntimeError(m_script, "Unexpected param type %02X in internal function call", param->varType);
				return false;
		}
	}

	return true;
}

bool InternalFunctionCaller::SetArgs(UInt8 numArgs, ...)
{
	va_list args;
	va_start(args, numArgs);
	bool result = vSetArgs(numArgs, args);
	va_end(args);
	return result;
}

bool InternalFunctionCaller::vSetArgs(UInt8 numArgs, va_list args)
{
	if (numArgs >= kMaxArgs) {
		return false;
	}

	m_numArgs = numArgs;
	for (UInt8 i = 0; i < numArgs; i++) {
		m_args[i] = va_arg(args, void*);
	}

	return true;
}

namespace PluginAPI
{
	bool CallFunctionScript(Script* fnScript, TESObjectREFR* callingObj, TESObjectREFR* container, 
		NVSEArrayVarInterface::Element* result, UInt8 numArgs, ...)
	{
		InternalFunctionCaller caller(fnScript, callingObj, container);
		va_list args;
		va_start(args, numArgs);
		bool success = caller.vSetArgs(numArgs, args);
		if (success)
		{
			ScriptToken* ret = UserFunctionManager::Call(std::move(caller));
			if (ret)
			{
				if (result)
				{
					switch (ret->Type())
					{
						case kTokenType_Number:
							*result =  ret->GetNumber();
							break;
						case kTokenType_Form:
							*result =  ret->GetTESForm();
							break;
						case kTokenType_Array:
							*result = ret->GetArray();
							break;
						case kTokenType_String:
							*result = ret->GetString();
							break;
						default:
							*result = NVSEArrayVarInterface::Element();
							ShowRuntimeError(fnScript, "Function script called from plugin returned unexpected type %02X", ret->Type());
							success = false;
					}
				}
				delete ret;
			}
		}
		return success;
	}
}
