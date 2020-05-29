#include "Commands_Scripting.h"
#include "GameForms.h"
#include "GameAPI.h"
#include <intrin.h>
#include "GameScript.h"
#include "utilities.h"

#include "ScriptUtils.h"
#include "Hooks_Script.h"

#include "Commands_Console.h"

#if RUNTIME

#include "GameAPI.h"
#include "GameObjects.h"
#include "InventoryReference.h"
#include "FunctionScripts.h"
#include "Loops.h"

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const void * kOpHandlerRetnAddr = (void *)0x005E234B;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
static const void * kOpHandlerRetnAddr = (void *)0x005E225B;
#else
#error
#endif

#endif

//static SavedIPInfo s_savedIPTable[kMaxSavedIPs] = { { 0 } };
typedef std::map<UInt32, SavedIPInfo> MapSavedIPInfo;
static std::map<UInt32, MapSavedIPInfo> s_savedIPTable;

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
	// make sure this is only called from the main execution loop
	ASSERT_STR(scriptData == scriptObj->data, "Label may not be called inside a set or if statement");
	ASSERT(_ReturnAddress() == kOpHandlerRetnAddr);

	UInt32	idx = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &idx)) return true;

	// this must happen after extractargs updates opcodeOffsetPtr so it points to the next instruction
	if (s_savedIPTable.find(scriptObj->refID) == s_savedIPTable.end())
	{
		MapSavedIPInfo * newMapSavedIPInfo = new MapSavedIPInfo;
		s_savedIPTable[scriptObj->refID] = *newMapSavedIPInfo;
	}
	if (s_savedIPTable.find(scriptObj->refID) == s_savedIPTable.end())
		return true;

	if (s_savedIPTable[scriptObj->refID].find(idx) == s_savedIPTable[scriptObj->refID].end())
	{
		SavedIPInfo* newSavedIPInfo = (SavedIPInfo*)new SavedIPInfo;
		s_savedIPTable[scriptObj->refID][idx] = *newSavedIPInfo;
	}
	if (s_savedIPTable[scriptObj->refID].find(idx) == s_savedIPTable[scriptObj->refID].end())
		return true;

	SavedIPInfo		* info = &s_savedIPTable[scriptObj->refID][idx];
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);

	// save ip
	info->ip = *opcodeOffsetPtr;

	// save if/else/endif stack
	info->stackDepth = scriptRunner->stackDepth;
	ASSERT((info->stackDepth + 1) < kMaxSavedIPStack);

	memcpy(info->stack, scriptRunner->stack, (info->stackDepth + 1) * sizeof(UInt32));

	return true;
}

bool Cmd_Goto_Execute(COMMAND_ARGS)
{
	// make sure this is only called from the main execution loop
	ASSERT_STR(scriptData == scriptObj->data, "Goto may not be called inside a set or if statement");
	ASSERT(_ReturnAddress() == kOpHandlerRetnAddr);

	UInt32	idx = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &idx)) return true;

	if (s_savedIPTable.find(scriptObj->refID) == s_savedIPTable.end())
		return true;
	if (s_savedIPTable[scriptObj->refID].find(idx) == s_savedIPTable[scriptObj->refID].end()) 
		return true;

	SavedIPInfo		* info = &s_savedIPTable[scriptObj->refID][idx];
	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);
	SInt32			* calculatedOpLength = GetCalculatedOpLength(opcodeOffsetPtr);

	// restore ip
	*calculatedOpLength += info->ip - *opcodeOffsetPtr;

	// restore the if/else/endif stack
	scriptRunner->stackDepth = info->stackDepth;
	memcpy(scriptRunner->stack, info->stack, (info->stackDepth + 1) * sizeof(UInt32));

	return true;
}

// *********** commands

