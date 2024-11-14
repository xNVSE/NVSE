#include "Commands_Script.h"

#include <filesystem>

#include "ParamInfos.h"
#include "GameScript.h"
#include "MemoizedMap.h"
#include "ScriptUtils.h"
#include "PluginManager.h"
#include "ScriptAnalyzer.h"

#if RUNTIME
#include "GameAPI.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "GameRTTI.h"
#include "GameData.h"

#include "EventManager.h"
#include "FunctionScripts.h"
#include <fstream>
#include "CachedScripts.h"
//#include "ModTable.h"

enum EScriptMode
{
	eScript_HasScript,
	eScript_Get,
	eScript_Remove,
};

static bool GetScript_Execute(COMMAND_ARGS, EScriptMode eMode)
{
	*result = 0;
	TESForm *form = nullptr;

	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	const bool parmForm = form ? true : false;

	form = form->TryGetREFRParent();
	if (!form)
	{
		if (!thisObj)
			return true;
		form = thisObj->baseForm;
	}

	const auto scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	Script *script = nullptr;
	EffectSetting *effect = nullptr;
	if (!scriptForm) // Let's try for a MGEF
	{
		effect = DYNAMIC_CAST(form, TESForm, EffectSetting);
		if (effect)
			script = effect->GetScript();
	}
	else
		script = (scriptForm) ? scriptForm->script : nullptr;

	switch (eMode)
	{
	case eScript_HasScript:
	{
		*result = (script != nullptr) ? 1 : 0;
		break;
	}
	case eScript_Get:
	{
		if (script)
		{
			const auto refResult = (UInt32 *)result;
			*refResult = script->refID;
		}
		break;
	}
	case eScript_Remove:
	{
		// simply forget about the script
		if (script)
		{
			const auto refResult = (UInt32 *)result;
			*refResult = script->refID;
		}
		if (scriptForm)
			scriptForm->script = nullptr;
		else if (effect)
			effect->RemoveScript();
		if (!parmForm && thisObj)
		{
			// Remove ExtraScript entry - otherwise the script will keep running until the reference is reloaded.
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
	const auto refResult = (UInt32 *)result;

	TESForm *form = nullptr;
	TESForm *pForm = nullptr;
	TESForm *scriptArg = nullptr;

	ExtractArgsEx(EXTRACT_ARGS_EX, &scriptArg, &form);
	const bool parmForm = form ? true : false;

	form = form->TryGetREFRParent();
	if (!form)
	{
		if (!thisObj)
			return true;
		form = thisObj->baseForm;
	}

	const auto scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	Script *oldScript = nullptr;
	EffectSetting *effect = nullptr;
	if (!scriptForm) // Let's try for a MGEF
	{
		effect = DYNAMIC_CAST(form, TESForm, EffectSetting);
		if (effect)
			oldScript = effect->GetScript();
		else
			return true;
	}
	else
		oldScript = scriptForm->script;

	const auto script = DYNAMIC_CAST(scriptArg, TESForm, Script);
	if (!script)
		return true;

	// we can only get a magic script here for an EffectSetting
	if (script->IsMagicScript() && !effect)
		return true;

	// we can't get an unknown script here
	if (script->IsUnkScript())
		return true;

	if (script->IsMagicScript())
	{
		effect->SetScript(script);
		// clean up event list here?
	}
	else if (effect) // we need a magic script and some var won't be initialized
		return true;

	if (oldScript)
	{
		*refResult = oldScript->refID;
	}

	if ((script->IsQuestScript() && form->typeID == kFormType_TESQuest) || script->IsObjectScript())
	{
		scriptForm->script = script;
		// clean up event list here?
		// This is necessary in order to make sure the script uses the correct questDelayTime.
		script->quest = DYNAMIC_CAST(form, TESForm, TESQuest);
	}
	if (script->IsObjectScript() && !parmForm && thisObj)
	{
		// Re-building ExtraScript entry - the new script then starts running immediately (instead of only on reload).
		thisObj->extraDataList.RemoveByType(kExtraData_Script);
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
	TESForm *pForm = nullptr;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pForm))
	{
		pForm = pForm->TryGetREFRParent();
		if (!pForm)
		{
			if (!thisObj)
				return true;
			pForm = thisObj->baseForm;
		}
		if (pForm)
		{
			*result = 1;
		}
		if (IsConsoleMode())
			Console_Print(*result == 1.0 ? "Valid Form!" : "Invalid Form");
	}
	return true;
}

bool Cmd_IsReference_Execute(COMMAND_ARGS)
{
	TESObjectREFR *refr = nullptr;
	*result = 0;
	if (ExtractArgs(EXTRACT_ARGS, &refr))
		*result = 1;
	if (IsConsoleMode())
		Console_Print(*result == 1.0 ? "IsReference" : "Not reference!");

	return true;
}

enum
{
	eScriptVar_Get = 1,
	eScriptVar_GetRef,
	eScriptVar_Has,
};

bool GetVariable_Execute(COMMAND_ARGS, UInt32 whichAction)
{
	char varName[256];
	TESQuest *quest = nullptr;
	Script *targetScript = nullptr;
	ScriptEventList *targetEventList = nullptr;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &quest))
		return true;
	if (quest)
	{
		const auto scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		const auto scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo *varInfo = targetScript->GetVariableByName(varName);
		if (varInfo)
		{
			if (whichAction == eScriptVar_Has)
				*result = 1;
			else
			{
				ScriptLocal *var = targetEventList->GetVariable(varInfo->idx);
				if (var)
				{
					if (whichAction == eScriptVar_Get)
						*result = var->data;
					else if (whichAction == eScriptVar_GetRef)
					{
						const auto refResult = (UInt32 *)result;
						*refResult = (*(UInt64 *)&var->data);
					}
				}
			}
		}
	}

	return true;
}

bool Cmd_SetVariable_Execute(COMMAND_ARGS)
{
	char varName[256];
	TESQuest *quest = nullptr;
	Script *targetScript = nullptr;
	ScriptEventList *targetEventList = nullptr;
	float value = 0;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &value, &quest))
		return true;
	if (quest)
	{
		const auto scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		const auto scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo *varInfo = targetScript->GetVariableByName(varName);
		if (varInfo)
		{
			ScriptLocal *var = targetEventList->GetVariable(varInfo->idx);
			if (var)
				var->data = value;
		}
	}

	return true;
}

