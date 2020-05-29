#include "Commands_Script.h"
#include "ParamInfos.h"
#include "GameScript.h"
#include "ScriptUtils.h"

#if RUNTIME
#include "GameAPI.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "GameRTTI.h"
#include "GameData.h"

#include "EventManager.h"
#include "FunctionScripts.h"
//#include "ModTable.h"

enum EScriptMode {
	eScript_HasScript,
	eScript_Get,
	eScript_Remove,
};

static bool GetScript_Execute(COMMAND_ARGS, EScriptMode eMode)
{
	*result = 0;
	TESForm* form = NULL;

	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	bool parmForm = form ? true : false;

	form = form->TryGetREFRParent();
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESScriptableForm* scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	Script* script = NULL;
	EffectSetting* effect = NULL;
	if (!scriptForm)	// Let's try for a MGEF
	{
		effect = DYNAMIC_CAST(form, TESForm, EffectSetting);
		if (effect)
			script = effect->GetScript();
	} else
		script = (scriptForm) ? scriptForm->script : NULL;

	switch(eMode) {
		case eScript_HasScript: {
			*result = (script != NULL) ? 1 : 0;
			break;
		}
		case eScript_Get: {
			if (script) {
				UInt32* refResult = (UInt32*)result;
				*refResult = script->refID;
			}
			break;
		}
		case eScript_Remove: {
			// simply forget about the script
			if (script) {
				UInt32* refResult = (UInt32*)result;
				*refResult = script->refID;
			}
			if (scriptForm)
				scriptForm->script = NULL;
			else if (effect)
				effect->RemoveScript();
			if (!parmForm && thisObj) {
				// Remove ExtraScript entry - otherwise the script will keep running until the reference is reloaded.
				if (thisObj->extraDataList.HasType(kExtraData_Script))
					thisObj->extraDataList.RemoveByType(kExtraData_Script);
			}
			break;
		}
	}
	return true;
}

bool Cmd_IsScripted_Execute(COMMAND_ARGS)
{
	return GetScript_Execute(PASS_COMMAND_ARGS, eScript_HasScript);
}

bool Cmd_GetScript_Execute(COMMAND_ARGS)
{
	return GetScript_Execute(PASS_COMMAND_ARGS, eScript_Get);
}

bool Cmd_RemoveScript_Execute(COMMAND_ARGS)
{
	return GetScript_Execute(PASS_COMMAND_ARGS, eScript_Remove);
}

bool Cmd_SetScript_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;

	TESForm* form = NULL;
	TESForm* pForm = NULL;
	TESForm* scriptArg = NULL;

	ExtractArgsEx(EXTRACT_ARGS_EX, &scriptArg, &form);
	bool parmForm = form ? true : false;

	form = form->TryGetREFRParent();
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESScriptableForm* scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	Script* oldScript = NULL;
	EffectSetting* effect = NULL;
	if (!scriptForm)	// Let's try for a MGEF
	{
		effect = DYNAMIC_CAST(form, TESForm, EffectSetting);
		if (effect)
			oldScript = effect->GetScript();
		else
			return true;
	} else
		oldScript = scriptForm->script;

	Script* script = DYNAMIC_CAST(scriptArg, TESForm, Script);
	if (!script) return true;

	// we can only get a magic script here for an EffectSetting
	if (script->IsMagicScript() && !effect) return true;

	// we can't get an unknown script here
	if (script->IsUnkScript()) return true;

	if (script->IsMagicScript()) {
		effect->SetScript(script);
		// clean up event list here?
	}
	else
		if (effect)	// we need a magic script and some var won't be initialized
			return true;

	if (oldScript) {
		*refResult = oldScript->refID;
	}

	if ((script->IsQuestScript() && form->typeID == kFormType_Quest) || script->IsObjectScript()) {
		scriptForm->script = script;
		// clean up event list here?
		// This is necessary in order to make sure the script uses the correct questDelayTime.
		script->quest = DYNAMIC_CAST(form, TESForm, TESQuest);
	}
	if (script->IsObjectScript() && !parmForm && thisObj) {
		// Re-building ExtraScript entry - the new script then starts running immediately (instead of only on reload).
		if (thisObj->extraDataList.HasType(kExtraData_Script)) thisObj->extraDataList.RemoveByType(kExtraData_Script);
		thisObj->extraDataList.Add(ExtraScript::Create(form, true));

		// Commented out until solved

		//if (thisObj->extraDataList.Add(ExtraScript::Create(form, true))) {
		//	ExtraScript* xScript = (ExtraScript*)thisObj->extraDataList.GetByType(kExtraData_Script);
		//	DoCheckScriptRunnerAndRun(thisObj, &(thisObj->extraDataList));
		//	//MarkBaseExtraListScriptEvent(thisObj, &(thisObj->extraDataList), ScriptEventList::kEvent_OnLoad); 
		//	if (xScript) {
		//		xScript->EventCreate(ScriptEventList::kEvent_OnLoad, NULL);
		//	}
		//}
	}
	return true;
}

