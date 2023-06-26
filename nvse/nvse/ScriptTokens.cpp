#include "ScriptTokens.h"

#include "GameAPI.h"
#include "ScriptUtils.h"
#include "GameRTTI.h"
#include "SmallObjectsAllocator.h"
#include "GameObjects.h"
#include "LambdaManager.h"

#ifdef DBG_EXPR_LEAKS
SInt32 TOKEN_COUNT = 0;
SInt32 EXPECTED_TOKEN_COUNT = 0;
#define INC_TOKEN_COUNT TOKEN_COUNT++;
#else
#define INC_TOKEN_COUNT
#endif

ScriptToken::~ScriptToken()
{
#ifdef DBG_EXPR_LEAKS
	TOKEN_COUNT--;
#endif
	if (type == kTokenType_String && value.str)
	{
		free(value.str);
		value.str = NULL;
	}
#if RUNTIME
	else if (type == kTokenType_StringVar && !value.var) // command result
	{
		auto& cache = GetFunctionResultCachedStringVar();
		if (cache.var == value.nvseVariable.stringVar)
			cache.inUse = false;
	}
#endif
	else if (type == kTokenType_Lambda && value.lambda)
	{
		value.lambda = nullptr;
	}
}

/*************************************

	ScriptToken constructors

*************************************/

// ScriptToken
ScriptToken::ScriptToken() : type(kTokenType_Invalid), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.num = 0;
}

ScriptToken::ScriptToken(Token_Type _type, UInt8 _varType, UInt16 _refIdx) : type(_type), variableType(_varType), refIdx(_refIdx){
																													  INC_TOKEN_COUNT}

																			 ScriptToken::ScriptToken(bool boolean) : type(kTokenType_Boolean), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.num = boolean ? 1 : 0;
}

ScriptToken::ScriptToken(double num) : type(kTokenType_Number), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.num = num;
}

ScriptToken::ScriptToken(Script::RefVariable *refVar, UInt16 refIdx) : type(kTokenType_Ref), refIdx(refIdx), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.refVar = refVar;
}

ScriptToken::ScriptToken(const std::string &str) : type(kTokenType_String), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.str = str.empty() ? NULL : CopyString(str.c_str());
}

ScriptToken::ScriptToken(const char *str) : type(kTokenType_String), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.str = (str && *str) ? CopyString(str) : NULL;
}

ScriptToken::ScriptToken(TESGlobal *global, UInt16 refIdx) : type(kTokenType_Global), refIdx(refIdx), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.global = global;
}

ScriptToken::ScriptToken(UInt32 id, Token_Type asType) : refIdx(0), type(asType), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	switch (asType)
	{
	case kTokenType_Form:
		value.formID = id;
		break;
#if RUNTIME
	case kTokenType_Array:
		value.arrID = id;
		type = (!id || g_ArrayMap.Get(id)) ? kTokenType_Array : kTokenType_Invalid;
		break;
#endif
	default:
		type = kTokenType_Invalid;
	}
}

ScriptToken::ScriptToken(Script *script) : type(kTokenType_Lambda), refIdx(0), variableType(Script::eVarType_Invalid)
{
	value.lambda = script;
}
#if RUNTIME
ScriptToken::ScriptToken(ScriptLocal* scriptLocal, StringVar* stringVar) : type(kTokenType_StringVar), variableType(Script::eVarType_String), refIdx(0), varIdx(0)
{
	value.nvseVariable = { scriptLocal, stringVar};
}
#endif

ScriptToken::ScriptToken(Operator *op) : type(kTokenType_Operator), refIdx(0), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.op = op;
}

ScriptToken::ScriptToken(ExpressionEvaluator &evaluator)
{
#if RUNTIME
	ReadFrom(&evaluator);
#endif
}

ScriptToken::ScriptToken(VariableInfo *varInfo, UInt16 refIdx, UInt32 varType) : refIdx(refIdx), variableType(varType)
{
	INC_TOKEN_COUNT
	value.varInfo = varInfo;
	switch (varType)
	{
	case Script::eVarType_Array:
		type = kTokenType_ArrayVar;
		break;
	case Script::eVarType_String:
		type = kTokenType_StringVar;
		break;
	case Script::eVarType_Integer:
	case Script::eVarType_Float:
		type = kTokenType_NumericVar;
		break;
	case Script::eVarType_Ref:
		type = kTokenType_RefVar;
		break;
	default:
#if EDITOR
		g_ErrOut.Show("Unable to resolve variable type for %s", varInfo->name.CStr());
#endif
		type = kTokenType_Invalid;
	}
}

ScriptToken::ScriptToken(CommandInfo *cmdInfo, UInt16 refIdx) : type(kTokenType_Command), refIdx(refIdx), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.cmd = cmdInfo;
}

bool ScriptToken::IsInvalid() const
{
	return this->type == kTokenType_Invalid;
}

bool ScriptToken::IsOperator() const
{
	return this->type == kTokenType_Operator;
}

bool ScriptToken::IsLogicalOperator() const
{
	if (Type() != kTokenType_Operator)
		return false;
	const auto &type = GetOperator()->type;
	return type == kOpType_LogicalOr || type == kOpType_LogicalAnd;
}

#if RUNTIME
std::string ScriptToken::GetVariableDataAsString() const
{
	switch (Type())
	{
	case kTokenType_NumericVar:
	{
		return FormatString("%g", GetNumber());
	}
	case kTokenType_RefVar:
	{
		auto *form = GetTESForm();
		if (form)
			return form->GetStringRepresentation();
		if (value.var && value.var->GetFormId())
			return FormatString("invalid form (%X)", value.var->GetFormId());
		return "uninitialized form (0)";
	}
	case kTokenType_StringVar:
	{
		return '"' + std::string(GetString()) + '"';
	}
	case kTokenType_ArrayVar:
	{
		auto *arr = GetArrayVar();
		if (arr)
		{
			if (arr->Size() <= 10)
			{
				auto str = FormatString("array keys [");
				const ArrayKey *key;
				ArrayElement *elem;
				if (arr->GetFirstElement(&elem, &key))
				{
					do
					{
						switch (arr->KeyType())
						{
						case kDataType_Numeric:
							str += FormatString("%g, ", key->key.num);
							break;
						case kDataType_String:
							str += FormatString("\"%s\", ", key->key.GetStr());
							break;
						default:
							break;
						}
					} while (arr->GetNextElement(key, &elem, &key));
					for (auto i = 0; i < 2 && !str.empty(); ++i)
						str.pop_back(); // remove ", "
				}
				str += ']';
				return str;
			}
			return FormatString("array size %d", arr->Size());
		}
		return "uninitialized array";
	}
	default:
		break;
	}
	return "";
}

const char *ScriptToken::GetVariableTypeString() const
{
	switch (this->Type())
	{
	case kTokenType_NumericVar:
		return "Numeric";
	case kTokenType_RefVar:
		return "Form";
	case kTokenType_StringVar:
		return "String";
	case kTokenType_ArrayVar:
		return "Array";
	default:
		return "Not a variable";
	}
}
#endif

#if RUNTIME
// ###TODO: Read() sets variable type; better to pass it to this constructor
ScriptToken::ScriptToken(ScriptLocal *var) : refIdx(0), type(kTokenType_Variable), variableType(Script::eVarType_Invalid)
{
	INC_TOKEN_COUNT
	value.var = var;
}