bool Cmd_SetRefVariable_Execute(COMMAND_ARGS)
{
	char varName[256];
	TESQuest *quest = nullptr;
	Script *targetScript = nullptr;
	ScriptEventList *targetEventList = nullptr;
	TESForm *value = nullptr;
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &varName, &value, &quest))
		return true;
	if (quest)
	{
		const auto scriptable = DYNAMIC_CAST(quest, TESQuest, TESScriptableForm);
		targetScript = scriptable->script;
		targetEventList = quest->scriptEventList;
	}
	else if (thisObj)
	{
		const auto scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm);
		if (scriptable)
		{
			targetScript = scriptable->script;
			targetEventList = thisObj->GetEventList();
		}
	}

	if (targetScript && targetEventList)
	{
		VariableInfo *varInfo = targetScript->GetVariableByName(varName);
		if (varInfo)
		{
			ScriptLocal *var = targetEventList->GetVariable(varInfo->idx);
			if (var)
			{
				auto refResult = (UInt32 *)result;
				(*(UInt64 *)&var->data) = value ? value->refID : 0;
			}
		}
	}

	return true;
}

bool Cmd_HasVariable_Execute(COMMAND_ARGS)
{
	GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_Has);
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
	if (!ExpressionEvaluator::Active())
	{
		ShowRuntimeError(scriptObj, "GetArrayVariable must be called within the context of an NVSE expression");
		return false;
	}

	GetVariable_Execute(PASS_COMMAND_ARGS, eScriptVar_Get);
	return true;
}

bool Cmd_CompareScripts_Execute(COMMAND_ARGS)
{
	Script *script1 = nullptr;
	Script *script2 = nullptr;
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

	ScriptEventList *list = eventList; //reset calling script by default
	if (thisObj)					   //call on a reference to reset that ref's script vars
		list = thisObj->GetEventList();

	if (list)
		*result = list->ResetAllVariables();

	return true;
}

class ExplicitRefFinder
{
public:
	bool Accept(const Script::RefVariable *var)
	{
		if (var && var->varIdx == 0)
			return true;

		return false;
	}
};

Script *GetScriptArg(TESObjectREFR *thisObj, TESForm *form)
{
	Script *targetScript = nullptr;
	if (form)
		targetScript = DYNAMIC_CAST(form, TESForm, Script);
	else if (thisObj)
	{
		if (const auto scriptable = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESScriptableForm))
			targetScript = scriptable->script;
	}

	return targetScript;
}

bool Cmd_GetNumExplicitRefs_Execute(COMMAND_ARGS)
{
	TESForm *form = nullptr;
	Script *targetScript = nullptr;
	*result = 0;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		targetScript = GetScriptArg(thisObj, form);
		if (targetScript)
			*result = targetScript->refList.CountIf(ExplicitRefFinder());
	}

	if (IsConsoleMode())
		Console_Print("GetNumExplicitRefs >> %.0f", *result);

	return true;
}

bool Cmd_GetNthExplicitRef_Execute(COMMAND_ARGS)
{
	TESForm *form = nullptr;
	UInt32 refIdx = 0;
	const auto refResult = reinterpret_cast<UInt32*>(result);
	*refResult = 0;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &refIdx, &form))
	{
		if (const Script *targetScript = GetScriptArg(thisObj, form))
		{
			UInt32 count = 0;
			const Script::RefVariable *entry = nullptr;
			while (count <= refIdx)
			{
				entry = targetScript->refList.Find(ExplicitRefFinder());
				if (!entry)
					break;

				count++;
			}

			if (count == refIdx + 1 && entry && entry->form)
			{
				*refResult = entry->form->refID;
				if (IsConsoleMode())
					Console_Print("GetNthExplicitRef >> %s (%08x)", GetFullName(entry->form), *refResult);
			}
		}
	}

	return true;
}

bool Cmd_RunScript_Execute(COMMAND_ARGS)
{
	TESForm *form = nullptr;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
		{
			if (!thisObj)
				return true;
			form = thisObj->baseForm;
		}

		const auto scriptForm = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
		Script *script = nullptr;
		if (!scriptForm) // Let's try for a MGEF
		{
			if (auto* const effect = DYNAMIC_CAST(form, TESForm, EffectSetting))
				script = effect->GetScript();
			else
				script = DYNAMIC_CAST(form, TESForm, Script);
		}
		else
			script = scriptForm->script;

		if (script)
		{
			*result = CALL_MEMBER_FN(script, Execute)(thisObj, nullptr, nullptr, 0);
		}
	}

	return true;
}

bool Cmd_GetCurrentScript_Execute(COMMAND_ARGS)
{
	// apparently this is useful
	const auto refResult = (UInt32 *)result;
	*refResult = scriptObj->refID;
	return true;
}

bool Cmd_GetCallingScript_Execute(COMMAND_ARGS)
{
	const auto refResult = (UInt32 *)result;
	*refResult = 0;
	Script *caller = UserFunctionManager::GetInvokingScript(scriptObj);
	if (caller)
	{
		*refResult = caller->refID;
	}

	return true;
}

// outPriority is not changed unless it was specified, i.e. if scripter wrote `"priority"::5`.
template <bool AllowOldFilters, bool AllowNewFilters>
bool ExtractEventCallback(ExpressionEvaluator &eval, EventManager::EventCallback &outCallback,
                          std::string &outName, int& outPriority)
{
	static_assert(AllowNewFilters || AllowOldFilters, "Must allow at least one filter type for extraction.");

	if (eval.ExtractArgs() && eval.NumArgs() >= 2)
	{
		const char *eventName = eval.Arg(0)->GetString();
		auto script = DYNAMIC_CAST(eval.Arg(1)->GetTESForm(), TESForm, Script);
		if (eventName && script) [[likely]]
		{
			outCallback.toCall = script;
			outName = eventName;
			// Must flush handlers with new filters on load, since save-baked arrays can be filters.
			// Loading back to before this handler was set could result in the stored array being invalid, hence this workaround.
			outCallback.flushOnLoad = AllowNewFilters;

			// any filters? Could also be priority
			for (auto i = 2; i < eval.NumArgs(); i++)
			{
				if (const TokenPair* pair = eval.Arg(i)->GetPair(); 
					pair && pair->left && pair->right) [[likely]]
				{
					if (pair->left->CanConvertTo(kTokenType_String))
					{
						const char* key = pair->left->GetString();
						if (!StrCompare(key, "priority"))
						{
							outPriority = static_cast<int>(pair->right->GetNumber());
							continue;
						}

						if constexpr (AllowOldFilters)
						{
							if (!StrCompare(key, "ref") || !StrCompare(key, "first"))
							{
								outCallback.source = pair->right->GetTESForm();
								continue;
							}
							if (!StrCompare(key, "object") || !StrCompare(key, "second"))
							{
								outCallback.object = pair->right->GetTESForm();
								continue;
							}
							eval.Error("Invalid string filter key \"%s\" passed, ignoring it.", key ? key : "NULL");
							continue;  // don't return false, in case previous mods would be broken by that change.
						}
						//else, assume AllowNewFilters is true
						eval.Error("String filter keys are not allowed for this function; use int codes.");
						return false;
					}

					// Else, handle number-key filter
					if constexpr (AllowNewFilters)
					{
						const auto index = static_cast<int>(pair->left->GetNumber());
						if (index < 0) [[unlikely]]
						{
							eval.Error("Invalid index %d passed (arg indices start from 1, and callingReference is filter #0).", index);
							return false;
						}
						if (index > EventManager::kNumMaxFilters) [[unlikely]]
						{
							eval.Error("Invalid index %d passed (max filter index is %d).", index, static_cast<int>(EventManager::kNumMaxFilters));
							return false;
						}

						const auto basicToken = pair->right->ToBasicToken();
						SelfOwningArrayElement element;
						if (basicToken && BasicTokenToElem(basicToken.get(), element)) [[likely]]
						{
							if (const auto [it, success] = outCallback.filters.emplace(index, std::move(element));
								!success) [[unlikely]]
							{
								eval.Error("Event filter index %u appears more than once.", index);
							}
						}
					}
					else // assume OldFilters are allowed
					{
						eval.Error("Int filter keys are not allowed for this function; use string.");
						// don't return false, in case previous mods would be broken by that change.
					}
				}
				else
				{
					eval.Error("Received invalid pair for arg %u somehow.", i);
				}
			}
			return true;
		}
	}

	return false;
}