bool Cmd_IsFormValid_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}
		if (pForm) {
			*result = 1;
		}
		if (IsConsoleMode())
			Console_Print(*result == 1.0 ? "Valid Form!" : "Invalid Form");
	}
	return true;
}

bool Cmd_IsReference_Execute(COMMAND_ARGS)
{
	TESObjectREFR* refr = NULL;
	*result = 0;
	if (ExtractArgs(EXTRACT_ARGS, &refr))
		*result = 1;
	if (IsConsoleMode())
		Console_Print(*result == 1.0 ? "IsReference" : "Not reference!");

	return true;
}

static enum {
	eScriptVar_Get = 1,
	eScriptVar_GetRef,
	eScriptVar_Has,
};

bool GetVariable_Execute(COMMAND_ARGS, UInt32 whichAction)
{
	char varName[256] = { 0 };
	TESQuest* quest = NULL;
	Script* targetScript = NULL;
	ScriptEventList* targetEventList = NULL;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &quest))
		return false;
	if (quest)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo* varInfo = targetScript->GetVariableByName(varName);
		if (whichAction == eScriptVar_Has)
			return varInfo ? true : false;
		else if (varInfo)
		{
			ScriptEventList::Var* var = targetEventList->GetVariable(varInfo->idx);
			if (var)
			{
				if (whichAction == eScriptVar_Get)
					*result = var->data;
				else if (whichAction == eScriptVar_GetRef)
				{
					UInt32* refResult = (UInt32*)result;
					*refResult = (*(UInt64*)&var->data);
				}
				return true;
			}
		}
	}

	return false;
}

bool Cmd_SetVariable_Execute(COMMAND_ARGS)
{
	char varName[256] = { 0 };
	TESQuest* quest = NULL;
	Script* targetScript = NULL;
	ScriptEventList* targetEventList = NULL;
	float value = 0;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &value, &quest))
		return true;
	if (quest)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo* varInfo = targetScript->GetVariableByName(varName);
		if (varInfo)
		{
			ScriptEventList::Var* var = targetEventList->GetVariable(varInfo->idx);
			if (var)
			{
				var->data = value;
				return true;
			}
		}
	}

	return false;
}

bool Cmd_SetRefVariable_Execute(COMMAND_ARGS)
{
	char varName[256] = { 0 };
	TESQuest* quest = NULL;
	Script* targetScript = NULL;
	ScriptEventList* targetEventList = NULL;
	TESForm * value = NULL;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &value, &quest))
		return true;
	if (quest)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo* varInfo = targetScript->GetVariableByName(varName);
		if (varInfo)
		{
			ScriptEventList::Var* var = targetEventList->GetVariable(varInfo->idx);
			if (var)
			{
				UInt32* refResult = (UInt32*)result;
				(*(UInt64*)&var->data) = value ? value->refID : 0 ;
				return true;
			}
		}
	}

	return false;
}

bool Cmd_HasVariable_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_Has))
		*result = 1;

	return true;
}

bool Cmd_GetVariable_Execute(COMMAND_ARGS)
{
	GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_Get);
	return true;
}