ScriptToken::ScriptToken(NVSEArrayVarInterface::Element& elem) : refIdx(0), variableType(Script::eVarType_Invalid)
{
	switch (elem.GetType())
	{
	case NVSEArrayVarInterface::Element::kType_Numeric:
		type = kTokenType_Number;
		value.num = elem.GetNumber();
		break;
	case NVSEArrayVarInterface::Element::kType_String:
	{
		type = kTokenType_String;
		auto str_ = elem.GetString();
		value.str = (str_ && *str_) ? CopyString(str_) : nullptr;
		break;
	}
	case NVSEArrayVarInterface::Element::kType_Array:
		value.arrID = reinterpret_cast<ArrayID>(elem.GetArray());
		type = (!value.arrID || g_ArrayMap.Get(value.arrID)) ? kTokenType_Array : kTokenType_Invalid;
		break;
	case NVSEArrayVarInterface::Element::kType_Form:
	{
		type = kTokenType_Form;
		auto const form = elem.GetTESForm();
		value.formID = form ? form->refID : 0;
		break;
	}
	default:
		type = kTokenType_Invalid;
		value.num = 0;
	}
}

ForEachContextToken::ForEachContextToken(UInt32 srcID, UInt32 iterID, UInt32 varType, ScriptLocal *var)
	: ScriptToken(kTokenType_ForEachContext, Script::eVarType_Invalid, 0), context(srcID, iterID, varType, var)
{
	value.formID = 0;
}

thread_local SmallObjectsAllocator::FastAllocator<ScriptToken, 32> g_scriptTokenAllocator;
thread_local SmallObjectsAllocator::FastAllocator<ArrayElementToken, 4> g_arrayElementTokenAllocator;
thread_local SmallObjectsAllocator::FastAllocator<ForEachContextToken, 16> g_forEachTokenAllocator;

void *ForEachContextToken::operator new(size_t size)
{
	return g_forEachTokenAllocator.Allocate();
}

void ForEachContextToken::operator delete(void *p)
{
	g_forEachTokenAllocator.Free(p);
}

std::unique_ptr<ScriptToken> ScriptToken::Create(ForEachContext *forEach)
{
	if (!forEach)
		return nullptr;

	if (forEach->variableType == Script::eVarType_String)
	{
		if (!g_StringMap.Get(forEach->iteratorID) || !g_StringMap.Get(forEach->sourceID))
			return nullptr;
	}
	else if (forEach->variableType == Script::eVarType_Array)
	{
		if (!g_ArrayMap.Get(forEach->sourceID) || !g_ArrayMap.Get(forEach->iteratorID))
			return nullptr;
	}
	else if (forEach->variableType == Script::eVarType_Ref)
	{
		auto const form = (TESForm *)forEach->sourceID;
		auto const target = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
		if (!target && NOT_ID(form, BGSListForm))
			return nullptr;
	}
	else
		return nullptr;

	return std::make_unique<ForEachContextToken>(forEach->sourceID, forEach->iteratorID, forEach->variableType, forEach->var);
}

std::unique_ptr<ScriptToken> ScriptToken::Create(ArrayID arrID, ArrayKey *key)
{
	if (key)
		return std::make_unique<ArrayElementToken>(arrID, key);
	return nullptr;
}

std::unique_ptr<ScriptToken> ScriptToken::Create(Slice *slice)
{
	if (slice)
		return std::make_unique<SliceToken>(slice);
	return nullptr;
}

std::unique_ptr<ScriptToken> ScriptToken::Create(ScriptToken *l, ScriptToken *r)
{
	return std::make_unique<PairToken>(l, r);
}

std::unique_ptr<ScriptToken> ScriptToken::Create(UInt32 varID, UInt32 lbound, UInt32 ubound)
{
	return std::make_unique<AssignableSubstringStringVarToken>(varID, lbound, ubound);
}

std::unique_ptr<ScriptToken> ScriptToken::Create(ArrayElementToken *elem, UInt32 lbound, UInt32 ubound)
{
	if (elem && elem->GetString())
	{
		return std::make_unique<AssignableSubstringArrayElementToken>(elem->GetOwningArrayID(), *elem->GetArrayKey(), lbound, ubound);
	}
	return nullptr;
}

void *ScriptToken::operator new(size_t size)
{
	return operator new(size, true);
}

void *ScriptToken::operator new(size_t size, const bool useMemoryPool)
{
	//return ::operator new(size);
	ScriptToken *alloc;
	if (useMemoryPool)
	{
		alloc = g_scriptTokenAllocator.Allocate();
		//alloc->memoryPooled = true;
		//alloc->cached = false;
	}
	else
	{
		alloc = static_cast<ScriptToken *>(::operator new(size));
		//alloc->memoryPooled = false;
		//alloc->cached = true;
	}
	return alloc;
}

void ScriptToken::operator delete(void *p, bool useMemoryPool)
{
	if (useMemoryPool)
		g_scriptTokenAllocator.Free(p);
	else
		::operator delete(p);
}

// C++20 destroying delete can avoid calling destructor if we don't want the object deleted
// derived classes will not call this delete (tested), they will continue using their own non-destroying operator delete overload
void ScriptToken::operator delete(ScriptToken *token, std::destroying_delete_t)
{
	if (!token || token->cached)
		return;
	token->~ScriptToken();

	if (token->memoryPooled)
		g_scriptTokenAllocator.Free(token);
	else
		::operator delete(token);
}

ArrayElementToken::ArrayElementToken(ArrayID arr, ArrayKey *_key)
	: ScriptToken(kTokenType_ArrayElement, Script::eVarType_Invalid, 0),
		key(*_key)
{
	value.arrID = arr;
}

TokenPair::TokenPair(ScriptToken *l, ScriptToken *r) : left(nullptr), right(nullptr)
{
	if (l && r)
	{
		left = l->ToBasicToken();
		if (left)
		{
			right = r->ToBasicToken();
			if (!right)
			{
				left = nullptr;
			}
		}
	}
}


PairToken::PairToken(ScriptToken *l, ScriptToken *r) : ScriptToken(kTokenType_Pair, Script::eVarType_Invalid, 0), pair(l, r)
{
	if (!pair.left || !pair.right)
		type = kTokenType_Invalid;
}

AssignableSubstringToken::AssignableSubstringToken(UInt32 _id, UInt32 lbound, UInt32 ubound)
	: ScriptToken(kTokenType_AssignableString, Script::eVarType_Invalid, 0), lower(lbound), upper(ubound), substring()
{
	value.arrID = _id;
	if (lower > upper)
	{
		lower = 0;
	}
}

AssignableSubstringStringVarToken::AssignableSubstringStringVarToken(UInt32 _id, UInt32 lbound, UInt32 ubound) : AssignableSubstringToken(_id, lbound, ubound)
{
	StringVar *strVar = g_StringMap.Get(value.arrID);
	if (strVar)
	{
		upper = (upper > strVar->GetLength()) ? strVar->GetLength() - 1 : upper;
		substring = strVar->SubString(lower, upper - lower + 1);
	}
}

