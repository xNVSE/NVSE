#include "ScriptUtils.h"
#include "CommandTable.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "Hooks_Script.h"
#include <string>
#include "ParamInfos.h"
#include "FunctionScripts.h"
#include "GameRTTI.h"

#if RUNTIME

#ifdef DBG_EXPR_LEAKS
	SInt32 FUNCTION_CONTEXT_COUNT = 0;
#endif

#include "GameData.h"
#include "common/ICriticalSection.h"
#include "ThreadLocal.h"
#include "PluginManager.h"

const char* GetEditorID(TESForm* form)
{
	return NULL;
}

static void ShowError(const char* msg)
{
	Console_Print(msg);
	_MESSAGE(msg);
}

static bool ShowWarning(const char* msg)
{
	Console_Print(msg);
	_MESSAGE(msg);
	return false;
}

#else

#include "GameApi.h"

const char* GetEditorID(TESForm* form)
{
	return form->editorData.editorID.m_data;
}

static void ShowError(const char* msg)
{
	MessageBox(NULL, msg, "NVSE", MB_ICONEXCLAMATION);
}

static bool ShowWarning(const char* msg)
{
	char msgText[0x400];
	sprintf_s(msgText, sizeof(msgText), "%s\n\n'Cancel' will disable this message for the remainder of the session.", msg);
	int result = MessageBox(NULL, msgText, "NVSE", MB_OKCANCEL | MB_ICONEXCLAMATION);
	return result == IDCANCEL;
}

#endif

ErrOutput g_ErrOut(ShowError, ShowWarning);

#if RUNTIME

// run-time errors

enum {
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

const char* OpTypeToSymbol(OperatorType op);

ScriptToken* Eval_Comp_Number_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
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
		return NULL;
	}
}

ScriptToken* Eval_Comp_String_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const char* lhs = lh->GetString();
	const char* rhs = rh->GetString();
	switch (op)
	{
	case kOpType_GreaterThan:
		return ScriptToken::Create(_stricmp(lhs, rhs) > 0);
	case kOpType_LessThan:
		return ScriptToken::Create(_stricmp(lhs, rhs) < 0);
	case kOpType_GreaterOrEqual:
		return ScriptToken::Create(_stricmp(lhs, rhs) >= 0);
	case kOpType_LessOrEqual:
		return ScriptToken::Create(_stricmp(lhs, rhs) <= 0);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Eq_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(FloatEqual(lh->GetNumber(), rh->GetNumber()));
	case kOpType_NotEqual:
		return ScriptToken::Create(!(FloatEqual(lh->GetNumber(), rh->GetNumber())));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Eq_Array(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(lh->GetArray() == rh->GetArray());
	case kOpType_NotEqual:
		return ScriptToken::Create(lh->GetArray() != rh->GetArray());
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Eq_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const char* lhs = lh->GetString();
	const char* rhs = rh->GetString();
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(_stricmp(lhs, rhs) == 0);
	case kOpType_NotEqual:
		return ScriptToken::Create(_stricmp(lhs, rhs) != 0);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Eq_Form(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	bool result = false;
	TESForm* lhForm = lh->GetTESForm();
	TESForm* rhForm = rh->GetTESForm();
	if (lhForm == NULL && rhForm == NULL)
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
		return NULL;
	}
}

ScriptToken* Eval_Eq_Form_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	bool result = false;
	if (rh->GetNumber() == 0 && lh->GetFormID() == 0)	// only makes sense to compare forms to zero
		result = true;
	switch (op)
	{
	case kOpType_Equals:
		return ScriptToken::Create(result);
	case kOpType_NotEqual:
		return ScriptToken::Create(!result);
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Logical(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	switch (op)
	{
	case kOpType_LogicalAnd:
		return ScriptToken::Create(lh->GetBool() && rh->GetBool());
	case kOpType_LogicalOr:
		return ScriptToken::Create(lh->GetBool()|| rh->GetBool());
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Add_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh->GetNumber() + rh->GetNumber());
}

ScriptToken* Eval_Add_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(std::string(lh->GetString()) + std::string(rh->GetString()));
}

ScriptToken* Eval_Arithmetic(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double l = lh->GetNumber();
	double r = rh->GetNumber();
	switch (op)
	{
	case kOpType_Subtract:
		return ScriptToken::Create(l - r);
	case kOpType_Multiply:
		return ScriptToken::Create(l * r);
	case kOpType_Divide:
		if (r != 0)
			return ScriptToken::Create(l / r);
		else {
			context->Error("Division by zero");
			return NULL;
		}
	case kOpType_Exponent:
		return ScriptToken::Create(pow(l, r));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}

}

ScriptToken* Eval_Integer(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	SInt64 l = lh->GetNumber();
	SInt64 r = rh->GetNumber();

	switch (op)
	{
	case kOpType_Modulo:
		if (r != 0)
			return ScriptToken::Create(double(l % r));
		else {
			context->Error("Division by zero");
			return NULL;
		}
	case kOpType_BitwiseOr:
		return ScriptToken::Create(double(l | r));
	case kOpType_BitwiseAnd:
		return ScriptToken::Create(double(l & r));
	case kOpType_LeftShift:
		return ScriptToken::Create(double(l << r));
	case kOpType_RightShift:
		return ScriptToken::Create(double(l >> r));
	default:
		context->Error("Unhandled operator %s", OpTypeToSymbol(op));
		return NULL;
	}
}

ScriptToken* Eval_Assign_Numeric(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double result = rh->GetNumber();
	if (lh->GetVariableType() == Script::eVarType_Integer)
		result = floor(result);

	lh->GetVar()->data = result;
	return ScriptToken::Create(result);
}

ScriptToken* Eval_Assign_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 strID = lh->GetVar()->data;
	StringVar* strVar = g_StringMap.Get(strID);
	if (!strVar)
	{
		strID = g_StringMap.Add(context->script->GetModIndex(), rh->GetString());
		lh->GetVar()->data = strID;
	}
	else
		strVar->Set(rh->GetString());

	return ScriptToken::Create(rh->GetString());
}

ScriptToken* Eval_Assign_AssignableString(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	AssignableStringToken* aStr = dynamic_cast<AssignableStringToken*> (lh);
	return aStr->Assign(rh->GetString()) ? ScriptToken::Create(aStr->GetString()) : NULL;
}

ScriptToken* Eval_Assign_Form(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt64* outRefID = (UInt64*)&(lh->GetVar()->data);
	*outRefID = rh->GetFormID();
	return ScriptToken::CreateForm(rh->GetFormID());
}

ScriptToken* Eval_Assign_Form_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt64* outRefID = (UInt64*)&(lh->GetVar()->data);
	*outRefID = rh->GetFormID();
	return ScriptToken::CreateForm(rh->GetFormID());
}

ScriptToken* Eval_Assign_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetGlobal()->data = rh->GetNumber();
	return ScriptToken::Create(rh->GetNumber());
}

ScriptToken* Eval_Assign_Array(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	g_ArrayMap.AddReference(&lh->GetVar()->data, rh->GetArray(), context->script->GetModIndex());
	return ScriptToken::CreateArray(lh->GetVar()->data);
}

ScriptToken* Eval_Assign_Elem_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	if (!key)
		return NULL;

	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, rh->GetNumber()) ? ScriptToken::Create(rh->GetNumber()) : NULL;
}

ScriptToken* Eval_Assign_Elem_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	if (!key)
		return NULL;

	return g_ArrayMap.SetElementString(lh->GetOwningArrayID(), *key, rh->GetString()) ? ScriptToken::Create(rh->GetString()) : NULL;
}

ScriptToken* Eval_Assign_Elem_Form(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	if (!key)
		return NULL;

	return g_ArrayMap.SetElementFormID(lh->GetOwningArrayID(), *key, rh->GetFormID()) ? ScriptToken::CreateForm(rh->GetFormID()) : NULL;
}

ScriptToken* Eval_Assign_Elem_Array(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	if (!key)
		return NULL;

	if (g_ArrayMap.SetElementArray(lh->GetOwningArrayID(), *key, rh->GetArray()))
	{
	//	ArrayID newID;
	//	// this is pre-reference-counting code; no longer necessary
	//	if (g_ArrayMap.GetElementArray(lh->GetOwningArrayID(), *key, &newID))
	//		return ScriptToken::Create(newID, kTokenType_Array);
		return ScriptToken::CreateArray(rh->GetArray());
	}

	return NULL;
}

ScriptToken* Eval_PlusEquals_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetVar()->data += rh->GetNumber();
	return ScriptToken::Create(lh->GetVar()->data);
}

ScriptToken* Eval_MinusEquals_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetVar()->data -= rh->GetNumber();
	return ScriptToken::Create(lh->GetVar()->data);
}

ScriptToken* Eval_TimesEquals(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetVar()->data *= rh->GetNumber();
	return ScriptToken::Create(lh->GetVar()->data);
}

ScriptToken* Eval_DividedEquals(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double rhNum = rh->GetNumber();
	if (rhNum == 0.0)
	{
		context->Error("Division by zero");
		return NULL;
	}
	lh->GetVar()->data /= rhNum;
	return ScriptToken::Create(lh->GetVar()->data);
}

ScriptToken* Eval_ExponentEquals(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double rhNum = rh->GetNumber();
	double lhNum = lh->GetVar()->data;
	lh->GetVar()->data = pow(lhNum,rhNum);
	return ScriptToken::Create(lh->GetVar()->data);
}


ScriptToken* Eval_PlusEquals_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetGlobal()->data += rh->GetNumber();
	return ScriptToken::Create(lh->GetGlobal()->data);
}

ScriptToken* Eval_MinusEquals_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetGlobal()->data -= rh->GetNumber();
	return ScriptToken::Create(lh->GetGlobal()->data);
}

ScriptToken* Eval_TimesEquals_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	lh->GetGlobal()->data *= rh->GetNumber();
	return ScriptToken::Create(lh->GetGlobal()->data);
}

ScriptToken* Eval_DividedEquals_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double num = rh->GetNumber();
	if (num == 0.0)
	{
		context->Error("Division by zero.");
		return NULL;
	}

	lh->GetGlobal()->data /= num;
	return ScriptToken::Create(lh->GetGlobal()->data);
}

ScriptToken* Eval_ExponentEquals_Global(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double lhNum = lh->GetGlobal()->data;
	lh->GetGlobal()->data = pow(lhNum,rh->GetNumber());
	return ScriptToken::Create(lh->GetGlobal()->data);
}


ScriptToken* Eval_PlusEquals_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 strID = lh->GetVar()->data;
	StringVar* strVar = g_StringMap.Get(strID);
	if (!strVar)
	{
		strID = g_StringMap.Add(context->script->GetModIndex(), "");
		lh->GetVar()->data = strID;
		strVar = g_StringMap.Get(strID);
	}

	strVar->Set((strVar->String() + rh->GetString()).c_str());
	return ScriptToken::Create(strVar->String());
}

ScriptToken* Eval_TimesEquals_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 strID = lh->GetVar()->data;
	StringVar* strVar = g_StringMap.Get(strID);
	if (!strVar)
	{
		strID = g_StringMap.Add(context->script->GetModIndex(), "");
		lh->GetVar()->data = strID;
		strVar = g_StringMap.Get(strID);
	}

	std::string str = strVar->String();
	std::string result = "";
	if (rh->GetNumber() > 0)
	{
		UInt32 rhNum = rh->GetNumber();
		for (UInt32 i = 0; i < rhNum; i++)
			result += str;
	}

	strVar->Set(result.c_str());
	return ScriptToken::Create(strVar->GetCString());
}

ScriptToken* Eval_Multiply_String_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	double rhNum = rh->GetNumber();
	std::string str = lh->GetString();
	std::string result = "";

	if (rhNum > 0)
	{
		UInt32 times = rhNum;
		for (UInt32 i =0; i < times; i++)
			result += str;
	}

	return ScriptToken::Create(result.c_str());
}	

ScriptToken* Eval_PlusEquals_Elem_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	double elemVal;
	if (!key || !g_ArrayMap.GetElementNumber(lh->GetOwningArrayID(), *key, &elemVal))
		return NULL;

	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, elemVal + rh->GetNumber()) ? ScriptToken::Create(elemVal + rh->GetNumber()) : NULL;
}

ScriptToken* Eval_MinusEquals_Elem_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	double elemVal;
	if (!key || !g_ArrayMap.GetElementNumber(lh->GetOwningArrayID(), *key, &elemVal))
		return NULL;

	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, elemVal - rh->GetNumber()) ? ScriptToken::Create(elemVal - rh->GetNumber()) : NULL;
}

ScriptToken* Eval_TimesEquals_Elem(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	double elemVal;
	if (!key || !g_ArrayMap.GetElementNumber(lh->GetOwningArrayID(), *key, &elemVal))
		return NULL;

	double result = elemVal * rh->GetNumber();
	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, result) ? ScriptToken::Create(result) : NULL;
}

ScriptToken* Eval_DividedEquals_Elem(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	double elemVal;
	if (!key || !g_ArrayMap.GetElementNumber(lh->GetOwningArrayID(), *key, &elemVal))
		return NULL;

	double result = rh->GetNumber();
	if (result == 0.0)
	{
		context->Error("Division by zero");
		return NULL;
	}

	result = elemVal / rh->GetNumber();
	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, result) ? ScriptToken::Create(result) : NULL;
}

ScriptToken* Eval_ExponentEquals_Elem(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const ArrayKey* key = lh->GetArrayKey();
	double elemVal;
	if (!key || !g_ArrayMap.GetElementNumber(lh->GetOwningArrayID(), *key, &elemVal))
		return NULL;

	double result = pow(elemVal,rh->GetNumber());
	return g_ArrayMap.SetElementNumber(lh->GetOwningArrayID(), *key, result) ? ScriptToken::Create(result) : NULL;
}

