#include "GameAPI.h"
#include "GameScript.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "CommandTable.h"
#include "GameData.h"
#include "GameRTTI.h"
#include "ScriptUtils.h"

// Does NOT list synonyms, only the base types.
const char *g_variableTypeNames[6] =
	{
		"float",
		"int",
		"string_var",
		"array_var",
		"ref",
		"invalid"};

// Lists all valid variable names, some of which may be synonyms.
const char* g_validVariableTypeNames[8] =
{
	"float",
	"int",
	"string_var",
	"array_var",
	"ref",
	"short",
	"long",
	"reference"};

#if RUNTIME
std::span<CommandInfo> g_eventBlockCommandInfos = {reinterpret_cast<CommandInfo*>(0x118E2F0), 38};
std::span<CommandInfo> g_scriptStatementCommandInfos = {reinterpret_cast<CommandInfo*>(0x118CB50), 16};
std::span<ScriptOperator> g_gameScriptOperators = {reinterpret_cast<ScriptOperator*>(0x118CAD0), 16};
std::span<ActorValueInfo*> g_actorValues = { reinterpret_cast<ActorValueInfo**>(0x11D61C8), eActorVal_FalloutMax };
#else
std::span<CommandInfo> g_eventBlockCommandInfos = {reinterpret_cast<CommandInfo*>(0xE9D598), 38};
std::span<CommandInfo> g_scriptStatementCommandInfos = {reinterpret_cast<CommandInfo*>(0xE9BDF8), 16};
std::span<ScriptOperator> g_gameScriptOperators = {reinterpret_cast<ScriptOperator*>(0xE9BD7D), 16};
#endif

Script::VariableType VariableTypeNameToType(const char *name)
{
	auto varType = Script::eVarType_Invalid;
	if (!StrCompare(name, "string_var"))
		varType = Script::eVarType_String;
	else if (!StrCompare(name, "array_var"))
		varType = Script::eVarType_Array;
	else if (!StrCompare(name, "float"))
		varType = Script::eVarType_Float;
	else if (!StrCompare(name, "long") || !StrCompare(name, "int") || !StrCompare(name, "short"))
		varType = Script::eVarType_Integer;
	else if (!StrCompare(name, "ref") || !StrCompare(name, "reference"))
		varType = Script::eVarType_Ref;
	return varType;
}

const char* VariableTypeToName(Script::VariableType type)
{
	switch (type) { case Script::eVarType_Float: return "float";
	case Script::eVarType_Integer: return "int";
	case Script::eVarType_String: return "string";
	case Script::eVarType_Array: return "array";
	case Script::eVarType_Ref: return "ref";
	case Script::eVarType_Invalid: 
	default: return "invalid";
	}
}