AssignableSubstringArrayElementToken::AssignableSubstringArrayElementToken(UInt32 _id, const ArrayKey &_key, UInt32 lbound, UInt32 ubound)
	: AssignableSubstringToken(_id, lbound, ubound), key(_key)
{
	ArrayVar *arr = g_ArrayMap.Get(value.arrID);
	if (!arr)
		return;

	const char *pElemStr;
	if (arr->GetElementString(&key, &pElemStr))
	{
		std::string elemStr(pElemStr);
		upper = (upper > elemStr.length()) ? elemStr.length() - 1 : upper;
		substring = elemStr.substr(lbound, ubound - lbound + 1);
	}
}

bool AssignableSubstringArrayElementToken::Assign(const char* str)
{
	ArrayElement* elem = g_ArrayMap.GetElement(value.arrID, &key);
	const char* pElemStr;
	if (elem && elem->GetAsString(&pElemStr) && (lower <= upper) && (upper < StrLen(pElemStr)))
	{
		std::string elemStr(pElemStr);
		elemStr.erase(lower, upper - lower + 1);
		if (str)
		{
			elemStr.insert(lower, str);
			elem->SetString(elemStr.c_str());
			substring = elemStr;
		}
		return true;
	}
	return false;
}

bool AssignableSubstringStringVarToken::Assign(const char *str)
{
	StringVar *strVar = g_StringMap.Get(value.arrID);
	if (strVar)
	{
		UInt32 len = strVar->GetLength();
		if (lower <= upper && upper < len)
		{
			strVar->Erase(lower, upper - lower + 1);
			if (str)
			{
				strVar->Insert(str, lower);
				substring = strVar->String();
			}
			return true;
		}
	}
	return false;
}

std::unique_ptr<ScriptToken> ScriptToken::ForwardEvalResult() const
{
	if (CanConvertTo(kTokenType_Number)) [[likely]]
		return Create(GetNumber());
	return ToBasicToken();
}

#endif

SliceToken::SliceToken(Slice *_slice) : ScriptToken(kTokenType_Slice, Script::eVarType_Invalid, 0), slice(_slice)
{
	//
}

const char *TokenTypeToString(Token_Type type)
{
	switch (type)
	{
	case kTokenType_Number:
		return "Number";
	case kTokenType_Boolean:
		return "Boolean";
	case kTokenType_String:
		return "String";
	case kTokenType_Form:
		return "Form";
	case kTokenType_Ref:
		return "Ref";
	case kTokenType_Global:
		return "Global";
	case kTokenType_Array:
		return "Array";
	case kTokenType_ArrayElement:
		return "Array Element";
	case kTokenType_Slice:
		return "Slice";
	case kTokenType_Command:
		return "Command";
	case kTokenType_Variable:
		return "Variable";
	case kTokenType_NumericVar:
		return "Numeric Variable";
	case kTokenType_RefVar:
		return "Reference Variable";
	case kTokenType_StringVar:
		return "String Variable";
	case kTokenType_ArrayVar:
		return "Array Variable";
	case kTokenType_Ambiguous:
		return "Ambiguous";
	case kTokenType_Operator:
		return "Operator";
	case kTokenType_ForEachContext:
		return "ForEach Context";
	case kTokenType_Byte:
		return "Byte";
	case kTokenType_Short:
		return "Short";
	case kTokenType_Int:
		return "Int";
	case kTokenType_Pair:
		return "Pair";
	case kTokenType_AssignableString:
		return "Assignable String";
	case kTokenType_Lambda:
		return "Anonymous Function";
	case kTokenType_Invalid:
		return "Invalid";
	case kTokenType_Empty:
		return "Empty";
	default:
		return "Unknown";
	}
}

Slice::Slice(const Slice *_slice)
{
	bIsString = _slice->bIsString;
	if (bIsString)
	{
		m_upperStr = _slice->m_upperStr;
		m_lowerStr = _slice->m_lowerStr;
	}
	else
	{
		m_upper = _slice->m_upper;
		m_lower = _slice->m_lower;
	}
}

/*****************************************

	ScriptToken value getters

*****************************************/

void ScriptToken::SetString(const char *srcStr)
{
	if (type == kTokenType_String)
	{
		if (value.str)
			free(value.str);
	}
	else
		type = kTokenType_String;
	value.str = (srcStr && *srcStr) ? CopyString(srcStr) : NULL;
}

#if RUNTIME
std::string ScriptToken::GetVariableName(Script* script) const
{
	if (refIdx == 0)
	{
		auto *varInfo = script->GetVariableInfo(varIdx);
		if (varInfo)
		{
			ScriptParsing::ScriptVariableToken scriptVarToken(script, ScriptParsing::ExpressionCode::None, varInfo, nullptr);
			return scriptVarToken.ToString();
		}
	}
	else
	{
		// reference.variable
		auto *refVar = script->GetRefFromRefList(refIdx);
		if (!refVar)
		{
			return "";
		}
		auto *refr = DYNAMIC_CAST(refVar->form, TESForm, TESObjectREFR);
		if (refr)
		{
			auto *extraScript = refr->GetExtraScript();
			if (extraScript && extraScript->script)
			{
				auto *varInfo = extraScript->script->GetVariableInfo(varIdx);
				if (varInfo && refr->GetName())
				{
					ScriptParsing::ScriptVariableToken scriptVarToken(extraScript->script, ScriptParsing::ExpressionCode::None, varInfo, refVar->form);
					return scriptVarToken.ToString();
				}
			}
		}
		else
		{
			auto *quest = DYNAMIC_CAST(refVar->form, TESForm, TESQuest);
			if (quest)
			{
				auto *refScript = quest->scriptable.script;
				if (!refScript)
					return "";
				auto *varInfo = refScript->GetVariableInfo(varIdx);
				if (varInfo)
				{
					ScriptParsing::ScriptVariableToken scriptVarToken(refScript, ScriptParsing::ExpressionCode::None, varInfo, refVar->form);
					return scriptVarToken.ToString();
				}
			}
		}
	}

	return "";
}
#endif

const char *ScriptToken::GetString() const
{
	static const char *empty = "";
	const char *result = NULL;

	if (type == kTokenType_String)
		result = value.str;
#if RUNTIME
	else if (type == kTokenType_StringVar)
	{
		if (value.nvseVariable.stringVar)
			return value.nvseVariable.stringVar->GetCString();
		if (!value.var)
		{
			return "";
		}
		StringVar *strVar = g_StringMap.Get(value.var->data);
		if (strVar)
			result = strVar->GetCString();
		else
			result = NULL;
	}
#endif
	if (result)
		return result;
	return empty;
}

std::size_t ScriptToken::GetStringLength() const
{
	if (type == kTokenType_String)
		return strlen(value.str);
#if RUNTIME
	if (type == kTokenType_StringVar)
	{
		StringVar *strVar = GetStringVar();
		if (strVar)
			return strVar->GetLength();
	}
#endif
	return 0;
}

UInt32 ScriptToken::GetFormID() const
{
	if (type == kTokenType_Form)
		return value.formID;
#if RUNTIME
	if (type == kTokenType_RefVar)
	{
		if (!value.var)
		{
			return 0;
		}
		return *reinterpret_cast<UInt32 *>(&value.var->data);
	}
#endif
	if (type == kTokenType_Number)
		return value.formID;
	if (type == kTokenType_Ref && value.refVar)
		return value.refVar->form ? value.refVar->form->refID : 0;
	if (type == kTokenType_Lambda && value.lambda)
		return value.lambda->refID;
	return 0;
}

