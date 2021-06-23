#include "LambdaManager.h"
#include "common/ICriticalSection.h"
#include <ranges>

#include "Commands_Scripting.h"
#include "GameAPI.h"
#include "GameObjects.h"
#include "Hooks_Other.h"

using ScriptLambda = Script;
using FormID = UInt32;

// need to look up event list for the lambda to use itself
static std::unordered_map<ScriptLambda*, ScriptEventList*> g_lambdaParentEventListMap;

// used for memoizing scripts and prevent endless allocation of scripts
static std::map<std::pair<UInt8*, FormID>, ScriptLambda*> g_lambdaScriptPosMap;

// for use when event lists in effect scripts and UDFs get deleted, but we keep them in case a lambda gets called later
// used per form ID to prevent leaking and pointer clashes
static std::unordered_map<FormID, ScriptEventList*> g_deletedEventListMap;

// contains all lambdas
static std::unordered_set<ScriptLambda*> g_lambdas;

// avoid concurrency issues
ICriticalSection LambdaManager::g_lambdaCs;

Script* LambdaManager::CreateLambdaScript(UInt8* position, const ScriptData& scriptData, const ExpressionEvaluator& exprEval)
{
	ScopedLock lock(g_lambdaCs);
	auto* parentEventList = exprEval.eventList;
	auto* parentScript = exprEval.script;

	// use ref ID instead of event list ptr to prevent memory leaking from effect scripts
	UInt32 ownerRefID;
	if (parentScript->IsQuestScript() && parentScript->quest)
		ownerRefID = parentScript->quest->refID;
	else if (parentScript->IsUserDefinedFunction())
		ownerRefID = parentScript->refID;
	else if (OtherHooks::g_lastScriptOwnerRef)
		ownerRefID = OtherHooks::g_lastScriptOwnerRef->refID;
	else
		return nullptr;
	
	// auto script = MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
	const auto key = std::make_pair(position, ownerRefID);
	if (const auto iter = g_lambdaScriptPosMap.find(key); iter != g_lambdaScriptPosMap.end())
	{
		return iter->second;
	}
	auto* scriptLambda = New<Script, 0x5AA0F0>();
	g_lambdaScriptPosMap[key] = scriptLambda;
	scriptLambda->data = scriptData.scriptData;
	scriptLambda->info.dataLength = scriptData.size;
	
	scriptLambda->varList = parentScript->varList; // CdeclCall(0x5AB930, &parentScript->varList, &scriptLambda->varList); // CopyVarList
	scriptLambda->refList = parentScript->refList; // CdeclCall(0x5AB7F0, &parentScript->refList, &scriptLambda->refList); // CopyRefList
	
	scriptLambda->info.numRefs = parentScript->info.numRefs;
	scriptLambda->info.varCount = parentScript->info.varCount;
	scriptLambda->info.unusedVariableCount = parentScript->info.unusedVariableCount;
	const auto nextFormId = GetNextFreeFormID(parentScript->refID);
	if (nextFormId >> 24 == parentScript->GetModIndex())
	{
		scriptLambda->SetRefID(nextFormId, true);
	}
	g_lambdaParentEventListMap[scriptLambda] = parentEventList;
	g_lambdas.emplace(scriptLambda);
	return scriptLambda;
}

bool RemoveEventList(ScriptEventList* parentEventList)
{
	return std::erase_if(g_lambdaParentEventListMap, [&](auto& p)
	{
		auto& [lambda, eventList] = p;
		return eventList == parentEventList;
	}) != 0;
}

FormID GetFormIDForLambda(Script* scriptLambda)
{
	const auto spIt = std::ranges::find_if(g_lambdaScriptPosMap, [&](auto& p)
	{
		auto& [pair, lambda] = p;
		return lambda == scriptLambda;
	});
	if (spIt == g_lambdaScriptPosMap.end())
		return 0;
	return spIt->first.second;
}

FormID GetFormIDForLambda(ScriptEventList* parentEventList)
{
	const auto lpIt = std::ranges::find_if(g_lambdaParentEventListMap, [&](auto& p)
	{
		auto& [lambdaScript, eventList] = p;
		return parentEventList == eventList;
	});
	if (lpIt == g_lambdaParentEventListMap.end()) 
		return 0;
	return GetFormIDForLambda(lpIt->first);
}

// gets a "deleted" event list that was saved instead so that a future callback can potentially use it
ScriptEventList* GetDeletedEventList(Script* scriptLambda)
{
	const auto formID = GetFormIDForLambda(scriptLambda);
	if (!formID)
		return nullptr;
	if (const auto deletedListIter = g_deletedEventListMap.find(formID); deletedListIter != g_deletedEventListMap.end())
		return deletedListIter->second;
	return nullptr;
}

ScriptEventList* LambdaManager::GetParentEventList(Script* scriptLambda)
{
	const auto iter = g_lambdaParentEventListMap.find(scriptLambda);
	if (iter != g_lambdaParentEventListMap.end())
		return iter->second;
	if (auto* deletedList = GetDeletedEventList(scriptLambda))
		return deletedList;
	return nullptr;
}

void LambdaManager::MarkParentAsDeleted(ScriptEventList*& parentEventList)
{
	ScopedLock lock(g_lambdaCs);
	const auto formID = GetFormIDForLambda(parentEventList);
	if (!formID)
		return;
	if (!RemoveEventList(parentEventList))
		return;
	auto*& deletedEventList = g_deletedEventListMap[formID];
	if (deletedEventList)
		OtherHooks::DeleteEventList(deletedEventList, false);
	deletedEventList = parentEventList;
	parentEventList = nullptr; // assign reference to null so it can't be deleted
}

bool LambdaManager::IsScriptLambda(Script* scriptLambda)
{
	return g_lambdas.contains(scriptLambda);
}

void LambdaManager::DeleteAllForParentScript(Script* parentScript)
{
	ScopedLock lock(g_lambdaCs);
	std::vector<Script*> lambdas;
	for (auto& [scriptLambda, parentEventList] : g_lambdaParentEventListMap)
	{
		if (parentEventList->m_script == parentScript)
		{
			g_lambdas.erase(scriptLambda);
			FormHeap_Free(scriptLambda->data); // ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free parentScript data pointer
			scriptLambda->data = nullptr; // parentScript data and tLists will be freed but parentScript won't be since plugins may store pointers to it to call
			lambdas.push_back(scriptLambda);
		}
	}
	std::erase_if(g_lambdaScriptPosMap, [&](auto& p) { return std::ranges::find(lambdas, p.second) != lambdas.end(); });
	std::erase_if(g_lambdaParentEventListMap, [&](auto& p) {return p.second->m_script == parentScript;});
}

void LambdaManager::ClearSavedDeletedEventLists()
{
	ScopedLock lock(g_lambdaCs);
	for (auto* eventList : g_deletedEventListMap | std::views::values)
	{
		OtherHooks::DeleteEventList(eventList, false);
	}
	g_deletedEventListMap.clear();
}

