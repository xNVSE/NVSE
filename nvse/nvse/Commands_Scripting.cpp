#include "Commands_Scripting.h"

#include <format>

#include "GameForms.h"
#include "GameAPI.h"
#include <intrin.h>
#include "GameScript.h"
#include "utilities.h"

#include "ScriptUtils.h"
#include "Hooks_Script.h"

#include "Commands_Console.h"
#include "PluginManager.h"

#if RUNTIME

#include "GameAPI.h"
#include "GameObjects.h"
#include "InventoryReference.h"
#include "FunctionScripts.h"
#include "Loops.h"

static const void * kOpHandlerRetnAddr = (void *)0x005E234B;

#endif

//static SavedIPInfo s_savedIPTable[kMaxSavedIPs] = { { 0 } };
typedef UnorderedMap<UInt32, SavedIPInfo> MapSavedIPInfo;
static UnorderedMap<UInt32, MapSavedIPInfo> s_savedIPTable;

// ### stack abuse! get a pointer to the parent's stack frame
// ### valid when parent is called from kOpHandlerRetnAddr
ScriptRunner * GetScriptRunner(UInt32 * opcodeOffsetPtr)
{
	UInt8			* scriptRunnerEBP = ((UInt8 *)opcodeOffsetPtr) + 0x10;
	ScriptRunner	** scriptRunnerPtr = (ScriptRunner **)(scriptRunnerEBP - 0xED0);	// this
	ScriptRunner	* scriptRunner = *scriptRunnerPtr;

	return scriptRunner;
}

SInt32 * GetCalculatedOpLength(UInt32 * opcodeOffsetPtr)
{
	UInt8	* scriptRunnerEBP = ((UInt8 *)opcodeOffsetPtr) + 0x10;
	UInt8	* parentEBP = scriptRunnerEBP + 0x7B0;
	SInt32	* opLengthPtr = (SInt32 *)(parentEBP - 0x14);

	return opLengthPtr;
}

#if RUNTIME

bool Cmd_Label_Execute(COMMAND_ARGS)
{
	UInt32	idx = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &idx)) return true;

#if USE_EXTRACT_ARGS_EX
	*opcodeOffsetPtr += *(UInt16*)((UInt8*)scriptData + *opcodeOffsetPtr - 2);
#endif

	SavedIPInfo		* info = &s_savedIPTable[scriptObj->refID][idx];
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	// save ip
	info->ip = *opcodeOffsetPtr;

	// save if/else/endif stack
	info->stackDepth = scriptRunner->ifStackDepth;
	ASSERT((info->stackDepth + 1) < kMaxSavedIPStack);

	memcpy(info->stack, scriptRunner->ifStack, (info->stackDepth + 1) * sizeof(UInt32));

	return true;
}

bool Cmd_Goto_Execute(COMMAND_ARGS)
{
	UInt32	idx = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &idx)) return true;

	MapSavedIPInfo *savedIPInfo = s_savedIPTable.GetPtr(scriptObj->refID);
	if (!savedIPInfo) return true;

	SavedIPInfo *info = savedIPInfo->GetPtr(idx);
	if (!info) return true;

#if USE_EXTRACT_ARGS_EX
	*opcodeOffsetPtr += *(UInt16*)((UInt8*)scriptData + *opcodeOffsetPtr - 2);
#endif

	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);
	SInt32			* calculatedOpLength = GetCalculatedOpLength(opcodeOffsetPtr);

	// restore ip
	*calculatedOpLength += info->ip - *opcodeOffsetPtr;

	// restore the if/else/endif stack
	scriptRunner->ifStackDepth = info->stackDepth;
	memcpy(scriptRunner->ifStack, info->stack, (info->stackDepth + 1) * sizeof(UInt32));

	return true;
}

// *********** commands

bool Cmd_Let_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator evaluator(PASS_COMMAND_ARGS);

	evaluator.ExtractArgs();

	return true;
}

// used to evaluate NVSE expressions within 'if' statements
// i.e. if eval (array[idx] == someThing)
bool Cmd_eval_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);

	if (eval.ExtractArgs() && eval.Arg(0))
		*result = eval.Arg(0)->GetBool() ? 1 : 0;

	return true;
}

// attempts to evaluate an expression. Returns false if error occurs, true otherwise. Suppresses error messages
bool Cmd_testexpr_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	eval.ToggleErrorSuppression(true);

	if (eval.ExtractArgs() && eval.Arg(0) && eval.Arg(0)->IsGood() && !eval.HasErrors())
	{
		if (eval.Arg(0)->Type() == kTokenType_ArrayElement)		// is it an array elem with valid index?
		{
			ArrayVar *arr = g_ArrayMap.Get(eval.Arg(0)->GetOwningArrayID());
			if (arr) *result = arr->HasKey(eval.Arg(0)->GetArrayKey());
		}
		else
			*result = 1;
	}
	return true;
}