bool ProcessEventHandler(std::string &eventName, EventManager::EventCallback &callback, bool addEvt, int priority, ExpressionEvaluator &eval)
{
	if (GetLNEventMask)
	{
		char* eventNameRaw = eventName.data();
		char *colon = strchr(eventNameRaw, ':');
		bool separatedStr = false;
		if (colon)
		{
			*(colon++) = 0;
			separatedStr = true;
		}
		if (const UInt32 eventMask = GetLNEventMask(eventNameRaw))
		{
			UInt32 const numFilter = (colon && *colon) ? atoi(colon) : 0;
			TESForm* formFilter = callback.source;

			if (addEvt)
			{
				if (priority != EventManager::kHandlerPriority_Default) [[unlikely]]
				{
					ShowRuntimeScriptError(callback.TryGetScript(), &eval, "Cannot use non-default (non-%i) priority %i for adding an LN event handler.", EventManager::kHandlerPriority_Default, priority);
					return false;
				}
				if (callback.IsAltRegistered()) [[unlikely]]
				{
					ShowRuntimeScriptError(callback.TryGetScript(), &eval, "Cannot use SetEventHandlerAlt for LN events; use SetEventHandler instead.");
					return false;
				}
			}
			else
			{
				if (priority != EventManager::kHandlerPriority_Default && priority != EventManager::kHandlerPriority_Invalid) [[unlikely]]
				{
					ShowRuntimeScriptError(callback.TryGetScript(), &eval, "Cannot use non-default (non-%i) and non-invalid (non-%i) priority %i for removing an LN event.", 
						EventManager::kHandlerPriority_Default, EventManager::kHandlerPriority_Invalid, priority);
					return false;
				}
				if (!formFilter)
				{
					// Support for using 1::SomeFilter instead of "source"::SomeFilter.
					if (auto const iter = callback.filters.find(1);
						iter != callback.filters.end())
					{
						UInt32 outRefID;
						if (iter->second.GetAsFormID(&outRefID))
							formFilter = LookupFormByID(outRefID);
						else
						{
							ShowRuntimeScriptError(callback.TryGetScript(), &eval, "Cannot use non-form filter %s for an LN event.", iter->second.GetStringRepresentation().c_str());
							return false;
						}
					}
				}
			}

			return ProcessLNEventHandler(eventMask, callback.TryGetScript(), addEvt, formFilter, numFilter);
		}
		// else, it's not an LN event.
		if (separatedStr)
		{
			//restore string back to how it was.
			--colon;
			*colon = ':';
		}
	}
	return addEvt ? EventManager::SetHandler<false>(eventName.c_str(), callback, priority, &eval)
		: EventManager::RemoveHandler(eventName.c_str(), callback, priority, &eval);
}

bool Cmd_SetEventHandler_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	EventManager::EventCallback callback;
	std::string eventName;
	int priority = EventManager::kHandlerPriority_Default;
	*result = (ExtractEventCallback<true, false>(eval, callback, eventName, priority)
		&& ProcessEventHandler(eventName, callback, true, priority, eval));
	return true;
}

bool Cmd_SetEventHandlerAlt_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	EventManager::EventCallback callback;
	std::string eventName;
	int priority = EventManager::kHandlerPriority_Default;
	*result = (ExtractEventCallback<false, true>(eval, callback, eventName, priority)
		&& ProcessEventHandler(eventName, callback, true, priority, eval));
	return true;
}

bool Cmd_RemoveEventHandler_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	EventManager::EventCallback callback;
	std::string eventName;
	int priority = EventManager::kHandlerPriority_Invalid;
	*result = (ExtractEventCallback<true, true>(eval, callback, eventName, priority)
		&& ProcessEventHandler(eventName, callback, false, priority, eval));
	return true;
}

bool Cmd_GetCurrentEventName_Execute(COMMAND_ARGS)
{
	AssignToStringVar(PASS_COMMAND_ARGS, EventManager::GetCurrentEventName());
	return true;
}

bool Cmd_DispatchEvent_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	*result = 0;
	if (!eval.ExtractArgs() || eval.NumArgs() == 0)
		return true;

	const char *eventName = eval.Arg(0)->GetString();
	if (!eventName) [[unlikely]]
		return true;

	ArrayID argsArrayId = 0;
	const char *senderName = nullptr;
	if (eval.NumArgs() > 1)
	{
		if (!eval.Arg(1)->CanConvertTo(kTokenType_Array))
			return true;
		argsArrayId = eval.Arg(1)->GetArrayID();

		if (eval.NumArgs() > 2)
			senderName = eval.Arg(2)->GetString();
	}

	*result = EventManager::DispatchUserDefinedEvent(eventName, scriptObj, argsArrayId, senderName, &eval);
	return true;
}

auto ExtractArgsAndArgTypes(const ExpressionEvaluator &eval, UInt8 argStart)
{
	EventManager::RawArgStack args;
	EventManager::ArgTypeStack argTypes;
	auto const numArgs = eval.NumArgs();
	for (size_t i = argStart; i < numArgs; i++)
	{
		auto const [rawArg, varType] = eval.Arg(i)->GetAsVoidArgAndVarType();
		auto const argType = EventManager::VarTypeToParamType(varType);
		args->push_back(rawArg);
		argTypes->push_back(argType);
	}
	return std::make_pair(args, argTypes);
}