ScriptToken* Eval_PlusEquals_Elem_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	std::string elemStr;
	const ArrayKey* key = lh->GetArrayKey();
	if (!key || !g_ArrayMap.GetElementString(lh->GetOwningArrayID(), *key, elemStr))
		return NULL;

	elemStr += rh->GetString();
	return g_ArrayMap.SetElementString(lh->GetOwningArrayID(), *key, elemStr.c_str()) ? ScriptToken::Create(elemStr) : NULL;
}

ScriptToken* Eval_Negation(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh->GetNumber() * -1);
}

ScriptToken* Eval_LogicalNot(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh->GetBool() ? false : true);
}

ScriptToken* Eval_Subscript_Array_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	if (!lh->GetArray())
	{
		context->Error("Invalid array access - the array was not initialized. 0");
		return NULL;
	}
	else if (g_ArrayMap.GetKeyType(lh->GetArray()) != kDataType_Numeric)
	{
		context->Error("Invalid array access - expected string index, received numeric.");
		return NULL;
	}

	ArrayKey key(rh->GetNumber());
	return ScriptToken::Create(lh->GetArray(), &key);
}

ScriptToken* Eval_Subscript_Elem_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 idx = rh->GetNumber();
	return ScriptToken::Create(dynamic_cast<ArrayElementToken*>(lh), idx, idx);
}

ScriptToken* Eval_Subscript_Elem_Slice(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const Slice* slice = rh->GetSlice();
	return (slice && !slice->bIsString) ? ScriptToken::Create(dynamic_cast<ArrayElementToken*>(lh), slice->m_lower, slice->m_upper) : NULL;
}

ScriptToken* Eval_Subscript_Array_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	if (!lh->GetArray())
	{
		context->Error("Invalid array access - the array was not initialized. 1");
		return NULL;
	}
	else if (g_ArrayMap.GetKeyType(lh->GetArray()) != kDataType_String)
	{
		context->Error("Invalid array access - expected numeric index, received string");
		return NULL;
	}

	ArrayKey key(rh->GetString());
	return ScriptToken::Create(lh->GetArray(), &key);
}

ScriptToken* Eval_Subscript_Array_Slice(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 slicedID = g_ArrayMap.MakeSlice(lh->GetArray(), rh->GetSlice(), context->script->GetModIndex());
	if (!slicedID)
	{
		context->Error("Invalid array slice operation - array is uninitialized or supplied index does not match key type");
		return NULL;
	}

	return ScriptToken::CreateArray(slicedID);
}

ScriptToken* Eval_Subscript_StringVar_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	ScriptEventList::Var* var = lh->GetVar();
	SInt32 idx = rh->GetNumber();
	if (var) {
		StringVar* strVar = g_StringMap.Get(var->data);
		if (!strVar) {
			return NULL;	// uninitialized
		}

		if (idx < 0) {
			// negative index counts from end of string
			idx += strVar->GetLength();
		}
	}

	return var ? ScriptToken::Create(var->data, idx, idx) : NULL;
}

ScriptToken* Eval_Subscript_StringVar_Slice(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	ScriptEventList::Var* var = lh->GetVar();
	const Slice* slice = rh->GetSlice();
	double upper = slice->m_upper;
	double lower = slice->m_lower;
	StringVar* strVar = g_StringMap.Get(var->data);
	if (!strVar) {
		return NULL;
	}

	UInt32 len = strVar->GetLength();
	if (upper < 0) {
		upper += len;
	}

	if (lower < 0) {
		lower += len;
	}

	if (var && slice && !slice->bIsString) {
		return ScriptToken::Create(var->data, lower, upper);
	}
	return NULL;
}

ScriptToken* Eval_Subscript_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	UInt32 idx = (rh->GetNumber() < 0) ? strlen(lh->GetString()) + rh->GetNumber() : rh->GetNumber();
	if (idx < strlen(lh->GetString()))
		return ScriptToken::Create(std::string(lh->GetString()).substr(idx, 1));
	else
		return ScriptToken::Create("");
}

ScriptToken* Eval_Subscript_String_Slice(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	const Slice* srcSlice = rh->GetSlice();
	std::string str = lh->GetString();

	if (!srcSlice || srcSlice->bIsString)
	{
		context->Error("Invalid string slice operation");
		return NULL;
	}

	Slice slice(srcSlice);
	if (slice.m_lower < 0)
		slice.m_lower += str.length();
	if (slice.m_upper < 0)
		slice.m_upper += str.length();

	if (slice.m_lower >= 0 && slice.m_upper < str.length() && slice.m_lower <= slice.m_upper)	// <=, not <, to support single-character slice
		return ScriptToken::Create(str.substr(slice.m_lower, slice.m_upper - slice.m_lower + 1));
	else
		return ScriptToken::Create("");
}

ScriptToken* Eval_MemberAccess(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	if (!lh->GetArray())
	{
		context->Error("Invalid array access - the array was not initialized. 2");
		return NULL;
	}
	else if (g_ArrayMap.GetKeyType(lh->GetArray()) != kDataType_String)
	{
		context->Error("Invalid array access - expected numeric index, received string");
		return NULL;
	}

	ArrayKey key(rh->GetString());
	return ScriptToken::Create(lh->GetArray(), &key);
}
ScriptToken* Eval_Slice_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	Slice slice(lh->GetString(), rh->GetString());
	return ScriptToken::Create(&slice);
}

ScriptToken* Eval_Slice_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	Slice slice(lh->GetNumber(), rh->GetNumber());
	return ScriptToken::Create(&slice);
}

ScriptToken* Eval_ToString_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh->GetString());
}

ScriptToken* Eval_ToString_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	char buf[0x20];
	sprintf_s(buf, sizeof(buf), "%g", lh->GetNumber());
	return ScriptToken::Create(std::string(buf));
}

ScriptToken* Eval_ToString_Form(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(std::string(GetFullName(lh->GetTESForm())));
}

ScriptToken* Eval_ToString_Array(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	char buf[0x20];
	sprintf_s(buf, sizeof(buf), "Array ID %d", lh->GetArray());
	return ScriptToken::Create(std::string(buf));
}

ScriptToken* Eval_ToNumber(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh->GetNumericRepresentation(false));
}

ScriptToken* Eval_In(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	switch (lh->GetVariableType())
	{
	case Script::eVarType_Array:
		{
			UInt32 iterID = g_ArrayMap.Create(kDataType_String, false, context->script->GetModIndex());
			//g_ArrayMap.AddReference(&lh->GetVar()->data, iterID, context->script->GetModIndex());

			ForEachContext con(rh->GetArray(), iterID, Script::eVarType_Array, lh->GetVar());
			ScriptToken* forEach = ScriptToken::Create(&con);
			//if (!forEach)
			//	g_ArrayMap.RemoveReference(&lh->GetVar()->data, context->script->GetModIndex());

			return forEach;
		}
	case Script::eVarType_String:
		{
			UInt32 iterID = lh->GetVar()->data;
			StringVar* sv = g_StringMap.Get(iterID);
			if (!sv)
			{
				iterID = g_StringMap.Add(context->script->GetModIndex(), "");
				lh->GetVar()->data = iterID;
			}

			UInt32 srcID = g_StringMap.Add(context->script->GetModIndex(), rh->GetString(), true);
			ForEachContext con(srcID, iterID, Script::eVarType_String, lh->GetVar());
			ScriptToken* forEach = ScriptToken::Create(&con);
			return forEach;
		}
	case Script::eVarType_Ref:
		{
			TESObjectREFR* src = DYNAMIC_CAST(rh->GetTESForm(), TESForm, TESObjectREFR);
			if (!src && rh->GetTESForm() && (rh->GetTESForm()->refID == playerID) )
				src = (TESObjectREFR*)LookupFormByID(playerRefID);
			if (src) {
				ForEachContext con((UInt32)src, 0, Script::eVarType_Ref, lh->GetVar());
				ScriptToken* forEach = ScriptToken::Create(&con);
				return forEach;
			}
			return NULL;
		}
	}

	return NULL;
}

ScriptToken* Eval_Dereference(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	// this is a convenience thing.
	// simplifies access to iterator value in foreach loops e.g.
	//	foreach iter <- srcArray
	//		let someVar := iter["value"]
	//		let someVar := *iter		; equivalent, more readable

	// in other contexts, returns the first element of the array
	// useful for people using array variables to hold a single value of undetermined type

	ArrayID arrID = lh->GetArray();
	if (!arrID)
	{
		context->Error("Invalid array access - the array was not initialized. 3");
		return NULL;
	}

	UInt32 size = g_ArrayMap.SizeOf(arrID);
	ArrayKey valueKey("value");
	// is this a foreach iterator?
	if (size == 2 && g_ArrayMap.HasKey(arrID, valueKey) && g_ArrayMap.HasKey(arrID, "key") && g_ArrayMap.HasKey(arrID, "value"))
		return ScriptToken::Create(arrID, &valueKey);

	ArrayElement elem;
	if (g_ArrayMap.GetFirstElement(arrID, &elem, &valueKey))
		return ScriptToken::Create(arrID, &valueKey);

	return NULL;
}

ScriptToken* Eval_Box_Number(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	// the inverse operation of dereference: given a value of any type, wraps it in a single-element array
	// again, a convenience request
	ArrayID arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	g_ArrayMap.SetElementNumber(arr, ArrayKey(0.0), lh->GetNumber());
	return ScriptToken::CreateArray(arr);
}

ScriptToken* Eval_Box_String(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	ArrayID arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	g_ArrayMap.SetElementString(arr, ArrayKey(0.0), lh->GetString());
	return ScriptToken::CreateArray(arr);
}

ScriptToken* Eval_Box_Form(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	ArrayID arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	TESForm* form = lh->GetTESForm();
	g_ArrayMap.SetElementFormID(arr, ArrayKey(0.0), form ? form->refID : 0);
	return ScriptToken::CreateArray(arr);
}

ScriptToken* Eval_Box_Array(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	ArrayID arr = g_ArrayMap.Create(kDataType_Numeric, true, context->script->GetModIndex());
	g_ArrayMap.SetElementArray(arr, ArrayKey(0.0), lh->GetArray());
	return ScriptToken::CreateArray(arr);
}


ScriptToken* Eval_Pair(OperatorType op, ScriptToken* lh, ScriptToken* rh, ExpressionEvaluator* context)
{
	return ScriptToken::Create(lh, rh);
}

#define OP_HANDLER(x) x
#else
#define OP_HANDLER(x) NULL
#endif

// Operator Rules
OperationRule kOpRule_Comparison[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean, NULL	},
	{	kTokenType_Ambiguous, kTokenType_Number, kTokenType_Boolean, NULL	},
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_Boolean, NULL	},
#endif
	{	kTokenType_Number, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Comp_Number_Number)	},
	{	kTokenType_String, kTokenType_String, kTokenType_Boolean, OP_HANDLER(Eval_Comp_String_String)	},
};

OperationRule kOpRule_Equality[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean	},
	{	kTokenType_Ambiguous, kTokenType_Number, kTokenType_Boolean	},
	{	kTokenType_Ambiguous, kTokenType_Form, kTokenType_Boolean	},
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_Boolean	},
#endif
	{	kTokenType_Number, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Number)	},
	{	kTokenType_String, kTokenType_String, kTokenType_Boolean, OP_HANDLER(Eval_Eq_String)	},
	{	kTokenType_Form, kTokenType_Form, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Form)	},
	{	kTokenType_Form, kTokenType_Number, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Form_Number), true	},
	{	kTokenType_Array, kTokenType_Array, kTokenType_Boolean, OP_HANDLER(Eval_Eq_Array)	}
};

OperationRule kOpRule_Logical[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Boolean	},
	{	kTokenType_Ambiguous, kTokenType_Boolean, kTokenType_Boolean	},
#endif
	{	kTokenType_Boolean, kTokenType_Boolean, kTokenType_Boolean, OP_HANDLER(Eval_Logical)	},
};

OperationRule kOpRule_Addition[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous	},
	{	kTokenType_Ambiguous, kTokenType_Number, kTokenType_Number	},
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_String	},
#endif
	{	kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Add_Number)	},
	{	kTokenType_String, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Add_String)	},
};

OperationRule kOpRule_Arithmetic[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number },
	{	kTokenType_Number, kTokenType_Ambiguous, kTokenType_Number },
#endif
	{	kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Arithmetic)	}
};

OperationRule kOpRule_Multiply[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Ambiguous	},
	{	kTokenType_String,		kTokenType_Ambiguous,	kTokenType_String	},
	{	kTokenType_Number,		kTokenType_Ambiguous,	kTokenType_Ambiguous	},
#endif
	{	kTokenType_Number,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_Arithmetic)	},
	{	kTokenType_String,		kTokenType_Number,		kTokenType_String,	OP_HANDLER(Eval_Multiply_String_Number)	},
};

OperationRule kOpRule_Integer[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Number	},
	{	kTokenType_Number, kTokenType_Ambiguous, kTokenType_Number	},
#endif
	{	kTokenType_Number, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Integer)	},
};

OperationRule kOpRule_Assignment[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Ambiguous, NULL, true	},
	{	kTokenType_Ambiguous,	kTokenType_String,		kTokenType_String,		NULL, true	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL, true	},
	{	kTokenType_Ambiguous,	kTokenType_Array,		kTokenType_Array,		NULL, true	},
	{	kTokenType_Ambiguous,	kTokenType_Form,		kTokenType_Form,		NULL, true	},

	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL, true	},
	{	kTokenType_RefVar,		kTokenType_Ambiguous,	kTokenType_Form,	NULL, true	},
	{	kTokenType_StringVar,	kTokenType_Ambiguous,	kTokenType_String,	NULL, true	},
	{	kTokenType_ArrayVar,	kTokenType_Ambiguous,	kTokenType_Array,	NULL, true	},
	{	kTokenType_ArrayElement,	kTokenType_Ambiguous,	kTokenType_Ambiguous,	NULL, true },
