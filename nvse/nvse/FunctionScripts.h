#pragma once
#include "ScriptUtils.h"
#include <unordered_map>
#include "FastStack.h"

struct UserFunctionParam
{
	UInt16	varIdx;
	Script::VariableType	varType;

	UserFunctionParam(UInt16 _idx, Script::VariableType _type) : varIdx(_idx), varType(_type) { }
	UserFunctionParam() : varIdx(-1), varType(Script::eVarType_Invalid) { }
};

#if RUNTIME

struct FunctionContext;

// base class for Template Method-ish objects to execute function scripts
// derive from it to allow function scripts to be invoked from script or internal code
class FunctionCaller
{
public:
	virtual ~FunctionCaller() = default;

	virtual UInt8 ReadCallerVersion() = 0;
	virtual Script* ReadScript() = 0;
	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) = 0;

	virtual TESObjectREFR* ThisObj() = 0;
	virtual TESObjectREFR* ContainingObj() = 0;
	virtual Script* GetInvokingScript() { return nullptr; }

};

// stores info about function script (params, etc). generated once per function script and cached
struct FunctionInfo
{
	DynamicParamInfo	m_dParamInfo;
	std::vector<UserFunctionParam> m_userFunctionParams;
	Script* m_script;			// function script
	UInt8				m_functionVersion;	// bytecode version of Function statement
	bool				m_bad;
	UInt8				m_instanceCount;
	ScriptEventList* m_eventList;		// cached for quicker construction of function script, but requires care when dealing with recursive function calls
#if _DEBUG
	const char* editorID;
#endif
	UInt8* m_singleLineLambdaPosition = nullptr;
	bool				m_isLambda;

	FunctionInfo() = default;
	FunctionInfo(Script* script);
	~FunctionInfo();

	FunctionContext* CreateContext(UInt8 version, Script* invokingScript);
	[[nodiscard]] bool IsGood() const { return !m_bad; }
	[[nodiscard]] bool IsActive() const { return m_instanceCount ? true : false; }
	[[nodiscard]] Script* GetScript() const { return m_script; }
	ParamInfo* Params() { return m_dParamInfo.Params(); }
	DynamicParamInfo& ParamInfo() { return m_dParamInfo; }
	UserFunctionParam* GetParam(UInt32 paramIndex);
	bool Execute(FunctionCaller& caller, FunctionContext* context);
	[[nodiscard]] ScriptEventList* GetEventList() const { return m_eventList; }
	UInt32 GetParamVarTypes(UInt8* out) const;	// returns count, if > 0 returns types as array
};

// represents a function executing on the stack
struct FunctionContext
{
private:
	FunctionInfo* m_info;
	ScriptEventList* m_eventList;		// temporary eventlist generated for function script
	std::unique_ptr<ScriptToken> m_result;
	Script* m_invokingScript;
	UInt8			m_callerVersion;
	bool			m_bad;
	bool m_lambdaBackupEventList; // if parent event list can't be retrieved, us
public:
	FunctionContext(FunctionInfo* info, UInt8 version, Script* invokingScript);
	~FunctionContext();

	bool Execute(FunctionCaller& caller) const;
	bool Return(ExpressionEvaluator* eval);
	[[nodiscard]] bool IsGood() const { return !m_bad; }
	[[nodiscard]] ScriptToken* Result() const { return m_result.get(); }
	[[nodiscard]] FunctionInfo* Info() const { return m_info; }
	[[nodiscard]] Script* InvokingScript() const { return m_invokingScript; }
	void* operator new(size_t size);
	void operator delete(void* p);
};

// controls user function calls.
// Manages a stack of function contexts
// Function args in Call bytecode. FunctionInfo encoded in Begin Function data. Return value from SetFunctionValue.
class UserFunctionManager
{
	static UserFunctionManager* GetSingleton();

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

	static bool	Return(ExpressionEvaluator* eval);
	static bool Enter(Script* funcScript);
	static std::unique_ptr<ScriptToken> Call(ExpressionEvaluator* eval);
	static std::unique_ptr<ScriptToken> Call(FunctionCaller&& caller);
	static UInt32 GetFunctionParamTypes(Script* fnScript, UInt8* typesOut);

