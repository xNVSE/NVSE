
#include "ScriptUtils.h"

#include <optional>
#include <set>

#include "CommandTable.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "Hooks_Script.h"
#include <string>

#include "containers.h"
#include "FastStack.h"
#include "ParamInfos.h"
#include "FunctionScripts.h"
#include "GameRTTI.h"
#include "LambdaManager.h"
#include <regex>
#include <utility>
#include <ranges>

#include "Commands_Script.h"
#include "Hooks_Editor.h"
#include "ScriptAnalyzer.h"
#include "StackVariables.h"

std::map<std::pair<Script*, std::string>, Script::VariableType> g_variableDefinitionsMap;

#if RUNTIME

#ifdef DBG_EXPR_LEAKS
SInt32 FUNCTION_CONTEXT_COUNT = 0;
#endif

#include "GameData.h"
#include "common/ICriticalSection.h"
#include "ThreadLocal.h"
#include "PluginManager.h"
#include "SmallObjectsAllocator.h"

const char *GetEditorID(TESForm *form)
{
	return nullptr;
}

static void ShowError(const char *msg)
{
	Console_Print(msg);
	_MESSAGE(msg);
}

static bool ShowWarning(const char *msg)
{
	Console_Print(msg);
	_MESSAGE(msg);
	return false;
}

#else

#include "GameAPI.h"
#include <ranges>


const char *GetEditorID(TESForm *form)
{
	return form->editorData.editorID.m_data;
}

static void ShowError(const char *msg)
{
	MessageBox(NULL, msg, "NVSE", MB_ICONEXCLAMATION);
}

static bool ShowWarning(const char *msg)
{
	char msgText[0x400];
	sprintf_s(msgText, sizeof(msgText), "%s\n\n'Cancel' will disable this message for the remainder of the session.", msg);
	int result = MessageBox(NULL, msgText, "NVSE", MB_OKCANCEL | MB_ICONEXCLAMATION);
	return result == IDCANCEL;
}

#endif

Script::VariableType GetSavedVarType(Script* script, const std::string& name)
{
	if (!script)
		return Script::eVarType_Invalid;
	if (const auto iter = g_variableDefinitionsMap.find(std::make_pair(script, name)); iter != g_variableDefinitionsMap.end())
		return iter->second;
	return Script::eVarType_Invalid;
}

void SaveVarType(Script* script, const std::string& name, Script::VariableType type)
{
	g_variableDefinitionsMap.emplace(std::make_pair(script, name), type);
}


ErrOutput g_ErrOut(ShowError, ShowWarning);

#if RUNTIME
UInt32 AddStringVar(const char *data, ScriptToken &lh, ExpressionEvaluator &context, StringVar** svOut)
{
	if (!lh.refIdx)
		AddToGarbageCollection(context.eventList, lh.GetScriptLocal(), NVSEVarType::kVarType_String);
	return g_StringMap.Add(context.script->GetModIndex(), data, false, svOut);
}

UInt32 AddStringVar(StringVar&& other, ScriptToken& lh, ExpressionEvaluator& context, StringVar** svOut)
{
	if (!lh.refIdx)
		AddToGarbageCollection(context.eventList, lh.GetScriptLocal(), NVSEVarType::kVarType_String);
	return g_StringMap.Add(std::move(other), false, svOut);
}
#endif

#if RUNTIME

// run-time errors
enum
{
	kScriptError_UnhandledOperator,
	kScriptError_DivisionByZero,
	kScriptError_InvalidArrayAccess,
	kScriptError_UninitializedArray,
	kScriptError_InvalidCallingObject,
	kScriptError_CommandFailed,
	kScriptError_MissingOperand,
	kScriptError_OperatorFailed,
	kScriptError_ExpressionFailed,
	kScriptError_UnexpectedTokenType,
	kScriptError_RefToTempArray,
};

// Operator routines

const char *OpTypeToSymbol(OperatorType op);

bool ValidateVariable(const std::string &varName, Script::VariableType varType, Script *script);

std::unique_ptr<ScriptToken> Eval_Comp_Number_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	switch (op)
	{
	case kOpType_GreaterThan:
		return ScriptToken::Create(lh->GetNumber() > rh->GetNumber());
	case kOpType_LessThan:
		return ScriptToken::Create(lh->GetNumber() < rh->GetNumber());
	case kOpType_GreaterOrEqual:
		return ScriptToken::Create(lh->GetNumber() >= rh->GetNumber());
	case kOpType_LessOrEqual:
		return ScriptToken::Create(lh->GetNumber() <= rh->GetNumber());
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Comp_String_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const char *lhs = lh->GetString();
	const char *rhs = rh->GetString();
	switch (op)
	{
	case kOpType_GreaterThan:
		return ScriptToken::Create(StrCompare(lhs, rhs) > 0);
	case kOpType_LessThan:
		return ScriptToken::Create(StrCompare(lhs, rhs) < 0);
	case kOpType_GreaterOrEqual:
		return ScriptToken::Create(StrCompare(lhs, rhs) >= 0);
	case kOpType_LessOrEqual:
		return ScriptToken::Create(StrCompare(lhs, rhs) <= 0);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Eq_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(FloatEqual(lh->GetNumber(), rh->GetNumber()));
	case kOpType_NotEqual:
		return ScriptToken::Create(!(FloatEqual(lh->GetNumber(), rh->GetNumber())));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Eq_Array(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	// Instead of comparing arrayIDs, compare the contents of the arrays.
	// For nested arrays, compare the arrayIDs to save on computing power. Use the Ar_DeepEquals function if needed.
	bool isEqual;
	const auto lhArr = g_ArrayMap.Get(lh->GetArrayID());
	const auto rhArr = g_ArrayMap.Get(rh->GetArrayID());

	if (lhArr && rhArr)
	{
		isEqual = lhArr->Equals(rhArr);
	}
	else if (lhArr || rhArr)  // one is null while the other is not.
	{
		isEqual = false;
	}
	else  // both are null.
	{
		isEqual = true;
	}
	
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(isEqual);
	case kOpType_NotEqual:
		return ScriptToken::Create(!isEqual);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Eq_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const char *lhs = lh->GetString();
	const char *rhs = rh->GetString();
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(!StrCompare(lhs, rhs));
	case kOpType_NotEqual:
		return ScriptToken::Create(StrCompare(lhs, rhs) != 0);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Eq_Form(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	bool result = false;
	TESForm *lhForm = lh->GetTESForm();
	TESForm *rhForm = rh->GetTESForm();
	if (lhForm == nullptr && rhForm == nullptr)
		result = true;
	else if (lhForm && rhForm && lhForm->refID == rhForm->refID)
		result = true;

	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(result);
	case kOpType_NotEqual:
		return ScriptToken::Create(!result);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Eq_Form_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	bool result = false;
	if (rh->GetNumber() == 0 && lh->GetFormID() == 0) // only makes sense to compare forms to zero
		result = true;
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(result);
	case kOpType_NotEqual:
		return ScriptToken::Create(!result);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Logical(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	switch (op)
	{
	case kOpType_LogicalAnd:
	{
		return lh->GetBool() && rh->GetBool() ? rh->ForwardEvalResult() : ScriptToken::Create(false);
	}
	case kOpType_LogicalOr:
		return lh->GetBool() || rh->GetBool() ? rh->ForwardEvalResult() : ScriptToken::Create(false);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Add_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(lh->GetNumber() + rh->GetNumber());
}

char *__fastcall ConcatStrings(const char *lStr, const char *rStr)
{
	const UInt32 lLen = StrLen(lStr), rLen = StrLen(rStr);
	if (lLen || rLen)
	{
		auto conStr = static_cast<char*>(malloc(lLen + rLen + 1));
		if (lLen)
			memcpy(conStr, lStr, lLen);
		memcpy(conStr + lLen, rStr, rLen + 1);
		return conStr;
	}
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_Add_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto token = ScriptToken::Create(static_cast<const char*>(nullptr));
	token->value.str = ConcatStrings(lh->GetString(), rh->GetString());
	return token;
}

std::unique_ptr<ScriptToken> Eval_Arithmetic(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double l = lh->GetNumber();
	const double r = rh->GetNumber();
	switch (op)
	{
	case kOpType_Subtract:
		return ScriptToken::Create(l - r);
	case kOpType_Multiply:
		return ScriptToken::Create(l * r);
	case kOpType_Divide:
		if (r != 0)
		{
			return ScriptToken::Create(l / r);
		}
		context->Error("Division by zero");
		return nullptr;
	case kOpType_Exponent:
		return ScriptToken::Create(pow(l, r));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

std::unique_ptr<ScriptToken> Eval_Integer(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	SInt64 l = lh->GetNumber();
	const SInt64 r = rh->GetNumber();

	switch (op)
	{
	case kOpType_Modulo:
		if (r != 0)
			return ScriptToken::Create(static_cast<double>(l % r));
		else
		{
			context->Error("Division by zero");
			return nullptr;
		}
	case kOpType_BitwiseOr:
		return ScriptToken::Create(static_cast<double>(l | r));
	case kOpType_BitwiseAnd:
		return ScriptToken::Create(static_cast<double>(l & r));
	case kOpType_LeftShift:
		return ScriptToken::Create(static_cast<double>(l << r));
	case kOpType_RightShift:
		return ScriptToken::Create(static_cast<double>(l >> r));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return nullptr;
	}
}

// Warning: does not currently support all operations, only a handful.
// Return value is up to the receiving function to interpret; can be used for "l &= r" or "l & r"
double Apply_LeftVal_RightVal_Operator(OperatorType op, double l, double r, ExpressionEvaluator *context, bool &hasError)
{
	hasError = false;
	switch (op)
	{
	case kOpType_BitwiseOr:
	case kOpType_BitwiseOrEquals:
		return (static_cast<SInt64>(l) | static_cast<SInt64>(r));
	case kOpType_BitwiseAnd:
	case kOpType_BitwiseAndEquals:
		return (static_cast<SInt64>(l) & static_cast<SInt64>(r));
	case kOpType_Modulo:
	case kOpType_ModuloEquals:
		if (static_cast<SInt64>(r) != 0)
			return (static_cast<SInt64>(l) % static_cast<SInt64>(r));
		else
		{
			hasError = true;
			context->Error("Division by zero");
			return NULL;
		}
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		hasError = true;
		return NULL;
	}
}

std::unique_ptr<ScriptToken> Eval_Assign_Numeric(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	double result = rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		result = floor(result);

	lh->GetScriptLocal()->data = result;

	return ScriptToken::Create(result);
}

std::unique_ptr<ScriptToken> Eval_Assign_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* lhVar = lh->GetScriptLocal();
	StringVar* lhStrVar = lh->GetStringVar();
	if (rh->type == kTokenType_StringVar && rh->value.var == nullptr) // nullptr = rvalue returned from a command call
	{
		// optimizations
		auto* rhStrVar = rh->GetStringVar();
		if (!rhStrVar)
			return nullptr;
		// move the string if it's temporary
		if (!lhStrVar)
		{
			lhVar->data = static_cast<int>(AddStringVar(std::move(*rhStrVar), *lh, *context, &lhStrVar));
			lhStrVar = g_StringMap.Get(static_cast<int>(lhVar->data));
		}
		else
			lhStrVar->Set(std::move(*rhStrVar));

		lhStrVar->SetOwningModIndex(context->script->GetModIndex());
		return ScriptToken::Create(lhVar, lhStrVar);
	}
	const char* str = rh->GetString();
	if (!lhStrVar)
		lhVar->data = static_cast<int>(AddStringVar(str, *lh, *context, &lhStrVar));
	else
		lhStrVar->Set(str);

#if _DEBUG
	lhStrVar->eventList = context->eventList;
	lhStrVar->script = context->script;
	lhStrVar->varName = lh->GetVariableName(context->script);
#endif

	return ScriptToken::Create(lhVar, lhStrVar);
}

std::unique_ptr<ScriptToken> Eval_Assign_AssignableString(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto const aStr = dynamic_cast<AssignableSubstringToken *>(lh);
	return aStr->Assign(rh->GetString()) ? ScriptToken::Create(aStr->GetString()) : nullptr;
}

std::unique_ptr<ScriptToken> Eval_Assign_Form(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const UInt32 formID = rh->GetFormID();
	if (auto lhVar = lh->GetScriptLocal())
	{
		auto* const outRefID = reinterpret_cast<UInt64*>(&(lhVar->data));
		*outRefID = formID;
	}
	return ScriptToken::CreateForm(formID);
}

std::unique_ptr<ScriptToken> Eval_Assign_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double value = rh->GetNumber();
	lh->GetGlobal()->data = value;
	return ScriptToken::Create(value);
}

std::unique_ptr<ScriptToken> Eval_Assign_Array(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var = lh->GetScriptLocal();
	g_ArrayMap.AddReference(&var->data, rh->GetArrayID(), context->script->GetModIndex());
	if (!lh->refIdx)
		AddToGarbageCollection(context->eventList, var, NVSEVarType::kVarType_Array);

#if _DEBUG
	auto *script = context->script;
	if (lh->refIdx)
		script = GetReferencedQuestScript(lh->refIdx, context->eventList);
	if (auto *arrayVar = rh->GetArrayVar(); arrayVar && var)
		arrayVar->varName = std::string(script->GetName()) + '.' + script->GetVariableInfo(var->id)->name.CStr();
	else if (arrayVar && arrayVar->varName.empty())
		arrayVar->varName = "<eval assign var not found>";
#endif

	return ScriptToken::CreateArray(var->data);
}

const auto g_invalidElemMessageStr = "Array element is invalid";

bool GetArrayAndArrayKey(ScriptToken *lh, const ArrayKey *&key, ArrayVar *&arr, ExpressionEvaluator *context)
{
	key = lh->GetArrayKey();
	if (!key)
	{
		context->Error(g_invalidElemMessageStr);
		return false;
	}
	arr = g_ArrayMap.Get(lh->GetOwningArrayID());
	if (!arr)
	{
		context->Error("Invalid array access - the array was not initialized.");
		return false;
	}
	return true;
}

std::unique_ptr<ScriptToken> Eval_Assign_Elem_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key;
	ArrayVar *arr;
	if (!GetArrayAndArrayKey(lh, key, arr, context))
	{
		return nullptr;
	}

	const double value = rh->GetNumber();
	if (key->KeyType() == kDataType_Numeric)
	{
		if (!arr->SetElementNumber(key->key.num, value))
			return nullptr;
	}
	else if (!arr->SetElementNumber(key->key.GetStr(), value))
		return nullptr;

	return ScriptToken::Create(value);
}

std::unique_ptr<ScriptToken> Eval_Assign_Elem_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key;
	ArrayVar *arr;
	if (!GetArrayAndArrayKey(lh, key, arr, context))
	{
		return nullptr;
	}

	const char *str = rh->GetString();
	if (key->KeyType() == kDataType_Numeric)
	{
		if (!arr->SetElementString(key->key.num, str))
		{
			context->Error("Element with key not found");
			return nullptr;
		}
	}
	else if (!arr->SetElementString(key->key.GetStr(), str))
	{
		context->Error("Element with key not found");
		return nullptr;
	}

	return ScriptToken::Create(str);
}

std::unique_ptr<ScriptToken> Eval_Assign_Elem_Form(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key;
	ArrayVar *arr;
	if (!GetArrayAndArrayKey(lh, key, arr, context))
	{
		return nullptr;
	}

	const UInt32 formID = rh->GetFormID();
	if (key->KeyType() == kDataType_Numeric)
	{
		if (!arr->SetElementFormID(key->key.num, formID))
		{
			context->Error("Element with key not found");
			return nullptr;
		}
	}
	else if (!arr->SetElementFormID(key->key.GetStr(), formID))
	{
		context->Error("Element with key not found");
		return nullptr;
	}

	return ScriptToken::CreateForm(formID);
}

std::unique_ptr<ScriptToken> Eval_Assign_Elem_Array(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key;
	ArrayVar *arr;
	if (!GetArrayAndArrayKey(lh, key, arr, context))
	{
		return nullptr;
	}

	const ArrayID rhArrID = rh->GetArrayID();
	if (key->KeyType() == kDataType_Numeric)
	{
		if (!arr->SetElementArray(key->key.num, rhArrID))
			return nullptr;
	}
	else if (!arr->SetElementArray(key->key.GetStr(), rhArrID))
		return nullptr;

	return ScriptToken::CreateArray(rhArrID);
}

std::unique_ptr<ScriptToken> Eval_PlusEquals_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var = lh->GetScriptLocal();
	var->data += rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		var->data = floor(var->data);

	return ScriptToken::Create(var->data);
}

std::unique_ptr<ScriptToken> Eval_MinusEquals_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var = lh->GetScriptLocal();
	var->data -= rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		var->data = floor(var->data);

	return ScriptToken::Create(var->data);
}

std::unique_ptr<ScriptToken> Eval_TimesEquals(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var = lh->GetScriptLocal();
	var->data *= rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		var->data = floor(var->data);

	return ScriptToken::Create(var->data);
}

std::unique_ptr<ScriptToken> Eval_DividedEquals(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double rhNum = rh->GetNumber();
	if (rhNum == 0.0)
	{
		context->Error("Division by zero");
		return nullptr;
	}

	ScriptLocal* var = lh->GetScriptLocal();
	var->data /= rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		var->data = floor(var->data);

	return ScriptToken::Create(var->data);
}

std::unique_ptr<ScriptToken> Eval_ExponentEquals(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var = lh->GetScriptLocal();
	const double rhNum = rh->GetNumber();
	const double lhNum = var->data;
	var->data = pow(lhNum, rhNum);
	if (lh->GetVariableType() == Script::eVarType_Integer)
		var->data = floor(var->data);
	return ScriptToken::Create(var->data);
}

std::unique_ptr<ScriptToken> Eval_HandleEquals(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double r = rh->GetNumber();
	ScriptLocal* var = lh->GetScriptLocal();
	const double l = var->data;

	bool hasError;
	double const result = Apply_LeftVal_RightVal_Operator(op, l, r, context, hasError);
	if (!hasError)
	{
		var->data = result;
		return ScriptToken::Create(var->data);
	}
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_PlusEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto* global = lh->GetGlobal();
	global->data += rh->GetNumber();
	if (global->type != TESGlobal::kType_Float)
		global->data = floor(global->data);
	return ScriptToken::Create(static_cast<double>(global->data));
}

std::unique_ptr<ScriptToken> Eval_MinusEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto* global = lh->GetGlobal();
	global->data -= rh->GetNumber();
	if (global->type != TESGlobal::kType_Float)
		global->data = floor(global->data);
	return ScriptToken::Create(static_cast<double>(global->data));
}

std::unique_ptr<ScriptToken> Eval_TimesEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto* global = lh->GetGlobal();
	global->data *= rh->GetNumber();
	if (global->type != TESGlobal::kType_Float)
		global->data = floor(global->data);
	return ScriptToken::Create(static_cast<double>(global->data));
}

std::unique_ptr<ScriptToken> Eval_DividedEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double num = rh->GetNumber();
	if (num == 0.0)
	{
		context->Error("Division by zero.");
		return nullptr;
	}

	auto* global = lh->GetGlobal();
	global->data /= num;
	if (global->type != TESGlobal::kType_Float)
		global->data = floor(global->data);
	return ScriptToken::Create(static_cast<double>(global->data));
}

std::unique_ptr<ScriptToken> Eval_ExponentEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto* global = lh->GetGlobal();
	global->data = pow(global->data, rh->GetNumber());
	if (global->type != TESGlobal::kType_Float)
		global->data = floor(global->data);
	return ScriptToken::Create(static_cast<double>(global->data));
}

std::unique_ptr<ScriptToken> Eval_HandleEquals_Global(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const double l = lh->GetGlobal()->data;
	const double r = rh->GetNumber();
	bool hasError;
	double const result = Apply_LeftVal_RightVal_Operator(op, l, r, context, hasError);
	if (!hasError)
	{
		lh->GetGlobal()->data = result;
		return ScriptToken::Create(static_cast<double>(lh->GetGlobal()->data));
	}
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_PlusEquals_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal *var = lh->GetScriptLocal();
	UInt32 strID = static_cast<int>(var->data);
	StringVar *strVar = g_StringMap.Get(strID);
	if (!strVar)
	{
		//strID = g_StringMap.Add(context->script->GetModIndex(), "");
		strID = AddStringVar("", *lh, *context, &strVar);
		var->data = static_cast<int>(strID);
	}

	strVar->StringRef() += rh->GetString();
	return ScriptToken::Create(var, strVar);
}

// Currently unused.
std::unique_ptr<ScriptToken> Eval_TimesEquals_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal *var = lh->GetScriptLocal();
	UInt32 strID = static_cast<int>(var->data);
	StringVar *strVar = g_StringMap.Get(strID);
	if (!strVar)
	{
		//strID = g_StringMap.Add(context->script->GetModIndex(), "");
		strID = AddStringVar("", *lh, *context, &strVar);
		var->data = static_cast<int>(strID);
		strVar = g_StringMap.Get(strID);
	}

	const std::string str = strVar->String();

	int rhNum = rh->GetNumber();
	while (rhNum > 0)
	{
		strVar->StringRef() += str;
		rhNum--;
	}

	return ScriptToken::Create(var, strVar);
}

std::unique_ptr<ScriptToken> Eval_Multiply_String_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const char *str = lh->GetString();
	std::string result;

	int rhNum = rh->GetNumber();
	while (rhNum > 0)
	{
		result += str;
		rhNum--;
	}

	return ScriptToken::Create(result.c_str());
}

