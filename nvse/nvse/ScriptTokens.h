#pragma once

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

struct Operator;
struct SliceToken;
struct ArrayElementToken;
struct ForEachContext;
class ExpressionEvaluator;
struct ScriptToken;

enum OperatorType : UInt8
{
	kOpType_Min		= 0,

	kOpType_Assignment	= 0,
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
	kOpType_In,				// '<-'
	kOpType_ToString,		// '$'
	kOpType_PlusEquals,
	kOpType_TimesEquals,
	kOpType_DividedEquals,
	kOpType_ExponentEquals,
	kOpType_MinusEquals,
	kOpType_ToNumber,		// '#'
	kOpType_Dereference,	// unary '*'
	kOpType_MemberAccess,	// stringmap->string, shortcut for stringmap["string"]
	kOpType_MakePair,		// 'a::b', e.g. for defining key-value pairs for map structures
	kOpType_Box,			// unary; wraps a value in a single-element array

	kOpType_LeftBrace,
	kOpType_RightBrace,

	kOpType_Max
};

enum Token_Type : UInt8
{
	kTokenType_Number	= 0,
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
	kTokenType_Short,		// 2 bytes
	kTokenType_Int,			// 4 bytes

	kTokenType_Pair,
	kTokenType_AssignableString,

	kTokenType_Invalid,
	kTokenType_Max = kTokenType_Invalid,

	// sigil value, returned when an empty expression is parsed
	kTokenType_Empty = kTokenType_Max + 1,
};

struct Slice		// a range used for indexing into a string or array, expressed as arr[a:b]
{
	bool			bIsString;
	double			m_upper;
	double			m_lower;
	std::string		m_lowerStr;
	std::string		m_upperStr;

	Slice(const Slice* _slice);
	Slice(const std::string& l, const std::string& u) : bIsString(true), m_lowerStr(l), m_upperStr(u), m_lower(0.0), m_upper(0.0) { }
	Slice(double l, double u) : bIsString(false), m_lower(l), m_upper(u) { }
	Slice(const char* l, const char* u) : bIsString(true), m_lowerStr(l), m_upperStr(u), m_lower(0.0), m_upper(0.0) { }
	void GetArrayBounds(ArrayKey& lo, ArrayKey& hi) const;
};

struct TokenPair	// a pair of tokens, specified as 'a::b'
{
	ScriptToken	* left;
	ScriptToken * right;

	TokenPair(ScriptToken* l, ScriptToken* r);
	~TokenPair();
};

#if RUNTIME

struct ForEachContext
{
	UInt32				sourceID;
	UInt32				iteratorID;
	UInt32				variableType;
	ScriptEventList::Var * var;

	ForEachContext(UInt32 src, UInt32 iter, UInt32 varType, ScriptEventList::Var* _var) : sourceID(src), iteratorID(iter), variableType(varType), var(_var) { }
};

#endif

// slightly less ugly but still cheap polymorphism
struct ScriptToken
{
protected:
	Token_Type	type;
	UInt8		variableType;
	UInt16		refIdx;

	struct Value {
		std::string					str;
		union {
			Script::RefVariable		* refVar;
			UInt32					formID;
			double					num;
			TESGlobal				* global;
			Operator				* op;
#if RUNTIME		// run-time only
			ArrayID					arrID;
			ScriptEventList::Var	* var;
#endif
			// compile-time only
			VariableInfo			* varInfo;
			CommandInfo				* cmd;
			ScriptToken				* token;
		};
	} value;

	ScriptToken();
	ScriptToken(Token_Type _type, UInt8 _varType, UInt16 _refIdx);
	ScriptToken(bool boolean);
	ScriptToken(double num);
	ScriptToken(Script::RefVariable* refVar, UInt16 refIdx);
	ScriptToken(VariableInfo* varInfo, UInt16 refIdx, UInt32 varType);
	ScriptToken(CommandInfo* cmdInfo, UInt16 refIdx);
	ScriptToken(const std::string& str);
	ScriptToken(const char* str);
	ScriptToken(TESGlobal* global, UInt16 refIdx);
	ScriptToken(Operator* op);
	ScriptToken(UInt32 data, Token_Type asType);		// ArrayID or FormID

	ScriptToken(const ScriptToken& rhs);	// unimplemented, don't want copy constructor called
#if RUNTIME
	ScriptToken(ScriptEventList::Var* var);
#endif

	Token_Type	ReadFrom(ExpressionEvaluator* context);	// reconstitute param from compiled data, return the type
public:
	virtual	~ScriptToken();