bool Cmd_Let_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator evaluator(PASS_COMMAND_ARGS);

	if (extraTraces)
		gLog.Indent();

	if (extraTraces)
		_MESSAGE("Extracting args for Let at %08X", *opcodeOffsetPtr - 4);

	evaluator.ExtractArgs();

	if (extraTraces)
		gLog.Outdent();

	return true;
}

// used to evaluate NVSE expressions within 'if' statements
// i.e. if eval (array[idx] == someThing)
bool Cmd_eval_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);

	if (extraTraces)
		gLog.Indent();

	if (extraTraces)
		_MESSAGE("Extracting args for Eval at %08X", *opcodeOffsetPtr - 4);

	if (eval.ExtractArgs() && eval.Arg(0))
		*result = eval.Arg(0)->GetBool() ? 1 : 0;

	if (extraTraces)
		gLog.Outdent();

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
			const ArrayKey* key = eval.Arg(0)->GetArrayKey();
			*result = (g_ArrayMap.HasKey(eval.Arg(0)->GetOwningArrayID(), *key)) ? 1 : 0;
		}
		else
			*result = 1;
	}

	eval.ToggleErrorSuppression(false);
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
		if (!context)
			ShowRuntimeError(scriptObj, "Cmd_ForEach: Expression does not evaluate to a ForEach context");
		else		// construct the loop
		{
			if (context->variableType == Script::eVarType_Array)
			{
				ArrayIterLoop* arrayLoop = new ArrayIterLoop(context, scriptObj->GetModIndex());
				loop = arrayLoop;
			}
			else if (context->variableType == Script::eVarType_String)
			{
				StringIterLoop* stringLoop = new StringIterLoop(context);
				loop = stringLoop;
			}
			else if (context->variableType == Script::eVarType_Ref)
			{
				loop = new ContainerIterLoop(context);
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
			sprintf_s(buf, sizeof(buf), "%g", eval.Arg(0)->GetNumber());
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
	if (eval.ExtractArgs() && eval.Arg(0) && eval.Arg(0)->CanConvertTo(kTokenType_String))
	{
		const char* str = eval.Arg(0)->GetString();
		if (strlen(str) < 512)
			Console_Print(str);
		else
			Console_Print_Long(str);
#if _DEBUG
		// useful for testing script output
		_MESSAGE(str);
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
		if (eval.ExtractArgs() && eval.Arg(0) && eval.Arg(0)->CanConvertTo(kTokenType_String))
		{
			const char* str = eval.Arg(0)->GetString();
			if (strlen(str) < 512)
				Console_Print(str);
			else
				Console_Print_Long(str);
#if _DEBUG
			// useful for testing script output
			_MESSAGE(str);
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
					fprintf(f, str);
					fprintf(f, "\r\n");
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
	std::string typeStr = "NULL";

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
			typeStr = g_ArrayMap.GetTypeString(eval.Arg(0)->GetArray());
	}

	AssignToStringVar(PASS_COMMAND_ARGS, typeStr.c_str());
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

	if (extraTraces)
		gLog.Indent();

	if (extraTraces)
		_MESSAGE("Extracting args for Call at %08X", *opcodeOffsetPtr - 4);

	ScriptToken* funcResult = UserFunctionManager::Call(&eval);
	if (funcResult)
	{
		if (funcResult->CanConvertTo(kTokenType_Number))
			*result = funcResult->GetNumber();
		else if (funcResult->CanConvertTo(kTokenType_String))
		{
			AssignToStringVar(PASS_COMMAND_ARGS, funcResult->GetString());
			eval.ExpectReturnType(kRetnType_String);
		}
		else if (funcResult->CanConvertTo(kTokenType_Form))
		{
			UInt32* refResult = (UInt32*)result;
			*refResult = funcResult->GetFormID();
			eval.ExpectReturnType(kRetnType_Form);
		}
		else if (funcResult->CanConvertTo(kTokenType_Array))
		{
			*result = funcResult->GetArray();
			eval.ExpectReturnType(kRetnType_Array);
		}
		else
			ShowRuntimeError(scriptObj, "Function call returned unexpected token type %d", funcResult->Type());
	}

	if (extraTraces)
		gLog.Outdent();

	delete funcResult;
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
	ArrayID arrID = g_ArrayMap.Create(kDataType_String, false, scriptObj->GetModIndex());
	*result = arrID;

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	g_ArrayMap.SetElementNumber(arrID, "Year", localTime.wYear);
	g_ArrayMap.SetElementNumber(arrID, "Month", localTime.wMonth);
	g_ArrayMap.SetElementNumber(arrID, "DayOfWeek", localTime.wDayOfWeek + 1);
	g_ArrayMap.SetElementNumber(arrID, "Day", localTime.wDay);
	g_ArrayMap.SetElementNumber(arrID, "Hour", localTime.wHour);
	g_ArrayMap.SetElementNumber(arrID, "Minute", localTime.wMinute);
	g_ArrayMap.SetElementNumber(arrID, "Second", localTime.wSecond);
	g_ArrayMap.SetElementNumber(arrID, "Millisecond", localTime.wMilliseconds);

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
	bool Set(UInt8 modIndex, const char* key, ScriptToken* data, ExpressionEvaluator& eval);
	bool Remove(UInt8 modIndex, const char* key);
	ArrayID GetAllAsNVSEArray(UInt8 modIndex);

	~ModLocalDataManager();
private:
	typedef std::map<const char*, ArrayElement*, bool (*)(const char*, const char*)> ModLocalData;
	typedef std::map<UInt8, ModLocalData*> ModLocalDataMap;

	ModLocalDataMap m_data;
};

ModLocalDataManager s_modDataManager;

ArrayElement* ModLocalDataManager::Get(UInt8 modIndex, const char* key)
{
	ModLocalDataMap::iterator iter = m_data.find(modIndex);
	if (iter != m_data.end() && iter->second) {
		ModLocalData::iterator dataIter = iter->second->find(key);
		if (dataIter != iter->second->end()) {
			return dataIter->second;
		}
	}

	return NULL;
}

ArrayID ModLocalDataManager::GetAllAsNVSEArray(UInt8 modIndex)
{
	ArrayID id = g_ArrayMap.Create(kDataType_String, false, modIndex);
	ModLocalDataMap::iterator iter = m_data.find(modIndex);
	if (iter != m_data.end() && iter->second) {
		for (ModLocalData::iterator dataIter = iter->second->begin(); dataIter != iter->second->end(); ++dataIter) {
			ArrayElement* elem = dataIter->second;
			const char* key = dataIter->first;
			g_ArrayMap.SetElement(id, key, *elem);
		}
	}

	return id;
}

bool ModLocalDataManager::Remove(UInt8 modIndex, const char* key)
{
	ModLocalDataMap::iterator iter = m_data.find(modIndex);
	if (iter != m_data.end() && iter->second) {
		ModLocalData::iterator dataIter = iter->second->find(key);
		if (dataIter != iter->second->end()) {
			iter->second->erase(dataIter);
			return true;
		}
	}

	return false;
}

bool ModLocalDataManager::Set(UInt8 modIndex, const char* key, const ArrayElement& data)
{
	ArrayID dummy = 0;
	if (key && !data.GetAsArray(&dummy)) {
		UInt32 len = strlen(key) + 1;
		char* newKey = new char[len+1];
		strcpy_s(newKey, len, key);
		MakeUpper(newKey);
		key = newKey;

		ModLocalDataMap::iterator indexIter = m_data.find(modIndex);
		if (indexIter == m_data.end()) {
			indexIter = m_data.insert(ModLocalDataMap::value_type(modIndex, new ModLocalData(ci_less))).first;
		}

		ModLocalData::iterator dataIter = indexIter->second->find(key);
		if (dataIter == indexIter->second->end()) {
			dataIter = indexIter->second->insert(ModLocalData::value_type(key, new ArrayElement())).first;
		}

		return dataIter->second->Set(data);
	}

	return false;
}

bool ModLocalDataManager::Set(UInt8 modIndex, const char* key, ScriptToken* data, ExpressionEvaluator& eval)
{
	if (data) {
		ArrayElement elem;
		if (BasicTokenToElem(data, elem, &eval)) {
			return Set(modIndex, key, elem);
		}
	}

	return false;
}

ModLocalDataManager::~ModLocalDataManager()
{
	for (ModLocalDataMap::iterator index = m_data.begin(); index != m_data.end(); ++index) {
		for (ModLocalData::iterator data = index->second->begin(); data != index->second->end(); ++data) {
			delete data->second;
			delete data->first;
		}
		delete index->second;
	}
}

bool Cmd_SetModLocalData_Execute(COMMAND_ARGS)
{
	*result = 0.0;

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 2 && eval.Arg(0)->CanConvertTo(kTokenType_String)) {
		*result = s_modDataManager.Set(scriptObj->GetModIndex(), eval.Arg(0)->GetString(), eval.Arg(1), eval) ? 1.0 : 0.0;
	}

	return true;
}

bool Cmd_RemoveModLocalData_Execute(COMMAND_ARGS)
{
	*result = 0.0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String)) {
		*result = s_modDataManager.Remove(scriptObj->GetModIndex(), eval.Arg(0)->GetString()) ? 1.0 : 0.0;
	}

	return true;
}