std::unique_ptr<ScriptToken> Eval_PlusEquals_Elem_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double elemVal;
		if (elem && elem->GetAsNumber(&elemVal))
		{
			elemVal += rh->GetNumber();
			elem->SetNumber(elemVal);
			return ScriptToken::Create(elemVal);
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_MinusEquals_Elem_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double elemVal;
		if (elem && elem->GetAsNumber(&elemVal))
		{
			elemVal -= rh->GetNumber();
			elem->SetNumber(elemVal);
			return ScriptToken::Create(elemVal);
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_TimesEquals_Elem(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double elemVal;
		if (elem && elem->GetAsNumber(&elemVal))
		{
			elemVal *= rh->GetNumber();
			elem->SetNumber(elemVal);
			return ScriptToken::Create(elemVal);
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_DividedEquals_Elem(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double elemVal;
		if (elem && elem->GetAsNumber(&elemVal))
		{
			const double result = rh->GetNumber();
			if (result != 0.0)
			{
				elemVal /= result;
				elem->SetNumber(elemVal);
				return ScriptToken::Create(elemVal);
			}
			context->Error("Division by zero");
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_ExponentEquals_Elem(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double elemVal;
		if (elem && elem->GetAsNumber(&elemVal))
		{
			const double result = pow(elemVal, rh->GetNumber());
			elem->SetNumber(result);
			return ScriptToken::Create(result);
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_HandleEquals_Elem(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayKey *const key = lh->GetArrayKey();
	if (key)
	{
		ArrayElement *const elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		double l;
		if (elem && elem->GetAsNumber(&l))
		{
			const double r = rh->GetNumber();
			bool hasError;
			double const result = Apply_LeftVal_RightVal_Operator(op, l, r, context, hasError);
			if (!hasError)
			{
				elem->SetNumber(result);
				return ScriptToken::Create(result);
			}
			return nullptr;
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_PlusEquals_Elem_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	if (const ArrayKey *key = lh->GetArrayKey())
	{
		ArrayElement *elem = g_ArrayMap.GetElement(lh->GetOwningArrayID(), key);
		const char *pElemStr;
		if (elem && elem->GetAsString(&pElemStr))
		{
			auto token = ScriptToken::Create(static_cast<const char*>(nullptr));
			char *conStr = ConcatStrings(pElemStr, rh->GetString());
			token->value.str = conStr;
			elem->SetString(conStr);
			return token;
		}
	}
	context->Error(g_invalidElemMessageStr);
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_Negation(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(-lh->GetNumber());
}

std::unique_ptr<ScriptToken> Eval_LogicalNot(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(!lh->GetBool());
}

std::unique_ptr<ScriptToken> Eval_BitwiseNot(OperatorType op, ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context) {
	return ScriptToken::Create(static_cast<double>(~static_cast<UInt32>(lhs->GetNumber())));
}

std::unique_ptr<ScriptToken> Eval_Subscript_Array_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayID arrID = lh->GetArrayID();
	ArrayVar *arr = arrID ? g_ArrayMap.Get(arrID) : nullptr;

	if (!arr)
	{
		context->Error("Invalid array access - the array was not initialized. 0");
		return nullptr;
	}
	if (arr->KeyType() != kDataType_Numeric)
	{
		context->Error("Invalid array access - expected string index, received numeric.");
		return nullptr;
	}
	ArrayKey key(rh->GetNumber());

	return ScriptToken::Create(arrID, &key);
}

std::unique_ptr<ScriptToken> Eval_Subscript_Elem_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto *arrayElement = dynamic_cast<ArrayElementToken *>(lh);

	// bugfix for `testexpr svKey := aTest["bleh"][0]` when bleh doesn't exist
	if (!arrayElement->CanConvertTo(kTokenType_String))
	{
		return nullptr;
	}

	const UInt32 idx = rh->GetNumber();
	return ScriptToken::Create(arrayElement, idx, idx);
}

std::unique_ptr<ScriptToken> Eval_Subscript_Elem_Slice(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const Slice *slice = rh->GetSlice();
	if (!slice || slice->bIsString)
	{
		context->Error("Invalid array slice operation - array is uninitialized or supplied index does not match key type");
	}
	return (slice && !slice->bIsString) ? ScriptToken::Create(dynamic_cast<ArrayElementToken *>(lh), slice->m_lower, slice->m_upper) : nullptr;
}

std::unique_ptr<ScriptToken> Eval_Subscript_Array_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayID arrID = lh->GetArrayID();
	ArrayVar *arr = arrID ? g_ArrayMap.Get(arrID) : nullptr;

	if (!arr)
	{
		context->Error("Invalid array access - the array was not initialized. 1");
		return nullptr;
	}
	if (arr->KeyType() != kDataType_String)
	{
		context->Error("Invalid array access - expected numeric index, received string");
		return nullptr;
	}

	ArrayKey key(rh->GetString());
	return ScriptToken::Create(arrID, &key);
}

std::unique_ptr<ScriptToken> Eval_Subscript_Array_Slice(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ArrayVar *srcArr = g_ArrayMap.Get(lh->GetArrayID());
	if (srcArr)
	{
		ArrayVar *sliceArr = srcArr->MakeSlice(rh->GetSlice(), context->script->GetModIndex());
		if (sliceArr)
			return ScriptToken::CreateArray(sliceArr->ID());
	}

	context->Error("Invalid array slice operation - array is uninitialized or supplied index does not match key type");
	return nullptr;
}

const auto *g_stringVarUninitializedMsg = "String var is uninitialized";

std::unique_ptr<ScriptToken> Eval_Subscript_StringVar_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	SInt32 lhIdx{};
	
	SInt32 rhIdx = rh->GetNumber();
	if (auto* lhVar = lh->GetScriptLocal())
	{
		lhIdx = static_cast<UInt32>(lhVar->data);
	}
	else {
		context->Error("Invalid variable");
		return nullptr;
	}

	StringVar* strVar = g_StringMap.Get(lhIdx);
	if (!strVar)
	{
		context->Error(g_stringVarUninitializedMsg);
		return nullptr; // uninitialized
	}

	if (rhIdx < 0)
	{
		// negative index counts from end of string
		rhIdx += strVar->GetLength();
	}

	return ScriptToken::Create(lhIdx, rhIdx, rhIdx);
}

std::unique_ptr<ScriptToken> Eval_Subscript_StringVar_Slice(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ScriptLocal* var{};
	UInt32 id{};

	const Slice* slice = rh->GetSlice();
	double upper = slice->m_upper;
	double lower = slice->m_lower;

	if (auto* lhVar = lh->GetScriptLocal()) {
		id = static_cast<UInt32>(lhVar->data);
	}
	else {
		context->Error("Invalid variable");
		return nullptr;
	}

	auto* strVar = g_StringMap.Get(id);
	if (!strVar)
	{
		context->Error(g_stringVarUninitializedMsg);
		return nullptr;
	}

	const UInt32 len = strVar->GetLength();
	if (upper < 0)
	{
		upper += len;
	}

	if (lower < 0)
	{
		lower += len;
	}

	if (slice && !slice->bIsString) {
		return ScriptToken::Create(id, lower, upper);
	}

	context->Error("Invalid string var slice operation - variable invalid or variable is not a string var");
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_Subscript_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const char *lStr = lh->GetString();
	const UInt32 lLen = StrLen(lStr);
	UInt32 idx = static_cast<int>(rh->GetNumber());
	if (idx < 0)
		idx += lLen;
	const UInt32 chr = (idx < lLen) ? lStr[idx] : 0;
	return ScriptToken::Create((const char *)&chr);
}

std::unique_ptr<ScriptToken> Eval_Subscript_String_Slice(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const Slice *srcSlice = rh->GetSlice();
	const std::string str = lh->GetString();

	if (!srcSlice || srcSlice->bIsString)
	{
		context->Error("Invalid string slice operation");
		return nullptr;
	}

	Slice slice(srcSlice);
	if (slice.m_lower < 0)
		slice.m_lower += str.length();
	if (slice.m_upper < 0)
		slice.m_upper += str.length();

	if (slice.m_lower >= 0 && slice.m_upper < str.length() && slice.m_lower <= slice.m_upper) // <=, not <, to support single-character slice
		return ScriptToken::Create(str.substr(slice.m_lower, slice.m_upper - slice.m_lower + 1));
	else
		return ScriptToken::Create("");
}

std::unique_ptr<ScriptToken> Eval_MemberAccess(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const ArrayID arrID = lh->GetArrayID();
	ArrayVar *arr = arrID ? g_ArrayMap.Get(arrID) : nullptr;

	if (!arr)
	{
		context->Error("Invalid array access - the array was not initialized. 2");
		return nullptr;
	}
	if (arr->KeyType() != kDataType_String)
	{
		context->Error("Invalid array access - expected numeric index, received string");
		return nullptr;
	}

	ArrayKey key(rh->GetString());
	return ScriptToken::Create(arrID, &key);
}
std::unique_ptr<ScriptToken> Eval_Slice_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	Slice slice(lh->GetString(), rh->GetString());
	return ScriptToken::Create(&slice);
}

std::unique_ptr<ScriptToken> Eval_Slice_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	Slice slice(lh->GetNumber(), rh->GetNumber());
	return ScriptToken::Create(&slice);
}

std::unique_ptr<ScriptToken> Eval_ToString_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(lh->GetString());
}

std::unique_ptr<ScriptToken> Eval_ToString_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	char buf[0x20];
	snprintf(buf, sizeof buf, "%g", lh->GetNumber());
	return ScriptToken::Create(static_cast<const char*>(buf));
}

std::unique_ptr<ScriptToken> Eval_ToString_Form(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(GetFullName(lh->GetTESForm()));
}

std::unique_ptr<ScriptToken> Eval_ToString_Array(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	const auto arrayId = lh->GetArrayID();
	const auto *arrayVar = g_ArrayMap.Get(arrayId);
	if (arrayVar)
		return ScriptToken::Create(arrayVar->GetStringRepresentation());
	return ScriptToken::Create("array ID " + std::to_string(arrayId) + " (invalid)");
}

std::unique_ptr<ScriptToken> Eval_ToNumber(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(lh->GetNumericRepresentation(false));
}

std::unique_ptr<ScriptToken> Eval_In(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	switch (lh->GetVariableType())
	{
	case Script::eVarType_Array:
	{
		const UInt32 iterID = g_ArrayMap.Create(kDataType_String, false, context->script->GetModIndex())->ID();

		if (auto* localVar = lh->GetScriptLocal())
		{
			ForEachContext con(rh->GetArrayID(), iterID, Variable{ localVar, Script::eVarType_Array });
			return ScriptToken::Create(&con);
		}
		break;
	}
	case Script::eVarType_String:
	{
		const UInt32 srcID = g_StringMap.Add(context->script->GetModIndex(), rh->GetString(), true, nullptr);
		if (ScriptLocal* var = lh->GetScriptLocal())
		{
			UInt32 iterID = static_cast<int>(var->data);
			StringVar* sv = g_StringMap.Get(iterID);
			if (!sv)
			{
				//iterID = g_StringMap.Add(context->script->GetModIndex(), "");
				iterID = AddStringVar("", *lh, *context, nullptr);
				var->data = static_cast<int>(iterID);
			}
			ForEachContext con(srcID, iterID, Variable{ var, Script::eVarType_String });
			return ScriptToken::Create(&con);
		}
		break;
	}
	case Script::eVarType_Ref:
	{
		TESForm *form = rh->GetTESForm();
		if (form && NOT_ID(form, BGSListForm))
		{
			if (form->refID == playerID)
				form = PlayerCharacter::GetSingleton();
			else form = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
		}
		if (form)
		{
			if (ScriptLocal* var = lh->GetScriptLocal())
			{
				ForEachContext con(reinterpret_cast<UInt32>(form), 0, Variable{ var, Script::eVarType_Ref });
				return ScriptToken::Create(&con);
			}
		}
		context->Error("Source is a base form (must be a reference)");
		return nullptr;
	}
	}
	context->Error("Unsupported variable type to iterate over (only array, string and ref variables supported for non-alt ForEach)");
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_Dereference(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	// this is a convenience thing.
	// simplifies access to iterator value in foreach loops e.g.
	//	foreach iter <- srcArray
	//		let someVar := iter["value"]
	//		let someVar := *iter		; equivalent, more readable

	// in other contexts, returns the first element of the array
	// useful for people using array variables to hold a single value of undetermined type

	const ArrayID arrID = lh->GetArrayID();
	if (!arrID)
	{
		context->Error("Invalid array access - the array was not initialized. 3");
		return nullptr;
	}

	if (ArrayVar *arr = g_ArrayMap.Get(arrID))
	{
		// is this a foreach iterator?
		if ((arr->Size() == 2) && arr->HasKey("key") && arr->HasKey("value"))
		{
			ArrayKey valueKey("value");
			return ScriptToken::Create(arrID, &valueKey);
		}

		const ArrayKey *firstKey;
		ArrayElement *elem;
		if (arr->GetFirstElement(&elem, &firstKey))
			return ScriptToken::Create(arrID, const_cast<ArrayKey *>(firstKey));
	}
	context->Error("Invalid array access - the array was not initialized.");
	return nullptr;
}

std::unique_ptr<ScriptToken> Eval_Box_Number(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	// the inverse operation of dereference: given a value of any type, wraps it in a single-element array
	// again, a convenience request
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	arr->SetElementNumber(0.0, lh->GetNumber());
	return ScriptToken::CreateArray(arr->ID());
}

std::unique_ptr<ScriptToken> Eval_Box_String(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	arr->SetElementString(0.0, lh->GetString());
	return ScriptToken::CreateArray(arr->ID());
}

std::unique_ptr<ScriptToken> Eval_Box_Form(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	TESForm *form = lh->GetTESForm();
	arr->SetElementFormID(0.0, form ? form->refID : 0);
	return ScriptToken::CreateArray(arr->ID());
}

std::unique_ptr<ScriptToken> Eval_Box_Array(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	arr->SetElementArray(0.0, lh->GetArrayID());
	return ScriptToken::CreateArray(arr->ID());
}

std::unique_ptr<ScriptToken> Eval_Pair(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	return ScriptToken::Create(lh, rh);
}

std::unique_ptr<ScriptToken> Eval_DotSyntax(OperatorType op, ScriptToken *lh, ScriptToken *rh, ExpressionEvaluator *context)
{
	auto *form = lh->GetTESForm();
	if (!rh->GetCommandInfo())
	{
		context->Error("Unable to resolve command");
		return nullptr;
	}
	if (!form)
	{
		context->Error("Attempting to call a command on a non-form or a NULL reference");
		return nullptr;
	}
	if (!form->GetIsReference())
	{
		context->Error("Attempting to call a function on a base object (this must be a reference)");
		return nullptr;
	}
	return context->ExecuteCommandToken(rh, static_cast<TESObjectREFR *>(form));
}

#define OP_HANDLER(x) x
#else
#define OP_HANDLER(x) NULL
#endif

// Operator Rules
OperationRule kOpRule_Comparison[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean, NULL},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Boolean, NULL},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_Boolean, NULL},

		{kTokenType_Number, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Comp_Number_Number)},
		{kTokenType_String, kTokenType_String, kTokenType_Boolean, OP_HANDLER(Eval_Comp_String_String)},
};

OperationRule kOpRule_Equality[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Boolean},
		{kTokenType_Ambiguous, kTokenType_Form, kTokenType_Boolean},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_Boolean},

		{kTokenType_Number, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Number)},
		{kTokenType_String, kTokenType_String, kTokenType_Boolean, OP_HANDLER(Eval_Eq_String)},
		{kTokenType_Form, kTokenType_Form, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Form)},
		{kTokenType_Form, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Form_Number), true},
		{kTokenType_Array, kTokenType_Array, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Array)}};

OperationRule kOpRule_Logical[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean},
		{kTokenType_Ambiguous, kTokenType_Boolean, kTokenType_Boolean},

		{kTokenType_Boolean, kTokenType_Boolean, kTokenType_RightToken, OP_HANDLER(Eval_Logical)},
};

OperationRule kOpRule_Addition[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_String},

		{kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Add_Number)},
		{kTokenType_String, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Add_String)},
};

OperationRule kOpRule_Arithmetic[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number},
		{kTokenType_Number, kTokenType_Ambiguous, kTokenType_Number},

		{kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Arithmetic)}};

OperationRule kOpRule_Multiply[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous},
		{kTokenType_String, kTokenType_Ambiguous, kTokenType_String},
		{kTokenType_Number, kTokenType_Ambiguous, kTokenType_Ambiguous},

		{kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Arithmetic)},
		{kTokenType_String, kTokenType_Number, kTokenType_String, OP_HANDLER(Eval_Multiply_String_Number)},
};

OperationRule kOpRule_Integer[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number},
		{kTokenType_Number, kTokenType_Ambiguous, kTokenType_Number},

		{kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Integer)},
};

OperationRule kOpRule_Assignment[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, true},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_String, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Array, kTokenType_Array, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Form, kTokenType_Form, NULL, true},
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_RefVar, kTokenType_Ambiguous, kTokenType_Form, NULL, true},
		{kTokenType_StringVar, kTokenType_Ambiguous, kTokenType_String, NULL, true},
		{kTokenType_ArrayVar, kTokenType_Ambiguous, kTokenType_Array, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, true},

		{kTokenType_AssignableString, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Assign_AssignableString), true},
		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Assign_Numeric), true},
		{kTokenType_StringVar, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Assign_String), true},
		{kTokenType_RefVar, kTokenType_Form, kTokenType_Form, OP_HANDLER(Eval_Assign_Form), true},
		{kTokenType_RefVar, kTokenType_Number, kTokenType_Form, OP_HANDLER(Eval_Assign_Form), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Assign_Global), true},
		{kTokenType_ArrayVar, kTokenType_Array, kTokenType_Array, OP_HANDLER(Eval_Assign_Array), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Assign_Elem_Number), true},
		{kTokenType_ArrayElement, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Assign_Elem_String), true},
		{kTokenType_ArrayElement, kTokenType_Form, kTokenType_Form, OP_HANDLER(Eval_Assign_Elem_Form), true},
		{kTokenType_ArrayElement, kTokenType_Array, kTokenType_Array, OP_HANDLER(Eval_Assign_Elem_Array), true}};

OperationRule kOpRule_PlusEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_StringVar, kTokenType_Ambiguous, kTokenType_String, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_String, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_PlusEquals_Number), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_PlusEquals_Elem_Number), true},
		{kTokenType_StringVar, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_PlusEquals_String), true},
		{kTokenType_ArrayElement, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_PlusEquals_Elem_String), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_PlusEquals_Global), true},
};

OperationRule kOpRule_MinusEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_MinusEquals_Number), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_MinusEquals_Elem_Number), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_MinusEquals_Global), true},
};

OperationRule kOpRule_TimesEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_TimesEquals), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_TimesEquals_Elem), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_TimesEquals_Global), true},
};

OperationRule kOpRule_DividedEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_DividedEquals), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_DividedEquals_Elem), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_DividedEquals_Global), true},
};

OperationRule kOpRule_ExponentEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_ExponentEquals), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_ExponentEquals_Elem), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_ExponentEquals_Global), true},
};

OperationRule kOpRule_HandleEquals[] =
	{
		{kTokenType_NumericVar, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_ArrayElement, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Global, kTokenType_Ambiguous, kTokenType_Number, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number, NULL, false},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number, NULL, true},

		{kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_HandleEquals), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_HandleEquals_Elem), true},
		{kTokenType_Global, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_HandleEquals_Global), true},
};

OperationRule kOpRule_Negation[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Number, NULL, true},

		{kTokenType_Number, kTokenType_Invalid, kTokenType_Number, OP_HANDLER(Eval_Negation), true},
};

OperationRule kOpRule_LogicalNot[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Boolean, NULL, true},

		{kTokenType_Boolean, kTokenType_Invalid, kTokenType_Boolean, OP_HANDLER(Eval_LogicalNot), true},
};

OperationRule kOpRule_LeftBracket[] =
	{
		{kTokenType_Array, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true},
		{kTokenType_String, kTokenType_Ambiguous, kTokenType_String, NULL, true},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_ArrayElement, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Ambiguous, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Slice, kTokenType_Ambiguous, NULL, true},

		{kTokenType_Array, kTokenType_Number, kTokenType_ArrayElement, OP_HANDLER(Eval_Subscript_Array_Number), true},
		{kTokenType_Array, kTokenType_String, kTokenType_ArrayElement, OP_HANDLER(Eval_Subscript_Array_String), true},
		{kTokenType_ArrayElement, kTokenType_Number, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_Elem_Number), true},
		{kTokenType_StringVar, kTokenType_Number, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_StringVar_Number), true},
		{kTokenType_ArrayElement, kTokenType_Slice, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_Elem_Slice), true},
		{kTokenType_StringVar, kTokenType_Slice, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_StringVar_Slice), true},
		{kTokenType_String, kTokenType_Number, kTokenType_String, OP_HANDLER(Eval_Subscript_String), true},
		{kTokenType_Array, kTokenType_Slice, kTokenType_Array, OP_HANDLER(Eval_Subscript_Array_Slice), true},
		{kTokenType_String, kTokenType_Slice, kTokenType_String, OP_HANDLER(Eval_Subscript_String_Slice), true}};

OperationRule kOpRule_MemberAccess[] =
	{
		{kTokenType_Array, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_ArrayElement, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true},

		{kTokenType_Array, kTokenType_String, kTokenType_ArrayElement, OP_HANDLER(Eval_MemberAccess), true}};