bool Cmd_DispatchEventAlt_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	*result = 0;
	if (!eval.ExtractArgs() || eval.NumArgs() == 0) [[unlikely]]
		return true;

	const char* eventName = eval.Arg(0)->GetString();
	if (!eventName) [[unlikely]]
		return true;

	// has this event been defined?
	auto const eventInfoPtr = EventManager::TryGetEventInfoForName(eventName);
	if (!eventInfoPtr)
	{
		*result = 1; // assume the event may not have any handlers Set.
		// Sucks we can't warn users about having a potentially invalid eventName, though.
		return true;
	}

	auto& eventInfo = *eventInfoPtr;
	if (!eventInfo.AllowsScriptDispatch()) [[unlikely]]
	{
		eval.Error("Event %s is set up to not allow script dispatch.", eventName);
		return true;
	}

	auto [args, argTypes] = ExtractArgsAndArgTypes(eval, 1);

	// allow (risky) dispatching outside main thread
	*result = EventManager::DispatchEventRawWithTypes<true>(eventInfo, thisObj, args, argTypes,
		nullptr, nullptr, false, nullptr, &eval) > EventManager::DispatchReturn::kRetn_GenericError;
	return true;
}

bool Cmd_DumpEventHandlers_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	EventManager::EventInfo* eventInfoPtr = nullptr;
	Script* scriptFilter = nullptr;
	int priorityFilter = EventManager::kHandlerPriority_Invalid;
	if (auto const eventNameArg = eval.Arg(0))
	{
		if (const char* eventName = eventNameArg->GetString();
			eventName && eventName[0]) //can pass null string to avoid filtering by eventName
		{
			eventInfoPtr = EventManager::TryGetEventInfoForName(eventName);
			if (!eventInfoPtr) //filtering by invalid eventName
				return true;
		}

		if (auto const scriptArg = eval.Arg(1))
		{
			scriptFilter = scriptArg->GetUserFunction();
			if (auto const priorityArg = eval.Arg(2))
			{
				priorityFilter = static_cast<int>(priorityArg->GetNumber());
			}
		}
	}

	auto [argsToFilter, argTypes] = ExtractArgsAndArgTypes(eval, 3);

	Console_Print("DumpEventHandlers >> Beginning dump.");

	// Dumps all (matching) callbacks of the EventInfo
	auto const DumpEventInfo = [&, thisObj, scriptFilter](const EventManager::EventInfo &info)
	{
		Console_Print("== Dumping for event %s ==", info.evName);

		if (!argTypes->empty() && !info.ValidateDispatchedArgTypes(argTypes, &eval))
			return;
		auto const accurateArgTypes = info.HasUnknownArgTypes() ? argTypes : info.GetArgTypesAsStackVector();

		auto const DumpHandlerInfo = [&, thisObj, scriptFilter](int priority, const EventManager::EventCallback& handler)
		{
			if ((!scriptFilter || scriptFilter == handler.TryGetScript()) 
				&& !handler.IsRemoved() 
				&& (argsToFilter->empty() || handler.DoNewFiltersMatch<true>(thisObj, argsToFilter, accurateArgTypes, info, &eval)))
			{
				std::string const toPrint = FormatString(">> Priority: %i, handler: %s, filters: %s", 
					priority, 
					handler.GetCallbackFuncAsStr().c_str(),
					handler.GetFiltersAsStr().c_str());
				Console_Print_Str(toPrint);
			}
		};

		if (priorityFilter != EventManager::kHandlerPriority_Invalid)
		{
			auto const range = info.callbacks.equal_range(priorityFilter);
			for (auto i = range.first; i != range.second; ++i) {
				DumpHandlerInfo(i->first, i->second);
			}
		}
		else
		{
			for (auto const& i : info.callbacks) {
				DumpHandlerInfo(i.first, i.second);
			}
		}
	};

	if (!eventInfoPtr)  // no eventName filter
	{
		if (!argsToFilter->empty()) //if there are pseudo-args...
		{
			eval.Error("Args were passed to get filtered, but no eventName was passed; halting the function to prevent potential error message spam due to passing invalidly typed args for some events.");
			return true;
		}

		// loop through all eventInfo callbacks, filtering by script + filters
		for (auto const &eventInfo : EventManager::s_eventInfos)
		{
			DumpEventInfo(eventInfo);
		}
	}
	else //filtered by eventName
	{
		DumpEventInfo(*eventInfoPtr);
	}

	return true;
}