TESForm *ScriptToken::GetTESForm() const
{
	// ###TODO: handle Ref (RefVariable)? Read() turns RefVariable into Form so that type is compile-time only
#if RUNTIME
	if (type == kTokenType_Form)
		return LookupFormByID(value.formID);
	if (type == kTokenType_RefVar && value.var)
		return LookupFormByID(*reinterpret_cast<UInt32 *>(&value.var->data));
	if (type == kTokenType_Number && formOrNumber)
		return LookupFormByID(*reinterpret_cast<UInt32 *>(const_cast<double*>(&value.num)));
#endif
	if (type == kTokenType_Lambda)
		return value.lambda;
	if (type == kTokenType_Ref && value.refVar)
		return value.refVar->form;
	return NULL;
}

double ScriptToken::GetNumber() const
{
	if (type == kTokenType_Number || type == kTokenType_Boolean)
		return value.num;
	else if (type == kTokenType_Global && value.global)
		return value.global->data;
#if RUNTIME
	else if ((type == kTokenType_NumericVar && value.var) ||
			 (type == kTokenType_StringVar && value.var))
	{
		if (!value.var)
		{
			return 0.0;
		}
		return value.var->data;
	}
#endif
	return 0.0;
}

bool ScriptToken::GetBool() const
{
	switch (type)
	{
	case kTokenType_Boolean:
	case kTokenType_Number:
		return value.num ? true : false;
	case kTokenType_Form:
		return value.formID ? true : false;
	case kTokenType_Global:
		return value.global->data ? true : false;
#if RUNTIME
	case kTokenType_Array:
		return value.arrID ? true : false;
	case kTokenType_NumericVar:
	case kTokenType_StringVar:
	case kTokenType_ArrayVar:
	case kTokenType_RefVar:
	{
		if (!value.var)
		{
			return false;
		}
		return value.var->data ? true : false;
	}
	case kTokenType_String:
	{
		return value.str ? true : false;
	}
#endif
	default:
		return false;
	}
}

void* ScriptToken::GetAsVoidArg() const
{
	if (CanConvertTo(kTokenType_Number))
	{
		auto res = static_cast<float>(GetNumber());
		return *(void**)(&res);	//conversion: *((float *)&nthArgGetNumber();
	}
	if (CanConvertTo(kTokenType_String))
		return const_cast<char*>(GetString());
	if (CanConvertTo(kTokenType_Form))
		return LookupFormByID(GetFormID());
#if RUNTIME
	if (CanConvertTo(kTokenType_Array))
		return reinterpret_cast<void*>(GetArrayID());
#endif
	return nullptr;
}

std::pair<void*, Script::VariableType> ScriptToken::GetAsVoidArgAndVarType() const
{
	if (CanConvertTo(kTokenType_Number))
	{
		auto num = static_cast<float>(GetNumber());
		void* rawNum = *(void**)(&num);	//conversion: *((float*)&nthArgGetNumber();
		return std::make_pair(rawNum, Script::VariableType::eVarType_Float);
	}
	if (CanConvertTo(kTokenType_String))
		return std::make_pair(const_cast<char*>(GetString()), Script::VariableType::eVarType_String);
	if (CanConvertTo(kTokenType_Form))
		return std::make_pair(LookupFormByID(GetFormID()), Script::VariableType::eVarType_Ref);
#if RUNTIME
	if (CanConvertTo(kTokenType_Array))
		return std::make_pair(reinterpret_cast<void*>(GetArrayID()), Script::VariableType::eVarType_Array);
#endif
	return std::make_pair(nullptr, Script::VariableType::eVarType_Invalid);
}

Operator *ScriptToken::GetOperator() const
{
	return type == kTokenType_Operator ? value.op : NULL;
}

#if RUNTIME
ArrayID ScriptToken::GetArrayID() const
{
	if (type == kTokenType_Array)
		return value.arrID;
	if (type == kTokenType_ArrayVar)
	{
		if (!value.var)
		{
			return false;
		}
		return (int)value.var->data;
	}
	return 0;
}

ArrayVar *ScriptToken::GetArrayVar() const
{
	return g_ArrayMap.Get(GetArrayID());
}

ScriptLocal *ScriptToken::GetVar() const
{
	if (!IsVariable())
		return nullptr;
	return value.var;
}

StringVar* ScriptToken::GetStringVar() const
{
	if (type != kTokenType_StringVar)
		return nullptr;
	if (value.nvseVariable.stringVar)
		return value.nvseVariable.stringVar;
	if (value.var)
		return g_StringMap.Get(static_cast<int>(value.var->data));
	return nullptr;
}

CommandReturnType ScriptToken::GetReturnType() const
{
	if (CanConvertTo(kTokenType_Number)) {
		return kRetnType_Default;
	}
	if (CanConvertTo(kTokenType_String))
	{
		return kRetnType_String;
	}
	if (CanConvertTo(kTokenType_Form))
	{
		return kRetnType_Form;
	}
	if (CanConvertTo(kTokenType_Array))
	{
		return kRetnType_Array;
	}
	return kRetnType_Ambiguous;
}

Script::VariableType ScriptToken::GetTokenTypeAsVariableType() const
{
	if (CanConvertTo(kTokenType_Number)) {
		return Script::VariableType::eVarType_Float;
	}
	if (CanConvertTo(kTokenType_String))
	{
		return Script::VariableType::eVarType_String;
	}
	if (CanConvertTo(kTokenType_Form))
	{
		return Script::VariableType::eVarType_Ref;
	}
	if (CanConvertTo(kTokenType_Array))
	{
		return Script::VariableType::eVarType_Array;
	}
	return Script::VariableType::eVarType_Invalid;
}

void ScriptToken::AssignResult(ExpressionEvaluator &eval) const
{
	eval.AssignAmbiguousResult(*this, GetReturnType());

}

ScriptLocal* GetScriptLocal(UInt32 varIdx, UInt32 refIdx, Script* script, ScriptEventList* eventList)
{
	if (refIdx)
	{
		Script::RefVariable* refVar = script->GetRefFromRefList(refIdx);
		if (refVar)
		{
			refVar->Resolve(eventList);
			if (refVar->form)
				eventList = EventListFromForm(refVar->form);
		}
	}
	if (eventList)
		return eventList->GetVariable(varIdx); // TODO: fix lambda w/ var capture crashing during runtime here
	return nullptr;
}

bool ScriptToken::ResolveVariable()
{
	auto* eventList = context->eventList;
	value.var = GetScriptLocal(varIdx, refIdx, context->script, eventList);
	if (!value.var)
		return false;
	// to be deleted on event list destruction, see Hooks_Other.cpp#CleanUpNVSEVars
	if ((type == kTokenType_ArrayVar || type == kTokenType_StringVar) && refIdx == 0)
		AddToGarbageCollection(eventList, value.var, type == kTokenType_StringVar ? NVSEVarType::kVarType_String : NVSEVarType::kVarType_Array);

	if (type == kTokenType_StringVar)
		value.nvseVariable.stringVar = g_StringMap.Get(value.var->data);

#if _DEBUG
	if (value.var && !refIdx)
	{
		this->varName = context->script->GetVariableInfo(varIdx)->name.CStr();
	}
	if (value.var && value.var->data && type == kTokenType_ArrayVar)
	{
		auto *script = context->script;
		if (refIdx)
			script = GetReferencedQuestScript(refIdx, eventList);
		if (auto *var = g_ArrayMap.Get(value.var->data))
		{
			if (auto *varInfo = script->GetVariableInfo(value.var->id))
				var->varName = std::string(script->GetName()) + "." + std::string(varInfo->name.CStr());
			else
				var->varName = "<no var info>";
			this->arrayVar = var;
		}
	}
#endif


	return true;
}