OperationRule kOpRule_Slice[] =
	{
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Slice},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Slice},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_Slice},

		{kTokenType_String, kTokenType_String, kTokenType_Slice, OP_HANDLER(Eval_Slice_String)},
		{kTokenType_Number, kTokenType_Number, kTokenType_Slice, OP_HANDLER(Eval_Slice_Number)},
};

OperationRule kOpRule_In[] =
	{
		{kTokenType_ArrayVar, kTokenType_Ambiguous, kTokenType_ForEachContext, NULL, true},

		{kTokenType_ArrayVar, kTokenType_Array, kTokenType_ForEachContext, OP_HANDLER(Eval_In), true},
		{kTokenType_StringVar, kTokenType_String, kTokenType_ForEachContext, OP_HANDLER(Eval_In), true},
		{kTokenType_RefVar, kTokenType_Form, kTokenType_ForEachContext, OP_HANDLER(Eval_In), true},
};

OperationRule kOpRule_ToString[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_String, NULL, true},

		{kTokenType_String, kTokenType_Invalid, kTokenType_String, OP_HANDLER(Eval_ToString_String), true},
		{kTokenType_Number, kTokenType_Invalid, kTokenType_String, OP_HANDLER(Eval_ToString_Number), true},
		{kTokenType_Form, kTokenType_Invalid, kTokenType_String, OP_HANDLER(Eval_ToString_Form), true},
		{kTokenType_Array, kTokenType_Invalid, kTokenType_String, OP_HANDLER(Eval_ToString_Array), true},
};

OperationRule kOpRule_ToNumber[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Number, NULL, true},

		{kTokenType_String, kTokenType_Invalid, kTokenType_Number, OP_HANDLER(Eval_ToNumber), true},
		{kTokenType_Number, kTokenType_Invalid, kTokenType_Number, OP_HANDLER(Eval_ToNumber), true},
};

OperationRule kOpRule_Dereference[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_ArrayElement, NULL, true},

		{kTokenType_Array, kTokenType_Invalid, kTokenType_ArrayElement, OP_HANDLER(Eval_Dereference), true},
};

OperationRule kOpRule_Box[] =
	{
		{kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Array, NULL, true},

		{kTokenType_Number, kTokenType_Invalid, kTokenType_Array, OP_HANDLER(Eval_Box_Number), true},
		{kTokenType_String, kTokenType_Invalid, kTokenType_Array, OP_HANDLER(Eval_Box_String), true},
		{kTokenType_Form, kTokenType_Invalid, kTokenType_Array, OP_HANDLER(Eval_Box_Form), true},
		{kTokenType_Array, kTokenType_Invalid, kTokenType_Array, OP_HANDLER(Eval_Box_Array), true},
};

OperationRule kOpRule_MakePair[] =
	{
		{kTokenType_String, kTokenType_Ambiguous, kTokenType_Pair, NULL, true},
		{kTokenType_Number, kTokenType_Ambiguous, kTokenType_Pair, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Number, kTokenType_Pair, NULL, true},
		{kTokenType_Ambiguous, kTokenType_String, kTokenType_Pair, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Array, kTokenType_Pair, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Form, kTokenType_Pair, NULL, true},
		{kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Pair, NULL, true},

		{kTokenType_String, kTokenType_Number, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_String, kTokenType_String, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_String, kTokenType_Form, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_String, kTokenType_Array, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_Number, kTokenType_Number, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_Number, kTokenType_String, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_Number, kTokenType_Form, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
		{kTokenType_Number, kTokenType_Array, kTokenType_Pair, OP_HANDLER(Eval_Pair), true},
};

OperationRule kOpRule_Dot[] =
{
		{kTokenType_Form, kTokenType_Command, kTokenType_Command, OP_HANDLER(Eval_DotSyntax), true}, // Form is used instead of Ref since commands return Form - no way to check which
		{kTokenType_ArrayElement, kTokenType_Command, kTokenType_Command, OP_HANDLER(Eval_DotSyntax), true},
#if EDITOR
		{kTokenType_Command, kTokenType_Command, kTokenType_Command, OP_HANDLER(Eval_DotSyntax), true}, // kTokenType_Command is used as token type when kRetnType_Default is returned
#endif
};

OperationRule kOpRule_BitwiseNot[] = {
		{kTokenType_Number, kTokenType_Invalid, kTokenType_Number, OP_HANDLER(Eval_BitwiseNot), true}
};

// Operator definitions
#define OP_RULES(x) SIZEOF_ARRAY(kOpRule_##x, OperationRule), kOpRule_##x

Operator s_operators[] =
	{
		{2, ":=", 2, kOpType_Assignment, OP_RULES(Assignment)},
		{5, "||", 2, kOpType_LogicalOr, OP_RULES(Logical)},
		{7, "&&", 2, kOpType_LogicalAnd, OP_RULES(Logical)},

		{9, ":", 2, kOpType_Slice, OP_RULES(Slice)},
		{13, "==", 2, kOpType_Equals, OP_RULES(Equality)},
		{13, "!=", 2, kOpType_NotEqual, OP_RULES(Equality)},

		{15, ">", 2, kOpType_GreaterThan, OP_RULES(Comparison)},
		{15, "<", 2, kOpType_LessThan, OP_RULES(Comparison)},
		{15, ">=", 2, kOpType_GreaterOrEqual, OP_RULES(Comparison)},
		{15, "<=", 2, kOpType_LessOrEqual, OP_RULES(Comparison)},

		{16, "|", 2, kOpType_BitwiseOr, OP_RULES(Integer)}, // ** higher precedence than in C++
		{17, "&", 2, kOpType_BitwiseAnd, OP_RULES(Integer)},

		{18, "<<", 2, kOpType_LeftShift, OP_RULES(Integer)},
		{18, ">>", 2, kOpType_RightShift, OP_RULES(Integer)},

		{19, "+", 2, kOpType_Add, OP_RULES(Addition)},
		{19, "-", 2, kOpType_Subtract, OP_RULES(Arithmetic)},

		{21, "*", 2, kOpType_Multiply, OP_RULES(Multiply)},
		{21, "/", 2, kOpType_Divide, OP_RULES(Arithmetic)},
		{21, "%", 2, kOpType_Modulo, OP_RULES(Integer)},

		{23, "^", 2, kOpType_Exponent, OP_RULES(Arithmetic)}, // exponentiation
		{25, "-", 1, kOpType_Negation, OP_RULES(Negation)},	  // unary minus in compiled script

		{27, "!", 1, kOpType_LogicalNot, OP_RULES(LogicalNot)},

		{80, "(", 0, kOpType_LeftParen, 0, nullptr},
		{80, ")", 0, kOpType_RightParen, 0, nullptr},

		{90, "[", 2, kOpType_LeftBracket, OP_RULES(LeftBracket)}, // functions both as paren and operator
		{90, "]", 0, kOpType_RightBracket, 0, nullptr},			  // functions only as paren

		{2, "<-", 2, kOpType_In, OP_RULES(In)},				// 'foreach iter <- arr'
		{25, "$", 1, kOpType_ToString, OP_RULES(ToString)}, // converts operand to string

		{2, "+=", 2, kOpType_PlusEquals, OP_RULES(PlusEquals)},
		{2, "*=", 2, kOpType_TimesEquals, OP_RULES(TimesEquals)},
		{2, "/=", 2, kOpType_DividedEquals, OP_RULES(DividedEquals)},
		{2, "^=", 2, kOpType_ExponentEquals, OP_RULES(ExponentEquals)},
		{2, "-=", 2, kOpType_MinusEquals, OP_RULES(MinusEquals)},

		{25, "#", 1, kOpType_ToNumber, OP_RULES(ToNumber)},

		{25, "*", 1, kOpType_Dereference, OP_RULES(Dereference)},

		{90, "->", 2, kOpType_MemberAccess, OP_RULES(MemberAccess)},
		{3, "::", 2, kOpType_MakePair, OP_RULES(MakePair)},
		{25, "&", 1, kOpType_Box, OP_RULES(Box)},

		{91, "{", 0, kOpType_LeftBrace, 0, nullptr},
		{91, "}", 0, kOpType_RightBrace, 0, nullptr},
		{90, ".", 2, kOpType_Dot, OP_RULES(Dot)},

		{2, "|=", 2, kOpType_BitwiseOrEquals, OP_RULES(HandleEquals)},
		{2, "&=", 2, kOpType_BitwiseAndEquals, OP_RULES(HandleEquals)},
		{2, "%=", 2, kOpType_ModuloEquals, OP_RULES(HandleEquals)},
		{27, "~", 1, kOpType_BitwiseNot, OP_RULES(BitwiseNot)},

};

STATIC_ASSERT(SIZEOF_ARRAY(s_operators, Operator) == kOpType_Max);

const char *OpTypeToSymbol(OperatorType op)
{
	if (op < kOpType_Max)
		return s_operators[op].symbol;
	return "<unknown>";
}

#if RUNTIME

// ExpressionEvaluator

bool ExpressionEvaluator::Active()
{
	return ThreadLocalData::Get().expressionEvaluator != nullptr;
}

ExpressionEvaluator &ExpressionEvaluator::Get()
{
	return *ThreadLocalData::Get().expressionEvaluator;
}

void ExpressionEvaluator::ToggleErrorSuppression(bool bSuppress)
{
	if (bSuppress)
	{
		m_flags.Clear(kFlag_ErrorOccurred);
		m_flags.Set(kFlag_SuppressErrorMessages);
	}
	else
		m_flags.Clear(kFlag_SuppressErrorMessages);
}

static UnorderedSet<const char *> s_warnedMods; // show corner message only once per mod script error



void ExpressionEvaluator::Error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	this->vError(fmt, args);

	va_end(args);
}

void ExpressionEvaluator::vError(const char* fmt, va_list fmtArgs)
{
	m_flags.Set(kFlag_ErrorOccurred);

	if (m_flags.IsSet(kFlag_SuppressErrorMessages))
		return;

	char errorMsg[0x400];
	vsprintf_s(errorMsg, 0x400, fmt, fmtArgs);

	this->errorMessages.emplace_back(errorMsg);
}

void ExpressionEvaluator::PrintStackTrace()
{
	std::stack<const ExpressionEvaluator *> stackCopy;
	char output[0x100];

	auto eval = this;
	while (eval)
	{
		CommandInfo *cmd = eval->GetCommand();
		sprintf_s(output, sizeof(output), "  %s @%04X script %08X", cmd ? cmd->longName : "<unknown>", eval->m_baseOffset, eval->script->refID);
		_MESSAGE(output);
		Console_Print(output);

		eval = eval->m_parent;
	}
}

#define OPERAND_CONVERT(x) x
#else
#define OPERAND_CONVERT(x) NULL
#endif

#if RUNTIME
thread_local SmallObjectsAllocator::FastAllocator<ExpressionEvaluator, 4> g_pluginExpEvalAllocator;

bool BasicTokenToElem(ScriptToken* token, ArrayElement& elem)
{
	auto const basicToken = token->ToBasicToken();
	if (!basicToken)
		return false;

	bool bResult = true;

	if (basicToken->CanConvertTo(kTokenType_Number))
		elem.SetNumber(basicToken->GetNumber());
	else if (basicToken->CanConvertTo(kTokenType_String))
		elem.SetString(basicToken->GetString());
	else if (basicToken->CanConvertTo(kTokenType_Form))
		elem.SetFormID(basicToken->GetFormID());
	else if (basicToken->CanConvertTo(kTokenType_Array))
		elem.SetArray(basicToken->GetArrayID());
	else
		bResult = false;

	return bResult;
}

void *__stdcall ExpressionEvaluatorCreate(COMMAND_ARGS)
{
	ExpressionEvaluator *expEval = g_pluginExpEvalAllocator.Allocate();
	if (expEval)
		new (expEval) ExpressionEvaluator(PASS_COMMAND_ARGS);
	return expEval;
}

void __fastcall ExpressionEvaluatorDestroy(void *expEval)
{
	static_cast<ExpressionEvaluator *>(expEval)->~ExpressionEvaluator();
	g_pluginExpEvalAllocator.Free(expEval);
}

bool __fastcall ExpressionEvaluatorExtractArgs(void *expEval)
{
	return static_cast<ExpressionEvaluator *>(expEval)->ExtractArgs();
}

UInt8 __fastcall ExpressionEvaluatorGetNumArgs(void *expEval)
{
	return static_cast<ExpressionEvaluator *>(expEval)->NumArgs();
}

PluginScriptToken *__fastcall ExpressionEvaluatorGetNthArg(void *expEval, UInt32 argIdx)
{
	return reinterpret_cast<PluginScriptToken *>(static_cast<ExpressionEvaluator *>(expEval)->Arg(argIdx));
}

void __fastcall ExpressionEvaluatorSetExpectedReturnType(void* expEval, UInt8 retnType)
{
	static_cast<ExpressionEvaluator*>(expEval)->ExpectReturnType(static_cast<CommandReturnType>(retnType));
}

void __fastcall ExpressionEvaluatorAssignCommandResultFromElement(void* expEval, NVSEArrayVarInterface::Element& result)
{
	auto const eval = static_cast<ExpressionEvaluator*>(expEval);
	eval->AssignAmbiguousResult(result, result.GetReturnType());
}

bool __fastcall ExpressionEvaluatorExtractArgsV(void* expEval, va_list list)
{
	auto const eval = static_cast<ExpressionEvaluator*>(expEval);
	return eval->ExtractArgsV(list);
}

void __fastcall ExpressionEvaluatorReportError(void* expEval, const char* fmt, va_list fmtArgs)
{
	auto const eval = static_cast<ExpressionEvaluator*>(expEval);
	eval->vError(fmt, fmtArgs);
}
#endif

// ExpressionParser

// Not particularly fond of this but it's become necessary to distinguish between a parser which is parsing part of a larger
// expression and one parsing an entire script line.
// Threading not a concern in script editor; ExpressionParser not used at run-time.
static SInt32 s_parserDepth = 0;

ExpressionParser::ExpressionParser(ScriptBuffer *scriptBuf, ScriptLineBuffer *lineBuf)
	: m_scriptBuf(scriptBuf), m_lineBuf(lineBuf), m_len(m_lineBuf->paramTextLen ? m_lineBuf->paramTextLen : strlen(m_lineBuf->paramText)), m_numArgsParsed(0)
{
	ASSERT(s_parserDepth >= 0);
	s_parserDepth++;
	memset(m_argTypes, kTokenType_Invalid, sizeof(m_argTypes));

	if (g_currentScriptStack.empty())
#if EDITOR
		m_script = nullptr;
#else
		m_script = m_scriptBuf->currentScript;
#endif
	else
		m_script = g_currentScriptStack.top();
}

ExpressionParser::~ExpressionParser()
{
	ASSERT(s_parserDepth > 0);
	s_parserDepth--;
	if (s_parserDepth == 0)
	{
		for (const auto& key : g_lambdaParentScriptMap | std::views::keys)
			key->Delete();
		g_lambdaParentScriptMap.clear();
	}
}

bool ExpressionParser::ParseArgs(ParamInfo *params, UInt32 numParams, bool bUsesNVSEParamTypes, bool parseWholeLine)
{
	// reserve space for UInt8 numargs at beginning of compiled code
	UInt8 *numArgsPtr = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	m_lineBuf->dataOffset += 1;

	char ch;
	UInt32 argsEndPos = m_len;
	if (numParams > 1) {
		// Comparison operators should never be parsed in function params unless they are enclosed in parens
		// if GetINIFltOrCreateC "Edge Settings:bCharismaAffectsReputationChange" "EDGE TTW Config.ini" == 0
		//                       |--------------------------------------------------------------------| <- Should be considered for params
		// or
		// if GetINIFltOrCreateC "Edge Settings:bCharismaAffectsReputationChange" "EDGE TTW Config.ini" (4 / 2 == 2) == 0
		//                       |---------------------------------------------------------------------------------|
		int depth = 0;
		bool instr = 0;
		int i = 0;
		while ((ch = Peek(i++)) && i < argsEndPos) {
			if (ch == '(' && !instr) {
				depth++;
			}
			else if (ch == ')' && !instr) {
				depth--;
			}
			else if (ch == '"') {
				instr = !instr;
			}
			else if (depth == 0 && !instr) {
				const auto oldOffset = Offset();
				Offset() = i;
				const auto op = ParseOperator(true, false);
				// ==, !=, <=, >=, ||, && - I consider all of these boundaries for parameters and they should never be parsed with parameters
				if (op != nullptr && ((op->type >= kOpType_Equals && op->type <= kOpType_LessOrEqual) || op->type == kOpType_LogicalAnd || op->type == kOpType_LogicalOr)) {
					Offset() = oldOffset;

					argsEndPos = i - 1;
					while (isspace(static_cast<unsigned char>(Peek(argsEndPos)))) {
						argsEndPos--;
					}

					break;
				}
				Offset() = oldOffset;
			}
			// Left the scope of command
			else if (depth == -1) {
				break;
			}
		}
	}

	// see if args are enclosed in {braces}, if so don't parse beyond closing brace
	while ((ch = Peek(Offset())))
	{
		if (!isspace(static_cast<unsigned char>(ch)))
			break;

		Offset()++;
	}
	UInt32 offset = Offset();
	UInt32 endOffset = argsEndPos;

	if (!HandleMacros())
		return false;

	if ('{' == Peek(Offset())) // restrict parsing to the enclosed text
		while ((ch = Peek(Offset())))
		{
			if (ch == '{')
			{
				offset++;
				Offset()++;

				const UInt32 bracketEndPos = MatchOpenBracket(&s_operators[kOpType_LeftBrace]);
				if (bracketEndPos == -1)
				{
					Message(kError_MismatchedBrackets);
					return false;
				}
				else if (bracketEndPos == -2)
				{
					Message(kError_MismatchedQuotes);
					return false;
				}
				else
					Offset() = bracketEndPos;
			}
			else if (ch == '}')
			{
				endOffset = Offset();
				argsEndPos = endOffset - 1;
				break;
			}
			else
				Offset()++;
		}
	Offset() = offset;

	while ((ch = Peek(Offset())))
	{
		if (!isspace(static_cast<unsigned char>(ch)))
			break;

		Offset()++;
	}

	while (m_numArgsParsed < numParams && Offset() < argsEndPos)
	{
		const auto argType = ParseArgument(argsEndPos);
		if (argType == kTokenType_Invalid)
			return false;
		if (argType == kTokenType_Empty)
			// reached end of args
			break;
		// is arg of expected type(s)?
		if (!ValidateArgType(static_cast<ParamType>(params[m_numArgsParsed].typeID), argType, bUsesNVSEParamTypes, g_scriptCommands.GetByOpcode(m_lineBuf->cmdOpcode)))
		{
#if RUNTIME
			ShowCompilerError(m_lineBuf, "Invalid expression for parameter %d. Expected %s.", m_numArgsParsed + 1, params[m_numArgsParsed].typeStr);
#else
			ShowCompilerError(m_scriptBuf, "Invalid expression for parameter %d. Expected %s.", m_numArgsParsed + 1, params[m_numArgsParsed].typeStr);
#endif
			return false;
		}
		m_argTypes[m_numArgsParsed++] = argType;
	}

	if (Offset() < argsEndPos && s_parserDepth == 1 && parseWholeLine) // some leftover stuff in text
	{
		// when parsing commands as args to other commands or components of larger expressions, we expect to have some leftovers
		// so this check is not necessary unless we're finished parsing the entire line
		Message(kError_TooManyArgs);
		return false;
	}
	if (parseWholeLine)
		Offset() = endOffset + 1;

	// did we get all required args?
	UInt32 numExpectedArgs = 0;
	for (UInt32 i = 0; i < numParams && !params[i].isOptional; i++)
		numExpectedArgs++;

	if (numExpectedArgs > m_numArgsParsed)
	{
		const ParamInfo *missingParam = &params[m_numArgsParsed];
		Message(kError_MissingParam, missingParam->typeStr, m_numArgsParsed + 1);
		return false;
	}

	*numArgsPtr = m_numArgsParsed;
	return true;
}

bool ExpressionParser::ValidateArgType(ParamType paramType, Token_Type argType, bool bIsNVSEParam, CommandInfo *cmdInfo)
{
	if (bIsNVSEParam)
	{
		bool bTypesMatch = false;
		if (paramType == kNVSEParamType_NoTypeCheck)
			bTypesMatch = true;
		else // ###TODO: this could probably done with bitwise AND much more efficiently
		{
			for (UInt32 i = 0; i < kTokenType_Max; i++)
			{
				if (paramType & (1 << i))
				{
					const auto type = static_cast<Token_Type>(i);
					if (CanConvertOperand(argType, type))
					{
						bTypesMatch = true;
						break;
					}
				}
			}
		}

		return bTypesMatch;
	}
	else
	{
		// vanilla paramInfo
		if (argType == kTokenType_Ambiguous)
		{
			// we'll find out at run-time
			return true;
		}
		
		switch (paramType)
		{
		case kParamType_String:
		case kParamType_Axis:
		case kParamType_Sex:
			return CanConvertOperand(argType, kTokenType_String);
		case kParamType_Float:
		case kParamType_Double:
		case kParamType_Integer:
		case kParamType_QuestStage:
		case kParamType_CrimeType:
			// string var included here b/c old sv_* cmds take strings as integer IDs
			if (argType != kTokenType_StringVar && CanConvertOperand(argType, kTokenType_String))
			{
				//auto* cmdInfo = g_scriptCommands.GetByOpcode(m_lineBuf->cmdOpcode);
				if (cmdInfo && (std::string_view(cmdInfo->longName).starts_with("sv_") || cmdInfo->params == kParams_FormatString || cmdInfo->numParams >= 20)) // only allow this for old sv commands that take int
					return true;
			}
			return CanConvertOperand(argType, kTokenType_Number) ||
				   CanConvertOperand(argType, kTokenType_StringVar) ||
				   CanConvertOperand(argType, kTokenType_Variable);
		case kParamType_AnimationGroup:
		case kParamType_ActorValue:
			// we accept string or int for this
			// at run-time convert string to int if necessary and possible
			return CanConvertOperand(argType, kTokenType_String) || CanConvertOperand(argType, kTokenType_Number);
		case kParamType_VariableName:
		case kParamType_FormType:
			// used only by condition functions
			return false;
		case kParamType_MagicEffect:
			// alleviate some of the annoyance of this param type by accepting string, form, or integer effect code
			return CanConvertOperand(argType, kTokenType_String) || CanConvertOperand(argType, kTokenType_Number) ||
				   CanConvertOperand(argType, kTokenType_Form);
		case kParamType_Array:
			return CanConvertOperand(argType, kTokenType_Array);
		case kParamType_ScriptVariable:
			return CanConvertOperand(argType, kTokenType_Variable);

		default:
			// all the rest are TESForm of some sort or another
			return CanConvertOperand(argType, kTokenType_Form);
		}
	}
}

