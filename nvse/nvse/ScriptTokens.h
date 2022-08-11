#pragma once
#include "LambdaManager.h"
#include "ScriptAnalyzer.h"
#include "SmallObjectsAllocator.h"

#if _DEBUG
#define DBG_EXPR_LEAKS 1
extern SInt32 TOKEN_COUNT;
extern SInt32 EXPECTED_TOKEN_COUNT;
extern SInt32 FUNCTION_CONTEXT_COUNT;
#endif

#include "GameScript.h"
#include "CommandTable.h"
#include "GameForms.h"
#include "ArrayVar.h"

#if RUNTIME
#include "StringVar.h"
#include "GameAPI.h"
#endif
#include <variant>

enum class NVSEVarType
{
	kVarType_Null = 0,
	kVarType_Array,
	kVarType_String
};

#if RUNTIME
#if _DEBUG
extern Map<ScriptEventList *, Map<ScriptLocal *, NVSEVarType>> g_nvseVarGarbageCollectionMap;
#else
extern UnorderedMap<ScriptEventList *, UnorderedMap<ScriptLocal *, NVSEVarType>> g_nvseVarGarbageCollectionMap;
#endif

extern ICriticalSection g_gcCriticalSection;
#endif

struct Operator;
struct SliceToken;
struct ArrayElementToken;
struct ForEachContext;
class ExpressionEvaluator;
struct ScriptToken;

enum OperatorType : UInt8
{
	kOpType_Min = 0,

	kOpType_Assignment = 0,
	kOpType_LogicalOr,
	kOpType_LogicalAnd,
	kOpType_Slice,
	kOpType_Equals,
	kOpType_NotEqual,
	kOpType_GreaterThan,
	kOpType_LessThan,
	kOpType_GreaterOrEqual,
	kOpType_LessOrEqual,
	kOpType_BitwiseOr,
	kOpType_BitwiseAnd,
	kOpType_LeftShift,
	kOpType_RightShift,
	kOpType_Add,
	kOpType_Subtract,
	kOpType_Multiply,
	kOpType_Divide,
	kOpType_Modulo,
	kOpType_Exponent,
	kOpType_Negation,
	kOpType_LogicalNot,
	kOpType_LeftParen,
	kOpType_RightParen,
	kOpType_LeftBracket,
	kOpType_RightBracket,
	kOpType_In,		  // '<-'
	kOpType_ToString, // '$'
	kOpType_PlusEquals,
	kOpType_TimesEquals,
	kOpType_DividedEquals,
	kOpType_ExponentEquals,
	kOpType_MinusEquals,
	kOpType_ToNumber,	  // '#'
	kOpType_Dereference,  // unary '*'
	kOpType_MemberAccess, // stringmap->string, shortcut for stringmap["string"]
	kOpType_MakePair,	  // 'a::b', e.g. for defining key-value pairs for map structures
	kOpType_Box,		  // unary; wraps a value in a single-element array

	kOpType_LeftBrace,
	kOpType_RightBrace,

	kOpType_Dot, // added in xNVSE 6.1 - allow for quest.var.command
	kOpType_BitwiseOrEquals,
	kOpType_BitwiseAndEquals,
	kOpType_ModuloEquals,

	kOpType_Max
};

enum Token_Type : UInt8
{
	kTokenType_Number = 0,
	kTokenType_Boolean,
	kTokenType_String,
	kTokenType_Form,
	kTokenType_Ref,
	kTokenType_Global,
	kTokenType_Array,
	kTokenType_ArrayElement,
	kTokenType_Slice,
	kTokenType_Command,
	kTokenType_Variable,
	kTokenType_NumericVar,
	kTokenType_RefVar,
	kTokenType_StringVar,
	kTokenType_ArrayVar,
	kTokenType_Ambiguous,
	kTokenType_Operator,
	kTokenType_ForEachContext,

