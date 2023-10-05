
#pragma once
#include <optional>
#include <unordered_set>

#include "containers.h"
#include "ThreadLocal.h"

/*
	Expressions are evaluated according to a set of rules stored in lookup tables. Each type of operand can 
	be resolved to zero or more	"lower" types as defined by ConversionRule. Operators use OperationRules based
	on the types of each operand to	determine the result type. Types can be ambiguous at compile-time (array 
	elements, command results) but always resolve to a concrete type at run-time. Rules are applied in the order 
	they are defined until the first one matching the operands is encountered. At run-time this means the routines
	which perform the operations can know that the operands are of the expected type.
*/

class ScriptLineMacro;
struct Operator;
struct ScriptEventList;
class ExpressionEvaluator;
struct UserFunctionParam;
struct FunctionInfo;
struct FunctionContext;
class FunctionCaller;
class OffsetOutOfBoundsError : public std::exception {};


#include "ScriptTokens.h"
#include <stack>
#include <utility>

#if RUNTIME
#include <cstdarg>

#if _DEBUG
extern const char* g_lastScriptName;
#endif
#endif

extern ErrOutput g_ErrOut;
extern std::unordered_map<Script*, Script*> g_lambdaParentScriptMap;
extern std::map<std::pair<Script*, std::string>, Script::VariableType> g_variableDefinitionsMap;

Script * GetLambdaParentScript(Script * scriptLambda);

// these are used in ParamInfo to specify expected Token_Type of args to commands taking NVSE expressions as args
enum {
	kNVSEParamType_Number =		(1 << kTokenType_Number) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_Boolean =	(1 << kTokenType_Boolean) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_String =		(1 << kTokenType_String) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_Form =		(1 << kTokenType_Form) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_Array =		(1 << kTokenType_Array) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_ArrayElement = 1 << (kTokenType_ArrayElement) | (1 << kTokenType_Ambiguous),
	kNVSEParamType_Slice =		1 << kTokenType_Slice,
	kNVSEParamType_Command =	1 << kTokenType_Command,
	kNVSEParamType_Variable =	1 << kTokenType_Variable,
	kNVSEParamType_NumericVar =	1 << kTokenType_NumericVar,
	kNVSEParamType_RefVar =		1 << kTokenType_RefVar,
	kNVSEParamType_StringVar =	1 << kTokenType_StringVar,
	kNVSEParamType_ArrayVar =	1 << kTokenType_ArrayVar,
	kNVSEParamType_ForEachContext = 1 << kTokenType_ForEachContext,

	kNVSEParamType_Collection = kNVSEParamType_Array | kNVSEParamType_String,
	kNVSEParamType_ArrayVarOrElement = kNVSEParamType_ArrayVar | kNVSEParamType_ArrayElement,
	kNVSEParamType_ArrayIndex = kNVSEParamType_String | kNVSEParamType_Number,
	kNVSEParamType_BasicType = kNVSEParamType_Array | kNVSEParamType_String | kNVSEParamType_Number | kNVSEParamType_Form,
	kNVSEParamType_NoTypeCheck = 0,

	kNVSEParamType_FormOrNumber = kNVSEParamType_Form | kNVSEParamType_Number,
	kNVSEParamType_StringOrNumber = kNVSEParamType_String | kNVSEParamType_Number,
	kNVSEParamType_Pair	=	1 << kTokenType_Pair,
};

#define NVSE_EXPR_MAX_ARGS 20		// max # of args we'll accept to a commmand

using ParamSize_t = UInt8;
static constexpr ParamSize_t kMaxUdfParams = 15;
static_assert(kMaxUdfParams <= NVSE_EXPR_MAX_ARGS);

// wraps a dynamic ParamInfo array
struct DynamicParamInfo
{
private:
	ParamInfo	m_paramInfo[kMaxUdfParams];
	ParamSize_t	m_numParams;

public:
	DynamicParamInfo(const std::vector<UserFunctionParam> &params);
	DynamicParamInfo() : m_numParams(0) { }