// User function definitions include a ParamInfo array defining the args
// When parsing a function call we match the passed args to the function definition
// However if using a ref variable like a function pointer we can't type-check the args
static ParamInfo kParams_DefaultUserFunctionParams[] =
	{
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
		{"argument", kNVSEParamType_NoTypeCheck, 1},
};

// records version of bytecode representation to avoid problems if representation changes later
static const UInt8 kUserFunction_Version = 1;

// Also returns param types if a variable was declared inside the param list.
bool GetUserFunctionParamNames(const std::string &scriptText, std::vector<std::string> &out)
{
	ScriptTokenizer tokenizer(scriptText);
	while (tokenizer.TryLoadNextLine())
	{
		auto lineText = tokenizer.GetLineText();
		if (lineText.size() <= 7) continue;
		auto lineToken = std::string(tokenizer.GetNextLineToken()); // get the first token in the line
		// use std::string for null terminator
		if (!StrCompare(lineToken.c_str(), "begin"))
		{
			return GetUserFunctionParamTokensFromLine(lineText, out);
		}
	}
	return false;
}

bool ExpressionParser::GetUserFunctionParams(
	const std::vector<std::string> &paramNames, 
	std::vector<UserFunctionParam> &outParams, 
	Script::VarInfoList *varList, 
	const std::string &fullScriptText, 
	Script *script) const
{
	auto lastVarType = Script::eVarType_Invalid;
	for (const auto &token : paramNames)
	{
		bool wasTypeFoundInParamList = false;
		if (lastVarType != Script::eVarType_Invalid)
		{
			CreateVariable(token, lastVarType);
			wasTypeFoundInParamList = true;
		}
		else if (const auto iter = ra::find_if(g_validVariableTypeNames, _L(const char* typeName, _stricmp(typeName, token.c_str()) == 0));
			iter != std::end(g_validVariableTypeNames))
		{
			lastVarType = VariableTypeNameToType(*iter);
			continue;
		}
		VariableInfo *varInfo = varList->GetVariableByName(token.c_str());
		if (!varInfo)
			return false;

		Script::VariableType varType;
		if (!wasTypeFoundInParamList) // optimization
		{
			varType = GetDeclaredVariableType(token.c_str(), fullScriptText.c_str(), script);
			if (varType == Script::eVarType_Invalid)
			{
				return false;
			}
		}
		else
		{
			varType = lastVarType;
			lastVarType = Script::eVarType_Invalid;
		}

		// make sure user isn't trying to use a var more than once as a param
		for (UInt32 i = 0; i < outParams.size(); i++)
			if (outParams[i].m_varIdx == varInfo->idx)
				return false;

		outParams.emplace_back(UserFunctionParam(varInfo->idx, varType));
	}
	if (lastVarType != Script::eVarType_Invalid)
		return false;
	return true;
}

// index into array with Script::eVarType_XXX
static ParamInfo kDynamicParams[] =
	{
		{"float", kNVSEParamType_Number, 0},
		{"integer", kNVSEParamType_Number, 0},
		{"string", kNVSEParamType_String, 0},
		{"array", kNVSEParamType_Array, 0},
		{"object", kNVSEParamType_Form, 0},
};

DynamicParamInfo::DynamicParamInfo(const std::vector<UserFunctionParam> &params)
{
	m_numParams = min(kMaxUdfParams, params.size());
	for (ParamSize_t i = 0; i < m_numParams; i++)
		m_paramInfo[i] = kDynamicParams[params[i].GetType()];
}

bool ExpressionParser::ParseUserFunctionParameters(std::vector<UserFunctionParam> &out, const std::string &funcScriptText, Script::VarInfoList *funcScriptVars, Script *script) const
{
	std::vector<std::string> funcParamNames;
	if (!GetUserFunctionParamNames(funcScriptText, funcParamNames))
	{
		Message(kError_UserFunctionParamsUndefined);
		return false;
	}

	if (!GetUserFunctionParams(funcParamNames, out, funcScriptVars, funcScriptText, script))
	{
		Message(kError_UserFunctionParamsUndefined);
		return false;
	}
	return true;
}

bool ExpressionParser::ParseUserFunctionCall()
{
	// bytecode (version 0):
	//	UInt8		version
	//	RefToken	function script
	//	UInt8		numArgs			<- written by ParseArgs()
	//	ScriptToken	args[numArgs]	<- as above

	// bytecode (version 1, 0019 beta 1):
	//	UInt8		version
	//	Expression	function script	<- modified to accept e.g. scripts stored in arrays
	//	UInt8		numArgs
	//	ScriptToken args[numArgs]

	// write version
	m_lineBuf->WriteByte(kUserFunction_Version);

	const UInt32 paramLen = strlen(m_lineBuf->paramText);

	// parse function object
	while (isspace(static_cast<unsigned char>(Peek())))
	{
		Offset()++;
		if (Offset() >= paramLen)
		{
			Message(kError_CantParse);
			return false;
		}
	}

	UInt32 peekLen = 0;
	const auto funcForm = PeekOperand(peekLen);

	TESForm* form;
	Script* funcScript{};
	if (funcForm && (form = funcForm->GetTESForm()) && (funcScript = DYNAMIC_CAST(form, TESForm, Script)))
	{
		// Script editor ID or lambda
		auto* savedLenPtr = reinterpret_cast<UInt16*>(m_lineBuf->dataBuf + m_lineBuf->dataOffset);
		const UInt16 startingOffset = m_lineBuf->dataOffset;
		m_lineBuf->dataOffset += 2;
		funcForm->Write(m_lineBuf);
		Offset() += peekLen;
		*savedLenPtr = m_lineBuf->dataOffset - startingOffset;
	}
	else
	{
		// array element, result of function call or ref var
		const auto type = ParseArgument(m_len);
		if (!CanConvertOperand(type, kTokenType_Form))
		{
			Message(kError_ExpectedUserFunction);
			return false;
		}
	}

	// skip any commas between function name and args
	// silly thing to have to fix but whatever
	while ((isspace(static_cast<unsigned char>(Peek())) || Peek() == ',') && Offset() < paramLen)
		Offset()++;

	// determine paramInfo for function and parse the args
	// lookup paramInfo from Script
	// if recursive call, look up from ScriptBuffer instead
	if (funcScript && funcScript->text)
	{
		const char *funcScriptText = funcScript->text;
		Script::VarInfoList *funcScriptVars = &funcScript->varList;
		auto *script = funcScript;
		if (!StrCompare(GetEditorID(funcScript), m_scriptBuf->scriptName.m_data))
		{
			funcScriptText = m_scriptBuf->scriptText;
			funcScriptVars = &m_scriptBuf->vars;
			script = m_script;
		}

		std::vector<UserFunctionParam> funcParams;
		if (!ParseUserFunctionParameters(funcParams, funcScriptText, funcScriptVars, script))
			return false;

		DynamicParamInfo dynamicParams(funcParams);

		ExpressionParser parser(m_scriptBuf, m_lineBuf); // created a new one instead of using this since since m_numArgsParsed is > 0 in Cmd_CallAfter_Parse
		return parser.ParseArgs(dynamicParams.Params(), dynamicParams.NumParams());
	}

	// using refVar as function pointer, use default params
	// or in-game console, array elem or function result
	ParamInfo *params = kParams_DefaultUserFunctionParams;
	constexpr UInt32 numParams = NUM_PARAMS(kParams_DefaultUserFunctionParams);

	return ParseArgs(params, numParams);
}

bool ExpressionParser::ParseUserFunctionDefinition() const
{
	// syntax: Begin Function arg1, arg2, ... arg10 where args are local variable names
	// requires:
	//	-all script variables declared before Begin Function block
	//	-only one script block (function definition) in script

	// bytecode (versions 0 and 1):
	//	UInt8				version
	//	UInt8				numParams
	//	UserFunctionParam	params[numParams]			{ UInt16 varIdx; UInt8 varType }
	//	UInt8				numLocalArrayVars
	//	UInt16				localArrayVarIndexes[numLocalArrayVars]

	// write version
	m_lineBuf->WriteByte(kUserFunction_Version);

	// parse parameter list
	std::vector<UserFunctionParam> params;
	if (!ParseUserFunctionParameters(params, m_scriptBuf->scriptText, &m_scriptBuf->vars, m_script))
		return false;

	// write param info
	m_lineBuf->WriteByte(params.size());
	for (auto iter = params.begin(); iter != params.end(); ++iter)
	{
		m_lineBuf->Write16(iter->m_varIdx);
		m_lineBuf->WriteByte(iter->GetType());
	}

	// determine which if any local variables must be destroyed on function exit (string and array vars)
	// ensure no variables declared after function definition
	// ensure only one Begin block in script
	UInt32 offset = 0;
	bool bFoundBegin = false;
	UInt32 endPos = 0;
	std::string scrText = m_scriptBuf->scriptText;

	const std::vector<UInt16> arrayVarIndexes;
	// deprecated, automatic garbage collection in place since xnvse 6
#if 0
	std::string lineText;
	Tokenizer lines(scrText.c_str(), "\r\n");
	while (lines.NextToken(lineText) != -1)
	{
		Tokenizer tokens(lineText.c_str(), " \t\r\n\0");
		std::string token;
		if (tokens.NextToken(token) != -1)
		{
			if (!StrCompare(token.c_str(), "begin"))
			{
				if (bFoundBegin)
				{
					Message(kError_UserFunctionContainsMultipleBlocks);
					return false;
				}

				bFoundBegin = true;
			}
			else if (!StrCompare(token.c_str(), "array_var"))
			{
				if (bFoundBegin)
				{
					Message(kError_UserFunctionVarsMustPrecedeDefinition);
					return false;
				}

				tokens.NextToken(token);	// variable name
				VariableInfo* varInfo = m_scriptBuf->vars.GetVariableByName(token.c_str());
				if (!varInfo)		// how did this happen?
				{
					_MESSAGE("GetVariableByName() returned NULL in ExpressionParser::ParseUserFunctionDefinition()");
					return false;
				}

				arrayVarIndexes.push_back(varInfo->idx);
			}
			else if (bFoundBegin)
			{
				if (!StrCompare(token.c_str(), "string_var") || !StrCompare(token.c_str(), "float") || !StrCompare(token.c_str(), "int") ||
					!StrCompare(token.c_str(), "ref") || !StrCompare(token.c_str(), "reference") || !StrCompare(token.c_str(), "short") ||
					!StrCompare(token.c_str(), "long"))
				{
					Message(kError_UserFunctionVarsMustPrecedeDefinition);
					return false;
				}
			}
		}
	}
#endif
	// write destructible var info
	m_lineBuf->WriteByte(arrayVarIndexes.size());
	for (UInt32 i = 0; i < arrayVarIndexes.size(); i++)
	{
		m_lineBuf->Write16(arrayVarIndexes[i]);
	}

	return true;
}

Token_Type ExpressionParser::Parse()
{
	UInt8 *dataStart = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	m_lineBuf->dataOffset += 2;

	const Token_Type result = ParseSubExpression(m_len);

	*((UInt16 *)dataStart) = (m_lineBuf->dataBuf + m_lineBuf->dataOffset) - dataStart;

	return result;
}

static ErrOutput::Message s_expressionErrors[] =
	{
		// errors
		{"Could not parse this line."},
		{"Too many operators."},
		{
			"Too many operands.",
		},
		{"Mismatched brackets."},
		{"Invalid operands for operator %s"},
		{"Mismatched quotes."},
		{"Left of dot must be quest or persistent reference."},
		{"Unknown variable '%s'."},
		{"Expected string variable after '$'."},
		{"Cannot access variable on unscripted object '%s'."},
		{"More args provided than expected by function or command."},
		{"Commands '%s' must be called on a reference."},
		{"Missing required parameter '%s' for parameter #'%d'."},
		{"Missing argument list for user-defined function '%s'."},
		{"Expected user function."},
		{"User function scripts may only contain one script block."},
		{"Variables in user function scripts must precede function definition."},
		{"Could not parse user function parameter list in function definition.\nMay be caused by undefined variable,  missing {brackets}, or attempt to use a single variable to hold more than one parameter."},
		{"Expected string literal."},
		{"Too much compiled data for a single line/multi-line parenthesis expression."},

		// warnings
		{"Unquoted argument '%s' will be treated as string by default. Check spelling if a form or variable was intended.", true},
		{"Usage of ref variables as pointers to user-defined functions prevents type-checking of function arguments. Make sure the arguments provided match those expected by the function being called.", true},

		// default
		{"Undefined error."}};

ErrOutput::Message *ExpressionParser::s_Messages = s_expressionErrors;

void ExpressionParser::Message(ScriptLineError errorCode, ...) const
{
	errorCode = errorCode > kError_Max ? kError_Max : errorCode;
	va_list args;
	va_start(args, errorCode);
	const ErrOutput::Message *msg = &s_Messages[errorCode];
	if (msg->bCanDisable)
		g_ErrOut.vShow(s_Messages[errorCode], args);
	else // prepend line # to message
	{
		char msgText[0x400];
#if RUNTIME
		sprintf_s(msgText, sizeof(msgText), "Error line %d\n\n%s", m_lineBuf->lineNumber, msg->fmt);
		g_ErrOut.vShow(msgText, args);
#else
		vsprintf_s(msgText, sizeof(msgText), msg->fmt, args);
		ShowCompilerError(m_scriptBuf, "%s", msgText);
#endif
	}

	va_end(args);
}

void ExpressionParser::PrintCompileError(const std::string &message) const
{
#if RUNTIME
	ShowCompilerError(m_lineBuf, message.c_str());
#else
	ShowCompilerError(m_scriptBuf, message.c_str());
#endif
}

UInt32 ExpressionParser::MatchOpenBracket(Operator *openBracOp) const
{
	const char closingBrac = openBracOp->GetMatchedBracket();
	const char openBrac = openBracOp->symbol[0];
	UInt32 openBracCount = 1;
	const char *text = Text();
	UInt32 i;
	for (i = Offset(); i < m_len && text[i]; i++)
	{
		if (text[i] == '"')
		{
			// Try to skip to the end of the quotes.
			// This way, we ignore brackets inside quotes.
			++i;
			bool foundMatchingEndQuote = false;
			for (; i < m_len && text[i]; i++)
			{
				if (text[i] == '"')
				{
					foundMatchingEndQuote = true;
					break;
				}
			}
			if (foundMatchingEndQuote)
				continue;
			// Else, couldn't find a matching end quote.
			return -2;
		}
		else if (text[i] == openBrac)
			openBracCount++;
		else if (text[i] == closingBrac)
			openBracCount--;

		if (openBracCount == 0)
			break;
	}

	return openBracCount ? -1 : i;
}

Token_Type g_lastCommandReturnType = kTokenType_Invalid;

void ExpressionParser::SaveScriptLine()
{
	savedScriptLines.emplace(CurText(), CurText(), m_len);
}

void ExpressionParser::RestoreScriptLine()
{
	const auto &line = savedScriptLines.top();
	if (_stricmp(line.curOffset, line.curExpr.c_str()) != 0)
	{
		const auto offset = line.curOffset - m_lineBuf->paramText + line.curExpr.size();
		strcpy_s(line.curOffset, sizeof m_lineBuf->paramText - (line.curOffset - m_lineBuf->paramText), line.curExpr.c_str());
		m_len = line.len;
		m_lineBuf->paramTextLen = line.len;
		Offset() = offset;
	}
	savedScriptLines.pop();
}

Token_Type ExpressionParser::ParseSubExpression(UInt32 exprLen)
{
	SaveScriptLine();

	std::stack<Operator *> ops;
	std::stack<Token_Type> operands;

	const UInt32 exprEnd = Offset() + exprLen;
	bool bLastTokenWasOperand = false; // if this is true, we expect binary operator, else unary operator or an operand

	char ch;
	while (Offset() < exprEnd && (ch = Peek()))
	{
		if (isspace(static_cast<unsigned char>(ch)))
		{
			Offset()++;
			continue;
		}

		Token_Type operandType = kTokenType_Invalid;

		if (!HandleMacros())
			return kTokenType_Invalid;

		// is it an operator?
		Operator *op = ParseOperator(bLastTokenWasOperand);
		if (op)
		{
			// if it's an open bracket, parse subexpression within
			if (op->IsOpenBracket())
			{
				if (op->numOperands)
				{
					// handles array subscript operator
					while (ops.size() && ops.top()->Precedes(op))
					{
						PopOperator(ops, operands);
					}
					ops.push(op);
				}

				const UInt32 endBracPos = MatchOpenBracket(op);
				if (endBracPos == -1)
				{
					Message(kError_MismatchedBrackets);
					return kTokenType_Invalid;
				}
				else if (endBracPos == -2)
				{
					Message(kError_MismatchedQuotes);
					return kTokenType_Invalid;
				}

				SaveScriptLine();
				// replace closing bracket with 0 to ensure subexpression doesn't try to read past end of expr (for rigging commands)
				m_lineBuf->paramText[endBracPos] = 0;

				operandType = ParseSubExpression(endBracPos - Offset());

				RestoreScriptLine();
				Offset() = endBracPos + 1; // skip the closing bracket
				bLastTokenWasOperand = true;
			}
			else if (op->IsClosingBracket())
			{
				Message(kError_MismatchedBrackets);
				return kTokenType_Invalid;
			}
			else // normal operator, handle or push
			{
				while (ops.size() && ops.top()->Precedes(op))
				{
					PopOperator(ops, operands);
				}

				ops.push(op);
				bLastTokenWasOperand = false;
				continue;
			}
		}
		else if (bLastTokenWasOperand || ParseOperator(!bLastTokenWasOperand, false)) // treat as arg delimiter?
			break;
		else // must be an operand (or a syntax error)
		{
			const auto operand = ParseOperand(ops.size() ? ops.top() : nullptr);
			if (!operand || operand->type == kTokenType_Invalid)
				return kTokenType_Invalid;

			if (operand->type == kTokenType_Command && !ops.empty() && ops.top()->type == kOpType_Dot)
			{
				if (operand->refIdx)
				{
					PrintCompileError("Parsing failure when interpreting dot syntax (token has ref index while dot operator is on operator stack)");
					return kTokenType_Invalid;
				}
				// makes thisObj be the top of the operand stack during runtime
				operand->useRefFromStack = true;
			}

			// write it to postfix expression, we'll check validity below
			if (!operand->Write(m_lineBuf))
			{
				PrintCompileError("Failed to compile token.");
				return kTokenType_Invalid;
			}
			operandType = operand->Type();

			CommandInfo *cmdInfo = operand->GetCommandInfo();

			// if command, parse it. also adjust operand type if return value of command is known
			if (operandType == kTokenType_Command)
			{
				const CommandReturnType retnType = g_scriptCommands.GetReturnType(cmdInfo);
				if (retnType == kRetnType_String)
					operandType = kTokenType_String;
				else if (retnType == kRetnType_Array)
					operandType = kTokenType_Array;
				else if (retnType == kRetnType_Form)
					operandType = kTokenType_Form;
				else if (retnType == kRetnType_Default)
					operandType = kTokenType_Number;

				if (operand->useRefFromStack)
				{
					g_lastCommandReturnType = operandType;
					// right dot op rule takes kTokenType_Command
					operandType = kTokenType_Command;
				}

				s_parserDepth++;
				const bool bParsed = ParseFunctionCall(cmdInfo);
				s_parserDepth--;

				if (!bParsed)
				{
					Message(kError_CantParse);
					return kTokenType_Invalid;
				}
			}
			bLastTokenWasOperand = true;
		}

		// operandType is an operand or result of a subexpression
		if (operandType == kTokenType_Invalid)
		{
			Message(kError_CantParse);
			return kTokenType_Invalid;
		}
		else
			operands.push(operandType);
	}

	// No more operands, clean off the operator stack
	while (ops.size())
	{
		if (PopOperator(ops, operands) == kTokenType_Invalid)
			return kTokenType_Invalid;
	}

	// restore line text if any macros were applied
	RestoreScriptLine();

	// done, make sure we've got a result
	if (ops.size() != 0)
	{
		Message(kError_TooManyOperators);
		return kTokenType_Invalid;
	}
	else if (operands.size() > 1)
	{
		Message(kError_TooManyOperands);
		return kTokenType_Invalid;
	}
	else if (operands.size() == 0)
	{
		return kTokenType_Empty;
	}
	else
	{
		return operands.top();
	}
}