#endif
	{	kTokenType_AssignableString, kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Assign_AssignableString), true },
	{	kTokenType_NumericVar, kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Assign_Numeric), true	},
	{	kTokenType_StringVar,	kTokenType_String, kTokenType_String, OP_HANDLER(Eval_Assign_String), true	},
	{	kTokenType_RefVar, kTokenType_Form, kTokenType_Form, OP_HANDLER(Eval_Assign_Form), true	},
	{	kTokenType_RefVar,		kTokenType_Number,	kTokenType_Form, OP_HANDLER(Eval_Assign_Form_Number), true },
	{	kTokenType_Global,	kTokenType_Number,	kTokenType_Number, OP_HANDLER(Eval_Assign_Global), true	},
	{	kTokenType_ArrayVar, kTokenType_Array, kTokenType_Array, OP_HANDLER(Eval_Assign_Array), true },
	{	kTokenType_ArrayElement,	kTokenType_Number, kTokenType_Number, OP_HANDLER(Eval_Assign_Elem_Number), true	},
	{	kTokenType_ArrayElement,	kTokenType_String,	kTokenType_String, OP_HANDLER(Eval_Assign_Elem_String), true	},
	{	kTokenType_ArrayElement,	kTokenType_Form,	kTokenType_Form, OP_HANDLER(Eval_Assign_Elem_Form), true	},
	{	kTokenType_ArrayElement, kTokenType_Array, kTokenType_Array, OP_HANDLER(Eval_Assign_Elem_Array), true	}
};

OperationRule kOpRule_PlusEquals[] =
{
#if !RUNTIME
	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_StringVar,	kTokenType_Ambiguous,	kTokenType_String,	NULL,	true	},
	{	kTokenType_ArrayElement,kTokenType_Ambiguous,	kTokenType_Ambiguous,	NULL,	true	},
	{	kTokenType_Global,		kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Ambiguous,	NULL,	false	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_String,		kTokenType_String,		NULL,	true	},
#endif
	{	kTokenType_NumericVar,	kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_PlusEquals_Number),	true	},
	{	kTokenType_ArrayElement,kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_PlusEquals_Elem_Number),	true	},
	{	kTokenType_StringVar,	kTokenType_String,		kTokenType_String,	OP_HANDLER(Eval_PlusEquals_String),	true	},
	{	kTokenType_ArrayElement,kTokenType_String,		kTokenType_String,	OP_HANDLER(Eval_PlusEquals_Elem_String),	true	},
	{	kTokenType_Global,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_PlusEquals_Global),	true	},
};

OperationRule kOpRule_MinusEquals[] =
{
#if !RUNTIME
	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_ArrayElement,kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Global,		kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	false	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL,	true	},
#endif
	{	kTokenType_NumericVar,	kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_MinusEquals_Number),	true	},
	{	kTokenType_ArrayElement,kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_MinusEquals_Elem_Number),	true	},
	{	kTokenType_Global,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_MinusEquals_Global),	true	},
};

OperationRule kOpRule_TimesEquals[] =
{
#if !RUNTIME
	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_ArrayElement,kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Global,		kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	false	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL,	true	},
#endif
	{	kTokenType_NumericVar,	kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_TimesEquals),	true	},
	{	kTokenType_ArrayElement,kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_TimesEquals_Elem),	true	},
	{	kTokenType_Global,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_TimesEquals_Global),	true	},
};

OperationRule kOpRule_DividedEquals[] =
{
#if !RUNTIME
	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_ArrayElement,kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Global,		kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	false	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL,	true	},
#endif
	{	kTokenType_NumericVar,	kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_DividedEquals),	true	},
	{	kTokenType_ArrayElement,kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_DividedEquals_Elem),	true	},
	{	kTokenType_Global,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_DividedEquals_Global),	true	},
};


OperationRule kOpRule_ExponentEquals[] =
{
#if !RUNTIME
	{	kTokenType_NumericVar,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_ArrayElement,kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Global,		kTokenType_Ambiguous,	kTokenType_Number,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Number,	NULL,	false	},
	{	kTokenType_Ambiguous,	kTokenType_Number,		kTokenType_Number,		NULL,	true	},
#endif
	{	kTokenType_NumericVar,	kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_ExponentEquals),	true	},
	{	kTokenType_ArrayElement,kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_ExponentEquals_Elem),	true	},
	{	kTokenType_Global,		kTokenType_Number,		kTokenType_Number,	OP_HANDLER(Eval_ExponentEquals_Global),	true	},
};


OperationRule kOpRule_Negation[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Number, NULL, true	},
#endif	
	{	kTokenType_Number, kTokenType_Invalid, kTokenType_Number, OP_HANDLER(Eval_Negation), true	},
};

OperationRule kOpRule_LogicalNot[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Boolean, NULL, true },
#endif
	{	kTokenType_Boolean, kTokenType_Invalid, kTokenType_Boolean, OP_HANDLER(Eval_LogicalNot), true },
};

OperationRule kOpRule_LeftBracket[] =
{
#if !RUNTIME
	{	kTokenType_Array, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true	},
	{	kTokenType_String, kTokenType_Ambiguous, kTokenType_String, NULL, true	},
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_ArrayElement, NULL, true	},
	{	kTokenType_Ambiguous, kTokenType_Number, kTokenType_Ambiguous, NULL, true	},
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Ambiguous, NULL, true	},
	{	kTokenType_Ambiguous, kTokenType_Slice, kTokenType_Ambiguous, NULL, true	},
#endif
	{	kTokenType_Array, kTokenType_Number, kTokenType_ArrayElement, OP_HANDLER(Eval_Subscript_Array_Number), true	},
	{	kTokenType_Array, kTokenType_String, kTokenType_ArrayElement, OP_HANDLER(Eval_Subscript_Array_String), true	},
	{	kTokenType_ArrayElement, kTokenType_Number, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_Elem_Number), true },
	{	kTokenType_StringVar,	kTokenType_Number,	kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_StringVar_Number), true },
	{	kTokenType_ArrayElement, kTokenType_Slice, kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_Elem_Slice), true },
	{	kTokenType_StringVar,	kTokenType_Slice,	kTokenType_AssignableString, OP_HANDLER(Eval_Subscript_StringVar_Slice), true },
	{	kTokenType_String, kTokenType_Number, kTokenType_String, OP_HANDLER(Eval_Subscript_String), true	},
	{	kTokenType_Array, kTokenType_Slice, kTokenType_Array, OP_HANDLER(Eval_Subscript_Array_Slice), true	},
	{	kTokenType_String, kTokenType_Slice, kTokenType_String, OP_HANDLER(Eval_Subscript_String_Slice), true	}
};

OperationRule kOpRule_MemberAccess[] =
{
#if !RUNTIME
	{	kTokenType_Array, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true },
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_ArrayElement, NULL, true },
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_ArrayElement, NULL, true },
#endif
	{	kTokenType_Array, kTokenType_String, kTokenType_ArrayElement, OP_HANDLER(Eval_MemberAccess), true }
};

OperationRule kOpRule_Slice[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Ambiguous, kTokenType_Slice	},
	{	kTokenType_Ambiguous, kTokenType_Number, kTokenType_Slice	},
	{	kTokenType_Ambiguous, kTokenType_String, kTokenType_Slice	},
#endif
	{	kTokenType_String, kTokenType_String, kTokenType_Slice, OP_HANDLER(Eval_Slice_String)	},
	{	kTokenType_Number, kTokenType_Number, kTokenType_Slice, OP_HANDLER(Eval_Slice_Number)	},
};

OperationRule kOpRule_In[] =
{
#if !RUNTIME
	{	kTokenType_ArrayVar,	kTokenType_Ambiguous,	kTokenType_ForEachContext,	NULL,	true	},
#endif
	{	kTokenType_ArrayVar,	kTokenType_Array,		kTokenType_ForEachContext,	OP_HANDLER(Eval_In), true },
	{	kTokenType_StringVar,	kTokenType_String,		kTokenType_ForEachContext,	OP_HANDLER(Eval_In), true },
	{	kTokenType_RefVar,		kTokenType_Form,		kTokenType_ForEachContext,	OP_HANDLER(Eval_In), true },
};

OperationRule kOpRule_ToString[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous,	kTokenType_Invalid,		kTokenType_String,			NULL,	true	},
#endif
	{	kTokenType_String,		kTokenType_Invalid,		kTokenType_String,		OP_HANDLER(Eval_ToString_String),	true	},
	{	kTokenType_Number,		kTokenType_Invalid,		kTokenType_String,		OP_HANDLER(Eval_ToString_Number),	true	},
	{	kTokenType_Form,		kTokenType_Invalid,		kTokenType_String,		OP_HANDLER(Eval_ToString_Form),		true	},
	{	kTokenType_Array,		kTokenType_Invalid,		kTokenType_String,		OP_HANDLER(Eval_ToString_Array),	true	},
};

OperationRule kOpRule_ToNumber[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous,	kTokenType_Invalid,		kTokenType_Number,			NULL,	true	},
#endif
	{	kTokenType_String,		kTokenType_Invalid,		kTokenType_Number,			OP_HANDLER(Eval_ToNumber),	true	},
	{	kTokenType_Number,		kTokenType_Invalid,		kTokenType_Number,			OP_HANDLER(Eval_ToNumber),	true	},
};

OperationRule kOpRule_Dereference[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_ArrayElement, NULL, true	},
#endif	
	{	kTokenType_Array, kTokenType_Invalid, kTokenType_ArrayElement, OP_HANDLER(Eval_Dereference), true	},
};

OperationRule kOpRule_Box[] =
{
#if !RUNTIME
	{	kTokenType_Ambiguous, kTokenType_Invalid, kTokenType_Array, NULL, true	},
#endif

	{	kTokenType_Number,	kTokenType_Invalid,	kTokenType_Array,	OP_HANDLER(Eval_Box_Number),	true	},
	{	kTokenType_String,	kTokenType_Invalid,	kTokenType_Array,	OP_HANDLER(Eval_Box_String),	true	},
	{	kTokenType_Form,	kTokenType_Invalid,	kTokenType_Array,	OP_HANDLER(Eval_Box_Form),		true	},
	{	kTokenType_Array,	kTokenType_Invalid,	kTokenType_Array,	OP_HANDLER(Eval_Box_Array),		true	},
};

OperationRule kOpRule_MakePair[] =
{
#if !RUNTIME
	{	kTokenType_String,	kTokenType_Ambiguous,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Number,	kTokenType_Ambiguous,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Number,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_String,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Array,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Form,	kTokenType_Pair,	NULL,	true	},
	{	kTokenType_Ambiguous,	kTokenType_Ambiguous,	kTokenType_Pair,	NULL,	true	},
#endif
	{	kTokenType_String, kTokenType_Number,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_String, kTokenType_String,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_String, kTokenType_Form,		kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_String, kTokenType_Array,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_Number, kTokenType_Number,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_Number, kTokenType_String,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_Number, kTokenType_Form,		kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
	{	kTokenType_Number, kTokenType_Array,	kTokenType_Pair,	OP_HANDLER(Eval_Pair),	true	},
};

// Operator definitions
#define OP_RULES(x) SIZEOF_ARRAY(kOpRule_ ## x, OperationRule), kOpRule_ ## x

Operator s_operators[] =
{
	{	2,	":=",	2,	kOpType_Assignment, OP_RULES(Assignment)	},
	{	5,	"||",	2,	kOpType_LogicalOr,	OP_RULES(Logical)		},
	{	7,	"&&",	2,	kOpType_LogicalAnd, OP_RULES(Logical)		},

	{	9,	":",	2,	kOpType_Slice,		OP_RULES(Slice)			},
	{	13,	"==",	2,	kOpType_Equals,		OP_RULES(Equality)		},
	{	13,	"!=",	2,	kOpType_NotEqual,	OP_RULES(Equality)		},

	{	15,	">",	2,	kOpType_GreaterThan,OP_RULES(Comparison)	},
	{	15,	"<",	2,	kOpType_LessThan,	OP_RULES(Comparison)	},
	{	15,	">=",	2,	kOpType_GreaterOrEqual,	OP_RULES(Comparison)	},			
	{	15,	"<=",	2,	kOpType_LessOrEqual,	OP_RULES(Comparison)	},									
									
	{	16,	"|",	2,	kOpType_BitwiseOr,	OP_RULES(Integer)		},		// ** higher precedence than in C++
	{	17,	"&",	2,	kOpType_BitwiseAnd,	OP_RULES(Integer)		},

	{	18,	"<<",	2,	kOpType_LeftShift,	OP_RULES(Integer)		},
	{	18,	">>",	2,	kOpType_RightShift,	OP_RULES(Integer)		},

	{	19,	"+",	2,	kOpType_Add,		OP_RULES(Addition)		},
	{	19,	"-",	2,	kOpType_Subtract,	OP_RULES(Arithmetic)	},

	{	21,	"*",	2,	kOpType_Multiply,	OP_RULES(Multiply)		},
	{	21,	"/",	2,	kOpType_Divide,		OP_RULES(Arithmetic)	},
	{	21,	"%",	2,	kOpType_Modulo,		OP_RULES(Integer)		},

	{	23,	"^",	2,	kOpType_Exponent,	OP_RULES(Arithmetic)	},		// exponentiation
	{	25,	"-",	1,	kOpType_Negation,	OP_RULES(Negation)		},		// unary minus in compiled script

	{	27, "!",	1,	kOpType_LogicalNot,	OP_RULES(LogicalNot)	},

	{	80,	"(",	0,	kOpType_LeftParen,	0,	NULL				},
	{	80,	")",	0,	kOpType_RightParen,	0,	NULL				},

	{	90, "[",	2,	kOpType_LeftBracket,	OP_RULES(LeftBracket)	},		// functions both as paren and operator
	{	90,	"]",	0,	kOpType_RightBracket,	0,	NULL				},		// functions only as paren

	{	2,	"<-",	2,	kOpType_In,			OP_RULES(In)			},			// 'foreach iter <- arr'
	{	25,	"$",	1,	kOpType_ToString,	OP_RULES(ToString)		},			// converts operand to string

	{	2,	"+=",	2,	kOpType_PlusEquals,	OP_RULES(PlusEquals)	},
	{	2,	"*=",	2,	kOpType_TimesEquals,	OP_RULES(TimesEquals)	},
	{	2,	"/=",	2,	kOpType_DividedEquals,	OP_RULES(DividedEquals)	},
	{	2,	"^=",	2,	kOpType_ExponentEquals,	OP_RULES(ExponentEquals)	},
	{	2,	"-=",	2,	kOpType_MinusEquals,	OP_RULES(MinusEquals)	},

	{	25,	"#",	1,	kOpType_ToNumber,		OP_RULES(ToNumber)	},

	{	25, "*",	1,	kOpType_Dereference,	OP_RULES(Dereference)	},

	{	90,	"->",	2,	kOpType_MemberAccess,	OP_RULES(MemberAccess)	},
	{	3,	"::",	2,	kOpType_MakePair,		OP_RULES(MakePair)	},
	{	25,	"&",	1,	kOpType_Box,			OP_RULES(Box)	},

	{	91,	"{",	0,	kOpType_LeftBrace,	0,	NULL				},
	{	91,	"}",	0,	kOpType_RightBrace,	0,	NULL				},

};

STATIC_ASSERT(SIZEOF_ARRAY(s_operators, Operator) == kOpType_Max);

const char* OpTypeToSymbol(OperatorType op)
{
	if (op < kOpType_Max)
		return s_operators[op].symbol;
	return "<unknown>";
}

#if 0

// Operand conversion routines
ScriptToken Number_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.num ? true : false);
}