bool Cmd_GetEventHandlers_Execute(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	*result = 0;
	if (!eval.ExtractArgs()) [[unlikely]]
		return true;
	EventManager::EventInfo* eventInfoPtr = nullptr;
	Script* scriptFilter = nullptr;
	int priorityFilter = EventManager::kHandlerPriority_Invalid;
	if (auto const eventNameArg = eval.Arg(0))
	{
		if (const char* eventName = eventNameArg->GetString();
			eventName && eventName[0]) //can pass null string to avoid filtering by eventName
		{
			eventInfoPtr = EventManager::TryGetEventInfoForName(eventName);
			if (!eventInfoPtr) //filtering by invalid eventName
				return true;
		}

		if (auto const scriptArg = eval.Arg(1))
		{
			scriptFilter = scriptArg->GetUserFunction();
			if (auto const priorityArg = eval.Arg(2))
			{
				priorityFilter = static_cast<int>(priorityArg->GetNumber());
			}
		}
	}

	auto [argsToFilter, argTypes] = ExtractArgsAndArgTypes(eval, 3);

	// Dumps all (matching) callbacks of the EventInfo
	auto const GetEventInfoHandlers = [=, &argsToFilter, &argTypes, &eval](const EventManager::EventInfo& info) -> ArrayVar*
	{
		if (!argTypes->empty() && !info.ValidateDispatchedArgTypes(argTypes, &eval))
			return g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());

		auto const accurateArgTypes = info.HasUnknownArgTypes() ? argTypes : info.GetArgTypesAsStackVector();

		auto const TryAddHandlerToArray = 
			[&, scriptFilter, thisObj, scriptObj]
			(EventManager::CallbackMap::const_iterator i, 
			int handlerPos,
			ArrayVar* arrOfHandlers)
		{
			const auto& handler = i->second;
			if ((!scriptFilter || scriptFilter == handler.TryGetScript())
				&& !handler.IsRemoved()
				&& (argsToFilter->empty() || handler.DoNewFiltersMatch<true>(thisObj, argsToFilter, accurateArgTypes, info, &eval)))
			{
				arrOfHandlers->SetElementArray(handlerPos, handler.GetAsArray(scriptObj)->ID());
			}
		};

		if (priorityFilter != EventManager::kHandlerPriority_Invalid)  
		{
			// filter by priority
			auto const priorityRange = info.callbacks.equal_range(priorityFilter);
			int handlerPos = 0;
			ArrayVar* arrOfHandlers = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
			for (auto i = priorityRange.first; i != priorityRange.second; ++i) {
				TryAddHandlerToArray(i, handlerPos, arrOfHandlers);
				++handlerPos;
			}
			return arrOfHandlers;
		}
		//else, no priority filter

		// Key = priority of the handler.
		// Value = array of handlers.
		ArrayVar* handlersForAllPriorities = g_ArrayMap.Create(kDataType_Numeric, false, scriptObj->GetModIndex());

		// Select the correct type for calling the equal_range function
		decltype(info.callbacks.equal_range(0)) priorityRange;

		// Copied code from https://stackoverflow.com/a/26528981 for the loop structure.
		for (auto c = info.callbacks.begin(); c != info.callbacks.end(); c = priorityRange.second) {
			priorityRange = info.callbacks.equal_range(c->first);
			int handlerPos = 0;
			ArrayVar* arrOfHandlers = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
			for (auto i = priorityRange.first; i != priorityRange.second; ++i) {
				TryAddHandlerToArray(i, handlerPos, arrOfHandlers);
				++handlerPos;
			}
			if (!arrOfHandlers->Empty())
				handlersForAllPriorities->SetElementArray(c->first, arrOfHandlers->ID());
		}
		return handlersForAllPriorities;
	};

	if (!eventInfoPtr)
	{
		if (!argsToFilter->empty()) //if there are pseudo-args...
		{
			eval.Error("Args were passed to get filtered, but no eventName was passed; halting the function to prevent potential error message spam due to passing invalidly typed args for some events.");
			return true;
		}

		// keys = event names, values = a map-type array (keys = priority) containing arrays that contain arrays of handlers that have [0] = callbackFunc, [1] = filters string map.
		// If priority is specified, map of priority is skipped and the array of handlers is given instead.
		ArrayVar* eventsMap = g_ArrayMap.Create(kDataType_String, false, scriptObj->GetModIndex());
		*result = eventsMap->ID();

		// loop through all eventInfo callbacks, filtering by script + filters
		for (auto const &eventInfo : EventManager::s_eventInfos)
		{
			eventsMap->SetElementArray(eventInfo.evName, GetEventInfoHandlers(eventInfo)->ID());
		}
	}
	else //filtered by eventName
	{
		// return an array containing arrays that have [0] = callbackFunc, [1] = filters string map.
		// if priority is not specified, then the parent array is contained in a map-type array (keys = priority)
		*result = GetEventInfoHandlers(*eventInfoPtr)->ID();
	}

	return true;
}

template <bool CheckFirst>
bool IsEventHandlerFirstOrLast_Call(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	*result = 0;
	if (!eval.ExtractArgs()) [[unlikely]]
		return true;

	const auto eventName = eval.Arg(0)->GetString();
	auto udf = eval.Arg(1)->GetUserFunction();
	const auto startPriority = static_cast<int>(eval.Arg(2)->GetNumber());

	if (startPriority == EventManager::kHandlerPriority_Invalid)
	{
		eval.Error("Cannot use reserved priority 0 for this function.");
		return true;
	}

	EventManager::ScriptHandlerFilters filters;
	if (auto const numArgs = eval.NumArgs();
		numArgs >= 4)
	{
		filters.scriptsToIgnore = eval.Arg(3)->GetTESForm();

		if (numArgs >= 5)
		{
			filters.pluginsToIgnore = eval.Arg(4)->GetArrayVar();
			if (numArgs >= 6)
			{
				filters.pluginHandlersToIgnore = eval.Arg(5)->GetArrayVar();
			}
		}
	}

	*result = EventManager::IsEventHandlerFirstOrLast<CheckFirst>(eventName, udf, startPriority, filters);
	return true;
}

bool Cmd_IsEventHandlerFirst_Execute(COMMAND_ARGS)
{
	return IsEventHandlerFirstOrLast_Call<true>(PASS_COMMAND_ARGS);
}
bool Cmd_IsEventHandlerLast_Execute(COMMAND_ARGS)
{
	return IsEventHandlerFirstOrLast_Call<false>(PASS_COMMAND_ARGS);
}


template <bool GetHigher>
bool GetHigherOrLowerPriorityEventHandlers_Call(COMMAND_ARGS)
{
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	*result = 0;
	if (!eval.ExtractArgs()) [[unlikely]]
		return true;

	const auto eventName = eval.Arg(0)->GetString();
	auto udf = eval.Arg(1)->GetUserFunction();
	const auto startPriority = static_cast<int>(eval.Arg(2)->GetNumber());

	if (startPriority == EventManager::kHandlerPriority_Invalid)
	{
		eval.Error("Cannot use reserved priority 0 for this function.");
		return true;
	}

	EventManager::ScriptHandlerFilters filters;
	if (auto const numArgs = eval.NumArgs();
		numArgs >= 4)
	{
		filters.scriptsToIgnore = eval.Arg(3)->GetTESForm();

		if (numArgs >= 5)
		{
			filters.pluginsToIgnore = eval.Arg(4)->GetArrayVar();
			if (numArgs >= 6)
			{
				filters.pluginHandlersToIgnore = eval.Arg(5)->GetArrayVar();
			}
		}
	}

	if (auto resultArr = EventManager::GetHigherOrLowerPriorityEventHandlers<GetHigher>(eventName, udf,
		startPriority, filters, scriptObj))
	{
		*result = resultArr->ID();
	}
	return true;
}

bool Cmd_GetHigherPriorityEventHandlers_Execute(COMMAND_ARGS)
{
	return GetHigherOrLowerPriorityEventHandlers_Call<true>(PASS_COMMAND_ARGS);
}
bool Cmd_GetLowerPriorityEventHandlers_Execute(COMMAND_ARGS)
{
	return GetHigherOrLowerPriorityEventHandlers_Call<false>(PASS_COMMAND_ARGS);
}



extern float g_gameSecondsPassed;