Token_Type ExpressionParser::PopOperator(std::stack<Operator *> &ops, std::stack<Token_Type> &operands) const
{
	Operator *topOp = ops.top();
	ops.pop();

	// pop the operands
	Token_Type lhType, rhType = kTokenType_Invalid;
	if (operands.size() < topOp->numOperands)
	{
		Message(kError_TooManyOperators);
		return kTokenType_Invalid;
	}

	switch (topOp->numOperands)
	{
	case 2:
		rhType = operands.top();
		operands.pop();
		// fall-through intentional
	case 1:
		lhType = operands.top();
		operands.pop();
		break;
	default: // a paren or right bracket ended up on stack somehow
		Message(kError_CantParse);
		return kTokenType_Invalid;
	}

	// get result of operation
	Token_Type result = topOp->GetResult(lhType, rhType);
	if (result == kTokenType_Invalid)
	{
		Message(kError_InvalidOperands, topOp->symbol);
		return kTokenType_Invalid;
	}

	if (topOp->type == kOpType_Dot)
	{
		if (g_lastCommandReturnType == kTokenType_Invalid)
		{
			PrintCompileError("Failure parsing chained dot syntax: the command return type was not saved.");
			return kTokenType_Invalid;
		}
		result = g_lastCommandReturnType;
	}

	operands.push(result);

	// write operator to postfix expression
	auto const opToken = ScriptToken::Create(topOp);
	opToken->Write(m_lineBuf);

	return result;
}

Token_Type ExpressionParser::ParseArgument(UInt32 argsEndPos)
{
	auto *dataStart = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	// reserve space to store expr length
	m_lineBuf->dataOffset += 2;

	const auto argType = ParseSubExpression(argsEndPos - Offset());

	// store expr length for this arg
	*reinterpret_cast<UInt16 *>(dataStart) = m_lineBuf->dataBuf + m_lineBuf->dataOffset - dataStart;

	return argType;
}

#if EDITOR
const void *g_scriptCompiler = reinterpret_cast<void *>(0xECFDF8);
#endif
std::unordered_map<Script*, Script*> g_lambdaParentScriptMap;

Script *GetLambdaParentScript(Script *scriptLambda)
{
	if (const auto iter = g_lambdaParentScriptMap.find(scriptLambda); iter != g_lambdaParentScriptMap.end())
		return iter->second;
	return nullptr;
}

const char* StringForNVSEParamType(NVSEParamType paramType)
{
	switch (paramType)
	{
	case kNVSEParamType_Number:
		return "Number";
	case kNVSEParamType_Boolean:
		return "Bool";
	case kNVSEParamType_String:
		return "String";
	case kNVSEParamType_Form:
		return "Form";
	case kNVSEParamType_Array:
		return "Array";
	case kNVSEParamType_ArrayElement:
		return "ArrayElement";
	case kNVSEParamType_Slice:
		return "Slice";
	case kNVSEParamType_Command:
		return "Command";
	case kNVSEParamType_Variable:
		return "Variable";
	case kNVSEParamType_NumericVar:
		return "NumericVar";
	case kNVSEParamType_RefVar:
		return "RefVar";
	case kNVSEParamType_StringVar:
		return "StringVar";
	case kNVSEParamType_ArrayVar:
		return "ArrayVar";
	case kNVSEParamType_ForEachContext:
		return "ForEachContext";
	case kNVSEParamType_Collection:
		return "Collection";
	case kNVSEParamType_ArrayVarOrElement:
		return "ArrayVarOrArrayElement";
	case kNVSEParamType_BasicType:
		return "AnyType";
	case kNVSEParamType_NoTypeCheck:
		return "AnyType";
	case kNVSEParamType_FormOrNumber:
		return "FormOrNumber";
	case kNVSEParamType_StringOrNumber:
		return "StringOrNumber";
	case kNVSEParamType_Pair:
		return "Pair";
	default:
		return "<unknown>";
	}
}

std::unique_ptr<ScriptToken> ExpressionParser::ParseLambda()
{
	bool editor;
#if EDITOR
	editor = true;
#else
	editor = false;
#endif
	const auto *beginData = CurText() - strlen("begin");
	auto nest = 1;
	while (nest != 0 && CurText())
	{
		std::string token;
		try { token = GetCurToken(); }
		catch (OffsetOutOfBoundsError&)
		{
			PrintCompileError("Lambda function syntax error");
			return nullptr;
		}
		
		if (token.empty())
		{
			++Offset();
			continue;
		}
		if (_stricmp(token.c_str(), "begin") == 0)
			++nest;
		else if (_stricmp(token.c_str(), "end") == 0)
			--nest;
	}
	if (nest)
	{
		PrintCompileError("Mismatched begin/end block in anonymous user function/lambda");
		return nullptr;
	}
	const auto textLen = CurText() - beginData + 1;
	auto *lambdaText = static_cast<char *>(FormHeap_Allocate(textLen));
	memset(lambdaText, 0, textLen);
	std::memcpy(lambdaText, beginData, textLen);
	const auto lambdaScriptBuf = ScriptBuffer::MakeUnique();
	auto scriptLambda = Script::MakeUnique();

	const auto varCount = m_scriptBuf->info.varCount;
	const auto numRefs = m_scriptBuf->info.numRefs;

	lambdaScriptBuf->partialScript = true;

	scriptLambda->info.varCount = varCount;
	scriptLambda->info.unusedVariableCount = varCount;
	scriptLambda->info.numRefs = numRefs;

	lambdaScriptBuf->info.varCount = varCount;
	lambdaScriptBuf->info.unusedVariableCount = varCount;
	lambdaScriptBuf->info.numRefs = numRefs;

	// count number of new lines before lambda for accurate line number
	const auto textBefore = std::string(m_lineBuf->paramText, beginData - m_lineBuf->paramText);
	auto numNewLines = ra::count(textBefore, '\n');
	if (!this->appliedMacros_.contains(MacroType::OneLineLambda))
		numNewLines--;

	lambdaScriptBuf->curLineNumber = m_lineBuf->lineNumber + numNewLines;
	lambdaScriptBuf->scriptName.Set(
		FormatString("%sLambdaAtLine%d", m_scriptBuf->scriptName.CStr(), lambdaScriptBuf->curLineNumber).c_str()
	);

	if (const auto iter = appliedMacros_.find(MacroType::OneLineLambda); iter != appliedMacros_.end())
	{
		lambdaScriptBuf->curLineNumber -= 2;
		appliedMacros_.erase(iter);
	}

	*lambdaScriptBuf->scriptData = 0x1D; // Fake a script name (1D 00 00 00) so that it's not an outlier from other scripts
	lambdaScriptBuf->dataOffset = 4;

	lambdaScriptBuf->scriptText = lambdaText;
	scriptLambda->text = lambdaText;

	// CdeclCall(0x5C3310, &this->m_scriptBuf->refVars, &lambdaScriptBuf->refVars, nullptr); // CopyRefList(from, to, unk)
	// CdeclCall(0x5C2CE0, &this->m_scriptBuf->vars, &lambdaScriptBuf->vars); // CopyVarList(from, to)

	// we want the compile to add onto out ref list and var list, so we copy the pointers
	lambdaScriptBuf->refVars = m_scriptBuf->refVars;
	lambdaScriptBuf->vars = m_scriptBuf->vars;

	auto *beginEndOffset = reinterpret_cast<UInt32 *>(editor ? 0xED9D54 : 0x11CADBC);
	const auto savedOffset = *beginEndOffset;
	*beginEndOffset = 0;

	g_lambdaParentScriptMap.emplace(scriptLambda.get(), m_script);

	// lambdaScriptBuf->currentScript = scriptLambda.get();
	// g_currentScriptStack will be updated inside a CompileScript hook.
	PatchDisable_ScriptBufferValidateRefVars(true);
	const auto compileResult = scriptLambda->Compile(lambdaScriptBuf.get()); // CompileScript
	PatchDisable_ScriptBufferValidateRefVars(false);

	*beginEndOffset = savedOffset;

	m_scriptBuf->refVars = lambdaScriptBuf->refVars;
	m_scriptBuf->vars = lambdaScriptBuf->vars;

	// prevent destructor from deleting our vars
	lambdaScriptBuf->refVars = {};
	lambdaScriptBuf->vars = {};
	lambdaScriptBuf->scriptText = nullptr;

	m_scriptBuf->info.numRefs = lambdaScriptBuf->info.numRefs;
	m_scriptBuf->info.varCount = lambdaScriptBuf->info.varCount;
	m_scriptBuf->info.unusedVariableCount = lambdaScriptBuf->info.unusedVariableCount;

	if (!compileResult)
	{
		g_lambdaParentScriptMap.erase(scriptLambda.get());
		scriptLambda->refList = {};
		scriptLambda->varList = {};
		return nullptr;
	}

	return std::make_unique<ScriptToken>(scriptLambda.release());
}

std::unique_ptr<ScriptToken> ExpressionParser::ParseOperand(bool (*pred)(ScriptToken *operand))
{
	char ch;
	while ((ch = Peek(Offset())))
	{
		if (!isspace(static_cast<unsigned char>(ch)))
			break;

		Offset()++;
	}

	auto token = ParseOperand();
	if (token)
	{
		if (!pred(token.get()))
		{
			token = nullptr;
		}
	}

	return token;
}

ParamParenthResult ExpressionParser::ParseParentheses(ParamInfo *paramInfo, UInt32 paramIndex)
{
	char c;
	auto index = Offset();
	while (((c = m_lineBuf->paramText[index])) && isspace(c))
		++index;

	if (c != '(')
		return kParamParent_NoParam;
	Offset() = index + 1;

	const auto endIdx = MatchOpenBracket(&s_operators[kOpType_LeftParen]);
	if (endIdx == -1)
	{
		Message(kError_MismatchedBrackets);
		return kParamParent_SyntaxError;
	}
	else if (endIdx == -2)
	{
		Message(kError_MismatchedQuotes);
		return kParamParent_SyntaxError;
	}

	m_lineBuf->Write16(0xFFFF);

	// prevent mismatched brackets errors inside if or set statements
	const auto len = sizeof m_lineBuf->paramText / sizeof(char);
	char realText[len];
	strcpy_s(realText, len, m_lineBuf->paramText);

	// replace closing bracket with 0 to ensure subexpression doesn't try to read past end of expr (for rigging commands)
	m_lineBuf->paramText[endIdx] = 0;

	const auto result = ParseArgument(endIdx);

	strcpy_s(m_lineBuf->paramText, len, realText);

	Offset() = endIdx + 1; // show that we parsed ')'

	if (result == kTokenType_Invalid)
	{
		return kParamParent_SyntaxError;
	}

	auto &param = paramInfo[paramIndex];
	if (!ValidateArgType(static_cast<ParamType>(param.typeID), result, false, g_scriptCommands.GetByOpcode(m_lineBuf->cmdOpcode)))
	{
#if RUNTIME
		ShowCompilerError(m_lineBuf, "Invalid expression for parameter %d. Expected %s (got %s).", paramIndex + 1, param.typeStr, TokenTypeToString(result));
#else
		ShowCompilerError(m_scriptBuf, "Invalid expression for parameter %d. Expected %s (got %s).", paramIndex + 1, param.typeStr, TokenTypeToString(result));
#endif
		return kParamParent_SyntaxError;
	}

	return kParamParent_Success;
}

#if RUNTIME
void ShowRuntimeScriptError(Script* script, ExpressionEvaluator* eval, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char errorMsg[0x800];
	vsprintf_s(errorMsg, sizeof(errorMsg), fmt, args);

	if (eval)
		eval->Error(errorMsg);
	else
		ShowRuntimeError(script, errorMsg);

	va_end(args);
}
#endif

Operator *ExpressionParser::ParseOperator(bool bExpectBinaryOperator, bool bConsumeIfFound) const
{
	// if bExpectBinary true, we expect a binary operator or a closing paren
	// if false, we expect unary operator or an open paren

	// If this returns NULL when we expect a binary operator, it likely indicates end of arg
	// Commas can optionally be used to separate expressions as args

	std::vector<Operator *> ops; // a list of possible matches
	Operator *op = nullptr;

	// check first character
	char ch = Peek();
	const auto firstChar = ch;
	if (ch == ',') // arg expression delimiter
	{
		Offset() += 1;
		return nullptr;
	}

	for (UInt32 i = 0; i < kOpType_Max; i++)
	{
		Operator *curOp = &s_operators[i];
		if (bExpectBinaryOperator)
		{
			if (!curOp->IsBinary() && !curOp->IsClosingBracket())
				continue;
		}
		else if (!curOp->IsUnary() && !curOp->IsOpenBracket())
			continue;
		if (ch == curOp->symbol[0])
			ops.push_back(curOp);
	}

	ch = Peek(Offset() + 1);
	if (ch && ispunct(static_cast<unsigned char>(ch))) // possibly a two-character operator, check second char
	{
		auto iter = ops.begin();
		while (iter != ops.end())
		{
			Operator *cur = *iter;
			if (cur->symbol[1] == ch) // definite match
			{
				op = cur;
				break;
			}
			else if (cur->symbol[1] != 0) // remove two-character operators which don't match
				iter = ops.erase(iter);
			else
				iter++;
		}
	}
	else // definitely single-character
	{
		for (UInt32 i = 0; i < ops.size(); i++)
		{
			if (ops[i]->symbol[1] == 0)
			{
				op = ops[i];
				break;
			}
		}

		// adding this for consistency with macro
		if (!op && firstChar == '=')
			op = &s_operators[kOpType_Assignment];
	}

	if (!op && ops.size() == 1)
		op = ops[0];

	if (op && bConsumeIfFound)
		Offset() += strlen(op->symbol);
	return op;
}

// format a string argument
static void FormatString(std::string &str)
{
	UInt32 pos = 0;

	while (((pos = str.find('%', pos)) != -1) && pos < str.length() - 1)
	{
		char toInsert = 0;
		switch (str[pos + 1])
		{
		case '%':
			pos += 2;
			continue;
		case 'r':
		case 'R':
			toInsert = '\n';
			break;
		case 'q':
		case 'Q':
			toInsert = '"';
			break;
		default:
			pos += 1;
			continue;
		}

		str.insert(pos, 1, toInsert); // insert char at current pos
		str.erase(pos + 1, 2);		  // erase format specifier
		pos += 1;
	}
}

std::unique_ptr<ScriptToken> ExpressionParser::PeekOperand(UInt32 &outReadLen)
{
	outReadLen = 0;
	const UInt32 curOffset = Offset();
	SkipSpaces();
	SaveScriptLine();
	while (*CurText() == '(')
	{
		++Offset();
		const auto endBracketPos = MatchOpenBracket(&s_operators[kOpType_LeftParen]);
		if (endBracketPos == -1)
		{
			Message(kError_MismatchedBrackets);
			return nullptr;
		}
		else if (endBracketPos == -2)
		{
			Message(kError_MismatchedQuotes);
			return nullptr;
		}
		m_lineBuf->paramText[endBracketPos] = 0;
		if (!outReadLen)
			outReadLen = endBracketPos + 1 - curOffset;
	}
	if (!HandleMacros())
		return nullptr;
	auto operand = ParseOperand();
	if (!outReadLen)
		outReadLen = Offset() - curOffset;
	RestoreScriptLine();
	Offset() = curOffset;
	return operand;
}

std::vector g_expressionParserMacros =
	{
		ScriptLineMacro([&](std::string &line, ScriptBuffer*, ScriptLineBuffer*) {
			// Lambda macro
			const std::regex oneLineLambdaRegex(R"(^\{([^{}]*)\}\s*=>\s*(.*))"); // match {iVar, rRef} => ...
			if (std::smatch m; std::regex_search(line, m, oneLineLambdaRegex) && m.size() == 3)
			{
				line = "begin function {" + m.str(1) + "}\r\nSetFunctionValue " + m.str(2) + "\r\nend";
				return true;
			}
			return false;
		}, MacroType::OneLineLambda),
};

bool ExpressionParser::HandleMacros()
{
	for (const auto &macro : g_expressionParserMacros)
	{
		const auto result = macro.EvalMacro(m_lineBuf, m_scriptBuf, this);
		if (result == ScriptLineMacro::MacroResult::Error)
			return false;
		if (result == ScriptLineMacro::MacroResult::Applied)
			appliedMacros_.emplace(macro.type);
	}
	return true;
}

bool ValidateVariable(const std::string &varName, Script::VariableType varType, Script *script)
{
	if (const auto savedVarType = GetSavedVarType(script, varName); savedVarType != varType && savedVarType != Script::eVarType_Invalid)
	{
		g_ErrOut.Show("Variable redefinition with a different type (saw first '%s', then '%s')", g_variableTypeNames[savedVarType], g_variableTypeNames[varType]);
		return false;
	}
	SaveVarType(script, varName, varType);
	return true;
}

VariableInfo* CreateVariable(Script* script, ScriptBuffer* scriptBuf, const std::string& varName, Script::VariableType varType, 
	const std::function<void(const std::string&)>& printCompileError)
{
	if (!script)
		return nullptr;
	if (auto* var = scriptBuf->vars.FindFirst([&](VariableInfo* v) { return _stricmp(varName.c_str(), v->name.CStr()) == 0; }))
	{
		if (!ValidateVariable(varName, varType, script))
			return nullptr;
		// all good, already exists
		return var;
	}
	if (varName.empty())
	{
		printCompileError("Got empty string for variable name");
		return nullptr;
	}
	if (GetFormByID(varName.c_str()))
	{
		printCompileError("Invalid variable name " + varName + ": Form with that Editor ID already exists.");
		return nullptr;
	}
	if (g_scriptCommands.GetByName(varName.c_str(), &g_currentCompilerPluginVersions.top()))
	{
		printCompileError("Invalid variable name " + varName + ": Command with that name already exists.");
		return nullptr;
	}
	auto* varInfo = New<VariableInfo>();
	if (const auto* existingVar = script->GetVariableByName(varName.c_str()))
		varInfo->idx = existingVar->idx;
	else
		varInfo->idx = ++scriptBuf->info.varCount;
	varInfo->name.Set(varName.c_str());
	varInfo->type = varType;
	scriptBuf->vars.Append(varInfo);
	SaveVarType(script, varName, varType);
	scriptBuf->ResolveRef(varName.c_str(), script); // ref var
	return varInfo;
}

VariableInfo *ExpressionParser::CreateVariable(const std::string &varName, Script::VariableType varType) const
{
	return ::CreateVariable(m_script, m_scriptBuf, varName, varType, _L(const auto& str, PrintCompileError(str)));
}

void ExpressionParser::SkipSpaces() const
{
	while (isspace(static_cast<unsigned char>(*CurText())))
	{
		Offset()++;
	}
}