	// numeric literals can optionally be encoded as one of the following
	// all are converted to _Number on evaluation
	kTokenType_Byte,
	kTokenType_Short, // 2 bytes
	kTokenType_Int,	  // 4 bytes

	kTokenType_Pair,
	kTokenType_AssignableString,
	// xNVSE 6.1.0
	kTokenType_Lambda,
	kTokenType_LambdaScriptData,

	// For operators to be able to work "dynamically" and inherit the return type of one of the tokens (ex: 1 && "str" returns String with RightToken)
	kTokenType_LeftToken,
	kTokenType_RightToken,

	kTokenType_Invalid,
	kTokenType_Max = kTokenType_Invalid,

	// sigil value, returned when an empty expression is parsed
	kTokenType_Empty = kTokenType_Max + 1,
};

const char *TokenTypeToString(Token_Type type);

struct Slice // a range used for indexing into a string or array, expressed as arr[a:b]
{
	bool bIsString;
	double m_upper;
	double m_lower;
	std::string m_lowerStr;
	std::string m_upperStr;

	Slice(const Slice *_slice);
	Slice(const std::string &l, const std::string &u) : bIsString(true), m_lowerStr(l), m_upperStr(u), m_lower(0.0), m_upper(0.0) {}
	Slice(double l, double u) : bIsString(false), m_lower(l), m_upper(u) {}
	Slice(const char *l, const char *u) : bIsString(true), m_lowerStr(l), m_upperStr(u), m_lower(0.0), m_upper(0.0) {}
	void GetArrayBounds(ArrayKey &lo, ArrayKey &hi) const;
};

struct TokenPair // a pair of tokens, specified as 'a::b'
{
	std::unique_ptr<ScriptToken> left;
	std::unique_ptr<ScriptToken> right;

	TokenPair() = default;
	TokenPair(ScriptToken *l, ScriptToken *r);
	~TokenPair() = default;
};

#if RUNTIME

struct ForEachContext
{
	UInt32 sourceID;
	UInt32 iteratorID;
	UInt32 variableType;
	ScriptLocal *var;

	ForEachContext(UInt32 src, UInt32 iter, UInt32 varType, ScriptLocal *_var) : sourceID(src), iteratorID(iter), variableType(varType), var(_var) {}
};

#endif

#if RUNTIME
struct CustomVariableContext
{
	ScriptLocal* scriptLocal;
	union
	{
		StringVar* stringVar;
		ArrayVar* arrayVar;
	};
};
#endif

// slightly less ugly but still cheap polymorphism
struct ScriptToken
{
	friend ExpressionEvaluator;

	Token_Type type;
	UInt8 variableType;

	union Value
	{
		Script::RefVariable *refVar;
		UInt32 formID;
		double num;
		char *str;
		TESGlobal *global;
		Operator *op;
#if RUNTIME // run-time only
		ArrayID arrID;
		ScriptLocal *var;
		LambdaManager::ScriptData lambdaScriptData;
		CustomVariableContext nvseVariable;
#endif
		// compile-time only
		VariableInfo *varInfo;
		CommandInfo *cmd;
		ScriptToken *token;
		Script *lambda;
	} value;

	

	ScriptToken(Token_Type _type, UInt8 _varType, UInt16 _refIdx);
	ScriptToken(bool boolean);
	ScriptToken(double num);
	ScriptToken(Script::RefVariable *refVar, UInt16 refIdx);
	ScriptToken(VariableInfo *varInfo, UInt16 refIdx, UInt32 varType);
	ScriptToken(CommandInfo *cmdInfo, UInt16 refIdx);
	ScriptToken(const std::string &str);
	ScriptToken(const char *str);
	ScriptToken(TESGlobal *global, UInt16 refIdx);
	ScriptToken(Operator *op);
	ScriptToken(UInt32 data, Token_Type asType); // ArrayID or FormID
	ScriptToken(Script *script);
#if RUNTIME
	ScriptToken(ScriptLocal* scriptLocal, StringVar* stringVar);
	ScriptToken(ScriptLocal *var);
	ScriptToken(NVSEArrayVarInterface::Element &elem);
#endif