template <bool PerSecondOrPerFrame>  //if false, then it's PerFrame
bool ExtractCallAfterInfo(ExpressionEvaluator& eval, std::list<DelayedCallInfo>& infos, ICriticalSection& cs)
{
	auto const time = static_cast<float>(eval.Arg(0)->GetNumber());
	Script* const callFunction = eval.Arg(1)->GetUserFunction();
	if (!callFunction)
		return false;

	//Optional args
	DelayedCallInfo::Mode mode = DelayedCallInfo::kMode_RunInGameModeOnly;
	CallArgs args{};

	auto const numArgs = eval.NumArgs();
	if (numArgs > 2)
	{
		mode = static_cast<DelayedCallInfo::Mode>(eval.Arg(2)->GetNumber());
		args.reserve(numArgs - 3);
		for (UInt32 i = 3; i < numArgs; i++)
		{
			if (auto const tok = eval.Arg(i))
			{
				SelfOwningArrayElement elem;
				BasicTokenToElem(tok, elem);
				args.emplace_back(std::move(elem));
			}
			else [[unlikely]]
				break;
		}
	}

	ScopedLock lock(cs);
	if constexpr (PerSecondOrPerFrame)
	{
		infos.emplace_back(callFunction, g_gameSecondsPassed + time, eval.m_thisObj, mode, std::move(args));
	}
	else
	{
		// time = frame count
		infos.emplace_back(callFunction, time, eval.m_thisObj, mode, std::move(args));
	}
	return true;
}

bool ExtractCallAfterInfo_OLD(COMMAND_ARGS, std::list<DelayedCallInfo>& infos, ICriticalSection& cs)
{
	float time;
	Script* callFunction;
	UInt32 runInMenuMode = false;
	if (!ExtractArgs(EXTRACT_ARGS, &time, &callFunction, &runInMenuMode) || !callFunction || !IS_ID(callFunction, Script))
		return false;

	ScopedLock lock(cs);
	infos.emplace_back(callFunction, g_gameSecondsPassed + time, thisObj, runInMenuMode ? DelayedCallInfo::kMode_AlsoRunInMenuMode : DelayedCallInfo::kMode_RunInGameModeOnly);
	return true;
}

std::list<DelayedCallInfo> g_callAfterInfos;
ICriticalSection g_callAfterInfosCS;

bool Cmd_CallAfterSeconds_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractCallAfterInfo<true>(eval, g_callAfterInfos, g_callAfterInfosCS);
	}
	return true;
}
bool Cmd_CallAfterSeconds_OLD_Execute(COMMAND_ARGS)
{
	*result = ExtractCallAfterInfo_OLD(PASS_COMMAND_ARGS, g_callAfterInfos, g_callAfterInfosCS);
	return true;
}

std::list<DelayedCallInfo> g_callAfterFramesInfos;
ICriticalSection g_callAfterFramesInfosCS;

bool Cmd_CallAfterFrames_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractCallAfterInfo<false>(eval, g_callAfterFramesInfos, g_callAfterFramesInfosCS);
	}
	return true;
}

std::list<DelayedCallInfo> g_callForInfos;
ICriticalSection g_callForInfosCS;

bool Cmd_CallForSeconds_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractCallAfterInfo<true>(eval, g_callForInfos, g_callForInfosCS);
	}
	return true;
}
bool Cmd_CallForSeconds_OLD_Execute(COMMAND_ARGS)
{
	*result = ExtractCallAfterInfo_OLD(PASS_COMMAND_ARGS, g_callForInfos, g_callForInfosCS);
	return true;
}

bool ExtractCallWhileInfo(ExpressionEvaluator &eval, std::list<CallWhileInfo> &infos, ICriticalSection &cs)
{
	Script* callFunction = eval.Arg(0)->GetUserFunction();
	Script* conditionFunction = eval.Arg(1)->GetUserFunction();
	if (!callFunction || !conditionFunction)
		return false;

	//Optional args
	CallWhileInfo::eFlags flags = CallWhileInfo::kFlags_None;
	CallArgs args{};

	auto const numArgs = eval.NumArgs();
	if (numArgs > 2)
	{
		flags = static_cast<CallWhileInfo::eFlags>(eval.Arg(2)->GetNumber());
		args.reserve(numArgs - 3);
		for (UInt32 i = 3; i < numArgs; i++)
		{
			if (auto const tok = eval.Arg(i))
			{
				SelfOwningArrayElement elem;
				BasicTokenToElem(tok, elem);
				args.emplace_back(std::move(elem));
			}
			else [[unlikely]]
				break;
		}
	}

	ScopedLock lock(cs);
	infos.emplace_back(callFunction, conditionFunction, eval.m_thisObj, flags, std::move(args));
	return true;
}
bool ExtractCallWhileInfo_OLD(COMMAND_ARGS, std::list<CallWhileInfo>& infos, ICriticalSection& cs)
{
	Script* callFunction;
	Script* conditionFunction;
	if (!ExtractArgs(EXTRACT_ARGS, &callFunction, &conditionFunction))
		return false;

	for (auto* form : { callFunction, conditionFunction })
		if (!form || !IS_ID(form, Script))
			return false;

	ScopedLock lock(cs);
	infos.emplace_back(callFunction, conditionFunction, thisObj, CallWhileInfo::kFlags_None);
	return true;
}

std::list<CallWhileInfo> g_callWhileInfos;
ICriticalSection g_callWhileInfosCS;

bool Cmd_CallWhile_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractCallWhileInfo(eval, g_callWhileInfos, g_callWhileInfosCS);
	}
	return true;
}
bool Cmd_CallWhile_OLD_Execute(COMMAND_ARGS)
{
	*result = ExtractCallWhileInfo_OLD(PASS_COMMAND_ARGS, g_callWhileInfos, g_callWhileInfosCS);
	return true;
}


std::list<CallWhileInfo> g_callWhenInfos;
ICriticalSection g_callWhenInfosCS;

bool Cmd_CallWhen_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractCallWhileInfo(eval, g_callWhenInfos, g_callWhenInfosCS);
	}
	return true;
}
bool Cmd_CallWhen_OLD_Execute(COMMAND_ARGS)
{
	*result = ExtractCallWhileInfo_OLD(PASS_COMMAND_ARGS, g_callWhenInfos, g_callWhenInfosCS);
	return true;
}

bool ExtractDelayedCallWhileInfo(ExpressionEvaluator& eval, std::list<DelayedCallWhileInfo>& infos, ICriticalSection& cs)
{
	float interval = eval.Arg(0)->GetNumber();
	Script* callFunction = eval.Arg(1)->GetUserFunction();
	Script* conditionFunction = eval.Arg(2)->GetUserFunction();
	if (!callFunction || !conditionFunction)
		return false;

	//Optional args
	DelayedCallWhileInfo::eFlags flags = DelayedCallWhileInfo::kFlags_None;
	CallArgs args{};

	auto const numArgs = eval.NumArgs();
	if (numArgs >= 4)
	{
		flags = static_cast<DelayedCallWhileInfo::eFlags>(eval.Arg(3)->GetNumber());
		args.reserve(numArgs - 4);
		for (UInt32 i = 4; i < numArgs; i++)
		{
			if (auto const tok = eval.Arg(i))
			{
				SelfOwningArrayElement elem;
				BasicTokenToElem(tok, elem);
				args.emplace_back(std::move(elem));
			}
			else [[unlikely]]
				break;
		}
	}

	ScopedLock lock(cs);
	infos.emplace_back(interval, g_gameSecondsPassed, callFunction, conditionFunction, eval.m_thisObj, flags, std::move(args));
	return true;
}

