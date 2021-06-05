#pragma once
#include "ScriptUtils.h"
#include <unordered_map>
#include "FastStack.h"

struct UserFunctionParam
{
	UInt16	varIdx;
	UInt8	varType;

	UserFunctionParam(UInt16 _idx, UInt16 _type) : varIdx(_idx), varType(_type) { }
	UserFunctionParam() : varIdx(-1), varType(Script::eVarType_Invalid) { }
};

#if RUNTIME

struct FunctionContext;

// base class for Template Method-ish objects to execute function scripts
// derive from it to allow function scripts to be invoked from script or internal code
class FunctionCaller
{
public:
	virtual ~FunctionCaller() { }

	virtual UInt8 ReadCallerVersion() = 0;
	virtual Script * ReadScript() = 0;
	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) = 0;

	virtual TESObjectREFR* ThisObj() = 0;
	virtual TESObjectREFR* ContainingObj() = 0;
	virtual Script* GetInvokingScript() { return NULL; }
};

// stores info about function script (params, etc). generated once per function script and cached
struct FunctionInfo
{
private:
	DynamicParamInfo	m_dParamInfo;
	std::vector<UserFunctionParam> m_userFunctionParams;
	Script				* m_script;			// function script
	UInt16				* m_destructibles;	// dynamic array of var indexes of local array vars to be destroyed on function return
	UInt8				m_numDestructibles;
	UInt8				m_functionVersion;	// bytecode version of Function statement
	bool				m_bad;
	UInt8				m_instanceCount;
	ScriptEventList		* m_eventList = nullptr;		// cached for quicker construction of function script, but requires care when dealing with recursive function calls

public:
	FunctionInfo() {}
	FunctionInfo(Script* script);
	~FunctionInfo();

	FunctionContext	* CreateContext(UInt8 version, Script* invokingScript);
	bool IsGood() { return !m_bad; }
	bool IsActive() { return m_instanceCount ? true : false; }
	Script* GetScript() { return m_script; }
	ParamInfo* Params() { return m_dParamInfo.Params(); }
	DynamicParamInfo& ParamInfo() { return m_dParamInfo; }
	UserFunctionParam* GetParam(UInt32 paramIndex);
	bool CleanEventList(ScriptEventList* eventList);
	bool Execute(FunctionCaller& caller, FunctionContext* context);
	ScriptEventList* GetEventList() { return m_eventList; }
	UInt32 GetParamVarTypes(UInt8* out) const;	// returns count, if > 0 returns types as array
};

// represents a function executing on the stack
struct FunctionContext
{
private:
	FunctionInfo	* m_info;
	ScriptEventList	* m_eventList;		// temporary eventlist generated for function script
	ScriptToken		* m_result;
	Script			* m_invokingScript;
	UInt8			m_callerVersion;
	bool			m_bad;
	bool			m_isLambda = false;
public:
	FunctionContext(FunctionInfo* info, UInt8 version, Script* invokingScript);
	~FunctionContext();

	bool Execute(FunctionCaller & caller);
	bool Return(ExpressionEvaluator* eval);
	bool IsGood() { return !m_bad; }
	ScriptToken*  Result() { return m_result; }
	FunctionInfo* Info() { return m_info; }
	Script* InvokingScript() { return m_invokingScript; }
	bool IsLambda() const { return m_isLambda; }
	void* operator new(size_t size);
	void operator delete(void* p);
};

// controls user function calls.
// Manages a stack of function contexts
// Function args in Call bytecode. FunctionInfo encoded in Begin Function data. Return value from SetFunctionValue.
class UserFunctionManager
{
	static UserFunctionManager	* GetSingleton();

	UserFunctionManager();

	static const UInt32	kMaxNestDepth = 30;	// arbitrarily low; have seen 180+ nested calls execute w/o problems
	
	UInt32								m_nestDepth;
	Stack<FunctionContext*>		m_functionStack;
	UnorderedMap<Script*, FunctionInfo>	m_functionInfos;

	// these take a ptr to the function script to check that it matches executing script
	FunctionContext* Top(Script* funcScript);
	bool Pop(Script* funcScript);
	void Push(FunctionContext* context) { m_functionStack.Push(context); }
	FunctionInfo* GetFunctionInfo(Script* funcScript);

public:
	~UserFunctionManager();

	enum { kVersion = 1 };	// increment when bytecode representation changes

	static ScriptToken* Call(ExpressionEvaluator* eval);
	static bool	Return(ExpressionEvaluator* eval);
	static bool Enter(Script* funcScript);
	static ScriptToken* Call(FunctionCaller&& caller);
	static UInt32 GetFunctionParamTypes(Script* fnScript, UInt8* typesOut);

	// return script that called fnScript
	static Script* GetInvokingScript(Script* fnScript);

	static void ClearInfos() { GetSingleton()->m_functionInfos.Clear(); }
};

// allows us to call function scripts directly
class InternalFunctionCaller : public FunctionCaller
{
public:
	InternalFunctionCaller(Script* script, TESObjectREFR* callingObj = NULL, TESObjectREFR* container = NULL)
		: m_callerVersion(UserFunctionManager::kVersion), m_numArgs(0), m_script(script), m_thisObj(callingObj), m_container(container) { }

	virtual ~InternalFunctionCaller() { }
	virtual UInt8 ReadCallerVersion() {	return m_callerVersion; }
	virtual Script * ReadScript() {	return m_script; }
	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info);
	virtual TESObjectREFR* ThisObj() { return m_thisObj; }
	virtual TESObjectREFR* ContainingObj() { return m_container; }
	
	bool SetArgs(UInt8 numArgs, ...);
	bool vSetArgs(UInt8 numArgs, va_list args);

protected:
	enum { kMaxArgs = 10 };	

	UInt8			m_callerVersion;
	UInt8			m_numArgs;
	Script			* m_script;
	void			* m_args[kMaxArgs];
	TESObjectREFR	* m_thisObj;
	TESObjectREFR	* m_container;

	virtual bool ValidateParam(UserFunctionParam* param, UInt8 paramIndex) { return param != nullptr; }
}; 

namespace PluginAPI {
	bool CallFunctionScript(Script* fnScript, TESObjectREFR* callingObj, TESObjectREFR* container,
		NVSEArrayVarInterface::Element* result, UInt8 numArgs, ...);
}

#endif