	ScriptToken();
	ScriptToken(ExpressionEvaluator &evaluator);

	virtual ~ScriptToken();

	[[nodiscard]] virtual const char *GetString() const;
	[[nodiscard]] std::size_t GetStringLength() const;
	[[nodiscard]] virtual UInt32 GetFormID() const;
	[[nodiscard]] virtual TESForm *GetTESForm() const;
	[[nodiscard]] virtual double GetNumber() const;
	[[nodiscard]] virtual const ArrayKey *GetArrayKey() const { return nullptr; }
	[[nodiscard]] virtual const ForEachContext *GetForEachContext() const { return nullptr; }
	[[nodiscard]] virtual const Slice *GetSlice() const { return nullptr; }
	[[nodiscard]] virtual bool GetBool() const;
	[[nodiscard]] void* GetAsVoidArg() const;
	[[nodiscard]] std::pair<void*, Script::VariableType> GetAsVoidArgAndVarType() const;
#if RUNTIME
	Token_Type ReadFrom(ExpressionEvaluator *context); // reconstitute param from compiled data, return the type
	[[nodiscard]] virtual ArrayID GetArrayID() const;
	[[nodiscard]] ArrayVar *GetArrayVar() const;
	[[nodiscard]] ScriptLocal *GetVar() const;
	[[nodiscard]] StringVar* GetStringVar() const;
	bool ResolveVariable();
	[[nodiscard]] Script* GetUserFunction() const;
	ScriptParsing::CommandCallToken GetCallToken(Script* script) const;
#endif
	[[nodiscard]] virtual bool CanConvertTo(Token_Type to) const; // behavior varies b/w compile/run-time for ambiguous types
	[[nodiscard]] virtual ArrayID GetOwningArrayID() const { return 0; }
	[[nodiscard]] virtual const ScriptToken *GetToken() const { return nullptr; }
	[[nodiscard]] virtual const TokenPair *GetPair() const { return nullptr; }

	// return clone as one of string, number, array, form
	[[nodiscard]] std::unique_ptr<ScriptToken> ToBasicToken() const; 

	[[nodiscard]] TESGlobal *GetGlobal() const;
	[[nodiscard]] Operator *GetOperator() const;
	[[nodiscard]] VariableInfo *GetVarInfo() const;
	[[nodiscard]] CommandInfo *GetCommandInfo() const;
	[[nodiscard]] UInt16 GetRefIndex() const { return IsGood() ? refIdx : 0; }
	[[nodiscard]] UInt8 GetVariableType() const { return IsVariable() ? variableType : Script::eVarType_Invalid; }
	[[nodiscard]] std::string GetStringRepresentation() const;

	[[nodiscard]] UInt32 GetActorValue() const; // kActorVal_XXX or kActorVal_NoActorValue if none
	[[nodiscard]] UInt32 GetAnimationGroup() const;
	[[nodiscard]] char GetAxis() const;			// 'X', 'Y', 'Z', or otherwise -1
	[[nodiscard]] UInt32 GetSex() const;		// 0=male, 1=female, otherwise -1

	bool Write(ScriptLineBuffer *buf) const;
	[[nodiscard]] Token_Type Type() const { return type; }

	[[nodiscard]] bool IsGood() const { return type != kTokenType_Invalid; }
	[[nodiscard]] bool IsVariable() const { return type >= kTokenType_NumericVar && type <= kTokenType_ArrayVar; }

	[[nodiscard]] double GetNumericRepresentation(bool bFromHex, bool* hasErrorOut = nullptr) const; // attempts to convert string to number
	[[nodiscard]] char *DebugPrint() const;
	[[nodiscard]] bool IsInvalid() const;
	[[nodiscard]] bool IsOperator() const;
	[[nodiscard]] bool IsLogicalOperator() const;
	[[nodiscard]] std::string GetVariableDataAsString() const;
	[[nodiscard]] const char *GetVariableTypeString() const;
	[[nodiscard]] CommandReturnType GetReturnType() const;
	[[nodiscard]] Script::VariableType GetTokenTypeAsVariableType() const;