bool Cmd_GetModLocalData_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	eval.ExpectReturnType(kRetnType_Default);
	*result = 0;

	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String)) {
		ArrayElement* data = s_modDataManager.Get(scriptObj->GetModIndex(), eval.Arg(0)->GetString());
		if (data) {
			double num = 0;
			UInt32 formID = 0;
			std::string str;

			if (data->GetAsNumber(&num)) {
				*result = num;
				return true;
			}
			else if (data->GetAsFormID(&formID)) {
				UInt32* refResult = (UInt32*)result;
				*refResult = formID;
				eval.ExpectReturnType(kRetnType_Form);
				return true;
			}
			else if (data->GetAsString(str)) {
				AssignToStringVar(PASS_COMMAND_ARGS, str.c_str());
				eval.ExpectReturnType(kRetnType_String);
				return true;
			}
		}
	}

	return true;
}

bool Cmd_ModLocalDataExists_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 1 && eval.Arg(0)->CanConvertTo(kTokenType_String)) {
		*result = s_modDataManager.Get(scriptObj->GetModIndex(), eval.Arg(0)->GetString()) ? 1.0 : 0.0;
	}

	return true;
}

bool Cmd_GetAllModLocalData_Execute(COMMAND_ARGS)
{
	*result = s_modDataManager.GetAllAsNVSEArray(scriptObj->GetModIndex());
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

#endif

bool Cmd_Let_Parse(UInt32 numParams, ParamInfo* paramInfo, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf)
{
#if RUNTIME
	Console_Print("Let cannot be called from the console.");
	return false;
#endif

	ExpressionParser parser(scriptBuf, lineBuf);
	if (!parser.ParseArgs(paramInfo, numParams))
		return false;

	// verify that assignment operator is last data recorded
	UInt8 lastData = lineBuf->dataBuf[lineBuf->dataOffset - 1];
	switch (lastData)
	{
	case kOpType_Assignment:
	case kOpType_PlusEquals:
	case kOpType_TimesEquals:
	case kOpType_DividedEquals:
	case kOpType_ExponentEquals:
	case kOpType_MinusEquals:
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
	UInt32 offset = endPtr - scriptBuf->scriptData;		// num bytes between beginning of script and instruction following Loop

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
	ExpressionParser parser(scriptBuf, lineBuf);
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

static ParamInfo kParams_OneBoolean[] =
{
	{	"boolean expression",	kNVSEParamType_Boolean,	0	},
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