Script::VariableType GetDeclaredVariableType(const char* varName, const char* scriptText, Script* script)
{
#if NVSE_CORE
	if (const auto savedVarType = GetSavedVarType(script, varName); savedVarType != Script::eVarType_Invalid)
		return savedVarType;
#endif
	ScriptTokenizer tokenizer(scriptText);
	while (tokenizer.TryLoadNextLine())
	{
		// Need a C-string w/ null terminator - hence std::string conversion.
		auto token1View = std::string(tokenizer.GetNextLineToken());
		if (token1View.empty())
			continue;

		// Check if var is declared in UDF parameters
		if (!StrCompare(token1View.c_str(), "begin")) [[unlikely]]
		{
			// assume the next token is "Function"
			std::vector<std::string> paramTokens;
			GetUserFunctionParamTokensFromLine(tokenizer.GetLineText(), paramTokens);
			if (paramTokens.size() <= 1)
				continue;

			// Find the varName, then check if the token before it is a varType.
			// Assume the varName can only appear once in the param list.
			auto strEqual = [varName](std::string &val) { return !StrCompare(val.c_str(), varName); };

			auto iter = ra::find_if(paramTokens, strEqual);
			if (iter != paramTokens.end() && iter != paramTokens.begin())
			{
				auto prevIter = iter - 1;
				const auto varType = VariableTypeNameToType(prevIter->c_str());
				if (varType != Script::eVarType_Invalid)
				{
				#if NVSE_CORE
					SaveVarType(script, varName, varType);
				#endif
					return varType;
				}
			}
			continue; // line began with "begin", so we can't find any var declarations outside of stuff in UDF params
		}

		// else, try matching w/ token1 = varType, token2 = varName.
		auto token2View = tokenizer.GetNextLineToken();
		if (token2View.empty())
			continue;

		const auto varType = VariableTypeNameToType(token1View.c_str());
		if (varType == Script::eVarType_Invalid)
			continue;

		// Handle possible multiple variable declarations on one line.
		auto existingVarName = std::string(token2View);
		bool prevVarHadComma;
		do
		{
			if (prevVarHadComma = existingVarName.back() == ',')
				existingVarName.pop_back(); // remove comma from name
			if (!StrCompare(existingVarName.c_str(), varName))
			{
			#if NVSE_CORE
				SaveVarType(script, varName, varType);
			#endif
				return varType;
			}
			
			existingVarName = std::string(tokenizer.GetNextLineToken());
		}
		while (!existingVarName.empty() && prevVarHadComma);
	}
#if NVSE_CORE
	if (auto *parent = GetLambdaParentScript(script))
		return GetDeclaredVariableType(varName, parent->text, parent);
#endif
	return Script::eVarType_Invalid;
}

Script *GetScriptFromForm(TESForm *form)
{
	TESObjectREFR *refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
	if (refr)
		form = refr->baseForm;

	TESScriptableForm *scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	return scriptable ? scriptable->script : NULL;
}

bool GetUserFunctionParamTokensFromLine(std::string_view lineText, std::vector<std::string>& out)
{
	UInt32 argStartPos = lineText.find('{');
	UInt32 argEndPos = lineText.find('}');
	if (argStartPos == -1 || argEndPos == -1 || (argStartPos > argEndPos))
		return false;

	std::string_view argStrView = lineText.substr(argStartPos + 1, argEndPos - argStartPos - 1);
	auto argStr = std::string(argStrView);
	Tokenizer argTokens(argStr.c_str(), "\t ,");
	std::string argToken;
	while (argTokens.NextToken(argToken) != -1)
	{
		out.push_back(argToken);
	}
	return true;
}

Script::VariableType Script::GetVariableType(VariableInfo* varInfo)
{
	if (text)
		return GetDeclaredVariableType(varInfo->name.m_data, text, this);
	else
	{
		// if it's a ref var a matching varIdx will appear in RefList
		if (refList.Contains([&](const RefVariable *ref) { return ref->varIdx == varInfo->idx; }))
			return eVarType_Ref;
		return static_cast<Script::VariableType>(varInfo->type);
	}
}

bool Script::IsUserDefinedFunction() const
{
	auto *scriptData = static_cast<UInt8 *>(data);
	return *(scriptData + 8) == 0x0D;
}

UInt32 Script::GetVarCount() const
{
	// info->varCount include index
	auto count = 0U;
	auto *node = this->varList.Head();
	while (node)
	{
		if (node->data->data)
			++count;
		node = node->Next();
	}
	return count;
}

UInt32 Script::GetRefCount() const
{
	return refList.Count();
}

tList<VariableInfo> *Script::GetVars()
{
	return reinterpret_cast<tList<VariableInfo> *>(&this->varList);
}

tList<Script::RefVariable> *Script::GetRefList()
{
	return reinterpret_cast<tList<RefVariable> *>(&this->refList);
}

void Script::Delete()
{
#if EDITOR
	::Delete<Script, 0x5C5220>(this);
#else
	::Delete<Script, 0x5AA1A0>(this);
#endif
}

game_unique_ptr<Script> Script::MakeUnique()
{
#if EDITOR
	return ::MakeUnique<Script, 0x5C1D60, 0x5C5220>();
#else
	return ::MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
#endif
}