	void AssignResult(ExpressionEvaluator& eval) const;

	static ScriptToken *Read(ExpressionEvaluator *context);

	// block implicit conversations
	template <typename T>
	static ScriptToken* Create(T value) = delete;

	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(bool boolean) { return std::make_unique<ScriptToken>(boolean); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(double num) { return  std::make_unique<ScriptToken>(num); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(Script::RefVariable *refVar, UInt16 refIdx) { return refVar ? std::make_unique<ScriptToken>(refVar, refIdx) : nullptr; }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(VariableInfo *varInfo, UInt16 refIdx, UInt32 varType) { return varInfo ? std::make_unique<ScriptToken>(varInfo, refIdx, varType) : nullptr; }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(CommandInfo *cmdInfo, UInt16 refIdx) { return cmdInfo ? std::make_unique<ScriptToken>(cmdInfo, refIdx) : nullptr; }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(const std::string &str) { return std::make_unique<ScriptToken>(str); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(const char *str) { return std::make_unique<ScriptToken>(str); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(TESGlobal *global, UInt16 refIdx) { return global ? std::make_unique<ScriptToken>(global, refIdx) : nullptr; }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(Operator *op) { return op ? std::make_unique<ScriptToken>(op) : nullptr; }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(TESForm *form) { return std::make_unique<ScriptToken>(form ? form->refID : 0, kTokenType_Form); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> CreateForm(UInt32 formID) { return std::make_unique<ScriptToken>(formID, kTokenType_Form); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> CreateArray(ArrayID arrID) { return std::make_unique<ScriptToken>(arrID, kTokenType_Array); }
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(ForEachContext *forEach);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(ArrayID arrID, ArrayKey *key);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(Slice *slice);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(ScriptToken *l, ScriptToken *r);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(UInt32 varID, UInt32 lbound, UInt32 ubound);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(ArrayElementToken *elem, UInt32 lbound, UInt32 ubound);
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(UInt32 bogus); // unimplemented, to block implicit conversion to double
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(Script *scriptLambda) { return scriptLambda ? std::make_unique<ScriptToken>(scriptLambda) : nullptr; }
#if RUNTIME
	[[nodiscard]] static std::unique_ptr<ScriptToken> Create(ScriptLocal* local, StringVar* stringVar) { return stringVar ? std::make_unique<ScriptToken>(local, stringVar) : nullptr; }
#endif
	void SetString(const char *srcStr);
	[[nodiscard]] std::string GetVariableName(Script* script) const;

	bool useRefFromStack = false; // when eval'ing commands, don't use refIdx but top of stack which is reference
	UInt16 refIdx;
#if RUNTIME
	void *operator new(size_t size);
	void *operator new(size_t size, bool useMemoryPool);
	void operator delete(ScriptToken *token, std::destroying_delete_t);
	void operator delete(void *p, bool useMemoryPool);
	void operator delete(void *p); // unimplemented: keeping this here to shut up the compiler warning about non matching delete

	ScriptToken(const ScriptToken& other) = delete;
	ScriptToken& operator=(const ScriptToken& other) = delete;

	bool cached = false;
	CommandReturnType returnType = kRetnType_Default;
	UInt32 cmdOpcodeOffset;
	ExpressionEvaluator *context = nullptr;
	UInt16 varIdx;
	OperatorType shortCircuitParentType = kOpType_Max;
	UInt8 shortCircuitDistance = 0;
	UInt8 shortCircuitStackOffset = 0;
	bool formOrNumber = false;

	// prevents token from being deleted after evaluation, should only be called in Eval_* statements where it is the operation result
	[[nodiscard]] std::unique_ptr<ScriptToken> ForwardEvalResult() const;
#if _DEBUG
	std::string varName;
	ArrayVar* arrayVar{};

#endif
private:
	bool memoryPooled = true;
#endif
};
//STATIC_ASSERT(sizeof(ScriptToken) == 0x30);

struct SliceToken : ScriptToken
{
	Slice slice;

	SliceToken(Slice *_slice);
	[[nodiscard]] const Slice *GetSlice() const override { return type == kTokenType_Slice ? &slice : nullptr; }
	[[nodiscard]] bool GetBool() const override { return true; }

	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
	}
};

struct PairToken : ScriptToken
{
	TokenPair pair;

	PairToken(ScriptToken *l, ScriptToken *r);
	[[nodiscard]] const TokenPair *GetPair() const override { return type == kTokenType_Pair ? &pair : nullptr; }
	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
	}
};

#if RUNTIME

struct PluginScriptToken;
UInt8 __fastcall ScriptTokenGetType(PluginScriptToken *scrToken);
bool __fastcall ScriptTokenCanConvertTo(PluginScriptToken *scrToken, UInt8 toType);
double __fastcall ScriptTokenGetFloat(PluginScriptToken *scrToken);
bool __fastcall ScriptTokenGetBool(PluginScriptToken *scrToken);
UInt32 __fastcall ScriptTokenGetFormID(PluginScriptToken *scrToken);
TESForm *__fastcall ScriptTokenGetTESForm(PluginScriptToken *scrToken);
const char *__fastcall ScriptTokenGetString(PluginScriptToken *scrToken);
UInt32 __fastcall ScriptTokenGetArrayID(PluginScriptToken *scrToken);
UInt32 __fastcall ScriptTokenGetActorValue(PluginScriptToken *scrToken);
ScriptLocal *__fastcall ScriptTokenGetScriptVar(PluginScriptToken *scrToken);
struct PluginTokenPair;
const PluginTokenPair *__fastcall ScriptTokenGetPair(PluginScriptToken *scrToken);
struct PluginTokenSlice;
const PluginTokenSlice *__fastcall ScriptTokenGetSlice(PluginScriptToken *scrToken);
UInt32 __fastcall ScriptTokenGetAnimationGroup(PluginScriptToken* scrToken);
void __fastcall ScriptTokenGetArrayElement(PluginScriptToken* scrToken, NVSEArrayVarInterface::Element& outElem);


struct ArrayElementToken : ScriptToken
{
	ArrayKey key;

	ArrayElementToken(ArrayID arr, ArrayKey *_key);
	[[nodiscard]] const ArrayKey *GetArrayKey() const override { return type == kTokenType_ArrayElement ? &key : nullptr; }
	[[nodiscard]] const char *GetString() const override;
	[[nodiscard]] double GetNumber() const override;
	[[nodiscard]] UInt32 GetFormID() const override;
	[[nodiscard]] ArrayID GetArrayID() const override;
	[[nodiscard]] TESForm *GetTESForm() const override;
	[[nodiscard]] bool GetBool() const override;
	[[nodiscard]] bool CanConvertTo(Token_Type to) const override;
	[[nodiscard]] ArrayID GetOwningArrayID() const override { return type == kTokenType_ArrayElement ? value.arrID : 0; }
	[[nodiscard]] ArrayVar *GetOwningArrayVar() const { return g_ArrayMap.Get(GetOwningArrayID()); }

	[[nodiscard]] ArrayElement *GetElement() const
	{
		auto *arrayVar = GetOwningArrayVar();
		if (!arrayVar)
			return nullptr;
		return arrayVar->Get(GetArrayKey(), false);
	}

	void *operator new(size_t size);

	void operator delete(void *p);
};

struct ForEachContextToken : ScriptToken
{
	ForEachContext context;

	ForEachContextToken(UInt32 srcID, UInt32 iterID, UInt32 varType, ScriptLocal *var);
	[[nodiscard]] const ForEachContext *GetForEachContext() const override { return Type() == kTokenType_ForEachContext ? &context : nullptr; }
	void *operator new(size_t size);

	void operator delete(void *p);
};

struct AssignableSubstringToken : ScriptToken
{
	UInt32 lower;
	UInt32 upper;
	std::string substring;

	AssignableSubstringToken(UInt32 _id, UInt32 lbound, UInt32 ubound);
	[[nodiscard]] const char *GetString() const override { return substring.c_str(); }
	virtual bool Assign(const char *str) = 0;

	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
	}

	[[nodiscard]] bool GetBool() const override
	{
		return true;
	}
};

struct AssignableSubstringStringVarToken : public AssignableSubstringToken
{
	AssignableSubstringStringVarToken(UInt32 _id, UInt32 lbound, UInt32 ubound);
	bool Assign(const char *str) override;

	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
	}
};

struct AssignableSubstringArrayElementToken : public AssignableSubstringToken
{
	ArrayKey key;

	AssignableSubstringArrayElementToken(UInt32 _id, const ArrayKey &_key, UInt32 lbound, UInt32 ubound);
	[[nodiscard]] ArrayID GetArrayID() const override { return value.arrID; }
	bool Assign(const char* str) override;

	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
	}
};

#endif

typedef std::unique_ptr<ScriptToken> (*Op_Eval)(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context);

struct OperationRule
{
	Token_Type lhs;
	Token_Type rhs;
	Token_Type result;
	Op_Eval eval;
	bool bAsymmetric; // does order matter? e.g. var := constant legal, constant := var illegal
};

struct Operator
{
	UInt8 precedence;
	char symbol[3];
	UInt8 numOperands;
	OperatorType type;
	UInt8 numRules;
	OperationRule *rules;

