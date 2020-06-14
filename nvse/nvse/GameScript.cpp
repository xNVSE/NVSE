#include "nvse/GameScript.h"

CRITICAL_SECTION	csGameScript;				// trying to avoid what looks like concurrency issues

UInt32 GetDeclaredVariableType(const char* varName, const char* scriptText)
{
	Tokenizer scriptLines(scriptText, "\n\r");
	std::string curLine;
	while (scriptLines.NextToken(curLine) != -1)
	{
		Tokenizer tokens(curLine.c_str(), " \t\n\r");
		std::string curToken;

		if (tokens.NextToken(curToken) != -1)
		{
			UInt32 varType = -1;

			// variable declaration?
			if (!_stricmp(curToken.c_str(), "string_var"))
				varType = Script::eVarType_String;
			else if (!_stricmp(curToken.c_str(), "array_var"))
				varType = Script::eVarType_Array;
			else if (!_stricmp(curToken.c_str(), "float"))
				varType = Script::eVarType_Float;
			else if (!_stricmp(curToken.c_str(), "long") || !_stricmp(curToken.c_str(), "int") || !_stricmp(curToken.c_str(), "short"))
				varType = Script::eVarType_Integer;
			else if (!_stricmp(curToken.c_str(), "ref") || !_stricmp(curToken.c_str(), "reference"))
				varType = Script::eVarType_Ref;

			if (varType != -1 && tokens.NextToken(curToken) != -1 && !_stricmp(curToken.c_str(), varName))
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
		ListNode<RefVariable>* varIter = refList.Head();
		RefVariable* refVar;
		do
		{
			refVar = varIter->data;
			if (refVar && (refVar->varIdx == varInfo->idx))
				return eVarType_Ref;
		} while (varIter = varIter->next);
		return varInfo->type;
	}
}

#if RUNTIME

void Script::RefVariable::Resolve(ScriptEventList *eventList)
{
	if (varIdx && eventList)
	{
		ScriptVar *var = eventList->GetVariable(varIdx);
		if (var) form = LookupFormByID(*(UInt32*)&var->data);
	}
}

ScriptEventList* Script::CreateEventList(void)
{
	return (ScriptEventList*)ThisStdCall(0x005ABF60, this);	// 4th sub above Script::Execute (was 1st above in Oblivion) Execute is the second to last call in Run
}

Script::RefVariable* ScriptBuffer::ResolveRef(const char* refName)
{
	// ###TODO: Handle player, ref vars, quests, globals
	return NULL;
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

#else		// CS-stuff below

Script::RefVariable* ScriptBuffer::ResolveRef(const char* refName)
{
	Script::RefVariable* newRef = NULL;
	Script::RefListEntry* listEnd = &refVars;

	// see if it's already in the refList
	for (Script::RefListEntry* cur = &refVars; cur; cur = cur->next)
	{
		listEnd = cur;
		if (cur->var && !_stricmp(cur->var->name.m_data, refName))
			return cur->var;
	}

	// not in list

	// is it a local ref variable?
	VariableInfo* varInfo = vars.GetVariableInfo(refName);
	if (varInfo && GetVariableType(varInfo, NULL) == Script::eVarType_Ref)
	{
		newRef = (Script::RefVariable*)FormHeap_Allocate(sizeof(Script::RefVariable));
		newRef->form = NULL;
	}
	else		// is it a form or global?
	{
		TESForm* form = GetFormByID(refName);
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

#endif

UInt32 ScriptBuffer::GetRefIdx(Script::RefVariable *refVar)
{
	return refVars.GetIndex(refVar);
}

UInt32 ScriptBuffer::GetVariableType(VariableInfo* varInfo, Script::RefVariable* refVar) const
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
	const char	*m_varName;

public:
	ScriptVarFinder(const char *varName) : m_varName(varName) {}

	bool Accept(VariableInfo *varInfo)
	{
		return StrEqualCI(varInfo->name.m_data, m_varName);
	}
};



Script::RefVariable	*Script::GetRefVariable(UInt32 reqIdx) const
{
	UInt32 idx = 1;	// yes, really starts at 1
	if (reqIdx)
	{
		ListNode<RefVariable> *varIter = refList.Head();
		do
		{
			if (idx == reqIdx)
				return varIter->data;
			idx++;
		}
		while (varIter = varIter->next);
	}
	return nullptr;
}

VariableInfo *Script::GetVariableInfo(UInt32 idx) const
{
	auto filter = [idx](auto* varInfo)
	{
		return varInfo->idx == idx;
	};	
	return varList.FindWhere(filter);
}

VariableInfo* Script::GetVariableInfo(const char* varName) const
{
	const auto filter = [varName](VariableInfo* varInfo)
	{
		return _stricmp(varInfo->name.CStr(), varName) == 0;
	};
	return varList.FindWhere(filter);
}

UInt32 Script::AddVariable(TESForm *form)
{
	RefVariable	*refVar = (RefVariable*)GameHeapAlloc(sizeof(RefVariable));
	refVar->name.Set("");
	refVar->form = form;
	refVar->varIdx = 0;

	UInt32 resultIdx = refList.Append(refVar) + 1;
	info.numRefs = resultIdx + 1;
	return resultIdx;
}

void Script::CleanupVariables()
{
	refList.RemoveAll();
}

UInt32 Script::RefVarList::GetIndex(RefVariable *refVar)
{
	UInt32 idx = 0;
	ListNode<RefVariable> *varIter = Head();
	do
	{
		idx++;
		if (varIter->data == refVar)
			return idx;
	}
	while (varIter = varIter->next);
	return 0;
}

VariableInfo* Script::VarInfoList::GetVariableByName(const char* name) const
{
	return FindWhere([name](VariableInfo* info) {return _stricmp(info->name.CStr(), name) == 0;});
}

/***********************************
 ScriptLineBuffer
***********************************/

//const char	* ScriptLineBuffer::kDelims_Whitespace = " \t\n\r";
//const char  * ScriptLineBuffer::kDelims_WhitespaceAndBrackets = " \t\n\r[]";

bool ScriptLineBuffer::Write(const void *buf, UInt32 bufsize)
{
	if ((dataOffset + bufsize) >= kBufferSize) return false;
	MemCopy(dataBuf + dataOffset, buf, bufsize);
	dataOffset += bufsize;
	return true;
}

bool ScriptLineBuffer::Write32(UInt32 buf)
{
	if ((dataOffset + 4) >= kBufferSize) return false;
	*(UInt32*)(dataBuf + dataOffset) = buf;
	dataOffset += 4;
	return true;
}

bool ScriptLineBuffer::WriteString(const char *buf)
{
	UInt32 len = StrLen(buf);
	if ((dataOffset + 2 + len) >= kBufferSize) return false;
	UInt8 *dataPtr = dataBuf + dataOffset;
	*(UInt16*)dataPtr = len;
	MemCopy(dataPtr + 2, buf, len);
	dataOffset += 2 + len;
	return true;
}

bool ScriptLineBuffer::Write16(UInt16 buf)
{
	if ((dataOffset + 2) >= kBufferSize) return false;
	*(UInt16*)(dataBuf + dataOffset) = buf;
	dataOffset += 2;
	return true;
}

bool ScriptLineBuffer::WriteByte(UInt8 buf)
{
	if ((dataOffset + 1) >= kBufferSize) return false;
	*(dataBuf + dataOffset) = buf;
	dataOffset++;
	return true;
}

bool ScriptLineBuffer::WriteFloat(double buf)
{
	if ((dataOffset + 8) >= kBufferSize) return false;
	MemCopy(dataBuf + dataOffset, &buf, 8);
	dataOffset += 8;
	return true;
}

