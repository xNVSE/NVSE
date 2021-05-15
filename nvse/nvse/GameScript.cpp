#include "GameAPI.h"
#include "GameScript.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "CommandTable.h"
#include "GameRTTI.h"

UInt32 GetDeclaredVariableType(const char* varName, const char* scriptText)
{
	Tokenizer scriptLines(scriptText, "\n\r");
	std::string curLine;
	while (scriptLines.NextToken(curLine) != -1)
	{
		Tokenizer tokens(curLine.c_str(), " \t\n\r;");
		std::string curToken;

		if (tokens.NextToken(curToken) != -1)
		{
			UInt32 varType = -1;

			// variable declaration?
			if (!StrCompare(curToken.c_str(), "string_var"))
				varType = Script::eVarType_String;
			else if (!StrCompare(curToken.c_str(), "array_var"))
				varType = Script::eVarType_Array;
			else if (!StrCompare(curToken.c_str(), "float"))
				varType = Script::eVarType_Float;
			else if (!StrCompare(curToken.c_str(), "long") || !StrCompare(curToken.c_str(), "int") || !StrCompare(curToken.c_str(), "short"))
				varType = Script::eVarType_Integer;
			else if (!StrCompare(curToken.c_str(), "ref") || !StrCompare(curToken.c_str(), "reference"))
				varType = Script::eVarType_Ref;

			if (varType != -1 && tokens.NextToken(curToken) != -1 && !StrCompare(curToken.c_str(), varName))
			{
				return varType;
			}
		}
	}

	return Script::eVarType_Invalid;
}

Script* GetScriptFromForm(TESForm* form)
{
	TESObjectREFR* refr =  DYNAMIC_CAST(form, TESForm, TESObjectREFR);
	if (refr)
		form = refr->baseForm;

	TESScriptableForm* scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	return scriptable ? scriptable->script : NULL;
}

UInt32 Script::GetVariableType(VariableInfo* varInfo)
{
	if (text)
		return GetDeclaredVariableType(varInfo->name.m_data, text);
	else
	{
		// if it's a ref var a matching varIdx will appear in RefList
		for (RefListEntry* refEntry = &refList; refEntry; refEntry = refEntry->next)
		{
			if (refEntry->var->varIdx == varInfo->idx)
				return eVarType_Ref;
		}

		return varInfo->type;
	}
}

bool Script::IsUserDefinedFunction() const
{
	auto* scriptData = static_cast<UInt8*>(data);
	return *(scriptData + 8) == 0x0D;
}

UInt32 Script::GetVarCount() const
{
	// info->varCount include index
	auto count = 0U;
	auto* node = &this->varList;
	while (node)
	{
		if (node->data)
			++count;
		node = node->Next();
	}
	return count;
}

UInt32 Script::GetRefCount() const
{
	auto count = 0U;
	auto* node = &this->refList;
	while (node)
	{
		if (node->var)
			++count;
		node = node->Next();
	}
	return count;
}

tList<VariableInfo>* Script::GetVars()
{
	return reinterpret_cast<tList<VariableInfo>*>(&this->varList);
}

tList<Script::RefVariable>* Script::GetRefList()
{
	return reinterpret_cast<tList<RefVariable>*>(&this->refList);
}

#if RUNTIME

void Script::RefVariable::Resolve(ScriptEventList * eventList)
{
	if(varIdx && eventList)
	{
		ScriptEventList::Var	* var = eventList->GetVariable(varIdx);
		if(var)
		{
			UInt32	refID = *((UInt32 *)&var->data);
			form = LookupFormByID(refID);
		}
	}
}

CRITICAL_SECTION	csGameScript;				// trying to avoid what looks like concurrency issues

ScriptEventList* Script::CreateEventList(void)
{
	ScriptEventList* result = NULL;
	::EnterCriticalSection(&csGameScript);
	try
	{
		result = (ScriptEventList*)ThisStdCall(0x005ABF60, this);	// 4th sub above Script::Execute (was 1st above in Oblivion) Execute is the second to last call in Run
	} catch(...) {}

	::LeaveCriticalSection(&csGameScript);
	return result;
}