bool Cmd_While_Execute(COMMAND_ARGS)
{
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	// read offset to end of loop
	UInt8* data = (UInt8*)scriptData + *opcodeOffsetPtr;
	UInt32 offsetToEnd = *(UInt32*)data;

	// calc offset of first instruction following this While expression
	data += 5;		// UInt32 offset + UInt8 numArgs
	UInt16 exprLen = *((UInt16*)data);
	UInt32 startOffset = *opcodeOffsetPtr + 5 + exprLen;

	// create the loop and add it to loop manager
	WhileLoop* loop = new WhileLoop(*opcodeOffsetPtr + 4);
	LoopManager* mgr = LoopManager::GetSingleton();
	mgr->Add(loop, scriptRunner, startOffset, offsetToEnd, PASS_COMMAND_ARGS);

	// test condition, break immediately if false
	*opcodeOffsetPtr = startOffset;		// need to update to point to _next_ instruction before calling loop->Update()
	if (!loop->Update(PASS_COMMAND_ARGS))
		mgr->Break(scriptRunner, PASS_COMMAND_ARGS);

	return true;
}

bool Cmd_ForEach_Execute(COMMAND_ARGS)
{
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	// get offset to end of loop
	UInt8* data = (UInt8*)scriptData + *opcodeOffsetPtr;
	UInt32 offsetToEnd = *(UInt32*)data;

	// calc offset to first instruction within loop
	data += 5;
	UInt16 exprLen = *((UInt16*)data);
	UInt32 startOffset = *opcodeOffsetPtr + 5 + exprLen;

	ForEachLoop* loop = NULL;

	// evaluate the expression to get the context
	*opcodeOffsetPtr += 4;				// set to start of expression
	{
		// ExpressionEvaluator enclosed in this scope so that it's lock is released once we've extracted the args.
		// This eliminates potential for deadlock when adding loop to LoopManager
		ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		bool bExtracted = eval.ExtractArgs();
		*opcodeOffsetPtr -= 4;				// restore

		if (!bExtracted || !eval.Arg(0))
		{
			ShowRuntimeError(scriptObj, "ForEach expression failed to return a value");
			return false;
		}

		const ForEachContext* context = eval.Arg(0)->GetForEachContext();
		if (!context) {
			ShowRuntimeError(scriptObj, "Cmd_ForEach: Expression does not evaluate to a ForEach context");
		}
		else // construct the loop
		{
			if (context->iterVar.type == Script::eVarType_Array)
			{
				ArrayIterLoop* arrayLoop = new ArrayIterLoop(context, scriptObj);
				AddToGarbageCollection(eventList, arrayLoop->m_valueIterVar.var, NVSEVarType::kVarType_Array);
				loop = arrayLoop;
			}
			else if (context->iterVar.type == Script::eVarType_String)
			{
				StringIterLoop* stringLoop = new StringIterLoop(context);
				loop = stringLoop;
			}
			else if (context->iterVar.type == Script::eVarType_Ref)
			{
				if IS_ID(((TESForm*)context->sourceID), BGSListForm)
					loop = new FormListIterLoop(context);
				else loop = new ContainerIterLoop(context);
			}
		}
	}

	if (loop)
	{
		LoopManager* mgr = LoopManager::GetSingleton();
		mgr->Add(loop, scriptRunner, startOffset, offsetToEnd, PASS_COMMAND_ARGS);
		if (loop->IsEmpty())
		{
			*opcodeOffsetPtr = startOffset;
			mgr->Break(scriptRunner, PASS_COMMAND_ARGS);
		}
	}

	return true;
}

namespace ForEachAlt
{
	bool ValidateKeyVariableType(Variable keyVar, ArrayVar* sourceArray, ExpressionEvaluator& eval)
	{
		auto arrayType = sourceArray->GetContainerType();
		if (arrayType == kContainer_Array || arrayType == kContainer_NumericMap)
		{
			if (keyVar.GetType() != Script::eVarType_Integer
				&& keyVar.GetType() != Script::eVarType_Float)
			{
				eval.Error("ForEachAlt >> Invalid variable type to receive array keys (keys were numeric-type, var was not)");
				return false;
			}
		}
		else { // assume stringmap
			if (keyVar.GetType() != Script::eVarType_String)
			{
				eval.Error("ForEachAlt >> Invalid variable type to receive array keys (keys were string-type, var was not)");
				return false;
			}
		}
		return true;
	}
}