// this is unused
ScriptToken Number_To_String(ScriptToken* token, ExpressionEvaluator* context)
{
	char str[0x20];
	sprintf_s(str, sizeof(str), "%f", token->value.num);
	return ScriptToken(std::string(str));
}

ScriptToken Bool_To_Number(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.num);
}

ScriptToken Cmd_To_Number(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.num);
}

ScriptToken Cmd_To_Form(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(*((UInt64*)(&token->value.num)), kTokenType_Form);
}

ScriptToken Cmd_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.num ? true : false);
}

ScriptToken Ref_To_Form(ScriptToken* token, ExpressionEvaluator* context)
{
	token->value.refVar->Resolve(context->eventList);
	return ScriptToken(token->value.refVar->form ? token->value.refVar->form->refID : 0, kTokenType_Form);
}

ScriptToken Ref_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	token->value.refVar->Resolve(context->eventList);
	return ScriptToken(token->value.refVar->form ? true : false);
}

ScriptToken Ref_To_RefVar(ScriptToken* token, ExpressionEvaluator* context)
{
	ScriptEventList::Var* var = context->eventList->GetVariable(token->value.refVar->varIdx);
	if (var)
		return ScriptToken(var);
	else
		return ScriptToken::Bad();
}

ScriptToken Global_To_Number(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.global->data);
}

ScriptToken Global_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.global->data ? true : false);
}

ScriptToken Form_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.formID ? true : false);
}

ScriptToken NumericVar_To_Number(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var->data);
}

ScriptToken NumericVar_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var->data ? true : false);
}

ScriptToken NumericVar_To_Variable(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var);
}

ScriptToken StringVar_To_String(ScriptToken* token, ExpressionEvaluator* context)
{
	StringVar* strVar = g_StringMap.Get(token->value.var->data);
	if (strVar)
		return ScriptToken(strVar->String());
	else
		return ScriptToken("");
}

ScriptToken StringVar_To_Variable(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var);
}

ScriptToken StringVar_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var->data ? true : false);
}

ScriptToken ArrayVar_To_Array(ScriptToken* token, ExpressionEvaluator* context)
{
	ArrayID id = token->value.var->data;
	return g_ArrayMap.Exists(id) ? ScriptToken(id, kTokenType_Array) : ScriptToken::Bad();
}

ScriptToken ArrayVar_To_Variable(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var);
}

ScriptToken ArrayVar_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var->data ? true : false);
}

ScriptToken Elem_To_Number(ScriptToken* token, ExpressionEvaluator* context)
{
	double num;
	ArrayKey* key = token->Key();
	if (!key || !g_ArrayMap.GetElementNumber(token->value.arrID, *key, &num))
		return ScriptToken::Bad();

	return ScriptToken(num);
}

ScriptToken Elem_To_Form(ScriptToken* token, ExpressionEvaluator* context)
{
	UInt32 formID;
	ArrayKey* key = token->Key();
	if (!key || !g_ArrayMap.GetElementFormID(token->value.arrID, *key, &formID))
		return ScriptToken::Bad();

	return ScriptToken(formID, kTokenType_Form);
}

ScriptToken Elem_To_String(ScriptToken* token, ExpressionEvaluator* context)
{
	std::string str;
	ArrayKey* key = token->Key();
	if (!key || !g_ArrayMap.GetElementString(token->value.arrID, *key, str))
		return ScriptToken::Bad();

	return ScriptToken(str);
}

ScriptToken Elem_To_Array(ScriptToken* token, ExpressionEvaluator* context)
{
	ArrayID arr;
	ArrayKey* key = token->Key();
	if (!key || !g_ArrayMap.GetElementArray(token->value.arrID, *key, &arr))
		return ScriptToken::Bad();

	return ScriptToken(arr, kTokenType_Array);
}

ScriptToken Elem_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	bool result;
	ArrayKey* key = token->Key();
	if (!key || !g_ArrayMap.GetElementAsBool(token->value.arrID, *key, &result))
		return ScriptToken::Bad();

	return ScriptToken(result);
}

ScriptToken RefVar_To_Form(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(*((UInt64*)(&token->value.var->data)), kTokenType_Form);
}

ScriptToken RefVar_To_Bool(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var->data ? true : false);
}

ScriptToken RefVar_To_Variable(ScriptToken* token, ExpressionEvaluator* context)
{
	return ScriptToken(token->value.var);
}

#endif

#if RUNTIME

// ExpressionEvaluator

bool ExpressionEvaluator::Active()
{
	return ThreadLocalData::Get().expressionEvaluator != NULL;
}

void ExpressionEvaluator::ToggleErrorSuppression(bool bSuppress) { 
	if (bSuppress) {
		m_flags.Clear(kFlag_ErrorOccurred);
		m_flags.Set(kFlag_SuppressErrorMessages); 
	}
	else 
		m_flags.Clear(kFlag_SuppressErrorMessages); 
}

void ExpressionEvaluator::Error(const char* fmt, ...)
{
	m_flags.Set(kFlag_ErrorOccurred);

	if (m_flags.IsSet(kFlag_SuppressErrorMessages))
		return;

	va_list args;
	va_start(args, fmt);

	char	errorMsg[0x400];
	vsprintf_s(errorMsg, 0x400, fmt, args);

	// include script data offset and command name/opcode
	UInt16* opcodePtr = (UInt16*)((UInt8*)script->data + m_baseOffset);
	CommandInfo* cmd = g_scriptCommands.GetByOpcode(*opcodePtr);

	// include mod filename, save having to ask users to figure it out themselves
	const char* modName = "Savegame";
	if (script->GetModIndex() != 0xFF)
	{
		modName = DataHandler::Get()->GetNthModName(script->GetModIndex());
		if (!modName || !modName[0])
			modName = "Unknown";
	}

	ShowRuntimeError(script, "%s\n    File: %s Offset: 0x%04X Command: %s", errorMsg, modName, m_baseOffset, cmd ? cmd->longName : "<unknown>");
	if (m_flags.IsSet(kFlag_StackTraceOnError))
		PrintStackTrace();
}