	ParamInfo* Params()	{	return m_paramInfo;	}
	[[nodiscard]] ParamSize_t NumParams() const { return m_numParams;	}
};

#if RUNTIME

template <typename T>
concept ArrayElementOrScriptToken = std::is_base_of_v<ArrayElement, T> || std::is_base_of_v<NVSEArrayVarInterface::Element, T> || std::is_base_of_v<ScriptToken, T>;

#endif

struct PluginScriptToken;

// If an ExpressionEvaluator is moved, we want to skip some logic in the destructor
// This struct avoids having to create a custom move constructor and move assignment operator
// in Expression Evaluator by having this struct define it's own move operations
struct MoveContainer
{
	bool moved = false;

	MoveContainer() = default;

	MoveContainer(const MoveContainer& other) = delete;

	MoveContainer(MoveContainer&& other) noexcept
	{
		other.moved = true;
	}

	MoveContainer& operator=(const MoveContainer& other) = delete;

	MoveContainer& operator=(MoveContainer&& other) noexcept
	{
		other.moved = true;
		return *this;
	}
};

class ExpressionEvaluator
{
	friend ScriptToken;
	enum { kMaxArgs = NVSE_EXPR_MAX_ARGS };

	enum {
		kFlag_SuppressErrorMessages	= 1 << 0,
		kFlag_ErrorOccurred			= 1 << 1,
		kFlag_StackTraceOnError		= 1 << 2,
	};
	MoveContainer moved_;
	bool m_pushedOnStack;
public:
	Bitfield<UInt32>	 m_flags;
	UInt8				* m_scriptData;
	UInt32				* m_opcodeOffsetPtr;
	double				* m_result;
	TESObjectREFR		* m_thisObj;
	TESObjectREFR		* m_containingObj;
	UInt8				* m_data;
	ScriptToken			* m_args[kMaxArgs];
	ParamInfo			* m_params;
	UInt8				m_numArgsExtracted;
	CommandReturnType	m_expectedReturnType;
	UInt16				m_baseOffset;
	ExpressionEvaluator	* m_parent;
	ThreadLocalData&	localData;
	std::vector<std::string> errorMessages;
	std::optional<CachedTokens> consoleTokens;

	ExpressionEvaluator(const ExpressionEvaluator& other) = delete;
	ExpressionEvaluator& operator=(const ExpressionEvaluator& other) = delete;

	CommandReturnType GetExpectedReturnType() { CommandReturnType type = m_expectedReturnType; m_expectedReturnType = kRetnType_Default; return type; }
	bool ParseBytecode(CachedTokens& cachedTokens);

	void PushOnStack();
	void PopFromStack() const;
	CachedTokens* GetTokens(std::optional<CachedTokens>* consoleTokensContainer);

	bool m_inline;

	static bool	Active();
	static ExpressionEvaluator& Get();

	ExpressionEvaluator(COMMAND_ARGS);
	~ExpressionEvaluator();

	// script analyzing constructor
	ExpressionEvaluator(UInt8* scriptData, Script* script, UInt32* opcodeOffsetPtr);

	Script			* script;
	ScriptEventList	* eventList;

	void						Error(const char* fmt, ...);
	void						vError(const char* fmt, va_list fmtArgs);
	[[nodiscard]] bool			HasErrors() const { return m_flags.IsSet(kFlag_ErrorOccurred); }

	// extract args compiled by ExpressionParser
	bool			ExtractArgs();
	bool			ExtractArgsV(void*, ...);
	bool			ExtractArgsV(va_list list);

	// extract args to function which normally uses Cmd_Default_Parse but has been compiled instead by ExpressionParser
	// bConvertTESForms will be true if invoked from ExtractArgs(), false if from ExtractArgsEx()
	bool			ExtractDefaultArgs(va_list varArgs, bool bConvertTESForms);