// More efficient ForEach loops for arrays.
// Instead of using an iterator array, populates passed variable(s) with value/key.
// However, this method doesn't work if there's more than one type of element values in the source array (throws error).
bool Cmd_ForEachAlt_Execute(COMMAND_ARGS)
{
	ScriptRunner* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	UInt32 offsetToEnd, startOffset = *opcodeOffsetPtr;
	{
		// get offset to end of loop
		UInt8* data = (UInt8*)scriptData + *opcodeOffsetPtr;
		offsetToEnd = *(UInt32*)data;
	}

	ForEachLoop* loop = nullptr;

	// evaluate the expression to get the context
	*opcodeOffsetPtr += 4;	// set to start of expression (skip 4 bonus bytes of loop info)
	{
		// ExpressionEvaluator enclosed in this scope so that it's lock is released once we've extracted the args.
		// This eliminates potential for deadlock when adding loop to LoopManager
		ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		bool bExtracted = eval.ExtractArgs();

		// calc offset to first instruction within loop
		startOffset = startOffset + (*opcodeOffsetPtr - startOffset) + 1;

		*opcodeOffsetPtr -= 4;	// restore

		if (!bExtracted || !eval.Arg(0) || eval.NumArgs() < 2) [[unlikely]]
		{
			ShowRuntimeError(scriptObj, "ForEachAlt >> Failed to extract args.");
			return false;
		}

		// Construct the loop
		if (auto* sourceArr = eval.Arg(0)->GetArrayVar()) [[likely]]
		{
			ArrayID sourceID = sourceArr->ID();
			Variable valueVar(eval.Arg(1)->GetScriptLocal(), static_cast<Script::VariableType>(eval.Arg(1)->variableType));
			std::optional<Variable> keyVar;
			if (eval.NumArgs() >= 3) {
				keyVar = Variable(eval.Arg(2)->GetScriptLocal(), static_cast<Script::VariableType>(eval.Arg(2)->variableType));
				if (keyVar->IsValid()) {
					if (!ForEachAlt::ValidateKeyVariableType(*keyVar, sourceArr, eval)) [[unlikely]] {
						sourceID = 0; // will make loop->IsEmpty() return true
					}
				}
			}
			else // If NumArgs == 2, then the sole iter variable could be a key iter if sourceArr is a map, or a value iter otherwise.
			{
				// valueVar could act as a keyVar if sourceArr is a map; verify var type if so.
				if (sourceArr->GetContainerType() != kContainer_Array) {
					if (!ForEachAlt::ValidateKeyVariableType(valueVar, sourceArr, eval)) [[unlikely]] {
						sourceID = 0; // will make loop->IsEmpty() return true
					}
				}
			}
			if (!valueVar.IsValid() && (!keyVar.has_value() || !keyVar->IsValid())) [[unlikely]] {
				sourceID = 0; // will make loop->IsEmpty() return true
			}
			ArrayIterLoop* arrayLoop = new ArrayIterLoop(sourceID, scriptObj, valueVar, keyVar);
			loop = arrayLoop;
		}
		else {
			eval.Error("ForEachAlt >> Invalid source array.");
			// todo: maybe reset opcodeOffsetPtr or something?
		}
	}

	if (loop) [[likely]]
	{
		LoopManager* mgr = LoopManager::GetSingleton();
		mgr->Add(loop, scriptRunner, startOffset, offsetToEnd, PASS_COMMAND_ARGS);
		if (loop->IsEmpty())
		{
			*opcodeOffsetPtr = startOffset;
			mgr->Break(scriptRunner, PASS_COMMAND_ARGS);
		}
	}

	return true;
}

bool Cmd_Break_Execute(COMMAND_ARGS)
{
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	LoopManager* mgr = LoopManager::GetSingleton();
	if (mgr->Break(scriptRunner, PASS_COMMAND_ARGS))
		return true;

	ShowRuntimeError(scriptObj, "Break called outside of a valid loop context.");
	return false;
}

bool Cmd_Continue_Execute(COMMAND_ARGS)
{
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	LoopManager* mgr = LoopManager::GetSingleton();
	if (mgr->Continue(scriptRunner, PASS_COMMAND_ARGS))
	{
		*opcodeOffsetPtr += 4;
		return true;
	}

	ShowRuntimeError(scriptObj, "Continue called outside of a valid loop context.");
	return false;
}

bool Cmd_Loop_Execute(COMMAND_ARGS)
{
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	LoopManager* mgr = LoopManager::GetSingleton();
	if (mgr->Continue(scriptRunner, PASS_COMMAND_ARGS))
		return true;

	ShowRuntimeError(scriptObj, "Loop called outside of a valid loop context.");
	return false;
}

bool Cmd_ToString_Execute(COMMAND_ARGS)
{
	*result = 0;
	std::string tokenAsString = "NULL";

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.Arg(0))
	{
		if (eval.Arg(0)->CanConvertTo(kTokenType_String))
			tokenAsString = eval.Arg(0)->GetString();
		else if (eval.Arg(0)->CanConvertTo(kTokenType_Number))
		{
			char buf[0x20];
			snprintf(buf, sizeof(buf), "%g", eval.Arg(0)->GetNumber());
			tokenAsString = buf;
		}
		else if (eval.Arg(0)->CanConvertTo(kTokenType_Form))
			tokenAsString = GetFullName(eval.Arg(0)->GetTESForm());
	}

	AssignToStringVar(PASS_COMMAND_ARGS, tokenAsString.c_str());
	return true;
}