bool Script::Compile(ScriptBuffer* buffer)
{
#if EDITOR
	const auto address = 0x5C96E0;
	auto* scriptCompiler = (void*)0xECFDF8;
#else
	constexpr auto address = 0x5AEB90;
	auto* scriptCompiler = ConsoleManager::GetSingleton()->scriptContext;
#endif
	return ThisStdCall<bool>(address, scriptCompiler, this, buffer); // CompileScript
}

#if NVSE_CORE && RUNTIME
static UInt32 g_partialScriptCount = 0;

Script* CompileScriptEx(const char* scriptText, const char* scriptName, bool assignFormID)
{
	const auto buffer = MakeUnique<ScriptBuffer, 0x5AE490, 0x5AE5C0>();

	// For some reason, calling SetRefID seems to behave slightly differently than allowing game to auto-assign formID, so...
	// Otherwise I encountered a bug where the refID of the script got swapped with another on load. -Demorome
	DataHandler::Get()->DisableAssignFormIDs(true);
	auto script = MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
	DataHandler::Get()->DisableAssignFormIDs(false);

	buffer->scriptName.Set(scriptName ? scriptName : FormatString("nvse_partial_script_%d", ++g_partialScriptCount).c_str());
	buffer->scriptText = const_cast<char*>(scriptText);
	script->text = const_cast<char*>(scriptText);
	buffer->partialScript = true;
	*buffer->scriptData = 0x1D;
	buffer->dataOffset = 4;
	buffer->runtimeMode = ScriptBuffer::kGameConsole;
	buffer->currentScript = script.get();
	const auto result = script->Compile(buffer.get());
	buffer->scriptText = nullptr;
	script->text = nullptr;
	if (!result)
		return nullptr;

	if (assignFormID)
		script->SetRefID(GetNextFreeFormID(), true);
/*
	if (assignFormID) // prevent refID from getting shuffled around (?)
		script->DoAddForm(script.get(), false, true);
*/
	return script.release();
}

Script* CompileScript(const char* scriptText)
{
	return CompileScriptEx(scriptText);
}

Script* CompileExpression(const char* scriptText)
{
	std::string condString = scriptText;
	std::string scriptSource;
	if (!FindStringCI(condString, "SetFunctionValue"))
	{
		scriptSource = FormatString("begin function{}\nSetFunctionValue (%s)\nend\n", condString.c_str());
	}
	else
	{
		ReplaceAll(condString, "%r", "\n");
		ReplaceAll(condString, "%R", "\n");
		scriptSource = FormatString("begin function{}\n%s\nend\n", condString.c_str());
	}
	return CompileScript(scriptSource.c_str());
}

#endif

#if RUNTIME

void Script::RefVariable::Resolve(ScriptEventList *eventList)
{
	if (varIdx && eventList)
	{
		ScriptLocal *var = eventList->GetVariable(varIdx);
		if (var)
		{
			UInt32 refID = *((UInt32 *)&var->data);
			form = LookupFormByID(refID);
		}
	}
}

ICriticalSection csGameScript; // trying to avoid what looks like concurrency issues

ScriptEventList *Script::CreateEventList(void)
{
	ScriptEventList *result = NULL;
	csGameScript.Enter();
	try
	{
		result = (ScriptEventList *)ThisStdCall(0x005ABF60, this); // 4th sub above Script::Execute (was 1st above in Oblivion) Execute is the second to last call in Run
	}
	catch (...)
	{
	}

	csGameScript.Leave();
	return result;
}

bool Script::RunScriptLine2(const char *text, TESObjectREFR *object, bool bSuppressOutput)
{
	//ToggleConsoleOutput(!bSuppressOutput);

	ConsoleManager *consoleManager = ConsoleManager::GetSingleton();

	UInt8 scriptBuf[sizeof(Script)];
	Script *script = (Script *)scriptBuf;

	CALL_MEMBER_FN(script, Constructor)
	();
	CALL_MEMBER_FN(script, MarkAsTemporary)
	();
	CALL_MEMBER_FN(script, SetText)
	(text);
	bool bResult = CALL_MEMBER_FN(script, Run)(consoleManager->scriptContext, true, object);
	CALL_MEMBER_FN(script, Destructor)
	();

	//ToggleConsoleOutput(true);
	return bResult;
}

