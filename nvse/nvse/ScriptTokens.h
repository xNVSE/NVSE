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
	ScriptToken *left;
	ScriptToken *right;

	TokenPair(ScriptToken *l, ScriptToken *r);
	~TokenPair();
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

	virtual const char *GetString() const;
	std::size_t GetStringLength() const;
	virtual UInt32 GetFormID() const;
	virtual TESForm *GetTESForm() const;
	virtual double GetNumber() const;
	virtual const ArrayKey *GetArrayKey() const { return NULL; }
	virtual const ForEachContext *GetForEachContext() const { return NULL; }
	virtual const Slice *GetSlice() const { return NULL; }
	virtual bool GetBool() const;
#if RUNTIME
	Token_Type ReadFrom(ExpressionEvaluator *context); // reconstitute param from compiled data, return the type
	virtual ArrayID GetArrayID() const;
	ArrayVar *GetArrayVar();
	ScriptLocal *GetVar() const;
	StringVar* GetStringVar() const;
	bool ResolveVariable();
	Script* GetUserFunction();
	ScriptParsing::CommandCallToken GetCallToken(Script* script) const;
#endif
	virtual bool CanConvertTo(Token_Type to) const; // behavior varies b/w compile/run-time for ambiguous types
	virtual ArrayID GetOwningArrayID() const { return 0; }
	virtual const ScriptToken *GetToken() const { return NULL; }
	virtual const TokenPair *GetPair() const { return NULL; }

	ScriptToken *ToBasicToken(); // return clone as one of string, number, array, form

	TESGlobal *GetGlobal() const;
	Operator *GetOperator() const;
	VariableInfo *GetVarInfo() const;
	CommandInfo *GetCommandInfo() const;
	UInt16 GetRefIndex() const { return IsGood() ? refIdx : 0; }
	UInt8 GetVariableType() const { return IsVariable() ? variableType : Script::eVarType_Invalid; }
	std::string GetStringRepresentation();

	UInt32 GetActorValue(); // kActorVal_XXX or kActorVal_NoActorValue if none
	UInt32 GetAnimationGroup();
	char GetAxis();			// 'X', 'Y', 'Z', or otherwise -1
	UInt32 GetSex();		// 0=male, 1=female, otherwise -1

	bool Write(ScriptLineBuffer *buf) const;
	Token_Type Type() const { return type; }

	bool IsGood() const { return type != kTokenType_Invalid; }
	bool IsVariable() const { return type >= kTokenType_NumericVar && type <= kTokenType_ArrayVar; }

	double GetNumericRepresentation(bool bFromHex); // attempts to convert string to number
	char *DebugPrint() const;
	bool IsInvalid() const;
	bool IsOperator() const;
	bool IsLogicalOperator() const;
	std::string GetVariableDataAsString();
	const char *GetVariableTypeString() const;
	CommandReturnType GetReturnType() const;
	void AssignResult(ExpressionEvaluator& eval) const;

	static ScriptToken *Read(ExpressionEvaluator *context);

	// block implicit conversations
	template <typename T>
	static ScriptToken* Create(T value) = delete;
	
	static ScriptToken *Create(bool boolean) { return new ScriptToken(boolean); }
	static ScriptToken *Create(double num) { return new ScriptToken(num); }
	static ScriptToken *Create(Script::RefVariable *refVar, UInt16 refIdx) { return refVar ? new ScriptToken(refVar, refIdx) : NULL; }
	static ScriptToken *Create(VariableInfo *varInfo, UInt16 refIdx, UInt32 varType) { return varInfo ? new ScriptToken(varInfo, refIdx, varType) : NULL; }
	static ScriptToken *Create(CommandInfo *cmdInfo, UInt16 refIdx) { return cmdInfo ? new ScriptToken(cmdInfo, refIdx) : NULL; }
	static ScriptToken *Create(const std::string &str) { return new ScriptToken(str); }
	static ScriptToken *Create(const char *str) { return new ScriptToken(str); }
	static ScriptToken *Create(TESGlobal *global, UInt16 refIdx) { return global ? new ScriptToken(global, refIdx) : NULL; }
	static ScriptToken *Create(Operator *op) { return op ? new ScriptToken(op) : NULL; }
	static ScriptToken *Create(TESForm *form) { return new ScriptToken(form ? form->refID : 0, kTokenType_Form); }
	static ScriptToken *CreateForm(UInt32 formID) { return new ScriptToken(formID, kTokenType_Form); }
	static ScriptToken *CreateArray(ArrayID arrID) { return new ScriptToken(arrID, kTokenType_Array); }
	static ScriptToken *Create(ForEachContext *forEach);
	static ScriptToken *Create(ArrayID arrID, ArrayKey *key);
	static ScriptToken *Create(Slice *slice);
	static ScriptToken *Create(ScriptToken *l, ScriptToken *r);
	static ScriptToken *Create(UInt32 varID, UInt32 lbound, UInt32 ubound);
	static ScriptToken *Create(ArrayElementToken *elem, UInt32 lbound, UInt32 ubound);
	static ScriptToken *Create(UInt32 bogus); // unimplemented, to block implicit conversion to double
	static ScriptToken *Create(Script *scriptLambda) { return scriptLambda ? new ScriptToken(scriptLambda) : nullptr; }