bool Cmd_Print_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.Arg(0))
	{
		const auto str = eval.Arg(0)->GetStringRepresentation();
		if (str.size() < 512)
			Console_Print("%s", str.c_str());
		else
			Console_Print_Long(str);
#if _DEBUG
		// useful for testing script output
		//_MESSAGE("%s", str.c_str());
#endif
	}

	return true;
}

bool Cmd_PrintDebug_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (ModDebugState(scriptObj))
	{
		ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		if (eval.ExtractArgs() && eval.Arg(0))
		{
			const auto str = eval.Arg(0)->GetStringRepresentation();
			if (str.size() < 512)
				Console_Print("%s", str.c_str());
			else
				Console_Print_Long(str);
#if _DEBUG
			// useful for testing script output
			_MESSAGE("%s", str.c_str());
#endif
		}
	}

	return true;
}

bool Cmd_PrintF(COMMAND_ARGS, bool debug)
{
	*result = 0;
	if (!debug || ModDebugState(scriptObj))
	{
		ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		if (eval.ExtractArgs() && eval.Arg(0) && eval.Arg(0)->CanConvertTo(kTokenType_String) && eval.Arg(1) && eval.Arg(1)->CanConvertTo(kTokenType_String))
		{
			try
			{
				FILE* f;
				errno_t e = fopen_s(&f, eval.Arg(0)->GetString(), "a+");
				if (!e)
				{
					const char* str = eval.Arg(1)->GetString();
					fprintf(f, "%s\n", str);
					fclose(f);
				}
				else
					_MESSAGE("Cannot open file %s [%d]", eval.Arg(0)->GetString(), e);

			}
			catch(...)
			{
				_MESSAGE("Cannot write to file %s", eval.Arg(0)->GetString());
			}
		}
	}
	return true;
}

bool Cmd_PrintF_Execute(COMMAND_ARGS)
{
	*result = 0;
	return Cmd_PrintF(PASS_COMMAND_ARGS, false);
}

bool Cmd_PrintDebugF_Execute(COMMAND_ARGS)
{
	*result = 0;
	return Cmd_PrintF(PASS_COMMAND_ARGS, true);
}

bool Cmd_TypeOf_Execute(COMMAND_ARGS)
{
	const char *typeStr = "NULL";

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.Arg(0))
	{
		if (eval.Arg(0)->CanConvertTo(kTokenType_Number))
			typeStr = "Number";
		else if (eval.Arg(0)->CanConvertTo(kTokenType_String))
			typeStr = "String";
		else if (eval.Arg(0)->CanConvertTo(kTokenType_Form))
			typeStr = "Form";
		else if (eval.Arg(0)->CanConvertTo(kTokenType_Array))
		{
			ArrayVar *arr = g_ArrayMap.Get(eval.Arg(0)->GetArrayID());
			if (!arr) typeStr = "<Bad Array>";
			else if (arr->KeyType() == kDataType_Numeric)
				typeStr = arr->IsPacked() ? "Array" : "Map";
			else if (arr->KeyType() == kDataType_String)
				typeStr = "StringMap";
		}
	}

	AssignToStringVar(PASS_COMMAND_ARGS, typeStr);
	return true;
}

bool Cmd_Function_Execute(COMMAND_ARGS)
{
	*result = UserFunctionManager::Enter(scriptObj) ? 1 : 0;
	return true;
}

bool Cmd_Call_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (auto const funcResult = UserFunctionManager::Call(&eval))
	{
		funcResult->AssignResult(eval);
	}
	return true;
}

// don't use this in scripts.
bool Cmd_CallFunctionCond_Execute(COMMAND_ARGS)
{
	*result = 0;
	return true;
}
bool Cmd_CallFunctionCond_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	std::unique_ptr<ScriptToken> tokenResult = nullptr;
	
	if (auto const pListForm = (BGSListForm*)arg1)  // safe to cast since the condition won't allow picking any other formType.
	{
		auto const bBreakIfFalse = (UInt32)arg2;  // if true, if a UDF returns false, breaks the formlist loop and returns 0.
		for (auto const &form : pListForm->list)
		{
			if (auto const scriptIter = DYNAMIC_CAST(form, TESForm, Script))
			{
				InternalFunctionCaller caller(scriptIter, thisObj, nullptr);
				caller.SetArgs(0);
				if (tokenResult = UserFunctionManager::Call(std::move(caller)))
				{
					if (bBreakIfFalse && !tokenResult->GetBool())
						return true;
				}
			}
		}
	}
	if (!tokenResult || !tokenResult->IsGood()) return true;
	if (auto const num = tokenResult->GetNumber(); num != 0.0)
	{
		*result = num;
	}
	else
	{
		*result = tokenResult->GetBool(); 
	}
	return true;
}


bool Cmd_SetFunctionValue_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!UserFunctionManager::Return(&eval))
		ShowRuntimeError(scriptObj, "SetFunctionValue statement failed.");

	return true;
}