bool Script::RunScriptLine2(const char * text, TESObjectREFR* object, bool bSuppressOutput)
{
	//ToggleConsoleOutput(!bSuppressOutput);

	ConsoleManager	* consoleManager = ConsoleManager::GetSingleton();

	UInt8	scriptBuf[sizeof(Script)];
	Script	* script = (Script *)scriptBuf;

	CALL_MEMBER_FN(script, Constructor)();
	CALL_MEMBER_FN(script, MarkAsTemporary)();
	CALL_MEMBER_FN(script, SetText)(text);
	bool bResult = CALL_MEMBER_FN(script, Run)(consoleManager->scriptContext, true, object);
	CALL_MEMBER_FN(script, Destructor)();

	//ToggleConsoleOutput(true);
	return bResult;
}

bool Script::RunScriptLine(const char * text, TESObjectREFR * object)
{
	return RunScriptLine2(text, object, false);
}

#endif

Script::RefVariable* ScriptBuffer::ResolveRef(const char* refName)
{
	Script::RefVariable* newRef = NULL;
	Script::RefListEntry* listEnd = &refVars;

	// see if it's already in the refList
	for (Script::RefListEntry* cur = &refVars; cur; cur = cur->next)
	{
		listEnd = cur;
		if (cur->var && cur->var->name.m_data && !_stricmp(cur->var->name.m_data, refName))
			return cur->var;
	}

	// not in list

	// is it a local ref variable?
	VariableInfo* varInfo = vars.GetVariableByName(refName);
	if (varInfo && GetVariableType(varInfo, NULL) == Script::eVarType_Ref)
	{
		newRef = (Script::RefVariable*)FormHeap_Allocate(sizeof(Script::RefVariable));
		newRef->form = NULL;
	}
	else		// is it a form or global?
	{
		TESForm* form;
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
			TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
			if (refr && !refr->IsPersistent())		// only persistent refs can be used in scripts
				return NULL;

			newRef = (Script::RefVariable*)FormHeap_Allocate(sizeof(Script::RefVariable));
			memset(newRef, 0, sizeof(Script::RefVariable));
			newRef->form = form;
		}
	}

	if (newRef)		// got it, add to refList
	{
		newRef->name.Set(refName);
		newRef->varIdx = 0;
		if (!refVars.var)
			refVars.var = newRef;
		else
		{
			Script::RefListEntry* entry = (Script::RefListEntry*)FormHeap_Allocate(sizeof(Script::RefListEntry));
			entry->var = newRef;
			entry->next = NULL;
			listEnd->next = entry;
		}

		numRefs++;
		return newRef;
	}
	else
		return NULL;
}

UInt32 ScriptBuffer::GetRefIdx(Script::RefVariable* ref)
{
	UInt32 idx = 0;
	for (Script::RefListEntry* curEntry = &refVars; curEntry && curEntry->var; curEntry = curEntry->next)
	{
		idx++;
		if (ref == curEntry->var)
			break;
	}

	return idx;
}

UInt32 ScriptBuffer::GetVariableType(VariableInfo* varInfo, Script::RefVariable* refVar)
{
	const char* scrText = scriptText;
	if (refVar)
	{
		if (refVar->form)
		{
			TESScriptableForm* scriptable = NULL;
			switch (refVar->form->typeID)
			{
			case kFormType_TESObjectREFR:
				{
					TESObjectREFR* refr = DYNAMIC_CAST(refVar->form, TESForm, TESObjectREFR);
					scriptable = DYNAMIC_CAST(refr->baseForm, TESForm, TESScriptableForm);
					break;
				}
			case kFormType_TESQuest:
				scriptable = DYNAMIC_CAST(refVar->form, TESForm, TESScriptableForm);
			}

			if (scriptable && scriptable->script)
			{
				if (scriptable->script->text)
					scrText = scriptable->script->text;
				else
					return scriptable->script->GetVariableType(varInfo);
			}
		}
		else			// this is a ref variable, not a literal form - can't look up script vars
			return Script::eVarType_Invalid;
	}

	return GetDeclaredVariableType(varInfo->name.m_data, scrText);
}