Script* ScriptToken::GetUserFunction() const
{
	auto* form = GetTESForm();
	if (!form)
		return nullptr;
	if (!IS_ID(form, Script))
		return nullptr;
	return static_cast<Script*>(form);
}

ScriptParsing::CommandCallToken ScriptToken::GetCallToken(Script* script) const
{
	auto* cmdInfo = GetCommandInfo();
	if (!cmdInfo)
		return ScriptParsing::CommandCallToken(cmdInfo, -1, nullptr, nullptr);
	ScriptParsing::CommandCallToken callToken(cmdInfo, cmdInfo ? cmdInfo->opcode : -1, script->GetRefFromRefList(refIdx), script);
	UInt8* data = script->data + cmdOpcodeOffset;
	UInt16 length = *reinterpret_cast<UInt16*>(data - 2);
	const ScriptParsing::ScriptIterator it(script, cmdInfo ? cmdInfo->opcode : -1, length, refIdx, script->data + cmdOpcodeOffset);
	callToken.ParseCommandArgs(it, it.length);
	return std::move(callToken); // otherwise tries to call deleted copy ctor.

}
#endif

TESGlobal *ScriptToken::GetGlobal() const
{
	if (type == kTokenType_Global)
		return value.global;
	return NULL;
}

VariableInfo *ScriptToken::GetVarInfo() const
{
	switch (type)
	{
	case kTokenType_Variable:
	case kTokenType_StringVar:
	case kTokenType_ArrayVar:
	case kTokenType_NumericVar:
	case kTokenType_RefVar:
		return value.varInfo;
	}

	return NULL;
}

CommandInfo *ScriptToken::GetCommandInfo() const
{
	if (type == kTokenType_Command)
		return value.cmd;
	return NULL;
}

#if RUNTIME

std::string ScriptToken::GetStringRepresentation() const
{
	if (CanConvertTo(kTokenType_String))
		return GetString();
	if (CanConvertTo(kTokenType_Number))
		return FormatString("%g", GetNumber());
	if (CanConvertTo(kTokenType_Form) && GetTESForm())
		return GetTESForm()->GetStringRepresentation();
	if (CanConvertTo(kTokenType_Array) && GetArrayVar())
		return GetArrayVar()->GetStringRepresentation();
	return "";
}

UInt32 ScriptToken::GetActorValue() const
{
	UInt32 actorVal = eActorVal_NoActorValue;
	if (CanConvertTo(kTokenType_Number))
	{
		int num = GetNumber();
		if (num >= 0 && num <= eActorVal_FalloutMax)
		{
			actorVal = num;
		}
	}
	else
	{
		const char *str = GetString();
		if (str)
		{
			actorVal = GetActorValueForString(str);
		}
	}

	return actorVal;
}

UInt32 ScriptToken::GetAnimationGroup() const
{
	if (CanConvertTo(kTokenType_Number))
		return GetNumber();
	if (CanConvertTo(kTokenType_String))
		return TESAnimGroup::AnimGroupForString(GetString());
	return TESAnimGroup::kAnimGroup_Max;
}

char ScriptToken::GetAxis() const
{
	char axis = -1;
	const char *str = GetString();
	if (str && str[0] && str[1] == '\0')
	{
		switch (str[0])
		{
		case 'x':
		case 'X':
			axis = 'X';
			break;
		case 'y':
		case 'Y':
			axis = 'Y';
			break;
		case 'z':
		case 'Z':
			axis = 'Z';
			break;
		}
	}

	return axis;
}

UInt32 ScriptToken::GetSex() const
{
	const char *str = GetString();
	if (str)
	{
		if (!StrCompare(str, "male"))
		{
			return 0;
		}
		else if (!StrCompare(str, "female"))
		{
			return 1;
		}
	}

	return -1;
}

#endif // RUNTIME

/*************************************************

	ScriptToken methods

*************************************************/

Token_Type ToTokenType(CommandReturnType type)
{
	switch (type)
	{
	case kRetnType_Default:
		return kTokenType_Number;
	case kRetnType_Form:
		return kTokenType_Form;
	case kRetnType_String:
		return kTokenType_String;
	case kRetnType_Array:
		return kTokenType_Array;
	default:
		return kTokenType_Invalid;
	}
}

bool ScriptToken::CanConvertTo(Token_Type to) const
{
	return CanConvertOperand(type, to);
}

#if RUNTIME

UInt8 __fastcall ScriptTokenGetType(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->Type();
}

bool __fastcall ScriptTokenCanConvertTo(PluginScriptToken *scrToken, UInt8 toType)
{
	auto const tok = reinterpret_cast<ScriptToken*>(scrToken);
	return tok->CanConvertTo(static_cast<Token_Type>(toType));
}

double __fastcall ScriptTokenGetFloat(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetNumber();
}

bool __fastcall ScriptTokenGetBool(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetBool();
}

UInt32 __fastcall ScriptTokenGetFormID(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetFormID();
}

TESForm *__fastcall ScriptTokenGetTESForm(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetTESForm();
}

const char *__fastcall ScriptTokenGetString(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetString();
}

UInt32 __fastcall ScriptTokenGetArrayID(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetArrayID();
}

UInt32 __fastcall ScriptTokenGetActorValue(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetActorValue();
}

ScriptLocal *__fastcall ScriptTokenGetScriptVar(PluginScriptToken *scrToken)
{
	return reinterpret_cast<ScriptToken *>(scrToken)->GetVar();
}

const PluginTokenPair *__fastcall ScriptTokenGetPair(PluginScriptToken *scrToken)
{
	return reinterpret_cast<const PluginTokenPair *>(reinterpret_cast<ScriptToken *>(scrToken)->GetPair());
}

const PluginTokenSlice *__fastcall ScriptTokenGetSlice(PluginScriptToken *scrToken)
{
	return reinterpret_cast<const PluginTokenSlice *>(reinterpret_cast<ScriptToken *>(scrToken)->GetSlice());
}

UInt32 __fastcall ScriptTokenGetAnimationGroup(PluginScriptToken* scrToken)
{
	return reinterpret_cast<ScriptToken*>(scrToken)->GetAnimationGroup();
}

void __fastcall ScriptTokenGetArrayElement(PluginScriptToken* scrToken, NVSEArrayVarInterface::Element& outElem)
{
	if (auto const token = reinterpret_cast<ScriptToken*>(scrToken);
		token->CanConvertTo(kTokenType_String))
	{
		outElem = token->GetString();
	}
	else if (token->CanConvertTo(kTokenType_Array))
		outElem = NVSEArrayVarInterface::Element(reinterpret_cast<NVSEArrayVarInterface::Array*>(token->GetArrayID()));
	else if (token->CanConvertTo(kTokenType_Form))
		outElem = LookupFormByID(token->GetFormID());
	else if (token->CanConvertTo(kTokenType_Number))
		outElem = token->GetNumber();
	else
	{
		outElem.Reset();
	}
}

ScriptToken *ScriptToken::Read(ExpressionEvaluator *context)
{
	auto *newToken = new (false) ScriptToken(); // false allocates the token on heap instead of memory pool
	newToken->cached = true;
	newToken->memoryPooled = false;

	if (newToken->ReadFrom(context) != kTokenType_Invalid)
		return newToken;

	delete newToken;
	return nullptr;
}