bool Cmd_GetUserTime_Execute(COMMAND_ARGS)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_String, false, scriptObj->GetModIndex());
	*result = arr->ID();

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	arr->SetElementNumber("Year", localTime.wYear);
	arr->SetElementNumber("Month", localTime.wMonth);
	arr->SetElementNumber("DayOfWeek", localTime.wDayOfWeek + 1);
	arr->SetElementNumber("Day", localTime.wDay);
	arr->SetElementNumber("Hour", localTime.wHour);
	arr->SetElementNumber("Minute", localTime.wMinute);
	arr->SetElementNumber("Second", localTime.wSecond);
	arr->SetElementNumber("Millisecond", localTime.wMilliseconds);

	return true;
}

class ModLocalDataManager
{
	// the idea here is to allow mods to store data in memory independent of any savegame
	// this lets them keep track of their mod state as savegames are loaded, resetting things if necessary or performing other housework
	// data is stored as key:value pairs, key is string, value is a formID, number, or string

public:
	ArrayElement* Get(UInt8 modIndex, const char* key);
	bool Set(UInt8 modIndex, const char* key, const ArrayElement& data);
	bool Remove(UInt8 modIndex, const char* key);
	ArrayID GetAllAsNVSEArray(UInt8 modIndex);

private:
	typedef UnorderedMap<char*, ArrayElement> ModLocalData;
	typedef UnorderedMap<UInt32, ModLocalData> ModLocalDataMap;

	ModLocalDataMap m_data;
};

ModLocalDataManager s_modDataManager;

ArrayElement* ModLocalDataManager::Get(UInt8 modIndex, const char* key)
{
	ModLocalData *modLocData = m_data.GetPtr(modIndex);
	if (modLocData)
		return modLocData->GetPtr(const_cast<char*>(key));
	return NULL;
}

ArrayID ModLocalDataManager::GetAllAsNVSEArray(UInt8 modIndex)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_String, false, modIndex);
	ModLocalData *modLocData = m_data.GetPtr(modIndex);
	if (modLocData)
		for (auto dataIter = modLocData->Begin(); !dataIter.End(); ++dataIter)
			arr->SetElement(dataIter.Key(), &dataIter.Get());
	return arr->ID();
}

bool ModLocalDataManager::Remove(UInt8 modIndex, const char* key)
{
	ModLocalData *modLocData = m_data.GetPtr(modIndex);
	if (modLocData)
	{
		auto dataIter = modLocData->Find(const_cast<char*>(key));
		if (!dataIter.End())
		{
			dataIter.Get().Unset();
			dataIter.Remove();
			return true;
		}
	}
	return false;
}

bool ModLocalDataManager::Set(UInt8 modIndex, const char* key, const ArrayElement& data)
{
	if (*key)
	{
		//MakeUpper(const_cast<char*>(key));
		m_data[modIndex][const_cast<char*>(key)].Set(&data);
		return true;
	}
	return false;
}

bool Cmd_SetModLocalData_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 2 && eval.Arg(0)->CanConvertTo(kTokenType_String))
	{
		ArrayElement elem;
		if (BasicTokenToElem(eval.Arg(1), elem) && (elem.DataType() != kDataType_Array) && s_modDataManager.Set(scriptObj->GetModIndex(), eval.Arg(0)->GetString(), elem))
			*result = 1;
	}
	return true;
}

bool Cmd_RemoveModLocalData_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String) && s_modDataManager.Remove(scriptObj->GetModIndex(), eval.Arg(0)->GetString()))
		*result = 1;
	else *result = 0;
	return true;
}

bool Cmd_GetModLocalData_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	eval.ExpectReturnType(kRetnType_Default);
	*result = 0;

	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String))
	{
		ArrayElement *data = s_modDataManager.Get(scriptObj->GetModIndex(), eval.Arg(0)->GetString());
		if (data)
		{
			switch (data->DataType())
			{
				case kDataType_Numeric:
					*result = data->m_data.num;
					break;
				case kDataType_Form:
				{
					UInt32 *refResult = (UInt32*)result;
					*refResult = data->m_data.formID;
					eval.ExpectReturnType(kRetnType_Form);
					break;
				}
				case kDataType_String:
					AssignToStringVar(PASS_COMMAND_ARGS, data->m_data.GetStr());
					eval.ExpectReturnType(kRetnType_String);
					break;
			}
		}
	}
	return true;
}

bool Cmd_ModLocalDataExists_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String) && s_modDataManager.Get(scriptObj->GetModIndex(), eval.Arg(0)->GetString()))
		*result = 1;
	else *result = 0;
	return true;
}

bool Cmd_GetAllModLocalData_Execute(COMMAND_ARGS)
{
	*result = s_modDataManager.GetAllAsNVSEArray(scriptObj->GetModIndex());
	return true;
}