	// return script that called fnScript
	static Script* GetInvokingScript(Script* fnScript);

	static void ClearInfos();
};

// allows us to call function scripts directly
class InternalFunctionCaller : public FunctionCaller
{
public:
	InternalFunctionCaller(Script* script, TESObjectREFR* callingObj = nullptr, TESObjectREFR* container = nullptr, bool allowSurplusDispatchArgs = false)
		: m_callerVersion(UserFunctionManager::kVersion), m_numArgs(0), m_script(script), m_thisObj(callingObj),
			m_container(container), m_allowSurplusDispatchArgs(allowSurplusDispatchArgs)
	{ }

	virtual ~InternalFunctionCaller() = default;
	virtual UInt8 ReadCallerVersion() { return m_callerVersion; }
	virtual Script* ReadScript() { return m_script; }
	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info);
	virtual TESObjectREFR* ThisObj() { return m_thisObj; }
	virtual TESObjectREFR* ContainingObj() { return m_container; }

	bool SetArgs(ParamSize_t numArgs, ...);
	bool vSetArgs(ParamSize_t numArgs, va_list args);
	bool SetArgsRaw(ParamSize_t numArgs, const void* args);

protected:
	UInt8				m_callerVersion;
	ParamSize_t			m_numArgs;
	Script*				m_script;
	void*				m_args[kMaxUdfParams];
	TESObjectREFR*		m_thisObj;
	TESObjectREFR*		m_container;
	bool				m_allowSurplusDispatchArgs;

	virtual bool ValidateParam(UserFunctionParam* param, ParamSize_t paramIndex) { return param != nullptr; }
};

// Unlike InternalFunctionCaller, expects numeric args to always be passed as floats.
class InternalFunctionCallerAlt : public InternalFunctionCaller
{
public:
	InternalFunctionCallerAlt(Script* script, TESObjectREFR* callingObj = nullptr, TESObjectREFR* container = nullptr, bool allowSurplusDispatchArgs = false)
		: InternalFunctionCaller(script, callingObj, container, allowSurplusDispatchArgs) { }

	~InternalFunctionCallerAlt() override = default;

	bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) override;

	bool SetArgs(ParamSize_t numArgs, ...) = delete;
	bool vSetArgs(ParamSize_t numArgs, va_list args) = delete;
	//can only use SetArgsRaw, since otherwise it's easier to trip up with va_args and pass an int.
};

template <typename T>
concept BaseOfArrayElement = std::is_base_of_v<ArrayElement, T>;

template <BaseOfArrayElement T>
class ArrayElementArgFunctionCaller : public FunctionCaller
{
public:
	ArrayElementArgFunctionCaller(Script* script, const std::vector<T>& args, TESObjectREFR* callingObj = nullptr, TESObjectREFR* container = nullptr)
		: m_script(script), m_thisObj(callingObj), m_container(container), m_args(&args) {}
	ArrayElementArgFunctionCaller(Script* script, TESObjectREFR* callingObj = nullptr, TESObjectREFR* container = nullptr)
		: m_script(script), m_thisObj(callingObj), m_container(container) {}

	UInt8 ReadCallerVersion() override { return UserFunctionManager::kVersion; }
	Script* ReadScript() override { return m_script; }
	TESObjectREFR* ThisObj() override { return m_thisObj; }
	TESObjectREFR* ContainingObj() override { return m_container; }

	bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) override;
	void SetArgs(const std::vector<T>& args);
protected:
	Script* m_script;
	TESObjectREFR* m_thisObj;
	TESObjectREFR* m_container;
	const std::vector<T>* m_args{};
};

namespace PluginAPI {
	bool BasicTokenToPluginElem(const ScriptToken* tok, NVSEArrayVarInterface::Element& outElem, Script* fnScript = nullptr);

	bool CallFunctionScript(Script* fnScript, TESObjectREFR* callingObj, TESObjectREFR* container,
		NVSEArrayVarInterface::Element* result, UInt8 numArgs, ...);

	bool CallFunctionScriptAlt(Script* fnScript, TESObjectREFR* callingObj, UInt8 numArgs, ...);
}

#endif