	// convert an extracted argument to type expected by ExtractArgs/Ex() and store in varArgs
	bool			ConvertDefaultArg(ScriptToken* arg, ParamInfo* info, bool bConvertTESForms, va_list& varArgs) const;

	// extract formatted string args compiled with compiler override
	bool ExtractFormatStringArgs(va_list varArgs, UInt32 fmtStringPos, char* fmtStringOut, UInt32 maxParams);

	std::unique_ptr<ScriptToken> ExecuteCommandToken(ScriptToken const* token, TESObjectREFR* stackRef);
	ScriptToken*	Evaluate();			// evaluates a single argument/token

	std::string GetLineText();
	std::string GetLineText(CachedTokens& tokens, ScriptToken* faultingToken) const;
	std::string GetVariablesText(CachedTokens& tokens) const;
	std::string GetVariablesText();

	void ResetCursor();

	[[nodiscard]] ScriptToken*	Arg(UInt32 idx) const
	{
		if (idx >= m_numArgsExtracted)
		{
			return nullptr;
		}
		return m_args[idx];
	}

	[[nodiscard]] UInt8			NumArgs() const { return m_numArgsExtracted; }
	void			SetParams(ParamInfo* newParams)	{	m_params = newParams;	}
	void			ExpectReturnType(CommandReturnType type) { m_expectedReturnType = type; }

#if RUNTIME
	template		<ArrayElementOrScriptToken T>
	void			AssignAmbiguousResult(T &result, CommandReturnType type);
#endif

	void			ToggleErrorSuppression(bool bSuppress);
	void			PrintStackTrace();

	[[nodiscard]] TESObjectREFR*	ThisObj() const { return m_thisObj; }
	[[nodiscard]] TESObjectREFR*	ContainingObj() const { return m_containingObj; }

	UInt8*&		Data() { return m_data; }
	UInt8		ReadByte();
	UInt16		Read16();
	double		ReadFloat();
	char		*ReadString(UInt32& incrData);
	SInt8		ReadSignedByte();
	SInt16		ReadSigned16();
	UInt32		Read32();
	SInt32		ReadSigned32();
	void ReadBuf(UInt32 len, UInt8* data);

	[[nodiscard]] UInt8* GetCommandOpcodePosition(UInt32* opcodeOffsetPtr) const;
	[[nodiscard]] CommandInfo* GetCommand() const;
};


#if RUNTIME

template <ArrayElementOrScriptToken T>
void ExpressionEvaluator::AssignAmbiguousResult(T &result, CommandReturnType type)
{
	switch (type)
	{
	case kRetnType_Default:
		*m_result = result.GetNumber();
		//should already expect default result.
		break;
	case kRetnType_String:
		AssignToStringVar(m_params, m_scriptData, ThisObj(), ContainingObj(), script,
			eventList, m_result, m_opcodeOffsetPtr, result.GetString());
		ExpectReturnType(kRetnType_String);
		break;
	case kRetnType_Form:
	{
		UInt32* refResult = (UInt32*)m_result;
		*refResult = result.GetFormID();
		ExpectReturnType(kRetnType_Form);
		break;
	}
	case kRetnType_Array:
		*m_result = result.GetArrayID();
		ExpectReturnType(kRetnType_Array);
		break;
	default:
		Error("Function call returned unexpected return type %d", type);
	}
}

bool BasicTokenToElem(ScriptToken* token, ArrayElement& elem);
#endif


#if RUNTIME
void* __stdcall ExpressionEvaluatorCreate(COMMAND_ARGS);
void __fastcall ExpressionEvaluatorDestroy(void *expEval);
bool __fastcall ExpressionEvaluatorExtractArgs(void *expEval);
UInt8 __fastcall ExpressionEvaluatorGetNumArgs(void *expEval);
PluginScriptToken* __fastcall ExpressionEvaluatorGetNthArg(void *expEval, UInt32 argIdx);
void __fastcall ExpressionEvaluatorSetExpectedReturnType(void* expEval, UInt8 retnType);
void __fastcall ExpressionEvaluatorAssignCommandResultFromElement(void* expEval, NVSEArrayVarInterface::Element &result);
bool __fastcall ExpressionEvaluatorExtractArgsV(void* expEval, va_list list);
void __fastcall ExpressionEvaluatorReportError(void* expEval, const char* fmt, va_list fmtArgs);
#endif