bool Cmd_GetRefVariable_Execute(COMMAND_ARGS)
{
	GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_GetRef);
	return true;
}

bool Cmd_GetArrayVariable_Execute(COMMAND_ARGS)
{
	if (!ExpressionEvaluator::Active()) {
		ShowRuntimeError(scriptObj, "GetArrayVariable must be called within the context of an NVSE expression");
		return false;
	}

	GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_Get);
	return true;
}

bool Cmd_CompareScripts_Execute(COMMAND_ARGS)
{
	Script* script1 = NULL;
	Script* script2 = NULL;
	*result = 0;

	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &script1, &script2))
		return true;
	script1 = DYNAMIC_CAST(script1, TESForm, Script);
	script2 = DYNAMIC_CAST(script2, TESForm, Script);

	if (script1 && script2 && script1->info.dataLength == script2->info.dataLength)
	{
		if (script1 == script2)
			*result = 1;
		else if (!memcmp(script1->data, script2->data, script1->info.dataLength))
			*result = 1;
	}

	return true;
}

bool Cmd_ResetAllVariables_Execute(COMMAND_ARGS)
{
	//sets all vars to 0
	*result = 0;

	ScriptEventList* list = eventList;			//reset calling script by default
	if (thisObj)								//call on a reference to reset that ref's script vars
		list = thisObj->GetEventList();

	if (list)
		*result = list->ResetAllVariables();

	return true;
}

class ExplicitRefFinder
{
public:
	bool Accept(const Script::RefVariable* var)
	{
		if (var && var->varIdx == 0)
			return true;

		return false;
	}
};

Script* GetScriptArg(TESObjectREFR* thisObj, TESForm* form)
{
	Script* targetScript = NULL;
	if (form)
		targetScript = DYNAMIC_CAST(form, TESForm, Script);
	else if (thisObj)
	{
		TESScriptableForm* scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
			targetScript = scriptable->script;
	}

	return targetScript;
}

bool Cmd_GetNumExplicitRefs_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	Script* targetScript = NULL;
	*result = 0;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		targetScript = GetScriptArg(thisObj, form);
		if (targetScript)
			*result = (Script::RefListVisitor(&targetScript->refList).CountIf(ExplicitRefFinder()));
	}

	if (IsConsoleMode())
		Console_Print("GetNumExplicitRefs >> %.0f", *result);

	return true;
}

bool Cmd_GetNthExplicitRef_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	UInt32 refIdx = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &refIdx, &form))
	{
		Script* targetScript = GetScriptArg(thisObj, form);
		if (targetScript)
		{
			Script::RefListVisitor visitor(&targetScript->refList);
			UInt32 count = 0;
			const Script::RefListEntry* entry = NULL;
			while (count <= refIdx)
			{
				entry = visitor.Find(ExplicitRefFinder(), entry);
				if (!entry)
					break;

				count++;
			}

			if (count == refIdx + 1 && entry && entry->var && entry->var->form)
			{
				*refResult = entry->var->form->refID;
				if (IsConsoleMode())
					Console_Print("GetNthExplicitRef >> %s (%08x)", GetFullName(entry->var->form), *refResult);
			}
		}
	}

	return true;
}

bool Cmd_RunScript_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{

		form = form->TryGetREFRParent();
		if (!form) {
			if (!thisObj) return true;
			form = thisObj->baseForm;
		}

		TESScriptableForm* scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
		Script* script = NULL;
		EffectSetting* effect = NULL;
		if (!scriptForm)	// Let's try for a MGEF
		{
			effect = DYNAMIC_CAST(form, TESForm, EffectSetting);
			if (effect)
				script = effect->GetScript();
			else
			{
				script = DYNAMIC_CAST(form, TESForm, Script);
			}
		} else
			script = scriptForm->script;

		if (script)
		{
			bool runResult = CALL_MEMBER_FN(script, Execute)(thisObj, 0, 0, 0);
			Console_Print("ran script, returned %s",runResult ? "true" : "false");
		}
	}

	return true;
}

bool Cmd_GetCurrentScript_Execute(COMMAND_ARGS)
{
	// apparently this is useful
	UInt32* refResult = (UInt32*)result;
	*refResult = scriptObj->refID;
	return true;
}