bool Script::RunScriptLine(const char *text, TESObjectREFR *object)
{
	return RunScriptLine2(text, object, false);
}

#endif

Script::RefVariable *ScriptBuffer::ResolveRef(const char *refName, Script *script)
{
	Script::RefVariable *newRef = NULL;

	// see if it's already in the refList
	auto *foundRef = refVars.FindFirst([&](Script::RefVariable *cur) { return cur->name.m_data && !_stricmp(cur->name.m_data, refName); });
	if (foundRef)
		newRef = foundRef;
	// not in list

	// is it a local ref variable?
	VariableInfo *varInfo = vars.GetVariableByName(refName);
	if (varInfo && GetVariableType(varInfo, NULL, script) == Script::eVarType_Ref)
	{
		if (!newRef)
			newRef = New<Script::RefVariable>();
		newRef->varIdx = varInfo->idx;
	}
	else // is it a form or global?
	{
		TESForm *form;
		if (_stricmp(refName, "player") == 0)
		{
			// PlayerRef (this is how the vanilla compiler handles it so I'm changing it for consistency and to fix issues)
			form = LookupFormByID(0x14);
		}
		else
		{
			form = GetFormByID(refName);
		}
		if (form)
		{
			TESObjectREFR *refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
			if (refr && !refr->IsPersistent()) // only persistent refs can be used in scripts
				return NULL;
			if (!newRef)
				newRef = New<Script::RefVariable>();
			newRef->form = form;
		}
	}

	if (!foundRef && newRef) // got it, add to refList
	{
		newRef->name.Set(refName);
		refVars.Append(newRef);
		info.numRefs++;
	}
	return newRef;
}

UInt32 ScriptBuffer::GetRefIdx(Script::RefVariable *ref)
{
	UInt32 idx = 0;
	for (Script::RefList::_Node *curEntry = refVars.Head(); curEntry && curEntry->data; curEntry = curEntry->next)
	{
		idx++;
		if (ref == curEntry->data)
			break;
	}

	return idx;
}

VariableInfo *ScriptBuffer::GetVariableByName(const char *name)
{
	if (VariableInfo * var; (var = vars.FindFirst([&](VariableInfo *v) { return _stricmp(name, v->name.CStr()) == 0; })))
		return var;
	return nullptr;
}

Script::VariableType ScriptBuffer::GetVariableType(VariableInfo* varInfo, Script::RefVariable* refVar, Script* script)
{
	const char *scrText = scriptText;
	if (refVar)
	{
		if (refVar->form)
		{
			TESScriptableForm *scriptable = NULL;
			switch (refVar->form->typeID)
			{
			case kFormType_TESObjectREFR:
			{
				TESObjectREFR *refr = DYNAMIC_CAST(refVar->form, TESForm, TESObjectREFR);
				scriptable = DYNAMIC_CAST(refr->baseForm, TESForm, TESScriptableForm);
				break;
			}
			case kFormType_TESQuest:
				scriptable = DYNAMIC_CAST(refVar->form, TESForm, TESScriptableForm);
			}

			if (scriptable && scriptable->script)
			{
				if (scriptable->script->text)
				{
					script = scriptable->script;
					scrText = scriptable->script->text;
				}
				else
					return scriptable->script->GetVariableType(varInfo);
			}
		}
		else // this is a ref variable, not a literal form - can't look up script vars
			return Script::eVarType_Invalid;
	}
	return GetDeclaredVariableType(varInfo->name.m_data, scrText, script);
}

/******************************
 Script
******************************/