VariableInfo* CreateVariable(Script* script, ScriptBuffer* scriptBuf, const std::string& varName, Script::VariableType varType, const std::function<void(const std::string&)>& printCompileError);

enum ParamParenthResult : UInt8
{
	kParamParent_NoParam,
	kParamParent_Success,
	kParamParent_SyntaxError
};

enum class MacroType
{
	OneLineLambda, AssignmentShortHand, IfEval, MultipleVariableDeclaration
};

struct SavedScriptLine
{
	std::string curExpr;
	char *curOffset;
	UInt32 len;

	SavedScriptLine(std::string curExpr, char *gCurOffs, UInt32 len) : curExpr(std::move(curExpr)), curOffset(gCurOffs), len(len)
	{
	}
};

class ExpressionParser
{
	friend ScriptLineMacro;
	enum { kMaxArgs = NVSE_EXPR_MAX_ARGS };
	std::stack<SavedScriptLine> savedScriptLines;
	ScriptBuffer		* m_scriptBuf;
	ScriptLineBuffer	* m_lineBuf;
	UInt32				m_len;
	Token_Type			m_argTypes[kMaxArgs];
	UInt8				m_numArgsParsed;
	std::unordered_set<MacroType> appliedMacros_;
	Script* m_script;

	enum ScriptLineError {								// varargs
		kError_CantParse,
		kError_TooManyOperators,
		kError_TooManyOperands,
		kError_MismatchedBrackets,
		kError_InvalidOperands,			// string:operator
		kError_MismatchedQuotes,
		kError_InvalidDotSyntax,
		kError_CantFindVariable,		// string:varName
		kError_ExpectedStringVariable,
		kError_UnscriptedObject,		// string:objName
		kError_TooManyArgs,	
		kError_RefRequired,				// string:commandName
		kError_MissingParam,			// string:paramName, int:paramIndex
		kError_UserFuncMissingArgList,	// string:userFunctionName
		kError_ExpectedUserFunction,
		kError_UserFunctionContainsMultipleBlocks,
		kError_UserFunctionVarsMustPrecedeDefinition,
		kError_UserFunctionParamsUndefined,
		kError_ExpectedStringLiteral,
		kError_LineDataOverflow,

		kWarning_UnquotedString,		// string:unquotedString
		kWarning_FunctionPointer,


		kError_Max
	};

	static ErrOutput::Message	* s_Messages;

	[[nodiscard]] char	Peek(UInt32 idx = -1) const
	{
		if (idx == -1)	idx = m_lineBuf->lineOffset;
		return (idx < m_len) ? m_lineBuf->paramText[idx] : 0;
	}

	[[nodiscard]] UInt32&	Offset() const { return m_lineBuf->lineOffset; }
	[[nodiscard]] char* Text() const { return m_lineBuf->paramText; }
	[[nodiscard]] char* CurText() const { return Text() + Offset(); }

	void	Message(ScriptLineError errorCode, ...) const;

	void PrintCompileError(const std::string& message) const;

	Token_Type		Parse();
	void SaveScriptLine();
	void RestoreScriptLine();
	Token_Type		ParseSubExpression(UInt32 exprLen);
	[[nodiscard]] Operator *		ParseOperator(bool bExpectBinaryOperator, bool bConsumeIfFound = true) const;
	std::unique_ptr<ScriptToken>	ParseOperand(Operator* curOp = nullptr);
	std::unique_ptr<ScriptToken>	PeekOperand(UInt32& outReadLen);
	bool			HandleMacros();
	VariableInfo* CreateVariable(const std::string& varName, Script::VariableType varType) const;
	void SkipSpaces() const;
	bool			ParseFunctionCall(CommandInfo* cmdInfo) const;
	Token_Type		PopOperator(std::stack<Operator*> & ops, std::stack<Token_Type> & operands) const;
	std::unique_ptr<ScriptToken> ParseLambda();