	bool Precedes(Operator *op) const
	{
		if (!IsRightAssociative())
			return op->precedence <= precedence;
		else
			return op->precedence < precedence;
	}

	[[nodiscard]] bool IsRightAssociative() const { return type == kOpType_Assignment || IsUnary() || (type >= kOpType_PlusEquals && type <= kOpType_MinusEquals); }
	[[nodiscard]] bool IsUnary() const { return numOperands == 1; }
	[[nodiscard]] bool IsBinary() const { return numOperands == 2; }
	[[nodiscard]] bool IsOpenBracket() const { return (type == kOpType_LeftParen || type == kOpType_LeftBrace || type == kOpType_LeftBracket); }
	[[nodiscard]] bool IsClosingBracket() const { return (type == kOpType_RightParen || type == kOpType_RightBrace || type == kOpType_RightBracket); }
	[[nodiscard]] bool IsBracket() const { return (IsOpenBracket() || IsClosingBracket()); }

	[[nodiscard]] char GetMatchedBracket() const
	{
		switch (type)
		{
		case kOpType_LeftBracket:
			return ']';
		case kOpType_RightBracket:
			return '[';
		case kOpType_LeftParen:
			return ')';
		case kOpType_RightParen:
			return '(';
		case kOpType_LeftBrace:
			return '}';
		case kOpType_RightBrace:
			return '{';
		default:
			return 0;
		}
	}

	[[nodiscard]] bool ExpectsStringLiteral() const { return type == kOpType_MemberAccess; }

	[[nodiscard]] Token_Type GetResult(Token_Type lhs, Token_Type rhs) const; // at compile-time determine type resulting from operation
#if !DISABLE_CACHING
	std::unique_ptr<ScriptToken> Evaluate(ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context, Op_Eval &cacheEval, bool &cacheSwapOrder); // at run-time, operate on the operands and return result
#else
	ScriptToken *Evaluate(ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context); // at run-time, operate on the operands and return result
#endif
};

bool CanConvertOperand(Token_Type from, Token_Type to); // don't use directly at run-time, use ScriptToken::CanConvertTo() instead
#if RUNTIME
void AddToGarbageCollection(ScriptEventList *eventList, ScriptLocal *var, NVSEVarType type);
#endif