void ExpressionEvaluator::PrintStackTrace() {
	std::stack<const ExpressionEvaluator*> stackCopy;
	char output[0x100];

	ExpressionEvaluator* eval = this;
	while (eval) {
		CommandInfo* cmd = g_scriptCommands.GetByOpcode(*((UInt16*)((UInt8*)eval->script->data + eval->m_baseOffset)));
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

// ExpressionParser

void PrintCompiledCode(ScriptLineBuffer* buf)
{
#if _DEBUG && 0		// unused, ShowCompilerError() will break build due to shademe's updates to it
	std::string bytes;
	char byte[5];

	for (UInt32 i = 0; i < buf->dataOffset; i++)
	{
		if (isprint(buf->dataBuf[i]))
			sprintf_s(byte, 4, "%c", buf->dataBuf[i]);
		else
			sprintf_s(byte, 4, "%02X", buf->dataBuf[i]);

		bytes.append(byte);
		bytes.append(" ");
	}

	ShowCompilerError(buf, "COMPILER OUTPUT\n\n%s", bytes.c_str());
#endif
}

// Not particularly fond of this but it's become necessary to distinguish between a parser which is parsing part of a larger
// expression and one parsing an entire script line.
// Threading not a concern in script editor; ExpressionParser not used at run-time.
static SInt32 s_parserDepth = 0;

ExpressionParser::ExpressionParser(ScriptBuffer* scriptBuf, ScriptLineBuffer* lineBuf) 
	: m_scriptBuf(scriptBuf), m_lineBuf(lineBuf), m_len(strlen(m_lineBuf->paramText)), m_numArgsParsed(0)
{ 
	ASSERT(s_parserDepth >= 0);
	s_parserDepth++;
	memset(m_argTypes, kTokenType_Invalid, sizeof(m_argTypes)); 
}

ExpressionParser::~ExpressionParser()
{
	ASSERT(s_parserDepth > 0);
	s_parserDepth--;
}

bool ExpressionParser::ParseArgs(ParamInfo* params, UInt32 numParams, bool bUsesNVSEParamTypes)
{
	// reserve space for UInt8 numargs at beginning of compiled code
	UInt8* numArgsPtr = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	m_lineBuf->dataOffset += 1;

	// see if args are enclosed in {braces}, if so don't parse beyond closing brace
	UInt32 argsEndPos = m_len;
	char ch = 0;
	UInt32 i = 0;

	while ((ch = Peek(Offset())))
	{
		if (!isspace(ch))
			break;

		Offset()++;		
	}
	UInt32 offset = Offset();
	UInt32 endOffset = argsEndPos;

	if ('{' == Peek(Offset())) // restrict parsing to the enclosed text
		while ((ch = Peek(Offset())))
		{
			if (ch == '{')
			{
				offset++;
				Offset()++;

				UInt32 bracketEndPos = MatchOpenBracket(&s_operators[kOpType_LeftBrace]);
				if (bracketEndPos == -1)
				{
					Message(kError_MismatchedBrackets);
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
		if (!isspace(ch))
			break;

		Offset()++;		
	}

	UInt8* dataStart = m_lineBuf->dataBuf + m_lineBuf->dataOffset;

	while (m_numArgsParsed < numParams && Offset() < argsEndPos)
	{
		// reserve space to store expr length
		m_lineBuf->dataOffset += 2;

		Token_Type argType = ParseSubExpression(argsEndPos - Offset());
		if (argType == kTokenType_Invalid)
			return false;
		else if (argType == kTokenType_Empty) {
			// reached end of args
			break;
		}
		else 		// is arg of expected type(s)?
		{
			if (!ValidateArgType(params[m_numArgsParsed].typeID, argType, bUsesNVSEParamTypes))
			{
				#if RUNTIME
					ShowCompilerError(m_lineBuf, "Invalid expression for parameter %d. Expected %s.", m_numArgsParsed + 1, params[m_numArgsParsed].typeStr);
				#else
					ShowCompilerError(m_scriptBuf, "Invalid expression for parameter %d. Expected %s.", m_numArgsParsed + 1, params[m_numArgsParsed].typeStr);
				#endif
				return false;
			}
		}

		m_argTypes[m_numArgsParsed++] = argType;

		// store expr length for this arg
		*((UInt16*)dataStart) = (m_lineBuf->dataBuf + m_lineBuf->dataOffset) - dataStart;
		dataStart = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	}

	if (Offset() < argsEndPos && s_parserDepth == 1)	// some leftover stuff in text
	{
		// when parsing commands as args to other commands or components of larger expressions, we expect to have some leftovers
		// so this check is not necessary unless we're finished parsing the entire line
		Message(kError_TooManyArgs);
		return false;
	} else
		Offset() = endOffset + 1;

	// did we get all required args?
	UInt32 numExpectedArgs = 0;
	for (UInt32 i = 0; i < numParams && !params[i].isOptional; i++)
		numExpectedArgs++;

	if (numExpectedArgs > m_numArgsParsed)
	{
		ParamInfo* missingParam = &params[m_numArgsParsed];
		Message(kError_MissingParam, missingParam->typeStr, m_numArgsParsed + 1);
		return false;
	}

	*numArgsPtr = m_numArgsParsed;
	//PrintCompiledCode(m_lineBuf);
	return true;
}

bool ExpressionParser::ValidateArgType(UInt32 paramType, Token_Type argType, bool bIsNVSEParam)
{
	if (bIsNVSEParam) {
		bool bTypesMatch = false;
		if (paramType == kNVSEParamType_NoTypeCheck)
			bTypesMatch = true;
		else		// ###TODO: this could probably done with bitwise AND much more efficiently
		{
			for (UInt32 i = 0; i < kTokenType_Max; i++)
			{
				if (paramType & (1 << i))
				{
					Token_Type type = (Token_Type)(i);
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
	else {
		// vanilla paramInfo
		if (argType == kTokenType_Ambiguous) {
			// we'll find out at run-time
			return true;
		}

		switch (paramType) {
			case kParamType_String:
			case kParamType_Axis:
			case kParamType_AnimationGroup:
			case kParamType_Sex:
				return CanConvertOperand(argType, kTokenType_String);
			case kParamType_Float:
			case kParamType_Integer:
			case kParamType_QuestStage:
			case kParamType_CrimeType:
				// string var included here b/c old sv_* cmds take strings as integer IDs
				return (CanConvertOperand(argType, kTokenType_Number) || CanConvertOperand(argType, kTokenType_StringVar) || 
					CanConvertOperand(argType, kTokenType_Variable));
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
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
	{	"argument",	kNVSEParamType_NoTypeCheck,	1	},
};

// records version of bytecode representation to avoid problems if representation changes later
static const UInt8 kUserFunction_Version = 1;

bool GetUserFunctionParams(const std::string& scriptText, std::vector<UserFunctionParam> &outParams, Script::VarInfoEntry* varList)
{
	std::string lineText;
	Tokenizer lines(scriptText.c_str(), "\r\n");
	while (lines.NextToken(lineText) != -1)
	{
		Tokenizer tokens(lineText.c_str(), " \t\r\n\0;");

		std::string token;
		if (tokens.NextToken(token) != -1)
		{
			if (!_stricmp(token.c_str(), "begin"))
			{
				UInt32 argStartPos = lineText.find("{");
				UInt32 argEndPos = lineText.find("}");
				if (argStartPos == -1 || argEndPos == -1 || (argStartPos > argEndPos))
					return false;

				std::string argStr = lineText.substr(argStartPos+1, argEndPos - argStartPos - 1);
				Tokenizer argTokens(argStr.c_str(), "\t ,");
				while (argTokens.NextToken(token) != -1)
				{
					VariableInfo* varInfo = varList->GetVariableByName(token.c_str());
					if (!varInfo)
						return false;

					UInt32 varType = GetDeclaredVariableType(token.c_str(), scriptText.c_str());
					if (varType == Script::eVarType_Invalid)
						return false;

					// make sure user isn't trying to use a var more than once as a param
					for (UInt32 i = 0; i < outParams.size(); i++)
						if (outParams[i].varIdx == varInfo->idx)
							return false;

					outParams.push_back(UserFunctionParam(varInfo->idx, varType));
				}

				return true;
			}
		}
	}

	return false;
}

// index into array with Script::eVarType_XXX
static ParamInfo kDynamicParams[] =
{
	{	"float",	kNVSEParamType_Number,	0	},
	{	"integer",	kNVSEParamType_Number,	0	},
	{	"string",	kNVSEParamType_String,	0	},
	{	"array",	kNVSEParamType_Array,	0	},
	{	"object",	kNVSEParamType_Form,	0	},
};

DynamicParamInfo::DynamicParamInfo(std::vector<UserFunctionParam> &params)
{
	m_numParams = params.size() > kMaxParams ? kMaxParams : params.size();
	for (UInt32 i = 0; i < m_numParams && i < kMaxParams; i++)
		m_paramInfo[i] = kDynamicParams[params[i].varType];
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

	UInt32 paramLen = strlen(m_lineBuf->paramText);

	// parse function object
	while (isspace(Peek()))
	{
		Offset()++;
		if (Offset() >= paramLen)
		{
			Message(kError_CantParse);
			return false;
		}
	}

	UInt32 peekLen = 0;
	bool foundFunc = false;
	Script* funcScript = NULL;
	ScriptToken* funcForm = PeekOperand(peekLen);
	UInt16* savedLenPtr = (UInt16*)(m_lineBuf->dataBuf + m_lineBuf->dataOffset);
	UInt16 startingOffset = m_lineBuf->dataOffset;
	m_lineBuf->dataOffset += 2;

	if (!funcForm)
		return false;
	else if (funcForm->Type() == kTokenType_ArrayVar) {
		foundFunc = CanConvertOperand(ParseSubExpression(paramLen - Offset()), kTokenType_Form);
	}
	else {
		TESForm * form = funcForm->GetTESForm();
		funcScript = DYNAMIC_CAST(form, TESForm, Script);
		if (!(!funcScript && (form || !funcForm->CanConvertTo(kTokenType_Form)))) {
			foundFunc = true;
			funcForm->Write(m_lineBuf);
			Offset() += peekLen;
		}
	}

	delete funcForm;
	funcForm = NULL;

	if (!foundFunc)	{
		Message(kError_ExpectedUserFunction);
		delete funcForm;
		return false;
	}
	else {
		*savedLenPtr = m_lineBuf->dataOffset - startingOffset;
	}

	// skip any commas between function name and args
	// silly thing to have to fix but whatever
	while ((isspace(Peek()) || Peek() == ',') && Offset() < paramLen)
		Offset()++;

	// determine paramInfo for function and parse the args
	bool bParsed = false;

	// lookup paramInfo from Script
	// if recursive call, look up from ScriptBuffer instead
	if (funcScript)
	{
		char* funcScriptText = funcScript->text;
		Script::VarInfoEntry* funcScriptVars = &funcScript->varList;

		if (!_stricmp(GetEditorID(funcScript), m_scriptBuf->scriptName.m_data))
		{
			funcScriptText = m_scriptBuf->scriptText;
			funcScriptVars = &m_scriptBuf->vars;
		}

		std::vector<UserFunctionParam> funcParams;
		if (!GetUserFunctionParams(funcScriptText, funcParams, funcScriptVars))
		{
			Message(kError_UserFunctionParamsUndefined);
			return false;
		}

		DynamicParamInfo dynamicParams(funcParams);
		bParsed = ParseArgs(dynamicParams.Params(), dynamicParams.NumParams());
	}
	else	// using refVar as function pointer, use default params
	{
		ParamInfo* params = kParams_DefaultUserFunctionParams;
		UInt32 numParams = NUM_PARAMS(kParams_DefaultUserFunctionParams);

		bParsed = ParseArgs(params, numParams);
	}

	return bParsed;
}

bool ExpressionParser::ParseUserFunctionDefinition()
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
	if (!GetUserFunctionParams(m_scriptBuf->scriptText, params, &m_scriptBuf->vars))
	{
		Message(kError_UserFunctionParamsUndefined);
		return false;
	}

	// write param info
	m_lineBuf->WriteByte(params.size());
	for (std::vector<UserFunctionParam>::iterator iter = params.begin(); iter != params.end(); ++iter)
	{
		m_lineBuf->Write16(iter->varIdx);
		m_lineBuf->WriteByte(iter->varType);
	}

	// determine which if any local variables must be destroyed on function exit (string and array vars)
	// ensure no variables declared after function definition
	// ensure only one Begin block in script
	UInt32 offset = 0;
	bool bFoundBegin = false;
	UInt32 endPos = 0;
	std::string scrText = m_scriptBuf->scriptText;

	std::vector<UInt16> arrayVarIndexes;

	std::string lineText;
	Tokenizer lines(scrText.c_str(), "\r\n");
	while (lines.NextToken(lineText) != -1)
	{
		Tokenizer tokens(lineText.c_str(), " \t\r\n\0");
		std::string token;
		if (tokens.NextToken(token) != -1)
		{
			if (!_stricmp(token.c_str(), "begin"))
			{
				if (bFoundBegin)
				{
					Message(kError_UserFunctionContainsMultipleBlocks);
					return false;
				}

				bFoundBegin = true;
			}
			else if (!_stricmp(token.c_str(), "array_var"))
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
				if (!_stricmp(token.c_str(), "string_var") || !_stricmp(token.c_str(), "float") || !_stricmp(token.c_str(), "int") ||
				!_stricmp(token.c_str(), "ref") || !_stricmp(token.c_str(), "reference") || !_stricmp(token.c_str(), "short") ||
				!_stricmp(token.c_str(), "long"))
				{
					Message(kError_UserFunctionVarsMustPrecedeDefinition);
					return false;
				}
			}
		}
	}
	
	// write destructible var info
	m_lineBuf->WriteByte(arrayVarIndexes.size());
	for (UInt32 i = 0; i < arrayVarIndexes.size(); i++)
	{
		m_lineBuf->Write16(arrayVarIndexes[i]);
	}

#if _DEBUG
	//PrintCompiledCode(m_lineBuf);
#endif

	return true;
}

Token_Type ExpressionParser::Parse()
{
	UInt8* dataStart = m_lineBuf->dataBuf + m_lineBuf->dataOffset;
	m_lineBuf->dataOffset += 2;

	Token_Type result = ParseSubExpression(m_len);

	*((UInt16*)dataStart) = (m_lineBuf->dataBuf + m_lineBuf->dataOffset) - dataStart;

	//PrintCompiledCode(m_lineBuf);
	return result;
}

static ErrOutput::Message s_expressionErrors[] =
{
	// errors
	{	"Could not parse this line."	},
	{	"Too many operators."	},
	{	"Too many operands.",	},
	{	"Mismatched brackets."	},
	{	"Invalid operands for operator %s."	},
	{	"Mismatched quotes."	},
	{	"Left of dot must be quest or persistent reference."	},
	{	"Unknown variable '%s'."	},
	{	"Expected string variable after '$'."	},
	{	"Cannot access variable on unscripted object '%s'."	},
	{	"More args provided than expected by function or command."	},
	{	"Commands '%s' must be called on a reference."		},
	{	"Missing required parameter '%s' for parameter #'%d'."	},
	{	"Missing argument list for user-defined function '%s'."	},
	{	"Expected user function."	},
	{	"User function scripts may only contain one script block."	},
	{	"Variables in user function scripts must precede function definition."	},
	{	"Could not parse user function parameter list in function definition.\nMay be caused by undefined variable,  missing {brackets}, or attempt to use a single variable to hold more than one parameter."	},
	{	"Expected string literal."	},

	// warnings
	{	"Unquoted argument '%s' will be treated as string by default. Check spelling if a form or variable was intended.", true	},
	{	"Usage of ref variables as pointers to user-defined functions prevents type-checking of function arguments. Make sure the arguments provided match those expected by the function being called.", true },

	// default
	{	"Undefined error."	}
};

ErrOutput::Message * ExpressionParser::s_Messages = s_expressionErrors;

void ExpressionParser::Message(UInt32 errorCode, ...)
{
	errorCode = errorCode > kError_Max ? kError_Max : errorCode;
	va_list args;
	va_start(args, errorCode);
	ErrOutput::Message* msg = &s_Messages[errorCode];
	if (msg->bCanDisable)
		g_ErrOut.vShow(s_Messages[errorCode], args);
	else		// prepend line # to message
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

UInt32	ExpressionParser::MatchOpenBracket(Operator* openBracOp)
{
	char closingBrac = openBracOp->GetMatchedBracket();
	char openBrac = openBracOp->symbol[0];
	UInt32 openBracCount = 1;
	const char* text = Text();
	UInt32 i;
	for (i = Offset(); i < m_len && text[i]; i++)
	{
		if (text[i] == openBrac)
			openBracCount++;
		else if (text[i] == closingBrac)
			openBracCount--;

		if (openBracCount == 0)
			break;
	}

	return openBracCount ? -1 : i;
}

Token_Type ExpressionParser::ParseSubExpression(UInt32 exprLen)
{
	std::stack<Operator*> ops;
	std::stack<Token_Type> operands;

	UInt32 exprEnd = Offset() + exprLen;
	bool bLastTokenWasOperand = false;	// if this is true, we expect binary operator, else unary operator or an operand

	char ch;
	while (Offset() < exprEnd && (ch = Peek()))
	{
		if (isspace(ch))
		{
			Offset()++;
			continue;
		}

		Token_Type operandType = kTokenType_Invalid;

		// is it an operator?
		Operator* op = ParseOperator(bLastTokenWasOperand);
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

				UInt32 endBracPos = MatchOpenBracket(op);
				if (endBracPos == -1)
				{
					Message(kError_MismatchedBrackets);
					return kTokenType_Invalid;
				}

				// replace closing bracket with 0 to ensure subexpression doesn't try to read past end of expr
				m_lineBuf->paramText[endBracPos] = 0;

				operandType = ParseSubExpression(endBracPos - Offset());
				Offset() = endBracPos + 1;	// skip the closing bracket
				bLastTokenWasOperand = true;
			}
			else if (op->IsClosingBracket())
			{
				Message(kError_MismatchedBrackets);
				return kTokenType_Invalid;
			}
			else		// normal operator, handle or push
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
		else if (bLastTokenWasOperand || ParseOperator(!bLastTokenWasOperand, false))		// treat as arg delimiter?
			break;
		else	// must be an operand (or a syntax error)
		{
			ScriptToken* operand = ParseOperand(ops.size() ? ops.top() : NULL);
			if (!operand)
				return kTokenType_Invalid;

			// write it to postfix expression, we'll check validity below
			operand->Write(m_lineBuf);
			operandType = operand->Type();

			CommandInfo* cmdInfo = operand->GetCommandInfo();
			delete operand;
			operand = NULL;

			// if command, parse it. also adjust operand type if return value of command is known
			if (operandType == kTokenType_Command)
			{
				CommandReturnType retnType = g_scriptCommands.GetReturnType(cmdInfo);
				if (retnType == kRetnType_String)		
					operandType = kTokenType_String;
				else if (retnType == kRetnType_Array)	
					operandType = kTokenType_Array;
				else if (retnType == kRetnType_Form)
					operandType = kTokenType_Form;

				s_parserDepth++;
				bool bParsed = ParseFunctionCall(cmdInfo);
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
	else if (operands.size() == 0) {
		return kTokenType_Empty;
	}
	else {
		return operands.top();
	}
}

Token_Type ExpressionParser::PopOperator(std::stack<Operator*> & ops, std::stack<Token_Type> & operands)
{
	Operator* topOp = ops.top();
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
	default:		// a paren or right bracket ended up on stack somehow
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

	operands.push(result);

	// write operator to postfix expression
	ScriptToken* opToken = ScriptToken::Create(topOp);
	opToken->Write(m_lineBuf);
	delete opToken;

	return result;
}
		
ScriptToken* ExpressionParser::ParseOperand(bool (* pred)(ScriptToken* operand))
{
	char ch;
	while ((ch = Peek(Offset())))
	{
		if (!isspace(ch))
			break;

		Offset()++;		
	}

	ScriptToken* token = ParseOperand();
	if (token) {
		if (!pred(token)) {
			delete token;
			token = NULL;
		}
	}

	return token;
}

Operator* ExpressionParser::ParseOperator(bool bExpectBinaryOperator, bool bConsumeIfFound)
{
	// if bExpectBinary true, we expect a binary operator or a closing paren
	// if false, we expect unary operator or an open paren

	// If this returns NULL when we expect a binary operator, it likely indicates end of arg
	// Commas can optionally be used to separate expressions as args

	std::vector<Operator*> ops;	// a list of possible matches
	Operator* op = NULL;

	// check first character
	char ch = Peek();
	if (ch == ',')		// arg expression delimiter
	{
		Offset() += 1;
		return NULL;
	}

	for (UInt32 i = 0; i < kOpType_Max; i++)
	{
		Operator* curOp = &s_operators[i];
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
	if (ch && ispunct(ch))		// possibly a two-character operator, check second char
	{
		std::vector<Operator*>::iterator iter = ops.begin();
		while (iter != ops.end())
		{
			Operator* cur = *iter;
			if (cur->symbol[1] == ch)		// definite match
			{
				op = cur;
				break;
			}
			else if (cur->symbol[1] != 0)	// remove two-character operators which don't match
				iter = ops.erase(iter);
			else
				iter++;
		}
	}
	else			// definitely single-character
	{
		for (UInt32 i = 0; i < ops.size(); i++)
		{
			if (ops[i]->symbol[1] == 0)
			{
				op = ops[i];
				break;
			}
		}
	}

	if (!op && ops.size() == 1)
		op = ops[0];

	if (op && bConsumeIfFound)
		Offset() += strlen(op->symbol);

	return op;
}	

// format a string argument
static void FormatString(std::string& str)
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
		
		str.insert(pos, 1, toInsert);	// insert char at current pos
		str.erase(pos + 1, 2);			// erase format specifier
		pos += 1;
	}
}

ScriptToken* ExpressionParser::PeekOperand(UInt32& outReadLen)
{
	UInt32 curOffset = Offset();
	ScriptToken* operand = ParseOperand();
	outReadLen = Offset() - curOffset;
	Offset() = curOffset;
	return operand;
}

ScriptToken* ExpressionParser::ParseOperand(Operator* curOp)
{
	char firstChar = Peek();
	bool bExpectStringVar = false;

	if (!firstChar)
	{
		Message(kError_CantParse);
		return NULL;
	}
	else if (firstChar == '"')			// string literal
	{
		Offset()++;
		const char* endQuotePtr = strchr(CurText(), '"');
		if (!endQuotePtr)
		{
			Message(kError_MismatchedQuotes);
			return NULL;
		}
		else
		{
			std::string strLit(CurText(), endQuotePtr - CurText());
			Offset() = endQuotePtr - Text() + 1;
			FormatString(strLit);
			return ScriptToken::Create(strLit);
		}
	}
	else if (firstChar == '$')	// string vars passed to vanilla cmds as '$var'; not necessary here but allowed for consistency
	{
		bExpectStringVar = true;
		Offset()++;
	}

	std::string token = GetCurToken();
	std::string refToken = token;

	// some operators (e.g. ->) expect a string literal, filter them out now
	if (curOp && curOp->ExpectsStringLiteral()) {
		if (!token.length() || bExpectStringVar) {
			Message(kError_ExpectedStringLiteral);
			return NULL;
		}
		else {
			return ScriptToken::Create(token);
		}
	}

	// try to convert to a number
	char* leftOvers = NULL;
	double dVal = strtod(token.c_str(), &leftOvers);
	if (*leftOvers == 0)	// entire string parsed as a double
		return ScriptToken::Create(dVal);

	// check for a calling object
	Script::RefVariable* callingObj = NULL;
	UInt16 refIdx = 0;
	UInt32 dotPos = token.find('.');
	if (dotPos != -1)
	{
		refToken = token.substr(0, dotPos);
		token = token.substr(dotPos + 1);
	}

	// before we go any further, check for local variable in case of name collisions between vars and other objects
	if (dotPos == -1)
	{
		VariableInfo* varInfo = LookupVariable(token.c_str(), NULL);
		if (varInfo)
			return ScriptToken::Create(varInfo, 0, m_scriptBuf->GetVariableType(varInfo, NULL));
	}

	// "player" can be base object or ref. Assume base object unless called with dot syntax
	if (!_stricmp(refToken.c_str(), "player") && dotPos != -1)
		refToken = "playerRef";

	Script::RefVariable* refVar = m_scriptBuf->ResolveRef(refToken.c_str());
	if (dotPos != -1 && !refVar)
	{
		Message(kError_InvalidDotSyntax);
		return NULL;
	}
	else if (refVar)
		refIdx = m_scriptBuf->GetRefIdx(refVar);

	if (refVar)
	{
		if (dotPos == -1)
		{
			if (refVar->varIdx)			// it's a variable
				return ScriptToken::Create(m_scriptBuf->vars.GetVariableByName(refVar->name.m_data), 0, Script::eVarType_Ref);
			else if (refVar->form && refVar->form->typeID == kFormType_Global)
				return ScriptToken::Create((TESGlobal*)refVar->form, refIdx);
			else						// literal reference to a form
				return ScriptToken::Create(refVar, refIdx);
		}
		else if (refVar->form && refVar->form->typeID != kFormType_Reference && refVar->form->typeID != kFormType_Quest)
		{
			Message(kError_InvalidDotSyntax);
			return NULL;
		}
	}

	// command?
	if (!bExpectStringVar)
	{
		CommandInfo* cmdInfo = g_scriptCommands.GetByName(token.c_str());
		if (cmdInfo)
		{
			// if quest script, check that calling obj supplied for cmds requiring it
			if (m_scriptBuf->scriptType == Script::eType_Quest && cmdInfo->needsParent && !refVar)
			{
				Message(kError_RefRequired, cmdInfo->longName);
				return NULL;
			}
			if (refVar && refVar->form && refVar->form->typeID != kFormType_Reference)	// make sure we're calling it on a reference
				return NULL;

			return ScriptToken::Create(cmdInfo, refIdx);
		}
	}

	// variable?
	VariableInfo* varInfo = LookupVariable(token.c_str(), refVar);
	if (!varInfo && dotPos != -1)
	{
		Message(kError_CantFindVariable, token.c_str());
		return NULL;
	}
	else if (varInfo)
	{
		UInt8 theVarType = m_scriptBuf->GetVariableType(varInfo, refVar);
		if (bExpectStringVar && theVarType != Script::eVarType_String)
		{
			Message(kError_ExpectedStringVariable);
			return NULL;
		}
		else
			return ScriptToken::Create(varInfo, refIdx, theVarType);

	}
	else if (bExpectStringVar)
	{
		Message(kError_ExpectedStringVariable);
		return NULL;
	}

	if (refVar != NULL)
	{
		Message(kError_InvalidDotSyntax);
		return NULL;
	}

	// anything else that makes it this far is treated as string
	if (!curOp || curOp->type != kOpType_MemberAccess) {
		Message(kWarning_UnquotedString, token.c_str());
	}

	FormatString(token);
	return ScriptToken::Create(token);
}

bool ExpressionParser::ParseFunctionCall(CommandInfo* cmdInfo)
{
	// trick Cmd_Parse into thinking it is parsing the only command on this line
	UInt32 oldOffset = Offset();
	UInt32 oldOpcode = m_lineBuf->cmdOpcode;
	UInt16 oldCallingRefIdx = m_lineBuf->callingRefIndex;

	// reserve space to record total # of bytes used for cmd args
	UInt16 oldDataOffset = m_lineBuf->dataOffset;
	UInt16* argsLenPtr = (UInt16*)(m_lineBuf->dataBuf + m_lineBuf->dataOffset);
	m_lineBuf->dataOffset += 2;

	// save the original paramText, overwrite with params following this function call
	UInt32 oldLineLength = m_lineBuf->paramTextLen;
	char oldLineText[0x200];
	memcpy(oldLineText, m_lineBuf->paramText, 0x200);
	memset(m_lineBuf->paramText, 0, 0x200);
	memcpy(m_lineBuf->paramText, oldLineText + oldOffset, 0x200 - oldOffset);

	// rig ScriptLineBuffer fields
	m_lineBuf->cmdOpcode = cmdInfo->opcode;
	m_lineBuf->callingRefIndex = 0;
	m_lineBuf->lineOffset = 0;
	m_lineBuf->paramTextLen = m_lineBuf->paramTextLen - oldOffset;
	
	// parse the command if numParams > 0
	bool bParsed = ParseNestedFunction(cmdInfo, m_lineBuf, m_scriptBuf);

	// restore original state, save args length
	m_lineBuf->callingRefIndex = oldCallingRefIdx;
	m_lineBuf->lineOffset += oldOffset;		// skip any text used as command arguments
	m_lineBuf->paramTextLen = oldLineLength;
	*argsLenPtr = m_lineBuf->dataOffset - oldDataOffset;
	m_lineBuf->cmdOpcode = oldOpcode;
	memcpy(m_lineBuf->paramText, oldLineText, 0x200);

	return bParsed;
}

VariableInfo* ExpressionParser::LookupVariable(const char* varName, Script::RefVariable* refVar)
{
	Script::VarInfoEntry* vars = &m_scriptBuf->vars;

	if (refVar)
	{
		if (!refVar->form)	// it's a ref variable, can't get var
			return NULL;

		Script* script = GetScriptFromForm(refVar->form);
		if (script)
			vars = &script->varList;
		else		// not a scripted object
			return NULL;
	}

	if (!vars)
		return NULL;

	return vars->GetVariableByName(varName);
}

std::string ExpressionParser::GetCurToken()
{
	char ch;
	const char* tokStart = CurText();
	while ((ch = Peek()))
	{
		if (isspace(ch) || (ispunct(ch) && ch != '_' && ch != '.'))
			break;
		Offset()++;
	}

	return std::string(tokStart, CurText() - tokStart);
}

// error routines

#if RUNTIME

void ShowRuntimeError(Script* script, const char* fmt, ...)
{
	char errorHeader[0x400];
	sprintf_s(errorHeader, 0x400, "Error in script %08x", script ? script->refID : 0);

	va_list args;
	va_start(args, fmt);

	char	errorMsg[0x400];
	vsprintf_s(errorMsg, 0x400, fmt, args);

	Console_Print(errorHeader);
	_MESSAGE(errorHeader);
	Console_Print(errorMsg);
	_MESSAGE(errorMsg);

	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RuntimeScriptError, errorMsg, 4, NULL); 

	va_end(args);
}

/**********************************
	ExpressionEvaluator
**********************************/

UInt8 ExpressionEvaluator::ReadByte()
{
	UInt8 byte = *Data();
	Data()++;
	return byte;
}

SInt8 ExpressionEvaluator::ReadSignedByte()
{
	SInt8 byte = *((SInt8*)Data());
	Data()++;
	return byte;
}

UInt16 ExpressionEvaluator::Read16()
{
	UInt16 data = *((UInt16*)Data());
	Data() += 2;
	return data;
}

SInt16 ExpressionEvaluator::ReadSigned16()
{
	SInt16 data = *((SInt16*)Data());
	Data() += 2;
	return data;
}

UInt32 ExpressionEvaluator::Read32()
{
	UInt32 data = *((UInt32*)Data());
	Data() += 4;
	return data;
}

SInt32 ExpressionEvaluator::ReadSigned32()
{
	SInt32 data = *((SInt32*)Data());
	Data() += 4;
	return data;
}

double ExpressionEvaluator::ReadFloat()
{
	double data = *((double*)Data());
	Data() += sizeof(double);
	return data;
}

std::string ExpressionEvaluator::ReadString()
{
	UInt16 len = Read16();
	char* buf = new char[len + 1];
	memcpy(buf, Data(), len);
	buf[len] = 0;
	std::string str = buf;
	Data() += len;
	delete buf;
	return str;
}

void ExpressionEvaluator::PushOnStack()
{
	ThreadLocalData& localData = ThreadLocalData::Get();
	ExpressionEvaluator* top = localData.expressionEvaluator;
	m_parent = top;
	localData.expressionEvaluator = this;

	// inherit properties of parent
	if (top) {
		// figure out base offset into script data
		if (top->script == script) {
			m_baseOffset = top->m_data - (UInt8*)script->data - 4;
		}
		else {	// non-recursive user-defined function call
			m_baseOffset = m_data - (UInt8*)script->data - 4;
		}

		// inherit flags
		m_flags.RawSet(top->m_flags.Get());
		m_flags.Clear(kFlag_ErrorOccurred);
	}
}

void ExpressionEvaluator::PopFromStack()
{
	if (m_parent) {
		// propogate info to parent
		m_parent->m_expectedReturnType = m_expectedReturnType;
		if (m_flags.IsSet(kFlag_ErrorOccurred)) {
			m_parent->m_flags.Set(kFlag_ErrorOccurred);
		}
	}

	ThreadLocalData& localData = ThreadLocalData::Get();
	localData.expressionEvaluator = m_parent;
}

ExpressionEvaluator::ExpressionEvaluator(COMMAND_ARGS) : m_opcodeOffsetPtr(opcodeOffsetPtr), m_result(result), 
	m_thisObj(thisObj), script(scriptObj), eventList(eventList), m_params(paramInfo), m_numArgsExtracted(0), m_baseOffset(0),
	m_expectedReturnType(kRetnType_Default)
{
	m_scriptData = (UInt8*)scriptData;
	m_data = m_scriptData + *m_opcodeOffsetPtr;

	memset(m_args, 0, sizeof(m_args));

	m_containingObj = containingObj;	
	m_baseOffset = *opcodeOffsetPtr - 4;

	m_flags.Clear();

	PushOnStack();
}

ExpressionEvaluator::~ExpressionEvaluator()
{
	PopFromStack();

	for (UInt32 i = 0; i < kMaxArgs; i++)
		delete m_args[i];
}

bool ExpressionEvaluator::ExtractArgs()
{
	UInt32 numArgs = ReadByte();
	UInt32 curArg = 0;

	if (extraTraces)
		gLog.Indent();

	while (curArg < numArgs)
	{
		ScriptToken* arg = Evaluate();
		if (!arg)
			break;

		if (extraTraces)
			_MESSAGE("Extracting arg %d : %s", curArg, arg->DebugPrint());

		m_args[curArg++] = arg;
	}

	if (extraTraces)
		gLog.Outdent();

	if (numArgs == curArg)			// all args extracted
	{
		m_numArgsExtracted = curArg;
		return true;
	}
	else
		return false;
}

bool ExpressionEvaluator::ExtractDefaultArgs(va_list varArgs, bool bConvertTESForms)
{
	if (ExtractArgs()) {
		for (UInt32 i = 0; i < NumArgs(); i++) {
			ScriptToken* arg = Arg(i);
			ParamInfo* info = &m_params[i];
			if (!ConvertDefaultArg(arg, info, bConvertTESForms, varArgs)) {
				DEBUG_PRINT("Couldn't convert arg %d", i);
				return false;
			}
		}

		return true;
	}
	else {
		DEBUG_PRINT("Couldn't extract default args");
		return false;
	}
}

class OverriddenScriptFormatStringArgs : public FormatStringArgs
{
public:
	OverriddenScriptFormatStringArgs(ExpressionEvaluator* eval, UInt32 fmtStringPos) : m_eval(eval), m_curArgIndex(fmtStringPos+1) { 
		m_fmtString = m_eval->Arg(fmtStringPos)->GetString();
	}

	virtual bool Arg(argType asType, void * outResult) {
		ScriptToken* arg = m_eval->Arg(m_curArgIndex);
		if (arg) {
			switch (asType) {
			case kArgType_Float:
				*((double*)outResult) = arg->GetNumber();
				break;
			case kArgType_Form:
				*((TESForm**)outResult) = arg->GetTESForm();
				break;
			default:
				return false;
			}

			m_curArgIndex += 1;
			return true;
		}
		else {
			return false;
		}
	}
			
	virtual bool SkipArgs(UInt32 numToSkip) { m_curArgIndex += numToSkip; return m_curArgIndex <= m_eval->NumArgs(); }
	virtual bool HasMoreArgs() { return m_curArgIndex < m_eval->NumArgs(); }
	virtual std::string GetFormatString() { return m_fmtString; }

	UInt32 GetCurArgIndex() const { return m_curArgIndex; }
private:
	ExpressionEvaluator		* m_eval;
	UInt32					m_curArgIndex;
	const char*				m_fmtString;
};

bool ExpressionEvaluator::ExtractFormatStringArgs(va_list varArgs, UInt32 fmtStringPos, char* fmtStringOut, UInt32 maxParams)
{
	// first simply evaluate all arguments, whether intended for fmt string or cmd args
	if (ExtractArgs()) {
		if (NumArgs() < fmtStringPos) {
			// uh-oh.
			return false;
		}

		// convert and store any cmd args preceding fmt string
		for (UInt32 i = 0; i < fmtStringPos; i++) {
			if (!ConvertDefaultArg(Arg(i), &m_params[i], false, varArgs)) {
				return false;
			}
		}

		// grab the format string
		const char* fmt = Arg(fmtStringPos)->GetString();
		if (!fmt) {
			return false;
		}

		// interpret the format string
		OverriddenScriptFormatStringArgs fmtArgs(this, fmtStringPos);
		if (ExtractFormattedString(fmtArgs, fmtStringOut)) {
			// convert and store any remaining cmd args
			UInt32 trailingArgsOffset = fmtArgs.GetCurArgIndex();
			if (trailingArgsOffset < NumArgs()) {
				for (UInt32 i = trailingArgsOffset; i < NumArgs(); i++) {
					// adjust index into params to account for 20 'variable' args to format string.
					if (!ConvertDefaultArg(Arg(i), &m_params[fmtStringPos+SIZEOF_FMT_STRING_PARAMS+(i-trailingArgsOffset)], false, varArgs)) {
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

bool ExpressionEvaluator::ConvertDefaultArg(ScriptToken* arg, ParamInfo* info, bool bConvertTESForms, va_list& varArgs)
{
	// hooray humongous switch statements
	switch (info->typeID) 
	{
		case kParamType_Array: 
			{
				UInt32* out = va_arg(varArgs, UInt32*);
				*out = arg->GetArray();
			}

			break;
		case kParamType_Integer:
			// handle string_var passed as integer to sv_* cmds
			if (arg->CanConvertTo(kTokenType_StringVar) && arg->GetVar()) {
				UInt32* out = va_arg(varArgs, UInt32*);
				*out = arg->GetVar()->data;
				break;
			}
			// fall-through intentional
		case kParamType_QuestStage:
		case kParamType_CrimeType:
		case kParamType_AnimationGroup:
		case kParamType_MiscellaneousStat:
		case kParamType_FormType:
		case kParamType_Alignment:
		case kParamType_EquipType:
		case kParamType_CriticalStage:
			if (arg->CanConvertTo(kTokenType_Number)) {
				UInt32* out = va_arg(varArgs, UInt32*);
				*out = arg->GetNumber();
			}
			else {
				return false;
			}

			break;
		case kParamType_Float:
			if (arg->CanConvertTo(kTokenType_Number)) {
				float* out = va_arg(varArgs, float*);
				*out = arg->GetNumber();
			}
			else {
				return false;
			}
			break;
		case kParamType_String:
			{
				const char* str = arg->GetString();
				if (str) {
					char* out = va_arg(varArgs, char*);
#pragma warning(push)
#pragma warning(disable: 4996)
					strcpy(out, str);
#pragma warning(pop)
				}
				else {
					return false;
				}
			}
			break;
		case kParamType_Axis:
			{
				char axis = arg->GetAxis();
				if (axis != -1) {
					char* out = va_arg(varArgs, char*);
					*out = axis;
				}
				else {
					return false;
				}
			}
			break;
		case kParamType_ActorValue:
			{
				UInt32 actorVal = arg->GetActorValue();
				if (actorVal != eActorVal_NoActorValue) {
					UInt32* out = va_arg(varArgs, UInt32*);
					*out = actorVal;
				}
				else {
					return false;
				}
			}
			break;
		//case kParamType_AnimationGroup:
		//	{
		//		UInt32 animGroup = arg->GetAnimGroup();
		//		if (animGroup != TESAnimGroup::kAnimGroup_Max) {
		//			UInt32* out = va_arg(varArgs, UInt32*);
		//			*out = animGroup;
		//		}
		//		else {
		//			return false;
		//		}
		//	}
		//	break;
		case kParamType_Sex:
			{
				UInt32 sex = arg->GetSex();
				if (sex != -1) {
					UInt32* out = va_arg(varArgs, UInt32*);
					*out = sex;
				}
				else {
					return false;
				}
			}
			break;
		case kParamType_MagicEffect:
			{
				EffectSetting* eff = arg->GetEffectSetting();
				if (eff) {
					EffectSetting** out = va_arg(varArgs, EffectSetting**);
					*out = eff;
				}
				else {
					return false;
				}
			}
			break;
		default:	// all the rest are TESForm
			{
				if (arg->CanConvertTo(kTokenType_Form)) {
					TESForm* form = arg->GetTESForm();

					if (!bConvertTESForms) {
						// we're done
						TESForm** out = va_arg(varArgs, TESForm**);
						*out = form;
					}
					else if (form) {	// expect form != null
						// gotta make sure form matches expected type, do pointer cast
						switch (info->typeID) {
							case kParamType_ObjectID:
								if (form->IsInventoryObject()) {
									TESForm** out = va_arg(varArgs, TESForm**);
									*out = form;
								}
								else {
									return false;
								}
								break;
							case kParamType_ObjectRef:
							case kParamType_MapMarker:
								{
									TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
									if (refr) {
										// kParamType_MapMarker must be a mapmarker refr
										if (info->typeID == kParamType_MapMarker && !refr->IsMapMarker()) {
											return false;
										}

										TESObjectREFR** out = va_arg(varArgs, TESObjectREFR**);
										*out = refr;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_Actor:
								{
									Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
									if (actor) {
										Actor** out = va_arg(varArgs, Actor**);
										*out = actor;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_SpellItem:
								{
									SpellItem* spell = DYNAMIC_CAST(form, TESForm, SpellItem);
									if (spell || form->typeID == kFormType_Book) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_Cell:
								{
									TESObjectCELL* cell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
									if (cell) {
										TESObjectCELL** out = va_arg(varArgs, TESObjectCELL**);
										*out = cell;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_MagicItem:
								{
									MagicItem* magic = DYNAMIC_CAST(form, TESForm, MagicItem);
									if (magic) {
										MagicItem** out = va_arg(varArgs, MagicItem**);
										*out = magic;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_TESObject:
								{
									TESObject* object = DYNAMIC_CAST(form, TESForm, TESObject);
									if (object) {
										TESObject** out = va_arg(varArgs, TESObject**);
										*out = object;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_ActorBase:
								{
									TESActorBase* base = DYNAMIC_CAST(form, TESForm, TESActorBase);
									if (base) {
										TESActorBase** out = va_arg(varArgs, TESActorBase**);
										*out = base;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_Container:
								{
									TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
									if (refr && refr->GetContainer()) {	
										TESObjectREFR** out = va_arg(varArgs, TESObjectREFR**);
										*out = refr;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_WorldSpace:
								{
									TESWorldSpace* space = DYNAMIC_CAST(form, TESForm, TESWorldSpace);
									if (space) {
										TESWorldSpace** out = va_arg(varArgs, TESWorldSpace**);
										*out = space;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_AIPackage:
								{
									TESPackage* pack = DYNAMIC_CAST(form, TESForm, TESPackage);
									if (pack) {
										TESPackage** out = va_arg(varArgs, TESPackage**);
										*out = pack;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_CombatStyle:
								{
									TESCombatStyle* style = DYNAMIC_CAST(form, TESForm, TESCombatStyle);
									if (style) {
										TESCombatStyle** out = va_arg(varArgs, TESCombatStyle**);
										*out = style;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_LeveledOrBaseChar:
								{
									TESNPC* NPC = DYNAMIC_CAST(form, TESForm, TESNPC);
									if (NPC) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										TESLevCharacter* lev = DYNAMIC_CAST(form, TESForm, TESLevCharacter);
										if (lev) {
											TESForm** out = va_arg(varArgs, TESForm**);
											*out = form;
										}
										else {
											return false;
										}
									}
								}
								break;
							case kParamType_LeveledOrBaseCreature:
								{
									TESCreature* crea = DYNAMIC_CAST(form, TESForm, TESCreature);
									if (crea) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										TESLevCreature* lev = DYNAMIC_CAST(form, TESForm, TESLevCreature);
										if (lev) {
											TESForm** out = va_arg(varArgs, TESForm**);
											*out = form;
										}
										else {
											return false;
										}
									}
								}
								break;
							case kParamType_Owner:
							case kParamType_AnyForm:
								{
									TESForm** out = va_arg(varArgs, TESForm**);
									*out = form;
								}
								break;
							case kParamType_InvObjOrFormList:
								{
									if (form->IsInventoryObject() || (form->typeID == kFormType_ListForm)) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										return false;
									}
								}
								break;
							case kParamType_NonFormList:
								{
									if (form->Unk_3A() && (form->typeID != kFormType_ListForm)) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										return false;
									}
								}
								break;
							default:
								// these all check against a particular formtype, return TESForm*
								{
									UInt32 typeToMatch = -1;
									switch (info->typeID) {
										case kParamType_Sound:
											typeToMatch = kFormType_Sound; break;
										case kParamType_Topic:
											typeToMatch = kFormType_DIAL; break;
										case kParamType_Quest:
											typeToMatch = kFormType_Quest; break;
										case kParamType_Race:
											typeToMatch = kFormType_Race; break;
										case kParamType_Faction:
											typeToMatch = kFormType_Faction; break;
										case kParamType_Class:
											typeToMatch = kFormType_Class; break;
										case kParamType_Global:
											typeToMatch = kFormType_Global; break;
										case kParamType_Furniture:
											typeToMatch = kFormType_Furniture; break;
										case kParamType_FormList:
											typeToMatch = kFormType_ListForm; break;
										//case kParamType_Birthsign:
										//	typeToMatch = kFormType_BirthSign; break;
										case kParamType_WeatherID:
											typeToMatch = kFormType_Weather; break;
										case kParamType_NPC:
											typeToMatch = kFormType_NPC; break;
										case kParamType_EffectShader:
											typeToMatch = kFormType_EffectShader; break;
										case kParamType_MenuIcon:
											typeToMatch = kFormType_MenuIcon; break;
										case kParamType_Perk:
											typeToMatch = kFormType_Perk; break;
										case kParamType_Note:
											typeToMatch = kFormType_Note; break;
										case kParamType_ImageSpaceModifier:
											typeToMatch = kFormType_ImageSpaceModifier; break;
										case kParamType_ImageSpace:
											typeToMatch = kFormType_ImageSpace; break;
										case kParamType_EncounterZone:
											typeToMatch = kFormType_EncounterZone; break;
										case kParamType_Message:
											typeToMatch = kFormType_Message; break;
										case kParamType_SoundFile:
											typeToMatch = kFormType_SoundFile; break;
										case kParamType_LeveledChar:
											typeToMatch = kFormType_LeveledCharacter; break;
										case kParamType_LeveledCreature:
											typeToMatch = kFormType_LeveledCreature; break;
										case kParamType_LeveledItem:
											typeToMatch = kFormType_LeveledItem; break;
										case kParamType_Reputation:
											typeToMatch = kFormType_Reputation; break;
										case kParamType_Casino:
											typeToMatch = kFormType_Casino; break;
										case kParamType_CasinoChip:
											typeToMatch = kFormType_CasinoChip; break;
										case kParamType_Challenge:
											typeToMatch = kFormType_Challenge; break;
										case kParamType_CaravanMoney:
											typeToMatch = kFormType_CaravanMoney; break;
										case kParamType_CaravanCard:
											typeToMatch = kFormType_CaravanCard; break;
										case kParamType_CaravanDeck:
											typeToMatch = kFormType_CaravanDeck; break;
										case kParamType_Region:
											typeToMatch = kFormType_Region; break;
									}

									if (form->typeID == typeToMatch) {
										TESForm** out = va_arg(varArgs, TESForm**);
										*out = form;
									}
									else {
										return false;
									}
								}
						}
					}
					else {
						// null form
						return false;
					}
				}
			}
	}

	return true;
}

ScriptToken* ExpressionEvaluator::Evaluate()
{
	std::stack<ScriptToken*> operands;
	
	UInt16 argLen = Read16();
	UInt8* endData = Data() + argLen - sizeof(UInt16);
	while (Data() < endData)
	{
		ScriptToken* curToken = ScriptToken::Read(this);
		if (!curToken)
			break;

		if (curToken->Type() == kTokenType_Command)
		{
			// execute the command
			CommandInfo* cmdInfo = curToken->GetCommandInfo();
			if (!cmdInfo)
			{
				delete curToken;
				curToken = NULL;
				break;
			}

			TESObjectREFR* callingObj = m_thisObj;
			Script::RefVariable* callingRef = script->GetVariable(curToken->GetRefIndex());
			if (callingRef)
			{
				callingRef->Resolve(eventList);
				if (callingRef->form && callingRef->form->IsReference())
					callingObj = DYNAMIC_CAST(callingRef->form, TESForm, TESObjectREFR);
				else
				{
					delete curToken;
					curToken = NULL;
					Error("Attempting to call a function on a NULL reference or base object");
					break;
				}
			}

			TESObjectREFR* contObj = callingRef ? NULL : m_containingObj;
			double cmdResult = 0;

			UInt16 argsLen = Read16();
			UInt32 numBytesRead = 0;
			UInt8* scrData = Data();

			ExpectReturnType(kRetnType_Default);	// expect default return type unless called command specifies otherwise
			bool bExecuted = cmdInfo->execute(cmdInfo->params, scrData, callingObj, contObj, script, eventList, &cmdResult, &numBytesRead);

			if (!bExecuted)
			{
				delete curToken;
				curToken = NULL;
				Error("Command %s failed to execute", cmdInfo->longName);
				break;
			}

			Data() += argsLen - 2;

			// create a new ScriptToken* based on result type, delete command token when done
			ScriptToken* cmdToken = curToken;
			curToken = ScriptToken::Create(cmdResult);

			// adjust token type if we know command return type
			CommandReturnType retnType = g_scriptCommands.GetReturnType(cmdInfo);
			if (retnType == kRetnType_Ambiguous || retnType == kRetnType_ArrayIndex)	// return type ambiguous, cmd will inform us of type to expect
				retnType = GetExpectedReturnType();

			switch (retnType)
			{
				case kRetnType_String:
				{
					StringVar* strVar = g_StringMap.Get(cmdResult);
					delete curToken;
					curToken = ScriptToken::Create(strVar ? strVar->GetCString() : "");
					break;
				}
				case kRetnType_Array:
				{
					// ###TODO: cmds can return arrayID '0', not necessarily an error, does this support that?
					if (g_ArrayMap.Exists(cmdResult) || !cmdResult)	
					{
						delete curToken;
						curToken = ScriptToken::CreateArray(cmdResult);
						break;
					}
					else
					{
						Error("A command returned an invalid array");
						break;
					}
				}
				case kRetnType_Form:
				{
					delete curToken;
					curToken = ScriptToken::CreateForm(*((UInt64*)&cmdResult));
					break;
				}
				case kRetnType_Default:
					delete curToken;
					curToken = ScriptToken::Create(cmdResult);
					break;
				default:
					Error("Unknown command return type %d while executing command in ExpressionEvaluator::Evaluate()", retnType);
			}

			delete cmdToken;
			cmdToken = NULL;
		}


		if (!(curToken->Type() == kTokenType_Operator))
			operands.push(curToken);
		else
		{
			Operator* op = curToken->GetOperator();
			ScriptToken* lhOperand = NULL;
			ScriptToken* rhOperand = NULL;

			if (op->numOperands > operands.size())
			{
				delete curToken;
				curToken = NULL;
				Error("Too few operands for operator %s", op->symbol);
				break;
			}

			switch (op->numOperands)
			{
			case 2:
				rhOperand = operands.top();
				operands.pop();
			case 1:
				lhOperand = operands.top();
				operands.pop();
			}

			ScriptToken* opResult = op->Evaluate(lhOperand, rhOperand, this);
			delete lhOperand;
			delete rhOperand;
			delete curToken;
			curToken = NULL;

			if (!opResult)
			{
				Error("Operator %s failed to evaluate to a valid result", op->symbol);
				break;
			}

			operands.push(opResult);
		}
	}

	// adjust opcode offset ptr (important for recursive calls to Evaluate()
	*m_opcodeOffsetPtr = m_data - m_scriptData;

	if (operands.size() != 1)		// should have one operand remaining - result of expression
	{
		Error("An expression failed to evaluate to a valid result");
		while (operands.size())
		{
			delete operands.top();
			operands.pop();
		}
		return NULL;
	}

	return operands.top();
}

//	Pop required operand(s)
//	loop through OperationRules until a match is found
//	check operand(s)->CanConvertTo() for rule types (also swap them and test if !asymmetric)
//	if can convert --> pass to rule handler, return result :: else, continue loop
//	if no matching rule return null
ScriptToken* Operator::Evaluate(ScriptToken* lhs, ScriptToken* rhs, ExpressionEvaluator* context)
{
	if (numOperands == 0)	// how'd we get here?
	{
		context->Error("Attempting to evaluate %s but this operator takes no operands", this->symbol);
		return NULL;
	}

	for (UInt32 i = 0; i < numRules; i++)
	{
		bool bRuleMatches = false;
		bool bSwapOrder = false;
		OperationRule* rule = &rules[i];
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
			return bSwapOrder ? rule->eval(type, rhs, lhs, context) : rule->eval(type, lhs, rhs, context);
	}

	return NULL;
}

bool BasicTokenToElem(ScriptToken* token, ArrayElement& elem, ExpressionEvaluator* context)
{
	ScriptToken* basicToken = token->ToBasicToken();
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
	{
		ArrayID arrID = basicToken->GetArray();
		elem.SetArray(arrID, g_ArrayMap.GetOwningModIndex(arrID));
	}
	else
		bResult = false;

	delete basicToken;
	return bResult;
}

#else			// CS only

static enum BlockType
{
	kBlockType_Invalid	= 0,
	kBlockType_ScriptBlock,
	kBlockType_Loop,
	kBlockType_If
};

struct Block
{
	enum {
		kFunction_Open		= 1,
		kFunction_Terminate	= 2,
		kFunction_Dual		= kFunction_Open | kFunction_Terminate
	};

	const char	* keyword;
	BlockType	type;
	UInt8		function;

	bool IsOpener() { return (function & kFunction_Open) == kFunction_Open; }
	bool IsTerminator() { return (function & kFunction_Terminate) == kFunction_Terminate; }
};

struct BlockInfo
{
	BlockType	type;
	UInt32		scriptLine;
};

static Block s_blocks[] =
{
	{	"begin",	kBlockType_ScriptBlock,	Block::kFunction_Open	},
	{	"end",		kBlockType_ScriptBlock,	Block::kFunction_Terminate	},
	{	"while",	kBlockType_Loop,		Block::kFunction_Open	},
	{	"foreach",	kBlockType_Loop,		Block::kFunction_Open	},
	{	"loop",		kBlockType_Loop,		Block::kFunction_Terminate	},
	{	"if",		kBlockType_If,			Block::kFunction_Open	},
	{	"elseif",	kBlockType_If,			Block::kFunction_Dual	},
	{	"else",		kBlockType_If,			Block::kFunction_Dual	},
	{	"endif",	kBlockType_If,			Block::kFunction_Terminate	},
};

static UInt32 s_numBlocks = SIZEOF_ARRAY(s_blocks, Block);

// Preprocessor
//	is used to check loop integrity, syntax before script is compiled
class Preprocessor
{
private:
	static std::string	s_delims;

	ScriptBuffer		* m_buf;
	UInt32				m_loopDepth;
	std::string			m_curLineText;
	UInt32				m_curLineNo;
	UInt32				m_curBlockStartingLineNo;
	std::string			m_scriptText;
	UInt32				m_scriptTextOffset;

	bool		HandleDirectives();		// compiler directives at top of script prefixed with '@'
	bool		ProcessBlock(BlockType blockType);
	bool		AdvanceLine();
	const char	* BlockTypeAsString(BlockType type);
public:
	Preprocessor(ScriptBuffer* buf);

	bool Process();		// returns false if an error is detected
};

std::string Preprocessor::s_delims = " \t\r\n(;";

const char* Preprocessor::BlockTypeAsString(BlockType type)
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

Preprocessor::Preprocessor(ScriptBuffer* buf) : m_buf(buf), m_loopDepth(0), m_curLineText(""), m_curLineNo(0),
	m_curBlockStartingLineNo(1), m_scriptText(buf->scriptText), m_scriptTextOffset(0)
{
	AdvanceLine();
}

bool Preprocessor::AdvanceLine()
{
	if (m_scriptTextOffset >= m_scriptText.length())
		return false;

	m_curLineNo++;

	UInt32 endPos = m_scriptText.find("\r\n", m_scriptTextOffset);
	if (endPos == -1)						// last line, no CRLF
	{
		m_curLineText = m_scriptText.substr(m_scriptTextOffset, m_scriptText.length() - m_scriptTextOffset);
		m_scriptTextOffset = m_scriptText.length();
		return true;
	}
	else if (m_scriptTextOffset == endPos)		// empty line
	{
		m_scriptTextOffset += 2;
		return AdvanceLine();
	}
	else									// line contains text
	{
		m_curLineText = m_scriptText.substr(m_scriptTextOffset, endPos - m_scriptTextOffset);

		// strip comments
		for (UInt32 i = 0; i < m_curLineText.length(); i++)
		{
			if (m_curLineText[i] == '"')
			{
				if (i + 1 == m_curLineText.length())	// trailing, mismatched quote - CS will catch
					break;

				i = m_curLineText.find('"', i+1);
				if (i == -1)		// mismatched quotes, CS compiler will catch
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

	if (!HandleDirectives()) {
		return false;
	}

	bool bContinue = true;
	while (bContinue)
	{
		Tokenizer tokens(m_curLineText.c_str(), s_delims.c_str());
		if (tokens.NextToken(token) == -1)		// empty line
		{
			bContinue = AdvanceLine();
			continue;
		}
		
		const char* tok = token.c_str();
		bool bIsBlockKeyword = false;
		for (UInt32 i = 0; i < s_numBlocks; i++)
		{
			Block* cur = &s_blocks[i];
			if (!_stricmp(tok, cur->keyword))
			{
				bIsBlockKeyword = true;
				if (cur->IsTerminator())
				{
					if (!blockStack.size() || blockStack.top() != cur->type)
					{
						const char* blockStr = BlockTypeAsString(blockStack.size() ? blockStack.top() : cur->type);
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
					const char* scriptText = m_buf->scriptText;

					UInt32 dotPos = varToken.find('.');
					if (dotPos != -1)
					{
						scriptText = NULL;
						std::string s = varToken.substr(0, dotPos);
						const char * temp = s.c_str();
						TESForm* refForm = GetFormByID(temp);
						//TESForm* refForm = GetFormByID(varToken.substr(0, dotPos).c_str());
						if (refForm)
						{
							Script* refScript = GetScriptFromForm(refForm);
							if (refScript)
								scriptText = refScript->text;
						}
						varName = varToken.substr(dotPos + 1);
					}

					if (scriptText)
					{
						UInt32 varType = GetDeclaredVariableType(varName.c_str(), scriptText);
						if (varType == Script::eVarType_Array)
						{
							g_ErrOut.Show("Error line %d:\nSet may not be used to assign to an array variable", m_curLineNo);
							return false;
						}
						else if (false && varType == Script::eVarType_String)	// this is un-deprecated b/c older plugins don't register return types
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
			else
			{
				// ###TODO: check for ResetAllVariables, anything else?
			}
		}

		if (bContinue)
			bContinue = AdvanceLine();
	}

	if (blockStack.size())
	{
		g_ErrOut.Show("Error: Mismatched block structure.");
		return false;
	}

	return true;
}

bool PrecompileScript(ScriptBuffer* buf)
{
	return Preprocessor(buf).Process();
}

#endif

bool Cmd_Expression_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
#if RUNTIME
	Console_Print("This command cannot be called from the console.");
	return false;
#endif

	ExpressionParser parser(scriptBuf, lineBuf);
	return (parser.ParseArgs(paramInfo, numParams));
}