std::list<DelayedCallWhileInfo> g_callWhilePerSecondsInfos;
ICriticalSection g_callWhilePerSecondsInfosCS;

bool Cmd_CallWhilePerSeconds_Execute(COMMAND_ARGS)
{
	*result = false; //bSuccess
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		*result = ExtractDelayedCallWhileInfo(eval, g_callWhilePerSecondsInfos, g_callWhilePerSecondsInfosCS);
	}
	return true;
}

void ClearDelayedCalls()
{
	g_callForInfos.clear();
	g_callWhileInfos.clear();
	g_callAfterInfos.clear();
	g_callAfterFramesInfos.clear();
	g_callWhenInfos.clear();
	g_callWhilePerSecondsInfos.clear();
}

void DecompileScriptToFolder(const std::string& scriptName, Script* script, const std::string& fileExtension, const std::string_view& modName)
{
	ScriptParsing::ScriptAnalyzer analyzer(script);
	if (analyzer.error)
	{
		if (IsConsoleMode())
			Console_Print("Script %s is not compiled", scriptName.c_str());
		return;
	}
	const auto* dirName = "DecompiledScripts";
	if (!std::filesystem::exists(dirName))
		std::filesystem::create_directory(dirName);
	const auto modDirName = FormatString("%s/%s", dirName, modName.data());
	if (!std::filesystem::exists(modDirName))
		std::filesystem::create_directory(modDirName);
	const auto filePath = modDirName + '/' + scriptName + '.' + fileExtension;
	std::ofstream os(filePath);
	os << analyzer.DecompileScript();
	if (IsConsoleMode())
		Console_Print("Decompiled script to '%s'", filePath.c_str());
}

bool Cmd_DecompileScript_Execute(COMMAND_ARGS)
{
	TESForm* form;
	*result = 0;
	char fileExtensionArg[0x100]{};
	if (!ExtractArgs(EXTRACT_ARGS, &form, &fileExtensionArg))
		return true;
	std::string fileExtension;
	if (fileExtensionArg[0])
		fileExtension = std::string(fileExtensionArg);
	else
		fileExtension = "gek";

	std::string formName = form->GetName();
	if (formName.empty())
		formName = FormatString("%08X", form->refID & 0x00FFFFFF);

	if (IS_ID(form, Script))
	{
		auto script = static_cast<Script*>(form);
		DecompileScriptToFolder(formName, script, fileExtension, GetModName(script));
	}
	else if (IS_ID(form, TESQuest))
	{
		auto quest = static_cast<TESQuest*>(form);
		for (auto stageIter = quest->stages.Begin(); !stageIter.End(); ++stageIter)
			if (*stageIter)
				if (auto logEntry = stageIter->logEntries.GetFirstItem(); logEntry && logEntry->resultScript.info.dataLength)
					DecompileScriptToFolder(FormatString("%s #%d", formName.c_str(), stageIter->stage), &logEntry->resultScript, fileExtension, GetModName(quest));
	}
	else if (IS_ID(form, TESPackage))
	{
		auto package = static_cast<TESPackage*>(form);
		for (auto& packageEvent : { std::make_pair(" OnBegin", &package->onBeginAction), std::make_pair(" OnEnd", &package->onEndAction), std::make_pair(" OnChange", &package->onChangeAction) })
		{
			auto& [evntName, action] = packageEvent;
			if (action->script)
				DecompileScriptToFolder(formName + evntName, action->script, fileExtension, GetModName(package));
		}
	}
	else if (IS_ID(form, TESTopicInfo))
	{
		auto topicInfo = static_cast<TESTopicInfo*>(form);
		if (auto bgnScript = topicInfo->GetResultScript(0))
			DecompileScriptToFolder(formName + " Begin", bgnScript, fileExtension, GetModName(topicInfo));
		if (auto endScript = topicInfo->GetResultScript(1))
			DecompileScriptToFolder(formName + " End", endScript, fileExtension, GetModName(topicInfo));
	}
	else if (IS_ID(form, BGSTerminal))
	{
		auto terminal = static_cast<BGSTerminal*>(form);
		int entryIdx = 0;
		for (auto entryIter = terminal->menuEntries.Begin(); !entryIter.End(); ++entryIter)
			if (*entryIter)
			{
				entryIdx++;
				if (entryIter->resultScript.info.dataLength)
					DecompileScriptToFolder(FormatString("%s #%d", formName.c_str(), entryIdx), &entryIter->resultScript, fileExtension, GetModName(terminal));
			}
	}
	else
		return true;
	*result = 1;
	return true;
}

bool Cmd_HasScriptCommand_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 commandOpcode;
	TESForm* form;
	UInt32 eventOpcode = -1;
	Script* script = nullptr;
	if (!ExtractArgs(EXTRACT_ARGS, &commandOpcode, &form, &eventOpcode))
		return true;
	if (!form)
		form = thisObj;
	if (!form)
		return true;
	if (IS_ID(form, Script))
		script = static_cast<Script*>(form);
	else if (form->GetIsReference())
	{
		const auto* ref = static_cast<TESObjectREFR*>(form);
		if (const auto* extraScript = ref->GetExtraScript())
			script = extraScript->script;
	}
	if (!script)
		return true;
	auto* cmdInfo = g_scriptCommands.GetByOpcode(commandOpcode);
	if (!cmdInfo)
		return true;
	CommandInfo* eventCmd = nullptr;
	if (eventOpcode != -1)
		eventCmd = GetEventCommandInfo(eventOpcode);
	if (ScriptParsing::ScriptContainsCommand(script, cmdInfo, eventCmd))
		*result = 1;
	return true;
}

static MemoizedMap<const char*, UInt32> s_opcodeMap;

bool Cmd_GetCommandOpcode_Execute(COMMAND_ARGS)
{
	*result = 0;
	char buf[0x400];
	if (!ExtractArgs(EXTRACT_ARGS, buf))
		return true;
	*result = s_opcodeMap.Get(buf, [](const char* buf)
	{
		auto* cmd = g_scriptCommands.GetByName(buf);
		if (!cmd)
			return 0u;
		return static_cast<unsigned>(cmd->opcode);
	});
	return true;
}