bool Cmd_PrintVar_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs() || !eval.Arg(0) || !eval.Arg(0)->GetScriptLocal())
		return true;
	UInt8 argNum = 0;
	const auto numArgs = eval.NumArgs();
	do // guaranteed to have at least 1 variable to print
	{
		auto* token = eval.Arg(argNum);
		std::string variableValue;
		switch (token->Type())
		{
		case kTokenType_NumericVar:
			variableValue = FormatString("%g", token->GetNumber());
			break;
		case kTokenType_StringVar:
			variableValue = FormatString(R"("%s")", token->GetString());
			break;
		case kTokenType_ArrayVar:
			if (auto* arrayVar = token->GetArrayVar())
				variableValue = arrayVar->GetStringRepresentation();
			else
				variableValue = "uninitialized or invalid array";
			break;
		case kTokenType_RefVar:
			if (auto* form = token->GetTESForm())
				variableValue = form->GetStringRepresentation();
			else
				variableValue = "invalid form";
			[[fallthrough]];
		default:
			break;
		}
		const auto toPrint = std::string(GetVariableName(token->GetScriptLocal(), scriptObj, eventList, token->refIdx))
			+ ": " + variableValue;
		Console_Print_Str(toPrint);

		++argNum;
	} while (argNum < numArgs);

	return true;
}

bool Cmd_DebugPrintVar_Execute(COMMAND_ARGS)
{
	if (ModDebugState(scriptObj))
		return Cmd_PrintVar_Execute(PASS_COMMAND_ARGS);
	return true;
}

bool Cmd_Internal_PushExecutionContext_Execute(COMMAND_ARGS)
{
	ExtractArgsOverride::PushContext(thisObj, containingObj, (UInt8*)scriptData, opcodeOffsetPtr);
	return true;
}

bool Cmd_Internal_PopExecutionContext_Execute(COMMAND_ARGS)
{
	return ExtractArgsOverride::PopContext();
}


bool Cmd_Assert_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs() || !eval.Arg(0)->GetNumber())
	{
		const auto lineText = eval.GetLineText();
		const auto varText = eval.GetVariablesText();
		Console_Print("Assertion failed!");
		if (!lineText.empty())
			Console_Print("\t%s", lineText.c_str());
		if (!varText.empty())
			Console_Print("\tWhere %s", varText.c_str());
	}
	return true;
}

bool Cmd_GetSelfAlt_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = thisObj ? thisObj->refID : 0;
	return true;
}

bool Cmd_GetSelfAlt_OLD_Execute(COMMAND_ARGS)
{
	return Cmd_GetSelfAlt_Execute(PASS_COMMAND_ARGS);
}


std::unordered_set<std::string> pluginWarnings{};
bool Cmd_PluginVersion_Execute(COMMAND_ARGS) {
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.Arg(0)->GetString() && eval.NumArgs() == 2) {
		const auto pluginName = eval.Arg(0)->GetString();
		const auto pluginVersion = static_cast<UInt32>(eval.Arg(1)->GetNumber());

		const auto scriptName = scriptObj->GetName();

		const auto &lowered = ToLower(std::string(pluginName));
		if (pluginWarnings.contains(lowered)) {
			return true;
		}

		const auto pluginHandle = g_pluginManager.LookupHandleFromName(pluginName);
		if (pluginHandle == kPluginHandle_Invalid) {
			const auto msg = std::format(
				"A script ('{}') requires the plugin named '{}' but it was not found. Your game will NOT work correctly. Please locate the plugin.",
				scriptName,
				pluginName
			);

			DisplayMessage(msg.c_str());
		}

		// Handle NVSE version separately as it is 4 packed numbers
		else if (!_stricmp(pluginName, "nvse")) {
			if (pluginVersion >= PACKED_NVSE_VERSION) {
				return true;
			}

			const auto major = ((pluginVersion >> 24) & 0xFF);
			const auto minor = ((pluginVersion >> 16) & 0xFF);
			const auto beta =  ((pluginVersion >> 4) & 0xFFF);

			const auto msg = std::format(
				"A script ('{}') requires xNVSE version >= {}.{}.{} (You have version {}.{}.{}).\nYour game will NOT work correctly. Please update xNVSE.",
				scriptName,
				major,
				minor,
				beta,
				NVSE_VERSION_INTEGER,
				NVSE_VERSION_INTEGER_MINOR,
				NVSE_VERSION_INTEGER_BETA
			);

			DisplayMessage(msg.c_str());
		}

		// All plugins use single integer version
		else {
			const auto info = g_pluginManager.GetInfoFromHandle(pluginHandle);
			if (pluginVersion >= info->version) {
				return true;
			}

			const auto msg = std::format(
				"A script ('{}') requires the plugin '{}' >= '{}' (You have version {}).\nYour game will NOT work correctly. Please update the plugin.",
				scriptName,
				pluginName,
				pluginVersion,
				info->version
			);

			DisplayMessage(msg.c_str());
		}

		pluginWarnings.emplace(lowered);
		return false;
	}
	return true;
}
#endif