class ScriptVarFinder
{
public:
	const char *m_varName;
	ScriptVarFinder(const char *varName) : m_varName(varName)
	{
	}
	bool Accept(VariableInfo *varInfo)
	{
		//_MESSAGE("  cur var: %s to match: %s", varInfo->name.m_data, m_varName);
		if (!StrCompare(m_varName, varInfo->name.m_data))
			return true;
		else
			return false;
	}
};

VariableInfo *Script::GetVariableByName(const char *varName)
{
	const auto *varIter = varList.Head();
	while (varIter) {
		auto* varInfo = varIter->data;
		if (varInfo && !StrCompare(varName, varInfo->name.m_data))
			return varInfo;

		varIter = varIter->next;
	}

	return NULL;
}

Script::RefVariable *Script::GetRefFromRefList(UInt32 refIdx)
{
	UInt32 idx = 1; // yes, really starts at 1
	if (refIdx)
	{
		auto *varIter = refList.Head();
		do
		{
			if (idx == refIdx)
				return varIter->data;
			idx++;
		} while (varIter = varIter->next);
	}
	return NULL;
}

VariableInfo *Script::GetVariableInfo(UInt32 idx)
{
	for (auto *entry = varList.Head(); entry; entry = entry->next)
		if (entry->data && entry->data->idx == idx)
			return entry->data;

	return NULL;
}

UInt32 Script::AddVariable(TESForm *form)
{
	auto *var = static_cast<RefVariable *>(FormHeap_Allocate(sizeof(RefVariable)));

	var->name.Set("");
	var->form = form;
	var->varIdx = 0;

	refList.Append(var);
	++info.numRefs;
	return info.numRefs;
}

void Script::CleanupVariables(void)
{
	refList.DeleteAll();
}

VariableInfo *Script::VarInfoList::GetVariableByName(const char *varName)
{
	return FindFirst([&](const VariableInfo *v) { return StrCompare(v->name.m_data, varName) == 0; });
}

#if RUNTIME
Script* Script::RefVariable::GetReferencedScript() const
{
	if (!form)
		return nullptr;
	if (IS_ID(form, TESQuest))
		return static_cast<TESQuest*>(form)->scriptable.script;
	if (auto* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR))
		if (auto* extraScript = refr->GetExtraScript())
			return extraScript->script;
	return nullptr;
}
#endif

UInt32 Script::RefList::GetIndex(Script::RefVariable *refVar)
{
	UInt32 idx = 0;
	for (auto cur = this->Begin(); !cur.End(); cur.Next())
	{
		idx++;
		if (*cur == refVar)
			return idx;
	}

	return 0;
}

/***********************************
 ScriptLineBuffer
***********************************/

//const char	* ScriptLineBuffer::kDelims_Whitespace = " \t\n\r";
//const char  * ScriptLineBuffer::kDelims_WhitespaceAndBrackets = " \t\n\r[]";

bool ScriptLineBuffer::Write(const void *buf, UInt32 bufsize)
{
	if (dataOffset + bufsize >= kBufferSize)
	{
#if NVSE_CORE
		g_ErrOut.Show("Line data buffer overflow! To fix this make the line shorter in length.");
#endif
		return false;
	}

	memcpy(dataBuf + dataOffset, buf, bufsize);
	dataOffset += bufsize;
	return true;
}

bool ScriptLineBuffer::Write32(UInt32 buf)
{
	return Write(&buf, sizeof(UInt32));
}

bool ScriptLineBuffer::WriteString(const char *buf)
{
	UInt32 len = StrLen(buf);
	if (len < 0x10000 && Write16(len) && len)
		return Write(buf, len);

	return true;
}

bool ScriptLineBuffer::Write16(UInt16 buf)
{
	return Write(&buf, sizeof(UInt16));
}

bool ScriptLineBuffer::WriteByte(UInt8 buf)
{
	return Write(&buf, sizeof(UInt8));
}

bool ScriptLineBuffer::WriteFloat(double buf)
{
	return Write(&buf, sizeof(double));
}