	virtual const char	*			GetString() const;
	virtual UInt32					GetFormID() const;
	virtual TESForm*				GetTESForm() const;
	virtual double					GetNumber() const;
	virtual const ArrayKey *		GetArrayKey() const { return NULL; }
	virtual const ForEachContext *	GetForEachContext() const { return NULL; }
	virtual const Slice *			GetSlice() const { return NULL; }
	virtual bool					GetBool() const;
#if RUNTIME
	virtual ArrayID					GetArray() const;
	ScriptEventList::Var *	GetVar() const;
#endif
	virtual bool			CanConvertTo(Token_Type to) const;	// behavior varies b/w compile/run-time for ambiguous types
	virtual ArrayID			GetOwningArrayID() const { return 0; }
	virtual const ScriptToken  *  GetToken() const { return NULL; }
	virtual const TokenPair	* GetPair() const { return NULL; }

	ScriptToken	*			ToBasicToken() const;		// return clone as one of string, number, array, form
	
	TESGlobal *				GetGlobal() const;
	Operator *				GetOperator() const;
	VariableInfo *			GetVarInfo() const;
	CommandInfo *			GetCommandInfo() const;
	Script::RefVariable*	GetRefVariable() const;
	UInt16					GetRefIndex() const { return IsGood() ? refIdx : 0; }
	UInt8					GetVariableType() const { return IsVariable() ? variableType : Script::eVarType_Invalid; }

	UInt32					GetActorValue() const;		// kActorVal_XXX or kActorVal_NoActorValue if none
	char					GetAxis() const;			// 'X', 'Y', 'Z', or otherwise -1
	UInt32					GetSex() const;				// 0=male, 1=female, otherwise -1
	UInt32					GetAnimGroup() const;		// TESAnimGroup::kAnimGroup_XXX (kAnimGroup_Max if none)
	EffectSetting *			GetEffectSetting() const;	// from string, effect code, or TESForm*

	bool					Write(ScriptLineBuffer* buf);
	Token_Type				Type() const		{ return type; }

	bool					IsGood() const		{ return type != kTokenType_Invalid;	}
	bool					IsVariable() const	{ return type >= kTokenType_NumericVar && type <= kTokenType_ArrayVar; }

	double					GetNumericRepresentation(bool bFromHex);	// attempts to convert string to number
	char*					DebugPrint(void);

	static ScriptToken* Read(ExpressionEvaluator* context);

	static ScriptToken* Create(bool boolean)													{ return new ScriptToken(boolean); }
	static ScriptToken* Create(double num)														{ return new ScriptToken(num);	}
	static ScriptToken* Create(Script::RefVariable* refVar, UInt16 refIdx)						{ return refVar ? new ScriptToken(refVar, refIdx) : NULL; }
	static ScriptToken* Create(VariableInfo* varInfo, UInt16 refIdx, UInt32 varType)			{ return varInfo ? new ScriptToken(varInfo, refIdx, varType) : NULL; }
	static ScriptToken* Create(CommandInfo* cmdInfo, UInt16 refIdx)								{ return cmdInfo ? new ScriptToken(cmdInfo, refIdx) : NULL;	}
	static ScriptToken* Create(const std::string& str)											{ return new ScriptToken(str);	}
	static ScriptToken* Create(const char* str)													{ return new ScriptToken(str);	}
	static ScriptToken* Create(TESGlobal* global, UInt16 refIdx)								{ return global ? new ScriptToken(global, refIdx) : NULL; }
	static ScriptToken* Create(Operator* op)													{ return op ? new ScriptToken(op) : NULL;	}
	static ScriptToken* Create(TESForm* form)													{ return new ScriptToken(form ? form->refID : 0, kTokenType_Form); }
	static ScriptToken* CreateForm(UInt32 formID)												{ return new ScriptToken(formID, kTokenType_Form); }
	static ScriptToken* CreateArray(ArrayID arrID)												{ return new ScriptToken(arrID, kTokenType_Array); }
	static ScriptToken* Create(ForEachContext* forEach);
	static ScriptToken* Create(ArrayID arrID, ArrayKey* key);
	static ScriptToken* Create(Slice* slice);
	static ScriptToken* Create(ScriptToken* l, ScriptToken* r);
	static ScriptToken* Create(UInt32 varID, UInt32 lbound, UInt32 ubound);
	static ScriptToken* Create(ArrayElementToken* elem, UInt32 lbound, UInt32 ubound);
	static ScriptToken* Create(UInt32 bogus);	// unimplemented, to block implicit conversion to double
};

struct SliceToken : public ScriptToken
{
	Slice	slice;