bool Cmd_Let_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	ExpressionParser parser(scriptBuf, lineBuf);
	if (!parser.ParseArgs(paramInfo, numParams))
		return false;

	// verify that assignment operator is last data recorded
	const UInt8 lastData = lineBuf->dataBuf[lineBuf->dataOffset - 1];
	switch (lastData)
	{
	case kOpType_Assignment:
	case kOpType_PlusEquals:
	case kOpType_TimesEquals:
	case kOpType_DividedEquals:
	case kOpType_ExponentEquals:
	case kOpType_MinusEquals:
	case kOpType_BitwiseOrEquals:
	case kOpType_BitwiseAndEquals:
	case kOpType_ModuloEquals:
		return true;
	default:
		#ifndef RUNTIME
			ShowCompilerError(scriptBuf, "Expected assignment in Let statement.\n\nCompiled script not saved.");
		#endif
		return false;
	}
}

bool Cmd_BeginLoop_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	// reserve space for a UInt32 storing offset past end of loop
	RegisterLoopStart(scriptBuf->scriptData + scriptBuf->dataOffset + lineBuf->dataOffset + 4);
	lineBuf->dataOffset += sizeof(UInt32);

	// parse the loop condition
	ExpressionParser parser(scriptBuf, lineBuf);
	return parser.ParseArgs(paramInfo, numParams);
}

bool Cmd_Loop_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	UInt8* endPtr = scriptBuf->scriptData + scriptBuf->dataOffset + lineBuf->dataOffset + 4;
	const UInt32 offset = endPtr - scriptBuf->scriptData;		// num bytes between beginning of script and instruction following Loop

	if (!HandleLoopEnd(offset))
	{
#ifndef RUNTIME
		ShowCompilerError(scriptBuf, "'Loop' encountered without matching 'While' or 'ForEach'.\n\nCompiled script not saved.");
#endif
		return false;
	}

	return true;
}

bool Cmd_Null_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	// takes no args, writes no data
	return true;
}

bool Cmd_Function_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	const ExpressionParser parser(scriptBuf, lineBuf);
	return parser.ParseUserFunctionDefinition();
}

bool Cmd_Call_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
	ExpressionParser parser(scriptBuf, lineBuf);
	return parser.ParseUserFunctionCall();
}

static ParamInfo kParams_OneBasicType[] =
{
	{	"expression",	kNVSEParamType_BasicType,	0	},
};