#if _DEBUG
Map<ScriptEventList *, Map<ScriptLocal *, NVSEVarType>> g_nvseVarGarbageCollectionMap;

#else
UnorderedMap<ScriptEventList *, UnorderedMap<ScriptLocal *, NVSEVarType>> g_nvseVarGarbageCollectionMap;
#endif
ICriticalSection g_gcCriticalSection;
void AddToGarbageCollection(ScriptEventList *eventList, ScriptLocal *var, NVSEVarType type)
{
	ScopedLock lock(g_gcCriticalSection);
	g_nvseVarGarbageCollectionMap[eventList].Emplace(var, type);
}

Token_Type ScriptToken::ReadFrom(ExpressionEvaluator *context)
{
	UInt8 typeCode = context->ReadByte();
	this->context = context;
	switch (typeCode)
	{
	case 'B':
	case 'b':
		type = kTokenType_Number;
		value.num = context->ReadByte();
		break;
	case 'I':
	case 'i':
		type = kTokenType_Number;
		value.num = context->Read16();
		break;
	case 'L':
	case 'l':
		type = kTokenType_Number;
		value.num = context->Read32();
		break;
	case 'Z':
		type = kTokenType_Number;
		value.num = context->ReadFloat();
		break;
	case 'S':
	{
		type = kTokenType_String;
		UInt32 incData = 0;
		value.str = context->ReadString(incData);
		break;
	}
	case 'R':
		type = kTokenType_Ref;
		refIdx = context->Read16();
		value.refVar = context->script->GetRefFromRefList(refIdx);
		if (!value.refVar)
		{
			context->Error("Could not find ref %X in ref list!", refIdx);
			type = kTokenType_Invalid;
		}
		else
		{
			type = kTokenType_Form;
			value.refVar->Resolve(context->eventList);
			value.formID = value.refVar->form ? value.refVar->form->refID : 0;
		}
		//incrementData = 3;
		break;
	case 'G':
	{
		type = kTokenType_Global;
		refIdx = context->Read16();
		Script::RefVariable *refVar = context->script->GetRefFromRefList(refIdx);
		if (!refVar)
		{
			context->Error("Could not resolve global %X", refIdx);
			type = kTokenType_Invalid;
			break;
		}
		refVar->Resolve(context->eventList);
		value.global = DYNAMIC_CAST(refVar->form, TESForm, TESGlobal);
		if (!value.global)
		{
			context->Error("Failed to resolve global");
			type = kTokenType_Invalid;
			break;
		}

		break;
	}
	case 'x':
		useRefFromStack = true;
	case 'X':
	{
		type = kTokenType_Command;
		refIdx = context->Read16();
		const auto opcode = context->Read16();
		value.cmd = g_scriptCommands.GetByOpcode(opcode);
		if (!value.cmd)
		{
			context->Error("Failed to resolve command with opcode %X", opcode);
			type = kTokenType_Invalid;
			break;
		}
		returnType = g_scriptCommands.GetReturnType(value.cmd);
		const auto argsLen = context->Read16();
		cmdOpcodeOffset = context->m_data - context->m_scriptData;
		context->m_data += argsLen - 2;
		break;
	}
	case 'V':
	{
		variableType = context->ReadByte();
		switch (variableType)
		{
		case Script::eVarType_Array:
			type = kTokenType_ArrayVar;
			break;
		case Script::eVarType_Integer:
		case Script::eVarType_Float:
			type = kTokenType_NumericVar;
			break;
		case Script::eVarType_Ref:
			type = kTokenType_RefVar;
			break;
		case Script::eVarType_String:
			type = kTokenType_StringVar;
			break;
		default:
			context->Error("Unsupported variable type %X", variableType);
			type = kTokenType_Invalid;
			return type;
		}

		refIdx = context->Read16();
		varIdx = context->Read16();
		value.nvseVariable = { nullptr, {nullptr}};
		break;
	}
	case 'F':
	{
		type = kTokenType_LambdaScriptData;
		// context: we need a script lambda per event list so that each lambda can copy the appropriate locals; tokens are created
		// once per script data so we need to create the Script object later
		const auto dataLen = context->Read32();
		auto *scriptData = static_cast<UInt8 *>(FormHeap_Allocate(dataLen));
		context->ReadBuf(dataLen, scriptData);
		value.lambdaScriptData = LambdaManager::ScriptData(scriptData, dataLen);
		break;
	}
	default:
	{
		if (typeCode < kOpType_Max)
		{
			type = kTokenType_Operator;
			value.op = &s_operators[typeCode];
		}
		else
		{
			context->Error("Unexpected token type %d (%02x) encountered", typeCode, typeCode);
			type = kTokenType_Invalid;
		}
		//incrementData = 1;
	}
	}
	return type;
}

#endif

// compiling typecodes to printable chars just makes verifying parser output much easier
// static char TokenCodes[kTokenType_Max] =
// { 'Z', '!', 'S', '!', 'R', 'G', '!', '!', '!', 'X', 'V', 'V', 'V', 'V', 'V', '!', 'O', '!', 'B', 'I', 'L', '!', '!', 'F' };
// STATIC_ASSERT(SIZEOF_ARRAY(TokenCodes, char) == kTokenType_Max);

const std::unordered_map g_tokenCodes =
	{
		std::make_pair(kTokenType_Number, 'Z'),
		std::make_pair(kTokenType_String, 'S'),
		std::make_pair(kTokenType_Ref, 'R'),
		std::make_pair(kTokenType_Global, 'G'),
		std::make_pair(kTokenType_Command, 'X'),
		std::make_pair(kTokenType_Variable, 'V'),
		std::make_pair(kTokenType_NumericVar, 'V'),
		std::make_pair(kTokenType_RefVar, 'V'),
		std::make_pair(kTokenType_StringVar, 'V'),
		std::make_pair(kTokenType_ArrayVar, 'V'),
		std::make_pair(kTokenType_Operator, 'O'),
		std::make_pair(kTokenType_Byte, 'B'),
		std::make_pair(kTokenType_Short, 'I'),
		std::make_pair(kTokenType_Int, 'L'),
		std::make_pair(kTokenType_Lambda, 'F'),
};

inline char TokenTypeToCode(Token_Type type)
{
	if (const auto iter = g_tokenCodes.find(type); iter != g_tokenCodes.end())
		return iter->second;
	return 0;
}