	SliceToken(Slice* _slice);
	virtual const Slice* GetSlice() const { return type == kTokenType_Slice ? &slice : NULL; }
};

struct PairToken : public ScriptToken
{
	TokenPair	pair;

	PairToken(ScriptToken* l, ScriptToken* r);
	virtual const TokenPair* GetPair() const { return type == kTokenType_Pair ? &pair : NULL; }
};

#if RUNTIME

struct ArrayElementToken : public ScriptToken
{
	ArrayKey	key;

	ArrayElementToken(ArrayID arr, ArrayKey* _key);
	virtual const ArrayKey*	GetArrayKey() const { return type == kTokenType_ArrayElement ? &key : NULL; }
	virtual const char*		GetString() const;
	virtual double			GetNumber() const;
	virtual UInt32			GetFormID() const;
	virtual ArrayID			GetArray() const;
	virtual TESForm*		GetTESForm() const;
	virtual bool			GetBool() const;
	virtual bool			CanConvertTo(Token_Type to) const;
	virtual ArrayID			GetOwningArrayID() const {
		return type == kTokenType_ArrayElement ? value.arrID : 0; }
};

struct ForEachContextToken : public ScriptToken
{
	ForEachContext		context;

	ForEachContextToken(UInt32 srcID, UInt32 iterID, UInt32 varType, ScriptEventList::Var* var);
	virtual const ForEachContext* GetForEachContext() const { return Type() == kTokenType_ForEachContext ? &context : NULL; }
};

struct AssignableStringToken : public ScriptToken
{
	UInt32		lower;
	UInt32		upper;
	std::string	substring;

	AssignableStringToken(UInt32 _id, UInt32 lbound, UInt32 ubound);
	virtual const char* GetString() const { return substring.c_str(); }
	virtual bool Assign(const char* str) = 0;
};

struct AssignableStringVarToken : public AssignableStringToken
{
	AssignableStringVarToken(UInt32 _id, UInt32 lbound, UInt32 ubound);
	virtual bool Assign(const char* str);
};

struct AssignableStringArrayElementToken : public AssignableStringToken
{
	ArrayKey	key;

	AssignableStringArrayElementToken(UInt32 _id, const ArrayKey& _key, UInt32 lbound, UInt32 ubound);
	virtual ArrayID		GetArray() const { return value.arrID; }
	virtual bool Assign(const char* str);
};

#endif

typedef ScriptToken* (* Op_Eval)(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context);

struct OperationRule
{
	Token_Type	lhs;
	Token_Type	rhs;
	Token_Type	result;
	Op_Eval		eval;
	bool		bAsymmetric;	// does order matter? e.g. var := constant legal, constant := var illegal
};

struct Operator
{
	UInt8			precedence;
	char			symbol[3];
	UInt8			numOperands;
	OperatorType	type;
	UInt8			numRules;
	OperationRule	* rules;

	bool Precedes(Operator* op) {
		if (!IsRightAssociative())
			return op->precedence <= precedence;
		else
			return op->precedence < precedence;
	}
	bool IsRightAssociative()	{ return type == kOpType_Assignment || IsUnary() || (type >= kOpType_PlusEquals && type <= kOpType_MinusEquals);	}
	bool IsUnary()	{ return numOperands == 1;	}
	bool IsBinary() { return numOperands == 2;	}
	bool IsOpenBracket() { return (type == kOpType_LeftParen || type == kOpType_LeftBrace || type == kOpType_LeftBracket); }
	bool IsClosingBracket() { return (type == kOpType_RightParen || type == kOpType_RightBrace || type == kOpType_RightBracket); }
	bool IsBracket() { return (IsOpenBracket() || IsClosingBracket()); }
	char GetMatchedBracket() {
		switch (type) {
			case kOpType_LeftBracket :	return ']';
			case kOpType_RightBracket : return '[';
			case kOpType_LeftParen:		return ')';
			case kOpType_RightParen:	return '(';
			case kOpType_LeftBrace:		return '}';
			case kOpType_RightBrace:	return '{';
			default: return 0;
		}
	}
	bool ExpectsStringLiteral() { return type == kOpType_MemberAccess; }

	Token_Type GetResult(Token_Type lhs, Token_Type rhs);	// at compile-time determine type resulting from operation
	ScriptToken* Evaluate(ScriptToken* lhs, ScriptToken* rhs, ExpressionEvaluator* context);	// at run-time, operate on the operands and return result
};

bool CanConvertOperand(Token_Type from, Token_Type to);	// don't use directly at run-time, use ScriptToken::CanConvertTo() instead