/******************************
 Script
******************************/

class ScriptVarFinder
{
public:
	const char* m_varName;
	ScriptVarFinder(const char* varName) : m_varName(varName)
		{	}
	bool Accept(VariableInfo* varInfo)
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
	VarInfoEntry *varIter = &varList;
	VariableInfo *varInfo;
	do
	{
		varInfo = varIter->data;
		if (varInfo && !StrCompare(varName, varInfo->name.m_data))
			return varInfo;
	}
	while (varIter = varIter->next);
	return NULL;
}

Script::RefVariable	* Script::GetRefFromRefList(UInt32 refIdx)
{
	UInt32 idx = 1;	// yes, really starts at 1
	if (refIdx)
	{
		RefListEntry *varIter = &refList;
		do
		{
			if (idx == refIdx)
				return varIter->var;
			idx++;
		}
		while (varIter = varIter->next);
	}
	return NULL;
}

VariableInfo* Script::GetVariableInfo(UInt32 idx)
{
	for (Script::VarInfoEntry* entry = &varList; entry; entry = entry->next)
		if (entry->data && entry->data->idx == idx)
			return entry->data;

	return NULL;
}

UInt32 Script::AddVariable(TESForm * form)
{
	UInt32		resultIdx = 1;

	RefVariable	* var = (RefVariable*)FormHeap_Allocate(sizeof(RefVariable));

	var->name.Set("");
	var->form = form;
	var->varIdx = 0;

	if(!refList.var)
	{
		// adding the first object
		refList.var = var;
		refList.next = NULL;
	}
	else
	{
		resultIdx++;

		// find the last RefListEntry
		RefListEntry	* entry;
		for(entry = &refList; entry->next; entry = entry->next, resultIdx++) ;

		RefListEntry	* newEntry = (RefListEntry *)FormHeap_Allocate(sizeof(RefListEntry));

		newEntry->var = var;
		newEntry->next = NULL;

		entry->next = newEntry;
	}

	info.numRefs = resultIdx + 1;

	return resultIdx;
}

void Script::CleanupVariables(void)
{
	delete refList.var;

	RefListEntry	* entry = refList.next;
	while(entry)
	{
		RefListEntry	* next = entry->next;

		delete entry->var;
		delete entry;

		entry = next;
	}
}

VariableInfo* Script::VarInfoEntry::GetVariableByName(const char* varName)
{
	for (Script::VarInfoEntry* entry = this; entry; entry = entry->next)
	{
		if (entry->data && !StrCompare(entry->data->name.m_data, varName))
			return entry->data;
	}

	return NULL;
}

UInt32 Script::RefListEntry::GetIndex(Script::RefVariable* refVar)
{
	UInt32 idx = 0;
	for (RefListEntry* cur = this; cur; cur = cur->next)
	{
		idx++;
		if (cur->var == refVar)
			return idx;
	}

	return 0;
}

/***********************************
 ScriptLineBuffer
***********************************/

//const char	* ScriptLineBuffer::kDelims_Whitespace = " \t\n\r";
//const char  * ScriptLineBuffer::kDelims_WhitespaceAndBrackets = " \t\n\r[]";

bool ScriptLineBuffer::Write(const void* buf, UInt32 bufsize)
{
	if (dataOffset + bufsize >= kBufferSize)
		return false;

	memcpy(dataBuf + dataOffset, buf, bufsize);
	dataOffset += bufsize;
	return true;
}

bool ScriptLineBuffer::Write32(UInt32 buf)
{
	return Write(&buf, sizeof(UInt32));
}

bool ScriptLineBuffer::WriteString(const char* buf)
{
	UInt32 len = StrLen(buf);
	if ((len < 0x10000) && Write16(len) && len)
		return Write(buf, len);

	return false;
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