bool ScriptToken::Write(ScriptLineBuffer *buf) const
{
	const auto code = TokenTypeToCode(type);
	if (!code)
	{
		g_ErrOut.Show("Unhandled token type %s for ScriptToken::Write", TokenTypeToString(type));
		return false;
	}

	if (type == kTokenType_Command && useRefFromStack)
		buf->WriteByte('x');
	else if (type != kTokenType_Operator && type != kTokenType_Number)
		buf->WriteByte(code);

	switch (type)
	{
	case kTokenType_Number:
	{
		if (floor(value.num) != value.num)
		{
			buf->WriteByte(TokenTypeToCode(kTokenType_Number));
			return buf->WriteFloat(value.num);
		}
		else // unary- compiled as operator. all numeric values in scripts are positive.
		{
			UInt32 val = value.num;
			if (val < 0x100)
			{
				buf->WriteByte(TokenTypeToCode(kTokenType_Byte));
				return buf->WriteByte((UInt8)val);
			}
			else if (val < 0x10000)
			{
				buf->WriteByte(TokenTypeToCode(kTokenType_Short));
				return buf->Write16((UInt16)val);
			}
			else
			{
				buf->WriteByte(TokenTypeToCode(kTokenType_Int));
				return buf->Write32(val);
			}
		}
	}
	case kTokenType_String:
		return buf->WriteString(value.str ? value.str : "");
	case kTokenType_Ref:
	case kTokenType_Global:
		return buf->Write16(refIdx);
	case kTokenType_Command:
		buf->Write16(refIdx);
		return buf->Write16(value.cmd->opcode);
	case kTokenType_NumericVar:
	case kTokenType_RefVar:
	case kTokenType_StringVar:
	case kTokenType_ArrayVar:
		buf->WriteByte(variableType);
		buf->Write16(refIdx);
		return buf->Write16(value.varInfo->idx);
	case kTokenType_Operator:
		return buf->WriteByte(value.op->type);
	case kTokenType_Lambda:
		// ref list and var list are shared by scripts
		buf->Write32(value.lambda->info.dataLength);
		return buf->Write(value.lambda->data, value.lambda->info.dataLength);
	// the rest are run-time only
	default:
		return false;
	}
}

#if RUNTIME
std::unique_ptr<ScriptToken> ScriptToken::ToBasicToken() const
{
	if (CanConvertTo(kTokenType_String))
		return Create(GetString());
	if (CanConvertTo(kTokenType_Array))
		return CreateArray(GetArrayID());
	if (CanConvertTo(kTokenType_Form))
		return CreateForm(GetFormID());
	if (CanConvertTo(kTokenType_Number))
		return Create(GetNumber());
	return nullptr;
}

double ScriptToken::GetNumericRepresentation(bool bFromHex, bool* hasErrorOut) const
{
	double result = 0.0;

	if (hasErrorOut)
		*hasErrorOut = false;

	if (CanConvertTo(kTokenType_Number))
		result = GetNumber();
	else if (CanConvertTo(kTokenType_String))
	{
		const char *str = GetString();
		bool bFromBin = false;
		if (!bFromHex)
		{
			// if string begins with "0x", interpret as hex
			// if it begins with "0b", interpret as binary
			Tokenizer tok(str, " \t\r\n");
			if (std::string pre;
				tok.NextToken(pre) != -1 && pre.length() >= 2)
			{
				if (!StrCompare(pre.substr(0, 2).c_str(), "0x"))
					bFromHex = true;
				else if (!StrCompare(pre.substr(0, 2).c_str(), "0b"))
				{
					str += 2;	//ignore first two characters, otherwise strtol breaks.
					bFromBin = true;
				}
			}
		}

		// errno can be set to any non-zero value by a library function call
		// regardless of whether there was an error, so it needs to be cleared
		// in order to check the error set by strtol / strtod
		errno = 0;

		if (bFromHex)
		{
			UInt32 hexInt = 0;
			int const successes = sscanf_s(str, "%x", &hexInt);
			result = (double)hexInt;
			if (hasErrorOut)
				*hasErrorOut = successes <= 0;
		}
		else if (bFromBin)
		{
			char* endPtr;
			result = static_cast<double>(strtol(str, &endPtr, 2));
			if (hasErrorOut)
				*hasErrorOut = endPtr == str || errno == ERANGE;
		}
		else
		{
			char* endPtr;
			result = std::strtod(str, &endPtr);
			if (hasErrorOut)
				*hasErrorOut = endPtr == str || errno == ERANGE;
		}
	}

	return result;
}

#endif

/****************************************

	ArrayElementToken

****************************************/

#if RUNTIME

bool ArrayElementToken::CanConvertTo(Token_Type to) const
{
	if (!IsGood())
		return false;
	else if (to == kTokenType_ArrayElement)
		return true;

	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (!arr)
		return false;

	DataType elemType = arr->GetElementType(&key);
	if (elemType == kDataType_Invalid)
		return false;

	switch (to)
	{
	case kTokenType_Boolean:
		return elemType == kDataType_Form || elemType == kDataType_Numeric;
	case kTokenType_String:
		return elemType == kDataType_String;
	case kTokenType_Number:
		return elemType == kDataType_Numeric;
	case kTokenType_Array:
		return elemType == kDataType_Array;
	case kTokenType_Form:
		return elemType == kDataType_Form;
	}

	return false;
}

void *ArrayElementToken::operator new(size_t size)
{
	return g_arrayElementTokenAllocator.Allocate();
}

void ArrayElementToken::operator delete(void *p)
{
	g_arrayElementTokenAllocator.Free(p);
}

double ArrayElementToken::GetNumber() const
{
	double out = 0.0;
	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (arr)
		arr->GetElementNumber(&key, &out);
	return out;
}

const char *ArrayElementToken::GetString() const
{
	const char *out = "";
	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (arr)
		arr->GetElementString(&key, &out);
	return out;
}

UInt32 ArrayElementToken::GetFormID() const
{
	UInt32 out = 0;
	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (arr)
		arr->GetElementFormID(&key, &out);
	return out;
}

TESForm *ArrayElementToken::GetTESForm() const
{
	TESForm *out = NULL;
	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (arr)
		arr->GetElementForm(&key, &out);
	return out;
}

bool ArrayElementToken::GetBool() const
{
	ArrayVar* arr = g_ArrayMap.Get(GetOwningArrayID());
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(&key, false);
	if (!elem)
		return false;

	if (elem->DataType() == kDataType_Numeric)
		return elem->m_data.num != 0;
	if (elem->DataType() == kDataType_Form)
		return elem->m_data.formID != 0;

	return false;
}

ArrayID ArrayElementToken::GetArrayID() const
{
	ArrayID out = 0;
	ArrayVar *arr = g_ArrayMap.Get(GetOwningArrayID());
	if (arr)
		arr->GetElementArray(&key, &out);
	return out;
}

#endif

// Rules for converting from one operand type to another
Token_Type kConversions_Number[] =
	{
		kTokenType_Boolean,
};

Token_Type kConversions_Boolean[] =
{
		kTokenType_Number,
};

Token_Type kConversions_String[] =
{
	kTokenType_Boolean
};

Token_Type kConversions_Command[] =
	{
		kTokenType_Ambiguous,

		kTokenType_Number,
		kTokenType_Form,
		kTokenType_Boolean,
};

Token_Type kConversions_Ref[] =
	{
		kTokenType_Form,
		kTokenType_Boolean,
};

Token_Type kConversions_Global[] =
	{
		kTokenType_Number,
		kTokenType_Boolean,
};

Token_Type kConversions_Form[] =
	{
		kTokenType_Boolean,
};

Token_Type kConversions_NumericVar[] =
	{
		kTokenType_Number,
		kTokenType_Boolean,
		kTokenType_Variable,
};

Token_Type kConversions_ArrayElement[] =
	{
#if !RUNTIME
		kTokenType_Ambiguous,
#endif

		kTokenType_Number,
		kTokenType_Form,
		kTokenType_String,
		kTokenType_Array,
		kTokenType_Boolean,
};

Token_Type kConversions_RefVar[] =
	{
		kTokenType_Form,
		kTokenType_Boolean,
		kTokenType_Variable,
};