bool Cmd_GetCallingScript_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	Script* caller = UserFunctionManager::GetInvokingScript(scriptObj);
	if (caller) {
		*refResult = caller->refID;
	}

	return true;
}

bool ExtractEventCallback(ExpressionEvaluator& eval, EventManager::EventCallback* outCallback, char* outName)
{
	if (eval.ExtractArgs() && eval.NumArgs() >= 2) {
		const char* eventName = eval.Arg(0)->GetString();
		Script* script = DYNAMIC_CAST(eval.Arg(1)->GetTESForm(), TESForm, Script);
		if (eventName && script) {
			strcpy_s(outName, 0x20, eventName);
			TESForm* sourceFilter = NULL;
			TESForm* targetFilter = NULL;
			TESObjectREFR* thisObjFilter = NULL;

			// any filters?
			for (UInt32 i = 2; i < eval.NumArgs(); i++) {
				const TokenPair* pair = eval.Arg(i)->GetPair();
				if (pair && pair->left && pair->right) {
					const char* key = pair->left->GetString();
					if (key) {
						if (!_stricmp(key, "ref") || !_stricmp(key, "first")) {
							sourceFilter = /* DYNAMIC_CAST( */ pair->right->GetTESForm() /* , TESForm, TESObjectREFR) */ ;
						}
						else if (!_stricmp(key, "object") || !_stricmp(key, "second")) {
							//// special-case MGEF
							//if (!_stricmp(eventName, "onmagiceffecthit")) {
							//	const char* effStr = pair->right->GetString();
							//	if (effStr && strlen(effStr) == 4) {
							//		targetFilter = EffectSetting::EffectSettingForC(*((UInt32*)effStr));
							//	}
							//}
							//else
							//if (!_stricmp(eventName, "onhealthdamage")) {
							//	// special-case OnHealthDamage - don't filter by damage, do filter by damaged actor
							//	thisObjFilter = /* DYNAMIC_CAST( */ pair->right->GetTESForm() /* , TESForm, TESObjectREFR) */ ;
							//}
							//else
							{
								targetFilter = pair->right->GetTESForm();
							}
						}
					}
				}
			}

			*outCallback = EventManager::EventCallback(script, sourceFilter, targetFilter, thisObjFilter);
			return true;
		}
	}

	return false;
}

bool Cmd_SetEventHandler_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	EventManager::EventCallback callback(NULL);
	char eventName[0x20];
	if (ExtractEventCallback(eval, &callback, eventName)) {
		if (EventManager::SetHandler(eventName, callback))
			*result = 1.0;
	}

	return true;
}

bool Cmd_RemoveEventHandler_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	EventManager::EventCallback callback(NULL);
	char eventName[0x20];
	if (ExtractEventCallback(eval, &callback, eventName)) {
		if (EventManager::RemoveHandler(eventName, callback))
			*result = 1.0;
	}

	return true;
}

bool Cmd_GetCurrentEventName_Execute(COMMAND_ARGS)
{
	std::string eventStr = EventManager::GetCurrentEventName();
	const char* eventName = eventStr.c_str();

	AssignToStringVar(PASS_COMMAND_ARGS, eventName);
	return true;
}

bool Cmd_DispatchEvent_Execute (COMMAND_ARGS)
{
	ExpressionEvaluator eval (PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs () || eval.NumArgs() == 0)
		return true;

	const char* eventName = eval.Arg(0)->GetString ();
	if (!eventName)
		return true;

	ArrayID argsArrayId = 0;
	const char* senderName = NULL;
	if (eval.NumArgs() > 1)
	{
		if (!eval.Arg(1)->CanConvertTo (kTokenType_Array))
			return true;
		argsArrayId = eval.Arg(1)->GetArray ();

		if (eval.NumArgs() > 2)
			senderName = eval.Arg(2)->GetString ();
	}

	*result = EventManager::DispatchUserDefinedEvent (eventName, scriptObj, argsArrayId, senderName) ? 1.0 : 0.0;
	return true;
}

#endif