std::unique_ptr<ScriptToken> ExpressionParser::ParseOperand(Operator *curOp)
{
	const char firstChar = Peek();
	bool bExpectStringVar = false;

	if (!firstChar)
	{
		Message(kError_CantParse);
		return nullptr;
	}
	if (firstChar == '"') // string literal
	{
		Offset()++;
		const char *endQuotePtr = strchr(CurText(), '"');
		if (!endQuotePtr)
		{
			Message(kError_MismatchedQuotes);
			return nullptr;
		}
		std::string strLit(CurText(), endQuotePtr - CurText());
		Offset() = endQuotePtr - Text() + 1;
		FormatString(strLit);
		return ScriptToken::Create(strLit);
	}
	if (firstChar == '$') // string vars passed to vanilla cmds as '$var'; not necessary here but allowed for consistency
	{
		bExpectStringVar = true;
		Offset()++;
	}

	std::string token;
	try { token = GetCurToken(); }
	catch (OffsetOutOfBoundsError&)
	{
		PrintCompileError("Failed to read script line; reached out of bounds");
		return nullptr;
	}
	std::string refToken = token;

	if (!bExpectStringVar)
	{
		if (_stricmp(token.c_str(), "_") == 0) {
			auto tok = std::make_unique<ScriptToken>();
			tok->type = kTokenType_OptionalEmpty;
			return tok;
		}

		if (_stricmp(token.c_str(), "begin") == 0)
		{
			return ParseLambda();
		}
		if (const auto varType = VariableTypeNameToType(token.c_str()); varType != Script::eVarType_Invalid)
		{
			SkipSpaces();
			std::string varName;
			try { varName = GetCurToken(); }
			catch (OffsetOutOfBoundsError&)
			{
				PrintCompileError("Failed to read variable name; line out of bounds");
				return nullptr;
			}
			auto *varInfo = CreateVariable(varName, varType);
			if (!varInfo)
				return nullptr;
			return ScriptToken::Create(varInfo, 0, varType);
		}
	}

	// some operators (e.g. ->) expect a string literal, filter them out now
	if (curOp && curOp->ExpectsStringLiteral())
	{
		if (!token.length() || bExpectStringVar)
		{
			Message(kError_ExpectedStringLiteral);
			return nullptr;
		}
		return ScriptToken::Create(token);
	}

	if (token.rfind("0x", 0) == 0)
	{
		// hexadecimal
		try
		{
			std::size_t parsed = 0;
			const auto result = std::stoul(token, &parsed, 16);
			if (parsed == token.size())
				return ScriptToken::Create(static_cast<double>(result));
		}
		catch (...)
		{
		}
		PrintCompileError("Invalid hexadecimal syntax");
		return nullptr;
	}
	if (token.rfind("0b", 0) == 0)
	{
		// binary
		try
		{
			token.erase(0, 2);
			std::size_t parsed = 0;
			const auto result = std::stoul(token, &parsed, 2);
			if (parsed == token.size())
				return ScriptToken::Create(static_cast<double>(result));
		}
		catch (...)
		{
		}
		PrintCompileError("Invalid binary syntax");
		return nullptr;
	}
	// try to convert to a number
	char *leftOvers = nullptr;
	const double dVal = strtod(token.c_str(), &leftOvers);
	if (*leftOvers == 0) // entire string parsed as a double
		return ScriptToken::Create(dVal);

	// check for a calling object
	Script::RefVariable *callingObj = nullptr;
	UInt16 refIdx = 0;
	bool hasDot = false;
	if (Peek() == '.')
	{
		++Offset();
		hasDot = true;
		refToken = std::move(token);

		try { token = GetCurToken(); }
		catch (OffsetOutOfBoundsError&)
		{
			PrintCompileError("Failed to read token; line out of bounds 2");
			return nullptr;
		}

	}

	// before we go any further, check for local variable in case of name collisions between vars and other objects
	if (!hasDot)
	{
		if (VariableInfo *varInfo = LookupVariable(token.c_str(), nullptr))
			return ScriptToken::Create(varInfo, 0, m_scriptBuf->GetVariableType(varInfo, nullptr, m_script));
	}
	const auto usesRefFromStack = curOp && curOp->type == kOpType_Dot;
	Script::RefVariable *refVar = m_scriptBuf->ResolveRef(refToken.c_str(), m_script);
	if (hasDot && !refVar)
	{
		//Message(kError_InvalidDotSyntax);
		//return NULL;

		// Use dot operator syntax instead, allowing chained dot syntax
		hasDot = false;
		Offset() -= token.size() + 1;
		token = std::move(refToken);
	}
	else if (refVar)
		refIdx = m_scriptBuf->GetRefIdx(refVar);

	if (refVar)
	{
		if (!hasDot)
		{
			if (refVar->varIdx) // it's a variable
				return ScriptToken::Create(m_scriptBuf->vars.GetVariableByName(refVar->name.m_data), 0, Script::eVarType_Ref);
			if (refVar->form && refVar->form->typeID == kFormType_TESGlobal)
				return ScriptToken::Create(static_cast<TESGlobal*>(refVar->form), refIdx);
			// literal reference to a form
			return ScriptToken::Create(refVar, refIdx);
		}
		if (refVar->form && !refVar->form->GetIsReference() && refVar->form->typeID != kFormType_TESQuest)
		{
			Message(kError_InvalidDotSyntax);
			return nullptr;
		}
	}

	// command?
	if (!bExpectStringVar)
	{
		CommandInfo *cmdInfo = g_scriptCommands.GetByName(token.c_str(), &g_currentCompilerPluginVersions.top());
		if (cmdInfo)
		{
			// if quest script, check that calling obj supplied for cmds requiring it
			if (m_scriptBuf->info.type == Script::eType_Quest && cmdInfo->needsParent && !refVar && !usesRefFromStack)
			{
				Message(kError_RefRequired, cmdInfo->longName);
				return nullptr;
			}
			if (refVar && refVar->form && !refVar->form->GetIsReference()) // make sure we're calling it on a reference
				return nullptr;

			return ScriptToken::Create(cmdInfo, refIdx);
		}
	}

	// variable?
	VariableInfo *varInfo = LookupVariable(token.c_str(), refVar);
	if (!varInfo && hasDot)
	{
		Message(kError_CantFindVariable, token.c_str());
		return nullptr;
	}
	if (varInfo)
	{
		const auto theVarType = m_scriptBuf->GetVariableType(varInfo, refVar, m_script);
		if (bExpectStringVar && theVarType != Script::eVarType_String)
		{
			Message(kError_ExpectedStringVariable);
			return nullptr;
		}
		return ScriptToken::Create(varInfo, refIdx, theVarType);
	}
	if (bExpectStringVar)
	{
		Message(kError_ExpectedStringVariable);
		return nullptr;
	}

	if (refVar != nullptr)
	{
		Message(kError_InvalidDotSyntax);
		return nullptr;
	}

	// anything else that makes it this far is treated as string
	if (!curOp || curOp->type != kOpType_MemberAccess)
	{
		Message(kWarning_UnquotedString, token.c_str());
	}

	FormatString(token);
	return ScriptToken::Create(token);
}

bool ExpressionParser::ParseFunctionCall(CommandInfo *cmdInfo) const
{
	// trick Cmd_Parse into thinking it is parsing the only command on this line
	const UInt32 oldOffset = Offset();
	const UInt32 oldOpcode = m_lineBuf->cmdOpcode;
	const UInt16 oldCallingRefIdx = m_lineBuf->callingRefIndex;

	// reserve space to record total # of bytes used for cmd args
	const UInt16 oldDataOffset = m_lineBuf->dataOffset;
	auto argsLenPtr = (UInt16 *)(m_lineBuf->dataBuf + m_lineBuf->dataOffset);
	m_lineBuf->dataOffset += 2;

	// save the original paramText, overwrite with params following this function call
	const UInt32 oldLineLength = m_lineBuf->paramTextLen;
	char oldLineText[0x200];
	memcpy(oldLineText, m_lineBuf->paramText, 0x200);
	memset(m_lineBuf->paramText, 0, 0x200);
	memcpy(m_lineBuf->paramText, oldLineText + oldOffset, 0x200 - oldOffset);

	// rig ScriptLineBuffer fields
	m_lineBuf->cmdOpcode = cmdInfo->opcode;
	m_lineBuf->callingRefIndex = 0;
	m_lineBuf->lineOffset = 0;
	m_lineBuf->paramTextLen = StrLen(m_lineBuf->paramText);

	// parse the command if numParams > 0
	const bool bParsed = ParseNestedFunction(cmdInfo, m_lineBuf, m_scriptBuf);

	// restore original state, save args length
	m_lineBuf->callingRefIndex = oldCallingRefIdx;
	m_lineBuf->lineOffset += oldOffset; // skip any text used as command arguments
	m_lineBuf->paramTextLen = oldLineLength;
	*argsLenPtr = m_lineBuf->dataOffset - oldDataOffset;
	m_lineBuf->cmdOpcode = oldOpcode;
	memcpy(m_lineBuf->paramText, oldLineText, 0x200);

	return bParsed;
}

VariableInfo *ExpressionParser::LookupVariable(const char *varName, Script::RefVariable *refVar) const
{
	Script::VarInfoList *vars = &m_scriptBuf->vars;

	if (refVar)
	{
		if (!refVar->form) // it's a ref variable, can't get var
			return nullptr;

		Script *script = GetScriptFromForm(refVar->form);
		if (script)
			vars = &script->varList;
		else // not a scripted object
			return nullptr;
	}

	if (!vars)
		return nullptr;

	return vars->GetVariableByName(varName);
}

std::string ExpressionParser::GetCurToken() const
{
	unsigned char ch;
	const char *tokStart = CurText();
	auto numeric = true;
	while ((ch = Peek()))
	{
		if (isspace(ch) || ispunct(ch) && ch != '_' && (ch != '.' || !numeric))
			break;
		Offset()++;
		if (!isdigit(ch))
			numeric = false;
	}
	auto result = std::string(tokStart, CurText() - tokStart);
	if (ch == 0 && result.empty())
		throw OffsetOutOfBoundsError();

	return result;
}

// error routines

#if RUNTIME

/**********************************
	ExpressionEvaluator
**********************************/

UInt8 ExpressionEvaluator::ReadByte()
{
	const UInt8 byte = *Data();
	Data()++;
	return byte;
}

SInt8 ExpressionEvaluator::ReadSignedByte()
{
	const SInt8 byte = *((SInt8 *)Data());
	Data()++;
	return byte;
}

UInt16 ExpressionEvaluator::Read16()
{
	const UInt16 data = *((UInt16 *)Data());
	Data() += 2;
	return data;
}

SInt16 ExpressionEvaluator::ReadSigned16()
{
	const SInt16 data = *((SInt16 *)Data());
	Data() += 2;
	return data;
}

UInt32 ExpressionEvaluator::Read32()
{
	const UInt32 data = *((UInt32 *)Data());
	Data() += 4;
	return data;
}

SInt32 ExpressionEvaluator::ReadSigned32()
{
	const SInt32 data = *((SInt32 *)Data());
	Data() += 4;
	return data;
}

void ExpressionEvaluator::ReadBuf(UInt32 len, UInt8 *data)
{
	std::memcpy(data, Data(), len);
	Data() += len;
}

UInt8 *ExpressionEvaluator::GetCommandOpcodePosition(UInt32* opcodeOffsetPtr) const
{
	return script->data + *opcodeOffsetPtr;
}

CommandInfo *ExpressionEvaluator::GetCommand() const
{
	if (m_inline)
		return nullptr;
	const auto *opcodePtr = reinterpret_cast<UInt16 *>(static_cast<UInt8 *>(m_scriptData) + m_baseOffset);
	return g_scriptCommands.GetByOpcode(*opcodePtr);
}

double ExpressionEvaluator::ReadFloat()
{
	const double data = *((double *)Data());
	Data() += sizeof(double);
	return data;
}

char *ExpressionEvaluator::ReadString(UInt32& incrData)
{
	const UInt16 len = Read16();
	incrData = 2 + len;
	if (len)
	{
		auto resStr = static_cast<char*>(malloc(len + 1));
		memcpy(resStr, Data(), len);
		resStr[len] = 0;
		Data() += len;
		return resStr;
	}
	return nullptr;
}

void ExpressionEvaluator::PushOnStack()
{
	ExpressionEvaluator *top = localData.expressionEvaluator;
	m_parent = top;
	localData.expressionEvaluator = this;

	// inherit properties of parent
	if (top)
	{
		// inherit flags
		m_flags.RawSet(top->m_flags.Get());
		m_flags.Clear(kFlag_ErrorOccurred);
	}
}

void ExpressionEvaluator::PopFromStack() const
{
	if (m_parent)
	{
		// propagate info to parent
		m_parent->m_expectedReturnType = m_expectedReturnType;
		if (m_parent->m_flags.Get(kFlag_SuppressErrorMessages))
			m_parent->m_flags.Set(m_flags.Get(kFlag_ErrorOccurred));
	}
	localData.expressionEvaluator = m_parent;
}

#if _DEBUG
const char *g_lastScriptName = nullptr;
#endif
ExpressionEvaluator::ExpressionEvaluator(COMMAND_ARGS) : m_opcodeOffsetPtr(opcodeOffsetPtr), m_result(result),
	m_thisObj(thisObj), m_containingObj(containingObj), m_params(paramInfo), m_numArgsExtracted(0), m_expectedReturnType(kRetnType_Default), m_baseOffset(0),
	localData(ThreadLocalData::Get()), m_inline(false), script(scriptObj), eventList(eventList)
{
	m_scriptData = static_cast<UInt8*>(scriptData);
	m_data = m_scriptData + *m_opcodeOffsetPtr;
	m_pushedOnStack = true;

	m_baseOffset = *opcodeOffsetPtr - 4;

	m_flags.Clear();

	PushOnStack();
}

ExpressionEvaluator::~ExpressionEvaluator()
{
	if (moved_.moved) [[unlikely]]
		return;
	if (m_pushedOnStack)
		PopFromStack();

	for (UInt32 i = 0; i < m_numArgsExtracted; i++)
	{
		delete m_args[i];
	}

	if (!this->errorMessages.empty())
	{
		std::string error;
		for (const auto &msg : errorMessages)
		{
			error += msg + '\n';
		}
		error.pop_back(); // remove last "\n"
		// include script data offset and command name/opcode
		CommandInfo *cmd = GetCommand();

		// include mod filename, save having to ask users to figure it out themselves
		const char *modName = GetModName(script);

		ShowRuntimeError(script, "%s\n    File: %s Offset: 0x%04X Command: %s", error.c_str(), modName, m_baseOffset, cmd ? cmd->longName : "<unknown>");
		if (m_flags.IsSet(kFlag_StackTraceOnError))
			PrintStackTrace();
	}
}

ExpressionEvaluator::ExpressionEvaluator(UInt8* scriptData, Script* script, UInt32* opcodeOffsetPtr) :
	m_pushedOnStack(false), m_scriptData(scriptData), m_opcodeOffsetPtr(opcodeOffsetPtr), m_result(nullptr), m_thisObj(nullptr), m_containingObj(nullptr),
	m_args{},
	m_params(nullptr),
	m_numArgsExtracted(0),
	m_expectedReturnType(),
	m_baseOffset(0), m_parent(nullptr), localData(ThreadLocalData::Get()), m_inline(false),
	script(script), eventList(nullptr)
{
	m_data = m_scriptData + *m_opcodeOffsetPtr;
	m_flags.Clear();
}

bool ExpressionEvaluator::ExtractArgs()
{
	const UInt32 numArgs = ReadByte();
	UInt32 curArg = 0;
	while (curArg < numArgs)
	{
		ScriptToken *arg = Evaluate();
		if (!arg)
			break;

		m_args[curArg++] = arg;
	}
	if (numArgs == curArg) // all args extracted
	{
		m_numArgsExtracted = curArg;
		return true;
	}
	else
		return false;
}

bool ExpressionEvaluator::ExtractArgsV(void* null, ...)
{

	va_list list;
	va_start(list, null);
	const auto result = ExtractArgsV(list);
	va_end(list);
	return result;
}

bool ExpressionEvaluator::ExtractArgsV(va_list list)
{
	if (!ExtractArgs())
		return false;
	for (int i = 0; i < NumArgs(); ++i)
	{
		auto* arg = Arg(i);
		if (!arg)
			return false;
		switch (arg->type) {
		case kTokenType_Number:
		case kTokenType_Boolean:
		case kTokenType_NumericVar:
		case kTokenType_Global:
		{
			*va_arg(list, double*) = arg->GetNumber();
			break;
		}
		case kTokenType_StringVar:
		case kTokenType_String:
		{
			*va_arg(list, const char**) = arg->GetString();
			break;
		}
		case kTokenType_Form:
		case kTokenType_Ref:
		case kTokenType_Lambda:
		case kTokenType_RefVar:
		{
			*va_arg(list, TESForm**) = arg->GetTESForm();
			break;
		}
		case kTokenType_Array:
		case kTokenType_ArrayVar:
		{
			*va_arg(list, ArrayVar**) = arg->GetArrayVar();
			break;
		}
		case kTokenType_Slice:
		{
			*va_arg(list, const Slice**) = arg->GetSlice();
			break;
		}
		default:
		{
			if (arg->CanConvertTo(kTokenType_Number))
				*va_arg(list, double*) = arg->GetNumber();
			else if (arg->CanConvertTo(kTokenType_Form))
				*va_arg(list, TESForm**) = arg->GetTESForm();
			else if (arg->CanConvertTo(kTokenType_String))
				*va_arg(list, const char**) = arg->GetString();
			else if (arg->CanConvertTo(kTokenType_Array))
				*va_arg(list, ArrayVar**) = arg->GetArrayVar();
			else
				*va_arg(list, void**) = nullptr;
			break;
		}
		}
	}
	return true;
}

bool ExpressionEvaluator::ExtractDefaultArgs(va_list varArgs, bool bConvertTESForms)
{
	if (ExtractArgs())
	{
		for (UInt32 i = 0; i < NumArgs(); i++)
		{
			ScriptToken *arg = Arg(i);
			ParamInfo *info = &m_params[i];
			if (!ConvertDefaultArg(arg, info, bConvertTESForms, varArgs))
			{
				DEBUG_PRINT("Couldn't convert arg %d", i);
				return false;
			}
		}

		return true;
	}
	else
	{
		DEBUG_PRINT("Couldn't extract default args");
		return false;
	}
}

class OverriddenScriptFormatStringArgs : public FormatStringArgs
{
public:
	OverriddenScriptFormatStringArgs(ExpressionEvaluator *eval, UInt32 fmtStringPos) : m_eval(eval), m_curArgIndex(fmtStringPos + 1)
	{
		m_fmtString = m_eval->Arg(fmtStringPos)->GetString();
	}