CommandInfo kCommandInfo_Let =
{
	"Let",
	"",
	0,
	"assigns the value of an expression to a variable",
	0,
	1,
	kParams_OneBasicType,
	HANDLER(Cmd_Let_Execute),
	Cmd_Let_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_eval =
{
	"eval",
	"",
	0,
	"evaluates an expression and returns a boolean result.",
	0,
	1,
	kParams_OneBoolean,
	HANDLER(Cmd_eval_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

static ParamInfo kParams_NoTypeChecking[] =
{
	{	"expression",	kNVSEParamType_NoTypeCheck,	0	},
};

CommandInfo kCommandInfo_testexpr =
{
	"testexpr",
	"",
	0,
	"returns false if errors occur while evaluating expression, true otherwise",
	0,
	1,
	kParams_NoTypeChecking,
	HANDLER(Cmd_testexpr_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_While =
{
	"while",
	"",
	0,
	"loops until the given condition evaluates to false",
	0,
	1,
	kParams_OneBoolean,
	HANDLER(Cmd_While_Execute),
	Cmd_BeginLoop_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Loop =
{
	"loop",
	"",
	0,
	"marks the end of a While or ForEach loop",
	0,
	1,
	kParams_OneOptionalInt,				// unused, but need at least one param for Parse() to be called
	HANDLER(Cmd_Loop_Execute),
	Cmd_Loop_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Continue =
{
	"continue",
	"",
	0,
	"returns control to the top of a loop",
	0,
	1,
	kParams_OneOptionalInt,				// unused, but need at least one param for Parse() to be called
	HANDLER(Cmd_Continue_Execute),
	Cmd_Null_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Break =
{
	"break",
	"",
	0,
	"exits a loop",
	0,
	1,
	kParams_OneOptionalInt,				// unused, but need at least one param for Parse() to be called
	HANDLER(Cmd_Break_Execute),
	Cmd_Null_Parse,
	NULL,
	0
};

static ParamInfo kParams_ForEach[] =
{
	{	"ForEach expression",	kNVSEParamType_ForEachContext,	0	},
};
CommandInfo kCommandInfo_ForEach =
{
	"ForEach",
	"",
	0,
	"iterates over the elements of an array",
	0,
	1,
	kParams_ForEach,
	HANDLER(Cmd_ForEach_Execute),
	Cmd_BeginLoop_Parse,
	NULL,
	0
};

// Technically either "valueVariable" or "keyVariable" are optional, 
// ..but new compiler will handle that by passing them with varIndex of 0.
static ParamInfo kParams_ForEachAlt[] =
{
	{	"sourceArray",		kNVSEParamType_Array,	0	},
	{	"valueVariable",	kNVSEParamType_Variable | kNVSEParamType_OptionalEmpty,	0	}, // if keyVariable wasn't passed, and sourceArray is a map, then this will act as a key iterator.
	{	"keyVariable",		kNVSEParamType_StringVar | kNVSEParamType_NumericVar | kNVSEParamType_OptionalEmpty,	1	},
};
CommandInfo kCommandInfo_ForEachAlt =
{
	"ForEachAlt",
	"",
	0,
	"iterates over the elements of an array, passing values directly into variables",
	false,
	std::size(kParams_ForEachAlt),
	kParams_ForEachAlt,
	HANDLER(Cmd_ForEachAlt_Execute),
	Cmd_BeginLoop_Parse,
	NULL,
	0
};

static ParamInfo kParams_OneNVSEString[] =
{
	{	"string",	kNVSEParamType_String,	0	},
};

static ParamInfo kParams_TwoNVSEString[] =
{
	{	"filename",	kNVSEParamType_String,	0	},
	{	"string",	kNVSEParamType_String,	0	},
};

CommandInfo kCommandInfo_ToString =
{
	"ToString",
	"",
	0,
	"attempts to convert an expression to a string",
	0,
	1,
	kParams_OneBasicType,
	HANDLER(Cmd_ToString_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_TypeOf =
{
	"TypeOf",
	"",
	0,
	"returns a string representing the type of the expression",
	0,
	1,
	kParams_OneBasicType,
	HANDLER(Cmd_TypeOf_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Print =
{
	"Print",
	"",
	0,
	"prints a string expression to the console",
	0,
	1,
	kParams_OneNVSEString,
	HANDLER(Cmd_Print_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_PrintDebug =
{
	"PrintDebug",
	"PrintD",
	0,
	"prints a string expression to the console in debug mode",
	0,
	1,
	kParams_OneNVSEString,
	HANDLER(Cmd_PrintDebug_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_PrintF =
{
	"PrintF",
	"",
	0,
	"prints a string expression to a file",
	0,
	2,
	kParams_TwoNVSEString,
	HANDLER(Cmd_PrintF_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_PrintDebugF =
{
	"PrintDebugF",
	"PrintDF",
	0,
	"prints a string expression to a file in debug mode",
	0,
	2,
	kParams_TwoNVSEString,
	HANDLER(Cmd_PrintDebugF_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Function =
{
	"Function",
	"",
	0,
	"defines a function",
	0,
	1,
	kParams_OneOptionalInt,
	HANDLER(Cmd_Function_Execute),
	Cmd_Function_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_Call =
{
	"Call",
	"",
	0,
	"calls a user-defined function",
	0,
	1,
	kParams_OneString,
	HANDLER(Cmd_Call_Execute),
	Cmd_Call_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_SetFunctionValue =
{
	"SetFunctionValue",
	"",
	0,
	"returns a value from a user-defined function",
	0,
	1,
	kParams_OneBasicType,
	HANDLER(Cmd_SetFunctionValue_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

static ParamInfo kNVSEParams_SetModLocalData[2] =
{
	{	"key",	kNVSEParamType_String,	0	},
	{	"data",	kNVSEParamType_String | kNVSEParamType_Number | kNVSEParamType_Form,	0	},
};

static ParamInfo kNVSEParams_OneString[1] =
{
	{	"string",	kNVSEParamType_String,	0	},
};

CommandInfo kCommandInfo_SetModLocalData =
{
	"SetModLocalData",
	"",
	0,
	"sets a key-value pair for the mod",
	0,
	2,
	kNVSEParams_SetModLocalData,
	HANDLER(Cmd_SetModLocalData_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_GetModLocalData =
{
	"GetModLocalData",
	"",
	0,
	"gets a key-value pair for the mod",
	0,
	1,
	kNVSEParams_OneString,
	HANDLER(Cmd_GetModLocalData_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_ModLocalDataExists =
{
	"ModLocalDataExists",
	"",
	0,
	"returns true if mod local data exists for the specified key",
	0,
	1,
	kNVSEParams_OneString,
	HANDLER(Cmd_ModLocalDataExists_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_RemoveModLocalData =
{
	"RemoveModLocalData", "", 0,
	"removes the specified entry from the mod's local data",
	0, 1, kNVSEParams_OneString,
	HANDLER(Cmd_RemoveModLocalData_Execute),
	Cmd_Expression_Parse, NULL, 0
};

CommandInfo kCommandInfo_Internal_PushExecutionContext =
{
	"@PushExecutionContext", "", 0,
	"internal command - pushes execution context for current script for use with ExtractArgsOverride",
	0, 0, NULL,
	HANDLER(Cmd_Internal_PushExecutionContext_Execute), Cmd_Null_Parse, NULL, 0
};

CommandInfo kCommandInfo_Internal_PopExecutionContext =
{
	"@PopExecutionContext", "", 0,
	"internal command - pops execution context for current script for use with ExtractArgsOverride",
	0, 0, NULL,
	HANDLER(Cmd_Internal_PopExecutionContext_Execute), Cmd_Null_Parse, NULL, 0
};