bool Cmd_DumpCommandWikiDoc_Execute(COMMAND_ARGS)
{
	char buf[0x400];
	if (!ExtractArgs(EXTRACT_ARGS, buf))
		return true;
	if (auto* commandInfo = g_scriptCommands.GetByName(buf))
	{
		if (IsConsoleMode())
			Console_Print("Dumping wiki-style documentation for function.");

		commandInfo->DumpWikiDocs();

		if (IsConsoleMode())
			Console_Print("Finished dumping wiki-style documentation for function.");
	}
	else if (IsConsoleMode())
		Console_Print("Unable to find command with that name. Double-check for a spelling error, or if it's being registered.");
	return true;
}

bool Cmd_DumpCommandWikiDocs_Execute(COMMAND_ARGS)
{
	UInt32 opcodeStart, opcodeEnd;
	char versionNumBuf[256]{};
	if (!ExtractArgs(EXTRACT_ARGS, &opcodeStart, &opcodeEnd, &versionNumBuf))
		return true;
	if (auto commandInfoVec = g_scriptCommands.GetByOpcodeRange(opcodeStart, opcodeEnd);
		!commandInfoVec.empty())
	{
		if (IsConsoleMode())
			Console_Print("Dumping wiki-style documentation for functions in opcode range.");

		std::for_each(commandInfoVec.begin(), commandInfoVec.end(), 
			[&versionNumBuf](CommandInfo* commandInfo) {commandInfo->DumpWikiDocs(versionNumBuf); });

		if (IsConsoleMode())
			Console_Print("Finished dumping wiki-style documentation for functions.");
	}
	else if (IsConsoleMode())
		Console_Print("Unable to find commands.");
	return true;
}

bool Cmd_TernaryUDF_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		auto const value = eval.Arg(0)->ToBasicToken();
		if (!value)
			return true;	// should never happen, could cause weird behavior otherwise.

		Script* call_udf = nullptr;
		if (value->GetBool()) {
			call_udf = eval.Arg(1)->GetUserFunction();
		}
		else {
			call_udf = eval.Arg(2)->GetUserFunction();
		}
		if (!call_udf)
			return true;

		InternalFunctionCaller caller(call_udf, thisObj, containingObj);
		caller.SetArgs(0);
		if (auto const tokenValResult = UserFunctionManager::Call(std::move(caller)))
			tokenValResult->AssignResult(eval);
	}
	return true;
}

class TernaryEvaluator
{
public:
	TernaryEvaluator(ExpressionEvaluator& context) : m_eval(context)
	{
	}

	bool ExtractArgs()
	{
		const UInt32 numArgs = m_eval.ReadByte();
		if (numArgs != 3)
			return false;
		auto* cond = m_eval.Evaluate();
		m_eval.m_args[0] = cond;
		ScriptToken* res;
		if (cond->GetBool()) {
			res = m_eval.Evaluate();

			const auto offset = *((UInt16*)m_eval.Data());
			m_eval.Data() += offset;
			*m_eval.m_opcodeOffsetPtr += offset;
		}
		else {
			const auto offset = *((UInt16*)m_eval.Data());
			m_eval.Data() += offset;
			*m_eval.m_opcodeOffsetPtr += offset;

			res = m_eval.Evaluate();
		}
		m_eval.m_args[1] = res;
		return true;
	}

	ScriptToken* GetResult() const 
	{
		return m_eval.m_args[1];
	}
private:
	ExpressionEvaluator& m_eval;
};

bool Cmd_Ternary_Execute(COMMAND_ARGS)
{
	*result = 0;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (TernaryEvaluator ternaryEval(eval);
		ternaryEval.ExtractArgs())
	{
		if (auto* const tokenValResult = ternaryEval.GetResult())
			tokenValResult->AssignResult(eval);
	}
	return true;
}

// Only works inside OnSell event.
bool Cmd_GetSoldItemInvRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (auto const invRef = EventManager::OnSell::GetSoldItemInvRef())
	{
		UInt32* refResult = (UInt32*)result;
		*refResult = invRef->refID;
	}
	return true;
}

bool Cmd_CompileScript_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		std::string pathStr;
		{
			auto const path = eval.Arg(0)->GetString();
			if (!path || !path[0])
				return true;
			pathStr = path;
		}
		ReplaceAll(pathStr, "/", "\\");

		bool forceRecompile = false;
		if (eval.NumArgs() > 1)
			forceRecompile = eval.Arg(1)->GetBool();

		UInt32* refResult = (UInt32*)result;

		if (forceRecompile)
		{
			if (auto* script = CompileAndCacheScript(pathStr.c_str()))
				*refResult = script->refID;
		}
		else
		{
			// Try to get the cached result
			ScopedLock lock(g_cachedUdfCS);
			if (auto iter = cachedFileUDFs.Find(const_cast<char*>(pathStr.c_str()));
				!iter.End())
			{
				*refResult = iter.Get()->refID;

#if _DEBUG
				if (IsConsoleMode())
					Console_Print("CompileScript >> Got cached script");
#endif
			}
			else
			{
				// No cached result, so create & cache
				// Should only happen if file wasn't pre-cached at startup, i.e it didn't exist and was created mid-game.
				if (auto* script = CompileAndCacheScript(pathStr.c_str()))
					*refResult = script->refID;
#if _DEBUG
				if (IsConsoleMode())
					Console_Print("CompileScript >> Had to compile script; couldn't find cached script.");
#endif
			}
		}
	}
	return true;
}

bool Cmd_MatchesAnyOf_Execute(COMMAND_ARGS)
{
	*result = false;
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		const auto valTokenToMatch = eval.Arg(0);
		const auto type = valTokenToMatch->GetTokenTypeAsVariableType();

		for (int i = 1; i < eval.NumArgs(); ++i)
		{
			switch (type)
			{
			case Script::VariableType::eVarType_Float:
			case Script::VariableType::eVarType_Integer:
				if (FloatEqual(valTokenToMatch->GetNumber(), eval.Arg(i)->GetNumber()))
				{
					*result = 1;
					return true;
				}
				break;

			case Script::VariableType::eVarType_String:
				if (StrEqual(valTokenToMatch->GetString(), eval.Arg(i)->GetString()))
				{
					*result = 1;
					return true;
				}
				break;

			case Script::VariableType::eVarType_Ref:
				if (valTokenToMatch->GetFormID() == eval.Arg(i)->GetFormID())
				{
					*result = 1;
					return true;
				}
				break;

			case Script::VariableType::eVarType_Array:
				if (valTokenToMatch->GetArrayVar()->Equals(eval.Arg(i)->GetArrayVar()))
				{
					*result = 1;
					return true;
				}
				break;

			default:
				return true;
			}
		}
	}
	return true;
}

#endif