	virtual bool Arg(argType asType, void *outResult)
	{
		ScriptToken *arg = m_eval->Arg(m_curArgIndex);
		if (arg)
		{
			switch (asType)
			{
			case kArgType_Float:
				*static_cast<double*>(outResult) = arg->GetNumber();
				break;
			case kArgType_Form:
				*static_cast<TESForm**>(outResult) = arg->GetTESForm();
				break;
			default:
				return false;
			}

			m_curArgIndex += 1;
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual bool SkipArgs(UInt32 numToSkip)
	{
		m_curArgIndex += numToSkip;
		return m_curArgIndex <= m_eval->NumArgs();
	}
	virtual bool HasMoreArgs() { return m_curArgIndex < m_eval->NumArgs(); }
	virtual char *GetFormatString() { return const_cast<char *>(m_fmtString); }

	UInt32 GetCurArgIndex() const { return m_curArgIndex; }

private:
	ExpressionEvaluator *m_eval;
	UInt32 m_curArgIndex;
	const char *m_fmtString;
};

bool ExpressionEvaluator::ExtractFormatStringArgs(va_list varArgs, UInt32 fmtStringPos, char *fmtStringOut, UInt32 maxParams)
{
	// first simply evaluate all arguments, whether intended for fmt string or cmd args
	if (ExtractArgs())
	{
		if (NumArgs() < fmtStringPos)
		{
			// uh-oh.
			return false;
		}

		// convert and store any cmd args preceding fmt string
		for (UInt32 i = 0; i < fmtStringPos; i++)
		{
			if (!ConvertDefaultArg(Arg(i), &m_params[i], false, varArgs))
			{
				return false;
			}
		}

		// grab the format string
		const char *fmt = Arg(fmtStringPos)->GetString();
		if (!fmt)
		{
			return false;
		}

		// interpret the format string
		OverriddenScriptFormatStringArgs fmtArgs(this, fmtStringPos);
		if (ExtractFormattedString(fmtArgs, fmtStringOut))
		{
			// convert and store any remaining cmd args
			const UInt32 trailingArgsOffset = fmtArgs.GetCurArgIndex();
			if (trailingArgsOffset < NumArgs())
			{
				for (UInt32 i = trailingArgsOffset; i < NumArgs(); i++)
				{
					// adjust index into params to account for 20 'variable' args to format string.
					if (!ConvertDefaultArg(Arg(i), &m_params[fmtStringPos + SIZEOF_FMT_STRING_PARAMS + (i - trailingArgsOffset)], false, varArgs))
					{
						return false;
					}
				}
			}

			// hooray
			return true;
		}
	}
	// something went wrong.
	return false;
}

bool ExpressionEvaluator::ConvertDefaultArg(ScriptToken *arg, ParamInfo *info, bool bConvertTESForms, va_list &varArgs) const
{
	const auto handlePrimitiveStringVar = [&]<typename T>()
	{
		// handle string_var passed as integer to sv_* cmds
		if (arg->CanConvertTo(kTokenType_StringVar))
		{
			ScriptLocal* var = arg->GetScriptLocal();
			if (var)
			{
				T* out = va_arg(varArgs, T*);
				*out = var->data;
				return true;
			}
		}
		if (arg->CanConvertTo(kTokenType_String))
		{
			T* out = va_arg(varArgs, T*);
			*out = g_StringMap.Add(script->GetModIndex(), arg->GetString(), true, nullptr);
			return true;
		}
		return false;
	};
	// hooray humongous switch statements
	switch (info->typeID)
	{
	case kParamType_Array:
	{
		UInt32 *out = va_arg(varArgs, UInt32 *);
		*out = arg->GetArrayID();
	}

	break;
	case kParamType_Integer:
	{
		if (handlePrimitiveStringVar.operator()<UInt32>())
			break;
	}
	// fall-through intentional
	case kParamType_QuestStage:
	case kParamType_CrimeType:
	case kParamType_MiscellaneousStat:
	case kParamType_FormType:
	case kParamType_Alignment:
	case kParamType_EquipType:
	case kParamType_CriticalStage:
		if (arg->CanConvertTo(kTokenType_Number))
		{
			UInt32 *out = va_arg(varArgs, UInt32 *);
			*out = arg->GetNumber();
		}
		else
		{
			return false;
		}

		break;
	case kParamType_Float:
		if (handlePrimitiveStringVar.operator()<float>())
			break;
		if (arg->CanConvertTo(kTokenType_Number))
		{
			float *out = va_arg(varArgs, float *);
			*out = arg->GetNumber();
		}
		else
		{
			return false;
		}
		break;
	case kParamType_Double:
		if (handlePrimitiveStringVar.operator()<double> ())
			break;
		if (arg->CanConvertTo(kTokenType_Number))
		{
			double *out = va_arg(varArgs, double *);
			*out = arg->GetNumber();
		}
		else
		{
			return false;
		}
		break;
	case kParamType_ScriptVariable:
		if (arg->CanConvertTo(kTokenType_Variable))
		{
			auto **out = va_arg(varArgs, ScriptLocal **);
			*out = arg->GetScriptLocal();
		}
		else
			return false;
		break;
	case kParamType_String:
	{
		const char *str = arg->GetString();
		if (str)
		{
			char *out = va_arg(varArgs, char *);
#pragma warning(push)
#pragma warning(disable : 4996)
			strcpy(out, str);
#pragma warning(pop)
		}
		else
		{
			return false;
		}
	}
	break;
	case kParamType_Axis:
	{
		char axis = arg->GetAxis();
		if (axis != -1)
		{
			char *out = va_arg(varArgs, char *);
			*out = axis;
		}
		else
		{
			return false;
		}
	}
	break;
	case kParamType_ActorValue:
	{
		UInt32 actorVal = arg->GetActorValue();
		if (actorVal != eActorVal_NoActorValue)
		{
			UInt32 *out = va_arg(varArgs, UInt32 *);
			*out = actorVal;
		}
		else
		{
			return false;
		}
	}
	break;
	case kParamType_AnimationGroup:
	{
		UInt32 animGroup = arg->GetAnimationGroup();
		if (animGroup != TESAnimGroup::kAnimGroup_Max)
		{
			UInt32* out = va_arg(varArgs, UInt32*);
			*out = animGroup;
		}
		else
			return false;
	}
	break;
	case kParamType_Sex:
	{
		UInt32 sex = arg->GetSex();
		if (sex != -1)
		{
			UInt32 *out = va_arg(varArgs, UInt32 *);
			*out = sex;
		}
		else
		{
			return false;
		}
	}
	break;
	case kParamType_MagicEffect:
	{
		return false;
	}
	break;
	default: // all the rest are TESForm
	{
		if (arg->CanConvertTo(kTokenType_Form) || arg->type == kTokenType_Number && arg->formOrNumber)
		{
			TESForm *form = arg->GetTESForm();

			if (!bConvertTESForms)
			{
				// we're done
				TESForm **out = va_arg(varArgs, TESForm **);
				*out = form;
			}
			else if (form)
			{ // expect form != null
				// gotta make sure form matches expected type, do pointer cast
				switch (info->typeID)
				{
				case kParamType_ObjectID:
					if (form->IsInventoryObject())
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						return false;
					}
					break;
				case kParamType_ObjectRef:
				case kParamType_MapMarker:
				{
					auto refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
					if (refr)
					{
						// kParamType_MapMarker must be a mapmarker refr
						if (info->typeID == kParamType_MapMarker && !refr->IsMapMarker())
						{
							return false;
						}

						TESObjectREFR **out = va_arg(varArgs, TESObjectREFR **);
						*out = refr;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_Actor:
				{
					auto actor = DYNAMIC_CAST(form, TESForm, Actor);
					if (actor)
					{
						Actor **out = va_arg(varArgs, Actor **);
						*out = actor;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_SpellItem:
				{
					auto spell = DYNAMIC_CAST(form, TESForm, SpellItem);
					if (spell || form->typeID == kFormType_TESObjectBOOK)
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_Cell:
				{
					auto cell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
					if (cell)
					{
						TESObjectCELL **out = va_arg(varArgs, TESObjectCELL **);
						*out = cell;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_MagicItem:
				{
					auto magic = DYNAMIC_CAST(form, TESForm, MagicItem);
					if (magic)
					{
						MagicItem **out = va_arg(varArgs, MagicItem **);
						*out = magic;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_TESObject:
				{
					auto object = DYNAMIC_CAST(form, TESForm, TESObject);
					if (object)
					{
						TESObject **out = va_arg(varArgs, TESObject **);
						*out = object;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_ActorBase:
				{
					auto base = DYNAMIC_CAST(form, TESForm, TESActorBase);
					if (base)
					{
						TESActorBase **out = va_arg(varArgs, TESActorBase **);
						*out = base;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_Container:
				{
					auto refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
					if (refr && refr->GetContainer())
					{
						TESObjectREFR **out = va_arg(varArgs, TESObjectREFR **);
						*out = refr;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_WorldSpace:
				{
					auto space = DYNAMIC_CAST(form, TESForm, TESWorldSpace);
					if (space)
					{
						TESWorldSpace **out = va_arg(varArgs, TESWorldSpace **);
						*out = space;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_AIPackage:
				{
					auto pack = DYNAMIC_CAST(form, TESForm, TESPackage);
					if (pack)
					{
						TESPackage **out = va_arg(varArgs, TESPackage **);
						*out = pack;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_CombatStyle:
				{
					auto style = DYNAMIC_CAST(form, TESForm, TESCombatStyle);
					if (style)
					{
						TESCombatStyle **out = va_arg(varArgs, TESCombatStyle **);
						*out = style;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_LeveledOrBaseChar:
				{
					auto NPC = DYNAMIC_CAST(form, TESForm, TESNPC);
					if (NPC)
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						auto lev = DYNAMIC_CAST(form, TESForm, TESLevCharacter);
						if (lev)
						{
							TESForm **out = va_arg(varArgs, TESForm **);
							*out = form;
						}
						else
						{
							return false;
						}
					}
				}
				break;
				case kParamType_LeveledOrBaseCreature:
				{
					auto crea = DYNAMIC_CAST(form, TESForm, TESCreature);
					if (crea)
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						auto lev = DYNAMIC_CAST(form, TESForm, TESLevCreature);
						if (lev)
						{
							TESForm **out = va_arg(varArgs, TESForm **);
							*out = form;
						}
						else
						{
							return false;
						}
					}
				}
				break;
				case kParamType_Owner:
				case kParamType_AnyForm:
				{
					TESForm **out = va_arg(varArgs, TESForm **);
					*out = form;
				}
				break;
				case kParamType_InvObjOrFormList:
				{
					if (form->IsInventoryObject() || (form->typeID == kFormType_BGSListForm))
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						return false;
					}
				}
				break;
				case kParamType_NonFormList:
				{
					if (form->IsBoundObject() && (form->typeID != kFormType_BGSListForm))
					{
						TESForm **out = va_arg(varArgs, TESForm **);
						*out = form;
					}
					else
					{
						return false;
					}
				}
				break;
				default:
					// these all check against a particular formtype, return TESForm*
					{
						UInt32 typeToMatch = -1;
						switch (info->typeID)
						{
						case kParamType_Sound:
							typeToMatch = kFormType_TESSound;
							break;
						case kParamType_Topic:
							typeToMatch = kFormType_TESTopic;
							break;
						case kParamType_Quest:
							typeToMatch = kFormType_TESQuest;
							break;
						case kParamType_Race:
							typeToMatch = kFormType_TESRace;
							break;
						case kParamType_Faction:
							typeToMatch = kFormType_TESFaction;
							break;
						case kParamType_Class:
							typeToMatch = kFormType_TESClass;
							break;
						case kParamType_Global:
							typeToMatch = kFormType_TESGlobal;
							break;
						case kParamType_Furniture:
							typeToMatch = kFormType_TESFurniture;
							break;
						case kParamType_FormList:
							typeToMatch = kFormType_BGSListForm;
							break;
						case kParamType_WeatherID:
							typeToMatch = kFormType_TESWeather;
							break;
						case kParamType_NPC:
							typeToMatch = kFormType_TESNPC;
							break;
						case kParamType_EffectShader:
							typeToMatch = kFormType_TESEffectShader;
							break;
						case kParamType_MenuIcon:
							typeToMatch = kFormType_BGSMenuIcon;
							break;
						case kParamType_Perk:
							typeToMatch = kFormType_BGSPerk;
							break;
						case kParamType_Note:
							typeToMatch = kFormType_BGSNote;
							break;
						case kParamType_ImageSpaceModifier:
							typeToMatch = kFormType_TESImageSpaceModifier;
							break;
						case kParamType_ImageSpace:
							typeToMatch = kFormType_TESImageSpace;
							break;
						case kParamType_EncounterZone:
							typeToMatch = kFormType_BGSEncounterZone;
							break;
						case kParamType_IdleForm:
							typeToMatch = kFormType_TESIdleForm;
							break;
						case kParamType_Message:
							typeToMatch = kFormType_BGSMessage;
							break;
						case kParamType_SoundFile:
							typeToMatch = kFormType_BGSMusicType;
							break;
						case kParamType_LeveledChar:
							typeToMatch = kFormType_TESLevCharacter;
							break;
						case kParamType_LeveledCreature:
							typeToMatch = kFormType_TESLevCreature;
							break;
						case kParamType_LeveledItem:
							typeToMatch = kFormType_TESLevItem;
							break;
						case kParamType_Reputation:
							typeToMatch = kFormType_TESReputation;
							break;
						case kParamType_Casino:
							typeToMatch = kFormType_TESCasino;
							break;
						case kParamType_CasinoChip:
							typeToMatch = kFormType_TESCasinoChips;
							break;
						case kParamType_Challenge:
							typeToMatch = kFormType_TESChallenge;
							break;
						case kParamType_CaravanMoney:
							typeToMatch = kFormType_TESCaravanMoney;
							break;
						case kParamType_CaravanCard:
							typeToMatch = kFormType_TESCaravanCard;
							break;
						case kParamType_CaravanDeck:
							typeToMatch = kFormType_TESCaravanDeck;
							break;
						case kParamType_Region:
							typeToMatch = kFormType_TESRegion;
							break;
						}

						if (form->typeID == typeToMatch)
						{
							TESForm **out = va_arg(varArgs, TESForm **);
							*out = form;
						}
						else
						{
							return false;
						}
					}
				}
			}
			else
			{
				// null form
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	}

	return true;
}

std::unique_ptr<ScriptToken> ExpressionEvaluator::ExecuteCommandToken(ScriptToken const *token, TESObjectREFR *stackRef = nullptr)
{
	// execute the command
	CommandInfo *cmdInfo = token->GetCommandInfo();
	if (!cmdInfo)
	{
		Error("Failed to execute command: command info not found.");
		return nullptr;
	}

	TESObjectREFR *callingObj = m_thisObj;
	Script::RefVariable *callingRef = nullptr;
	if (const auto refIdx = token->GetRefIndex())
	{
		callingRef = script->GetRefFromRefList(refIdx);
		if (callingRef)
		{
			callingRef->Resolve(eventList);
			if (!callingRef->form)
			{
				Error("Attempting to call a function on a NULL reference");
				return nullptr;
			}
			callingObj = DYNAMIC_CAST(callingRef->form, TESForm, TESObjectREFR);
			if (!callingObj)
			{
				Error("Attempting to call a function on a base object (this must be a reference)");
				return nullptr;
			}
		}
	}
	else if (stackRef)
	{
		// Used in chained dot syntax (https://geckwiki.com/index.php?title=Chained_dot_syntax)
		callingObj = stackRef;
	}

	if (cmdInfo->needsParent && !callingObj)
	{
		Error("Function %s requires a calling reference but none or NULL was provided", cmdInfo->longName);
		return nullptr;
	}

	TESObjectREFR *contObj = callingRef ? nullptr : m_containingObj;
	double cmdResult = 0;


	UInt32 opcodeOffset = token->cmdOpcodeOffset;
	CommandReturnType retnType = token->returnType;

	ExpectReturnType(kRetnType_Default); // expect default return type unless called command specifies otherwise
	const bool bExecuted = cmdInfo->execute(cmdInfo->params, m_scriptData, callingObj, contObj, script, eventList, &cmdResult, &opcodeOffset);

	if (!bExecuted)
	{
		Error("Command %s failed to execute", cmdInfo->longName);
		return nullptr;
	}


	bool const retnTypeAmb = retnType == kRetnType_Ambiguous || retnType == kRetnType_ArrayIndex;
	if (retnTypeAmb) // return type ambiguous, cmd will inform us of type to expect
	{
		retnType = GetExpectedReturnType();
	}
	
	std::unique_ptr<ScriptToken> tokRes = nullptr;
	switch (retnType)
	{
	case kRetnType_Default:
	{
		tokRes = ScriptToken::Create(cmdResult);
		// since there are no return types in most commands, we check if it's possible that it returned a form
		if (*(reinterpret_cast<UInt32 *>(&cmdResult) + 1) == 0 && LookupFormByID((*reinterpret_cast<UInt32 *>(&cmdResult))))
			tokRes->formOrNumber = true; // Can be either
		break;
	}
	case kRetnType_Form:
	{
		tokRes = ScriptToken::CreateForm(*reinterpret_cast<UInt32 *>(&cmdResult));
		break;
	}
	case kRetnType_String:
	{
		StringVar *strVar = g_StringMap.Get(cmdResult);
		if (!strVar)
			Error("Failed to resolve string return result (string ID was %g)", cmdResult);
		tokRes = ScriptToken::Create(nullptr, strVar);
		break;
	}
	case kRetnType_Array:
	{
		// ###TODO: cmds can return arrayID '0', not necessarily an error, does this support that?
		if (g_ArrayMap.Get(cmdResult) || !cmdResult)
		{
			tokRes = ScriptToken::CreateArray(cmdResult);
		}
		else
			Error("A command returned an invalid array");
		break;
	}
	default:
		Error("Unknown command return type %d while executing command in ExpressionEvaluator::Evaluate()", retnType);
	}

	if (tokRes && retnTypeAmb)
	{
		//Inform that this ScriptToken is the result of an ambiguous function call.
		tokRes->returnType = kRetnType_Ambiguous;	
	}
	
	return tokRes;
}

using CachedTokenIter = Vector<TokenCacheEntry>::Iterator;
TokenCacheEntry *GetOperatorParent(TokenCacheEntry *iter, TokenCacheEntry *iterEnd)
{
	iter++;
	int count = 0;
	while (iter < iterEnd)
	{
		const ScriptToken *token = iter->token;
		if (token->IsOperator())
		{
			if (count == 0)
			{
				return iter;
			}

			// e.g. `1 0 ! &&` prevent ! being parent of 1
			count -= token->GetOperator()->numOperands - 1;
		}
		else
		{
			count++;
		}
		if (count == 0)
		{
			return iter;
		}
		iter++;
	}
	return iterEnd;
}

constexpr auto g_noShortCircuit = kOpType_Max;

void ParseShortCircuit(CachedTokens &cachedTokens)
{
	TokenCacheEntry *end = cachedTokens.DataEnd();
	for (auto iter = cachedTokens.Begin(); !iter.End(); ++iter)
	{
		TokenCacheEntry *curr = &iter.Get();
		ScriptToken &token = *curr->token;
		TokenCacheEntry *grandparent = curr;
		TokenCacheEntry *furthestParent;
		TokenCacheEntry* nextNeighbor;
		do
		{
			// Find last "parent" operator of same type. E.g `0 1 && 1 && 1 &&` should jump straight to end of expression.
			furthestParent = grandparent;
			grandparent = GetOperatorParent(grandparent, end);
			nextNeighbor = furthestParent + 1;
		} while (grandparent < end && grandparent->token->IsLogicalOperator() && (furthestParent == curr || 
			grandparent->token->GetOperator() == furthestParent->token->GetOperator() && (nextNeighbor == end || !nextNeighbor->token->IsOperator())));

		if (furthestParent != curr && furthestParent->token->IsLogicalOperator())
		{
			token.shortCircuitParentType = furthestParent->token->GetOperator()->type;
			token.shortCircuitDistance = furthestParent - curr;

			auto *parent = GetOperatorParent(curr, end);
			token.shortCircuitStackOffset = curr + 1 == parent ? 2 : 1;
		}
	}
}

bool ExpressionEvaluator::ParseBytecode(CachedTokens &cachedTokens)
{
	const UInt8 *dataBeforeParsing = m_data;
	const UInt16 argLen = Read16();
	const UInt8 *endData = m_data + argLen - sizeof(UInt16);
	while (m_data < endData)
	{
		auto *token = ScriptToken::Read(this);
		if (!token)
			return false;
		cachedTokens.Append(token);
	}
	cachedTokens.incrementData = m_data - dataBeforeParsing;
	ParseShortCircuit(cachedTokens);
	return true;
}

using OperandStack = FastStack<ScriptToken *>;

bool ShortCircuit(OperandStack &operands, CachedTokenIter &iter)
{
	ScriptToken *lastToken = operands.Top();
	const OperatorType type = lastToken->shortCircuitParentType;
	if (type == g_noShortCircuit)
		return true;

	const bool eval = lastToken->GetBool();
	if (type == kOpType_LogicalAnd && !eval || type == kOpType_LogicalOr && eval)
	{
		iter += lastToken->shortCircuitDistance;
		for (UInt32 i = 0; i < lastToken->shortCircuitStackOffset; ++i)
		{
			if (operands.Empty())
				return false;
			// Make sure only one operand is left in RPN stack
			ScriptToken *operand = operands.Top();
			if (operand && operand != lastToken)
				delete operand;
			operands.Pop();
		}
		operands.Push(lastToken);
	}
	return true;
}

void CopyShortCircuitInfo(ScriptToken *to, ScriptToken *from)
{
	to->shortCircuitParentType = from->shortCircuitParentType;
	to->shortCircuitDistance = from->shortCircuitDistance;
	to->shortCircuitStackOffset = from->shortCircuitStackOffset;
}

thread_local TokenCache g_tokenCache;

#if _DEBUG && RUNTIME
thread_local std::string g_curLineText;
#endif

CachedTokens* ExpressionEvaluator::GetTokens(std::optional<CachedTokens>* consoleTokensContainer)
{
	// consoleTokensContainer serves as storage for CachedTokens if scriptData is not permanent memory
	const bool isConsole = script->GetModIndex() == 0xFF && consoleTokensContainer;
	CachedTokens &cache = !isConsole ? g_tokenCache.Get(GetCommandOpcodePosition(m_opcodeOffsetPtr)) : *(*consoleTokensContainer = CachedTokens());
	if (isConsole)
		cache.Clear();
	if (cache.Empty() || isConsole)
	{
		if (!ParseBytecode(cache))
		{
			Error("Failed to parse script data");
			return nullptr;
		}
	}
	else
	{
		m_data += cache.incrementData;
	}
	// adjust opcode offset ptr (important for recursive calls to Evaluate()
	*m_opcodeOffsetPtr += cache.incrementData;
	return &cache;
}

ScriptToken *ExpressionEvaluator::Evaluate()
{
	CachedTokens* cachePtr = GetTokens(&this->consoleTokens);
	if (!cachePtr)
		return nullptr;
	auto& cache = *cachePtr;
#if _DEBUG && 0
	g_curLineText = this->GetLineText(cache, nullptr);
#endif
	OperandStack operands;
	auto iter = cache.Begin();
	for (; !iter.End(); ++iter)
	{
		TokenCacheEntry &entry = iter.Get();
		ScriptToken *curToken = entry.token;
		curToken->context = this;

		if (curToken->Type() != kTokenType_Operator)
		{
			if (curToken->Type() == kTokenType_Command && !curToken->useRefFromStack)
			{
				auto const cmdToken = ExecuteCommandToken(curToken).release();
				if (cmdToken == nullptr)
				{
					break;
				}
				CopyShortCircuitInfo(cmdToken, curToken);
				curToken = cmdToken;
			}
			else if (curToken->IsVariable() && !curToken->ResolveVariable())
			{
				Error("Failed to resolve variable");
				break;
			}
			else if (curToken->type == kTokenType_LambdaScriptData)
			{
				// There needs to be a unique lambda per script event list so that variables can have the correct values
				// curToken needs not be deleted since it's always cached
				auto *script = CreateLambdaScript(GetCommandOpcodePosition(m_opcodeOffsetPtr), curToken->value.lambdaScriptData, *this);
				if (!script)
				{
					Error("Failed to create lambda script");
					break;
				}
				curToken = ScriptToken::Create(script).release();
			}
			operands.Push(curToken);
		}
		else
		{
			Operator *op = curToken->GetOperator();
			ScriptToken *lhOperand = nullptr;
			ScriptToken *rhOperand = nullptr;

			if (op->numOperands > operands.Size())
			{
				Error("Too few operands for operator %s", op->symbol);
				break;
			}

			switch (op->numOperands)
			{
			case 2:
				rhOperand = operands.Top();
				operands.Pop();
			case 1:
				lhOperand = operands.Top();
				operands.Pop();
			}
	
			ScriptToken *opResult;
			if (entry.eval == nullptr)
			{
				opResult = op->Evaluate(lhOperand, rhOperand, this, entry.eval, entry.swapOrder).release();
			}
			else
			{
				opResult = entry.swapOrder ? entry.eval(op->type, rhOperand, lhOperand, this).release() : entry.eval(op->type, lhOperand, rhOperand, this).release();
			}

			delete lhOperand;
			delete rhOperand;

			if (!opResult)
			{
				Error("Operator %s failed to evaluate to a valid result", op->symbol);
				break;
			}

			opResult->context = this;
			CopyShortCircuitInfo(opResult, curToken);
			operands.Push(opResult);
		}
		if (!ShortCircuit(operands, iter))
		{
			Error("An internal NVSE error occurred (short circuit stack mismatch)");
			break;
		}
	}
	
	if (operands.Size() != 1 || (this->HasErrors() && !m_flags.IsSet(kFlag_SuppressErrorMessages))) // should have one operand remaining - result of expression
	{
		auto* faultingToken = !iter.End() ? iter.Get().token : nullptr;
		const auto currentLine = this->GetLineText(cache, faultingToken);
		if (!currentLine.empty())
		{
			auto* cmd = GetCommand();
			Error("Script line approximation: %s %s (error wrapped in ##'s)", cmd ? cmd->longName : "", currentLine.c_str());
			const auto variablesText = this->GetVariablesText(cache);
			if (!variablesText.empty())
				Error("\tWhere %s", variablesText.c_str());
		}
		else
		{
			if (operands.Size() != 1)
				Error("An expression failed to evaluate to a valid result. (Failed to approximate script line) 0");
			else
				Error("An expression failed to evaluate to a valid result. (Failed to approximate script line) 1");
		}
		while (operands.Size())
		{
			const auto *operand = operands.Top();
			delete operand;
			operands.Pop();
		}
		return nullptr;
	}

	return operands.Top();
}

std::string ExpressionEvaluator::GetLineText()
{
	ResetCursor();
	const UInt32 numArgs = ReadByte();
	std::string lineText;
	if (const auto* command = GetCommand())
	{
		lineText += std::string(command->longName) + " ";
	}
	for (int i = 0; i < numArgs; ++i)
	{
		std::optional<CachedTokens> consoleTokens;
		const auto tokens = this->GetTokens(&consoleTokens);
		const auto arg = this->GetLineText(*tokens, nullptr);
		if (numArgs != 1 && tokens->Size() > 1) // if multiple args, separate args with brackets
			lineText += '(' + arg + ')';
		else
			lineText += arg;
		if (i != numArgs - 1)
			lineText += ' ';
	}
	return lineText;
}

std::string ExpressionEvaluator::GetLineText(CachedTokens &tokens, ScriptToken *faultingToken) const
{
	std::vector<std::string> operands;
	std::vector<Operator*> operators;
	std::unordered_set<std::string> composites;
	for (auto iter = tokens.Begin(); !iter.End(); ++iter)
	{
		auto &token = *iter.Get().token;
		if (!token.IsOperator())
		{
			switch (token.Type())
			{
			case kTokenType_Number:
				operands.push_back(FormatString("%g", token.GetNumber()));
				break;
			case kTokenType_String:
			{
				auto str = std::string(token.GetString());
				UnformatString(str);
				operands.push_back('"' + str + '"');
				break;
			}
			case kTokenType_Form:
			{
				auto *form = token.GetTESForm();
				if (form)
				{
					auto *formName = form->GetEditorID();
					if (!formName || StrLen(formName) == 0)
					{
						operands.push_back(FormatString("<%X>", form->refID));
					}
					else
						operands.push_back(std::string(formName));
					break;
				}
				operands.push_back("<bad form>");
				break;
			}

			case kTokenType_Global:
			{
				auto *global = token.GetGlobal();
				if (global)
				{
					operands.push_back(std::string(global->name.CStr()));
					break;
				}
				operands.push_back("<bad global>");
				break;
			}

			case kTokenType_Command:
			{
				auto *cmdInfo = token.GetCommandInfo();
				if (cmdInfo)
				{
					auto callToken = token.GetCallToken(script);
					if (callToken.error)
					{
						operands.push_back("<failed resolve cmd>");
						break;
					}
					operands.push_back(callToken.ToString());
					bool appliedParams = false;
					if (!callToken.expressionEvalArgs.empty() && !(iter+1).End())
					{
						appliedParams = true;
						operands.back() = '(' + operands.back() + ')';
					}
					std::span paramInfo {cmdInfo->params, cmdInfo->numParams};
					if (!appliedParams && std::ranges::any_of(paramInfo, [](auto& p) {return p.isOptional;}) && !(iter+1).End())
						operands.back() = '(' + operands.back() + ')';
					break;
				}
				operands.push_back("<bad command>");
				break;
			}
			case kTokenType_NumericVar:
			case kTokenType_RefVar:
			case kTokenType_StringVar:
			case kTokenType_ArrayVar:
			{
				const auto varName = token.GetVariableName(script);
				if (!varName.empty())
				{
					operands.push_back(varName);
					break;
				}
				operands.push_back(FormatString("<failed to get var name %d %d>", token.refIdx, token.varIdx));
				break;
			}
			case kTokenType_LambdaScriptData:
			{
				auto scriptLambda = MakeUnique<Script>(CreateLambdaScript(token.value.lambdaScriptData, script));
				ScriptParsing::ScriptAnalyzer analyzer(scriptLambda.get());
				analyzer.isLambdaScript = true;
				auto result = analyzer.DecompileScript();
				
				// trim whitespace
				while (!result.empty() && isspace(static_cast<unsigned char>(*result.begin())))
					result.erase(result.begin());
				while (!result.empty() && isspace(static_cast<unsigned char>(result.back())))
					result.pop_back();

				operands.push_back(result);
				break;
			}
			case kTokenType_OptionalEmpty: {
				operands.push_back("_");
				break;
			}
			default:
				operands.emplace_back("<can't decompile token>");
				break;
			}
		}
		else
		{
			auto *op = token.GetOperator();
			if (!op)
				return "";
			if (operands.size() < op->numOperands)
				return "";

			if (!operators.empty() && !operands.empty())
			{
				auto* lastOp = operators.back();
				operators.pop_back();
				if (lastOp->precedence < op->precedence)
				{
					auto& rhOperand = operands.back();
					if (composites.contains(rhOperand))
						rhOperand = '(' + rhOperand + ')';
					if (operands.size() > 1)
					{
						auto& lhOperand = operands.at(operands.size() - 2);
						if (composites.contains(lhOperand))
							lhOperand = '(' + lhOperand + ')';
					}
				}
			}
			operators.push_back(op);

			if (op->numOperands == 2)
			{
				auto rh = operands.back();
				operands.pop_back();
				auto lh = operands.back();
				operands.pop_back();

				if (op->type == kOpType_LeftBracket)
					operands.push_back(lh + '[' + rh + ']');
				else if (op->type == kOpType_Slice || op->type == kOpType_Exponent || op->type == kOpType_MemberAccess || op->type == kOpType_MakePair || op->type == kOpType_Dot)
					operands.push_back(lh + std::string(op->symbol) + rh);
				else
					operands.push_back(lh + " " + std::string(op->symbol) + " " + rh);
			}
			else if (op->numOperands == 1)
			{
				auto& operand = operands.back();
				operand = std::string(op->symbol) + operand;
			}
			composites.insert(operands.back());
		}
		if (faultingToken && faultingToken == &token && !operands.empty())
		{
			auto& lastStr = operands.back();
			lastStr = "##" + lastStr + "##";
		}
	}
	if (operands.size() == 1)
	{
		if (m_inline)
			return "(" + operands.back() + ")";
		return operands.back();
	}
	return "";
}

std::string ExpressionEvaluator::GetVariablesText(CachedTokens &tokens) const
{
	std::string result;
	std::set<std::pair<UInt32, UInt32>> printedVars;
	for (auto iter = tokens.Begin(); !iter.End(); ++iter)
	{
		auto &token = *iter.Get().token;
		if (printedVars.find(std::make_pair(token.refIdx, token.varIdx)) != printedVars.end())
			continue;
		if (token.IsVariable())
		{
			result += token.GetVariableName(script) + "=" + token.GetVariableDataAsString();
			printedVars.insert(std::make_pair(token.refIdx, token.varIdx));
			result += ", ";
		}
		else if (token.Type() == kTokenType_Command && token.refIdx)
		{
			auto *ref = script->GetRefFromRefList(token.refIdx);
			if (ref && ref->varIdx)
			{
				auto *varInfo = script->GetVariableInfo(ref->varIdx);
				if (varInfo && varInfo->name.m_bufLen)
				{
					auto *var = eventList->GetVariable(varInfo->idx);
					if (var)
					{
						const auto *varName = varInfo->name.CStr();
						auto *form = LookupFormByID(var->GetFormId());
						if (form)
							result += FormatString("%s=%s", varName, form->GetStringRepresentation().c_str());
						else if (var->GetFormId())
							result += FormatString("%s=invalid form (%x)", varName, var->GetFormId());
						else
							result += FormatString("%s=uninitialized form (0)", varName);
					}
				}
			}
			result += ", ";
		}
	}
	for (auto i = 0; i < 2 && !result.empty(); ++i)
		result.pop_back(); // remove ", "
	return result;
}

std::string ExpressionEvaluator::GetVariablesText()
{
	ResetCursor();
	const auto numArgs = ReadByte();
	std::string varText;
	for (int i = 0; i < numArgs; ++i)
	{
		std::optional<CachedTokens> consoleTokens;
		const auto tokens = this->GetTokens(&consoleTokens);
		varText += this->GetVariablesText(*tokens);
		if (i != numArgs - 1)
			varText += '\n';
	}
	return varText;
}

// Resets position of data pointer and opcode offset pointer
void ExpressionEvaluator::ResetCursor()
{
	*m_opcodeOffsetPtr = m_baseOffset + 4;
	m_data = m_scriptData + *m_opcodeOffsetPtr;
}

//	Pop required operand(s)
//	loop through OperationRules until a match is found
//	check operand(s)->CanConvertTo() for rule types (also swap them and test if !asymmetric)
//	if can convert --> pass to rule handler, return result :: else, continue loop
//	if no matching rule return null
std::unique_ptr<ScriptToken> Operator::Evaluate(ScriptToken *lhs, ScriptToken *rhs, ExpressionEvaluator *context, Op_Eval &cacheEval, bool &cacheSwapOrder)
{
	if (numOperands == 0) // how'd we get here?
	{
		context->Error("Attempting to evaluate %s but this operator takes no operands", this->symbol);
		return nullptr;
	}

	for (UInt32 i = 0; i < numRules; i++)
	{
		bool bRuleMatches = false;
		bool bSwapOrder = false;
		const OperationRule *rule = &rules[i];
		if (!rule->eval)
			continue;

		if (IsUnary() && lhs->CanConvertTo(rule->lhs))
			bRuleMatches = true;
		else
		{
			if (lhs->CanConvertTo(rule->lhs) && rhs->CanConvertTo(rule->rhs))
				bRuleMatches = true;
			else if (!rule->bAsymmetric && rhs->CanConvertTo(rule->lhs) && lhs->CanConvertTo(rule->rhs))
			{
				bSwapOrder = true;
				bRuleMatches = true;
			}
		}
		if (bRuleMatches)
		{
			auto shouldCache = true;
			if (lhs && lhs->Type() == kTokenType_ArrayElement || rhs && rhs->Type() == kTokenType_ArrayElement) // array elements depend on CanConvertTo to fail, can't cache eval
			{
				shouldCache = false;
			}

			auto constexpr isOperandResultOfAmbiguousFunction = [](ScriptToken* operand) -> bool
			{
				if (!operand) return false;
				return operand->returnType == kRetnType_Ambiguous;
			};

			//Cannot cache eval for ambiguous return types; function can return new type, requiring new eval function overload.
			if (isOperandResultOfAmbiguousFunction(lhs) || isOperandResultOfAmbiguousFunction(rhs))
				shouldCache = false;
			
			if (shouldCache)
			{
				cacheEval = rule->eval;
				cacheSwapOrder = bSwapOrder;
			}
			return bSwapOrder ? rule->eval(type, rhs, lhs, context) : rule->eval(type, lhs, rhs, context);
		}
	}

	// relay error message
	for (auto *token : {lhs, rhs})
	{
		if (auto const elemToken = dynamic_cast<ArrayElementToken *>(token))
		{
			if (!elemToken->GetElement())
			{
				context->Error("Array does not contain key");
				return nullptr;
			}
		}
	}

	const auto *lhsStr = TokenTypeToString(lhs->Type());
	auto *lhsElement = dynamic_cast<ArrayElementToken *>(lhs);
	if (lhsElement && lhsElement->GetElement())
		lhsStr = DataTypeToString(lhsElement->GetElement()->DataType());
	if (rhs)
	{
		const auto *rhsStr = TokenTypeToString(rhs->Type());
		auto *rhsElement = dynamic_cast<ArrayElementToken *>(rhs);
		if (rhsElement && rhsElement->GetElement())
			rhsStr = DataTypeToString(rhsElement->GetElement()->DataType());

		context->Error("Cannot evaluate %s %s %s", lhsStr, this->symbol, rhsStr);
	}
	else
	{
		context->Error("Operators %s cannot be used for type %s", this->symbol, lhsStr);
	}

	return nullptr;
}

#endif

enum BlockType
{
	kBlockType_Invalid = 0,
	kBlockType_ScriptBlock,
	kBlockType_Loop,
	kBlockType_If
};

struct Block
{
	enum
	{
		kFunction_Open = 1,
		kFunction_Terminate = 2,
		kFunction_Dual = kFunction_Open | kFunction_Terminate
	};

	const char *keyword;
	BlockType type;
	UInt8 function;

	bool IsOpener() { return (function & kFunction_Open) == kFunction_Open; }
	bool IsTerminator() { return (function & kFunction_Terminate) == kFunction_Terminate; }
};

struct BlockInfo
{
	BlockType type;
	UInt32 scriptLine;
};

static Block s_blocks[] =
	{
		// Commented out since for lambdas

		// {	"begin",	kBlockType_ScriptBlock,	Block::kFunction_Open	},
		// {	"end",		kBlockType_ScriptBlock,	Block::kFunction_Terminate	},
		{"while", kBlockType_Loop, Block::kFunction_Open},
		{"foreach", kBlockType_Loop, Block::kFunction_Open},
		{"loop", kBlockType_Loop, Block::kFunction_Terminate},
		{"if", kBlockType_If, Block::kFunction_Open},
		{"elseif", kBlockType_If, Block::kFunction_Dual},
		{"else", kBlockType_If, Block::kFunction_Dual},
		{"endif", kBlockType_If, Block::kFunction_Terminate},
};

static UInt32 s_numBlocks = SIZEOF_ARRAY(s_blocks, Block);

// Preprocessor
//	is used to check loop integrity, syntax before script is compiled
class Preprocessor
{
private:
	static std::string s_delims;

	ScriptBuffer *m_buf;
	UInt32 m_loopDepth;
	std::string m_curLineText;
	UInt32 m_curLineNo;
	UInt32 m_curBlockStartingLineNo;
	std::string m_scriptText;
	UInt32 m_scriptTextOffset;
	Script *m_script;

	bool HandleDirectives(); // compiler directives at top of script prefixed with '@'
	bool ProcessBlock(BlockType blockType);
	bool AdvanceLine();
	const char *BlockTypeAsString(BlockType type);

public:
	Preprocessor(ScriptBuffer *buf);
	void RemoveMultiLineComments();

	bool Process(); // returns false if an error is detected
};

std::string Preprocessor::s_delims = " \t\r\n(;";

const char *Preprocessor::BlockTypeAsString(BlockType type)
{
	switch (type)
	{
	case kBlockType_ScriptBlock:
		return "Begin/End";
	case kBlockType_Loop:
		return "Loop";
	case kBlockType_If:
		return "If/EndIf";
	default:
		return "Unknown block type";
	}
}

Preprocessor::Preprocessor(ScriptBuffer *buf) : m_buf(buf), m_loopDepth(0), m_curLineText(""), m_curLineNo(0),
												m_curBlockStartingLineNo(1), m_scriptText(buf->scriptText), m_scriptTextOffset(0), m_script(g_currentScriptStack.top())
{
	RemoveMultiLineComments();
	AdvanceLine();
}

void Preprocessor::RemoveMultiLineComments()
{
	bool inString = false;
	bool inComment = false;
	std::string newScriptText;
	char curChar;
	const auto proceed = [&]() { newScriptText += curChar; };
	for (auto iter = m_scriptText.begin(); iter != m_scriptText.end(); ++iter)
	{
		curChar = *iter;
		auto nextIter = iter + 1;
		if (nextIter == m_scriptText.end())
		{
			proceed();
			continue;
		}
		const auto nextChar = *nextIter;
		if (curChar == '"')
			inString = !inString;
		if (inString)
		{
			proceed();
			continue;
		}
		if (curChar == '/' && nextChar == '*')
			inComment = true;
		else if (curChar == '*' && nextChar == '/')
		{
			++iter;
			inComment = false;
			continue;
		}
		if (!inComment)
			proceed();
	}
	m_scriptText = newScriptText;
}

bool Preprocessor::AdvanceLine()
{
	if (m_scriptTextOffset >= m_scriptText.length())
		return false;

	m_curLineNo++;

	const UInt32 endPos = m_scriptText.find("\r\n", m_scriptTextOffset);

	if (endPos == -1) // last line, no CRLF
	{
		m_curLineText = m_scriptText.substr(m_scriptTextOffset, m_scriptText.length() - m_scriptTextOffset);
		m_scriptTextOffset = m_scriptText.length();
		return true;
	}
	if (m_scriptTextOffset == endPos) // empty line
	{
		m_scriptTextOffset += 2;
		return AdvanceLine();
	}
	// line contains text
	m_curLineText = m_scriptText.substr(m_scriptTextOffset, endPos - m_scriptTextOffset);

	// strip comments
	for (UInt32 i = 0; i < m_curLineText.length(); i++)
	{
		if (m_curLineText[i] == '"')
		{
			if (i + 1 == m_curLineText.length()) // trailing, mismatched quote - CS will catch
				break;

			i = m_curLineText.find('"', i + 1);
			if (i == -1) // mismatched quotes, CS compiler will catch
				break;
			else
				i++;
		}
		else if (m_curLineText[i] == ';')
		{
			m_curLineText = m_curLineText.substr(0, i);
			break;
		}
	}

	m_scriptTextOffset = endPos + 2;
	return true;
}

bool Preprocessor::HandleDirectives()
{
	// does nothing at present
	return true;
}

bool Preprocessor::Process()
{
	std::string token;
	std::stack<BlockType> blockStack;

	if (!HandleDirectives())
	{
		return false;
	}

	bool bContinue = true;
	while (bContinue)
	{
		Tokenizer tokens(m_curLineText.c_str(), s_delims.c_str());
		if (tokens.NextToken(token) == -1) // empty line
		{
			bContinue = AdvanceLine();
			continue;
		}

		const char *tok = token.c_str();
		bool bIsBlockKeyword = false;
		for (UInt32 i = 0; i < s_numBlocks; i++)
		{
			Block *cur = &s_blocks[i];
			if (!_stricmp(tok, cur->keyword))
			{
				bIsBlockKeyword = true;
				if (cur->IsTerminator())
				{
					if (blockStack.empty() || blockStack.top() != cur->type)
					{
						const char *blockStr = BlockTypeAsString(!blockStack.empty() ? blockStack.top() : cur->type);
						g_ErrOut.Show("Invalid %s block structure on line %d.", blockStr, m_curLineNo);
						return false;
					}

					blockStack.pop();
					if (cur->type == kBlockType_Loop)
						m_loopDepth--;
				}

				if (cur->IsOpener())
				{
					blockStack.push(cur->type);
					if (cur->type == kBlockType_Loop)
						m_loopDepth++;
				}
			}
		}

		if (!bIsBlockKeyword)
		{
			if (!_stricmp(tok, "continue") || !_stricmp(tok, "break"))
			{
				if (!m_loopDepth)
				{
					g_ErrOut.Show("Error line %d:\nFunction %s must be called from within a loop.", m_curLineNo, tok);
					return false;
				}
			}
			else if (!_stricmp(tok, "set"))
			{
				std::string varToken;
				if (tokens.NextToken(varToken) != -1)
				{
					std::string varName = varToken;
					const char *scriptText = m_buf->scriptText;

					UInt32 dotPos = varToken.find('.');
					if (dotPos != -1)
					{
						scriptText = nullptr;
						std::string s = varToken.substr(0, dotPos);
						const char *temp = s.c_str();
						TESForm *refForm = GetFormByID(temp);
						//TESForm* refForm = GetFormByID(varToken.substr(0, dotPos).c_str());
						if (refForm)
						{
							Script *refScript = GetScriptFromForm(refForm);
							if (refScript)
								scriptText = refScript->text;
						}
						varName = varToken.substr(dotPos + 1);
					}

					if (scriptText)
					{
						auto const varType = GetDeclaredVariableType(varName.c_str(), scriptText, m_buf->currentScript);
						if (varType == Script::eVarType_Array)
						{
							g_ErrOut.Show("Error line %d:\nSet may not be used to assign to an array variable", m_curLineNo);
							return false;
						}
						else if (false && varType == Script::eVarType_String) // this is un-deprecated b/c older plugins don't register return types
						{
							g_ErrOut.Show("Error line %d:\nUse of Set to assign to string variables is deprecated. Use Let instead.", m_curLineNo);
							return false;
						}
					}
				}
			}
			else if (!_stricmp(tok, "return") && m_loopDepth)
			{
				g_ErrOut.Show("Error line %d:\nReturn cannot be called within the body of a loop.", m_curLineNo);
				return false;
			}
			else if (const auto type = VariableTypeNameToType(tok); type != Script::eVarType_Invalid)
			{
				auto line = tokens.ToNewLine();
				if (ra::all_of(line, _L(char c, isalnum(c) || c == '_' || c == ',' || isspace(c)))) // ignore `int i = 0`
				{
					auto varNames = SplitString(line, ",");
					if (!ra::all_of(varNames, _L(auto & varName, ValidateVariable(StripSpace(std::move(varName)), type, m_script))))
						return false;
				}
			}
			else
			{
				// ###TODO: check for ResetAllVariables, anything else?
			}
		}

		if (bContinue)
			bContinue = AdvanceLine();
	}

	if (!blockStack.empty())
	{
		g_ErrOut.Show("Error: Mismatched block structure.");
		return false;
	}
	return true;
}

bool PrecompileScript(ScriptBuffer *buf)
{
	return Preprocessor(buf).Process();
}

bool Cmd_Expression_Parse(UInt32 numParams, ParamInfo *paramInfo, ScriptLineBuffer *lineBuf, ScriptBuffer *scriptBuf)
{
	ExpressionParser parser(scriptBuf, lineBuf);
	return (parser.ParseArgs(paramInfo, numParams));
}

ScriptLineMacro::ScriptLineMacro(ModifyFunction modifyFunction, MacroType type) : modifyFunction_(std::move(modifyFunction)), type(type)
{
}

ScriptLineMacro::MacroResult ScriptLineMacro::EvalMacro(ScriptLineBuffer *lineBuf, ScriptBuffer* scriptBuf, ExpressionParser *parser) const
{
	auto *str = lineBuf->paramText + lineBuf->lineOffset;
	std::string copy = str;
	if (!modifyFunction_(copy, scriptBuf, lineBuf))
		return MacroResult::Skipped;
	const auto charsLeft = sizeof lineBuf->paramText - lineBuf->lineOffset;
	if (copy.size() > charsLeft)
	{
		g_ErrOut.Show("Line length limit reached, could not translate macro.");
		return MacroResult::Error;
	}
	strcpy_s(str, charsLeft, copy.c_str());
	lineBuf->paramTextLen = copy.size() + lineBuf->lineOffset;
	if (parser)
		parser->m_len = lineBuf->paramTextLen;
	return MacroResult::Applied;
}