#if RUNTIME
	static ScriptToken* Create(ScriptLocal* local, StringVar* stringVar) { return stringVar ? new ScriptToken(local, stringVar) : nullptr; }
#endif
	void SetString(const char *srcStr);
	std::string GetVariableName(Script* script) const;

	bool useRefFromStack = false; // when eval'ing commands, don't use refIdx but top of stack which is reference
	UInt16 refIdx;
#if RUNTIME
	void *operator new(size_t size);
	void *operator new(size_t size, bool useMemoryPool);
	void operator delete(ScriptToken *token, std::destroying_delete_t);
	void operator delete(void *p, bool useMemoryPool);
	void operator delete(void *p); // unimplemented: keeping this here to shut up the compiler warning about non matching delete

	ScriptToken(const ScriptToken& other) = delete;

	ScriptToken(ScriptToken&& other) noexcept;

	ScriptToken& operator=(const ScriptToken& other) = delete;

	ScriptToken& operator=(ScriptToken&& other) noexcept;

	bool cached = false;
	CommandReturnType returnType = kRetnType_Default;
	UInt32 cmdOpcodeOffset;
	ExpressionEvaluator *context = nullptr;
	UInt16 varIdx;
	OperatorType shortCircuitParentType = kOpType_Max;
	UInt8 shortCircuitDistance = 0;
	UInt8 shortCircuitStackOffset = 0;
	bool formOrNumber = false;

#if _DEBUG
	std::string varName;
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
	virtual const Slice *GetSlice() const { return type == kTokenType_Slice ? &slice : NULL; }

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
	virtual const TokenPair *GetPair() const { return type == kTokenType_Pair ? &pair : NULL; }
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
	const ArrayKey *GetArrayKey() const override { return type == kTokenType_ArrayElement ? &key : NULL; }
	const char *GetString() const override;
	double GetNumber() const override;
	UInt32 GetFormID() const override;
	ArrayID GetArrayID() const override;
	TESForm *GetTESForm() const override;
	bool GetBool() const override;
	bool CanConvertTo(Token_Type to) const override;
	ArrayID GetOwningArrayID() const override { return type == kTokenType_ArrayElement ? value.arrID : 0; }
	ArrayVar *GetOwningArrayVar() const { return g_ArrayMap.Get(GetOwningArrayID()); }
	ArrayElement *GetElement() const
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
	virtual const ForEachContext *GetForEachContext() const { return Type() == kTokenType_ForEachContext ? &context : NULL; }
	void *operator new(size_t size);

	void operator delete(void *p);
};

struct AssignableSubstringToken : ScriptToken
{
	UInt32 lower;
	UInt32 upper;
	std::string substring;

	AssignableSubstringToken(UInt32 _id, UInt32 lbound, UInt32 ubound);
	const char *GetString() const override { return substring.c_str(); }
	virtual bool Assign(const char *str) = 0;

	void *operator new(size_t size)
	{
		return ::operator new(size);
	}

	void operator delete(void *p)
	{
		::operator delete(p);
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
	ArrayID GetArrayID() const override { return value.arrID; }
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

#endif

typedef ScriptToken *(*Op_Eval)(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context);

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

	bool Precedes(Operator *op)
	{
		if (!IsRightAssociative())
			return op->precedence <= precedence;
		else
			return op->precedence < precedence;
	}
	bool IsRightAssociative() { return type == kOpType_Assignment || IsUnary() || (type >= kOpType_PlusEquals && type <= kOpType_MinusEquals); }
	bool IsUnary() { return numOperands == 1; }
	bool IsBinary() { return numOperands == 2; }
	bool IsOpenBracket() { return (type == kOpType_LeftParen || type == kOpType_LeftBrace || type == kOpType_LeftBracket); }
	bool IsClosingBracket() { return (type == kOpType_RightParen || type == kOpType_RightBrace || type == kOpType_RightBracket); }
	bool IsBracket() { return (IsOpenBracket() || IsClosingBracket()); }
	char GetMatchedBracket()
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
	bool ExpectsStringLiteral() { return type == kOpType_MemberAccess; }

	Token_Type GetResult(Token_Type lhs, Token_Type rhs); // at compile-time determine type resulting from operation
#if !DISABLE_CACHING
	ScriptToken *Evaluate(ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context, Op_Eval &cacheEval, bool &cacheSwapOrder); // at run-time, operate on the operands and return result
#else
	ScriptToken *Evaluate(ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context); // at run-time, operate on the operands and return result
#endif
};

bool CanConvertOperand(Token_Type from, Token_Type to); // don't use directly at run-time, use ScriptToken::CanConvertTo() instead
#if RUNTIME
void AddToGarbageCollection(ScriptEventList *eventList, ScriptLocal *var, NVSEVarType type);
#endif