Token_Type kConversions_StringVar[] =
{
		kTokenType_String,
		kTokenType_Variable,
		kTokenType_Boolean,
};

Token_Type kConversions_ArrayVar[] =
	{
		kTokenType_Array,
		kTokenType_Variable,
		kTokenType_Boolean,
};

Token_Type kConversions_Array[] =
{
		kTokenType_Boolean, // true if arrayID != 0, false if 0
};

Token_Type kConversions_AssignableString[] =
{
		kTokenType_String,
};

Token_Type kConversions_Lambda[] =
{
		kTokenType_Form
};

// just an array of the types to which a given token type can be converted
struct Operand
{
	Token_Type *rules;
	UInt8 numRules;
};

// Operand definitions
#define OPERAND(x) kConversions_##x, SIZEOF_ARRAY(kConversions_##x, Token_Type)

static Operand s_operands[] =
	{
		{OPERAND(Number)},
		{OPERAND(Boolean)},
		{OPERAND(String)}, // string has no conversions
		{OPERAND(Form)},
		{OPERAND(Ref)},
		{OPERAND(Global)},
		{OPERAND(Array)},
		{OPERAND(ArrayElement)},
		{NULL, 0}, // slice
		{OPERAND(Command)},
		{NULL, 0}, // variable
		{OPERAND(NumericVar)},
		{OPERAND(RefVar)},
		{OPERAND(StringVar)},
		{OPERAND(ArrayVar)},
		{NULL, 0}, // operator
		{NULL, 0}, // ambiguous
		{NULL, 0}, // forEachContext
		{NULL, 0}, // numeric placeholders, used only in bytecode
		{NULL, 0},
		{NULL, 0},
		{NULL, 0}, // pair
		{OPERAND(AssignableString)},
		{OPERAND(Lambda)},
		{NULL, 0}, // LeftToken
		{NULL, 0}, // RightToken
		{NULL, 0} // Max
};

STATIC_ASSERT(SIZEOF_ARRAY(s_operands, Operand) == kTokenType_Max);

bool CanConvertOperand(Token_Type from, Token_Type to)
{
	if (from == to)
		return true;
	if (from >= kTokenType_Invalid || to >= kTokenType_Invalid)
		return false;

	Operand *op = &s_operands[from];
	for (UInt32 i = 0; i < op->numRules; i++)
	{
		const auto rule = op->rules[i];
		if (rule == to)
			return true;
	}

	return false;
}

// Operator
Token_Type Operator::GetResult(Token_Type lhs, Token_Type rhs) const
{
	Token_Type result = kTokenType_Invalid;
	for (UInt32 i = 0; i < numRules; i++)
	{
		OperationRule *rule = &rules[i];
		if (CanConvertOperand(lhs, rule->lhs) && CanConvertOperand(rhs, rule->rhs))
			result = rule->result;
		if (!rule->bAsymmetric && CanConvertOperand(lhs, rule->rhs) && CanConvertOperand(rhs, rule->lhs))
			result = rule->result;

		if (result == kTokenType_LeftToken)
			result = lhs;
		else if (result == kTokenType_RightToken)
			result = rhs;

		if (result != kTokenType_Invalid)
			return result;
	}

	return kTokenType_Invalid;
}

#if RUNTIME

void Slice::GetArrayBounds(ArrayKey &lo, ArrayKey &hi) const
{
	if (bIsString)
	{
		lo = ArrayKey(m_lowerStr.c_str());
		hi = ArrayKey(m_upperStr.c_str());
	}
	else
	{
		lo = ArrayKey(m_lower);
		hi = ArrayKey(m_upper);
	}
}

#endif

char debugPrint[512];

char *ScriptToken::DebugPrint() const
{
	switch (type)
	{
	case kTokenType_Number:
		sprintf_s(debugPrint, 512, "[Type=Number, Value=%g]", value.num);
		break;
	case kTokenType_Boolean:
		sprintf_s(debugPrint, 512, "[Type=Boolean, Value=%s]", value.num ? "true" : "false");
		break;
	case kTokenType_String:
		sprintf_s(debugPrint, 512, "[Type=String, Value=%s]", value.str ? value.str : "");
		break;
	case kTokenType_Form:
		sprintf_s(debugPrint, 512, "[Type=Form, Value=%08X]", value.formID);
		break;
	case kTokenType_Ref:
		sprintf_s(debugPrint, 512, "[Type=Ref, Value=%s]", value.refVar->name.CStr());
		break;
	case kTokenType_Global:
		sprintf_s(debugPrint, 512, "[Type=Global, Value=%s]", value.global->GetName());
		break;
	case kTokenType_ArrayElement:
		sprintf_s(debugPrint, 512, "[Type=ArrayElement, Value=%g]", value.num);
		break;
	case kTokenType_Slice:
		sprintf_s(debugPrint, 512, "[Type=Slice, Value=%g]", value.num);
		break;
	case kTokenType_Command:
		sprintf_s(debugPrint, 512, "[Type=Command, Value=%d]", value.cmd->opcode);
		break;
#if RUNTIME
	case kTokenType_Array:
		sprintf_s(debugPrint, 512, "[Type=Array, Value=%lu]", value.arrID);
		break;
	case kTokenType_Variable:
		sprintf_s(debugPrint, 512, "[Type=Variable, Value=%lu]", value.var->id);
		break;
	case kTokenType_NumericVar:
		sprintf_s(debugPrint, 512, "[Type=NumericVar, Value=%g]", value.var->data);
		break;
	case kTokenType_ArrayVar:
		sprintf_s(debugPrint, 512, "[Type=ArrayVar, Value=%d]", value.arrID);
		break;
	case kTokenType_StringVar:
		sprintf_s(debugPrint, 512, "[Type=StringVar, Value=%g]", value.num);
		break;
#endif
	case kTokenType_RefVar:
		sprintf_s(debugPrint, 512, "[Type=RefVar, EDID=%s]", value.refVar->form->GetName());
		break;
	case kTokenType_Ambiguous:
		sprintf_s(debugPrint, 512, "[Type=Ambiguous, no Value]");
		break;
	case kTokenType_Operator:
		sprintf_s(debugPrint, 512, "[Type=Operator, Value=%hhd]", value.op->type);
		break;
	case kTokenType_ForEachContext:
		sprintf_s(debugPrint, 512, "[Type=ForEachContext, Value=%g]", value.num);
		break;
	case kTokenType_Byte:
		sprintf_s(debugPrint, 512, "[Type=Byte, Value=%g]", value.num);
		break;
	case kTokenType_Short:
		sprintf_s(debugPrint, 512, "[Type=Short, Value=%g]", value.num);
		break;
	case kTokenType_Int:
		sprintf_s(debugPrint, 512, "[Type=Int, Value=%g]", value.num);
		break;
	case kTokenType_Pair:
		sprintf_s(debugPrint, 512, "[Type=Pair, Value=%g]", value.num);
		break;
	case kTokenType_AssignableString:
		sprintf_s(debugPrint, 512, "[Type=AssignableString, Value=%s]", value.str ? value.str : "");
		break;
	case kTokenType_Invalid:
		sprintf_s(debugPrint, 512, "[Type=Invalid, no Value]");
		break;
	case kTokenType_Empty:
		sprintf_s(debugPrint, 512, "[Type=Empty, no Value]");
		break;
	}
	return debugPrint;
}