	// Returns -1 for non-match, -2 if a quote inside brackets was somehow missing a closing quote ('"' chars).
	UInt32	MatchOpenBracket(Operator* openBracOp) const;
	[[nodiscard]] std::string GetCurToken() const;
	VariableInfo* LookupVariable(const char* varName, Script::RefVariable* refVar = nullptr) const;

public:
	ExpressionParser(ScriptBuffer* scriptBuf, ScriptLineBuffer* lineBuf);
	~ExpressionParser();

	bool			ParseArgs(ParamInfo* params, UInt32 numParams, bool bUsesNVSEParamTypes = true, bool parseWholeLine = true);
	[[nodiscard]] bool			ValidateArgType(ParamType paramType, Token_Type argType, bool bIsNVSEParam) const;
	bool GetUserFunctionParams(const std::vector<std::string>& paramNames, std::vector<UserFunctionParam>& outParams,
	                           Script::VarInfoList* varList, const std::string& fullScriptText, Script* script) const;
	bool ParseUserFunctionParameters(std::vector<UserFunctionParam>& out, const std::string& funcScriptText,
	                                 Script::VarInfoList* funcScriptVars, Script* script) const;
	bool ParseUserFunctionCall();
	bool ParseUserFunctionDefinition() const;
	std::unique_ptr<ScriptToken>	ParseOperand(bool (* pred)(ScriptToken* operand));
	[[nodiscard]] Token_Type		ArgType(UInt32 idx) const { return idx < kMaxArgs ? m_argTypes[idx] : kTokenType_Invalid; }
	Token_Type ParseArgument(UInt32 argsEndPos);
	ParamParenthResult ParseParentheses(ParamInfo* paramInfo, UInt32 paramIndex);
};

#if RUNTIME
// If eval is null, will call ShowRuntimeError().
// Otherwise reports the error via eval.Error().
// Less efficient, but easier to write.
void ShowRuntimeScriptError(Script* script, ExpressionEvaluator* eval, const char* fmt, ...);
#endif

bool PrecompileScript(ScriptBuffer* buf);

// NVSE analogue for Cmd_Default_Parse, accepts expressions as args
bool Cmd_Expression_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf);

extern Operator s_operators[];

class ScriptLineMacro
{
	using ModifyFunction = std::function<bool(std::string&, ScriptBuffer*, ScriptLineBuffer*)>;
	ModifyFunction  modifyFunction_;
public:
	MacroType type;
	explicit ScriptLineMacro(ModifyFunction modifyFunction, MacroType type);
	enum class MacroResult
	{
		Error, Skipped, Applied 
	};
	
	MacroResult EvalMacro(ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf, ExpressionParser* parser = nullptr) const;
};

#if _DEBUG && RUNTIME
extern thread_local std::string g_curLineText;

template <typename T>
class MemoryLeakDebugCollector
{
public:
	struct Info
	{
		T* ptr;
		std::vector<void*> callstack;
		std::string scriptLine;

		Info(T* ptr, const std::vector<void*>& callstack, const std::string& scriptLine)
			: ptr(ptr),
			  callstack(callstack),
			  scriptLine(scriptLine)
		{
		}
	};
	std::list<Info> infos;

	void Add(T* t)
	{
		std::vector<void*> vecTrace(12);
		CaptureStackBackTrace(1, 12, reinterpret_cast<PVOID*>(vecTrace.data()), nullptr);
		std::string scriptLine;
		infos.emplace_back(t, vecTrace, g_curLineText);
	}

	void Remove(T* t)
	{
		infos.remove_if([&](auto& info) {return info.ptr == t; });
	}
};

#endif

Script::VariableType GetSavedVarType(Script* script, const std::string& name);

void SaveVarType(Script* script, const std::string& name, Script::VariableType type);
