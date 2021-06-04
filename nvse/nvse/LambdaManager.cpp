#include "LambdaManager.h"

#include <ranges>

#include "GameAPI.h"

// set in ScriptToken::ReadFrom
thread_local LambdaManager::ScriptData LambdaManager::g_lastScriptData;

// need to look up event list for the lambda to use itself
static std::unordered_map<Script*, ScriptEventList*> g_lambdaParentEventListMap;
// needs to be kept since if an event list is deleted the pointer becomes invalid; clear both maps so that it uses an empty event list instead
static std::unordered_map<ScriptEventList*, std::vector<Script*>> g_parentEventListLambdaMap;
// used for memoizing scripts and prevent endless allocation of scripts
static std::map<std::pair<UInt8*, ScriptEventList*>, Script*> g_lambdaScriptPosMap;

// avoid concurrency issues
ICriticalSection LambdaManager::g_lambdaCs;
std::atomic<bool> LambdaManager::g_lambdasCleared = false;

Script* LambdaManager::CreateLambdaScript(UInt8* position, ScriptEventList* parentEventList, Script* parentScript)
{
	// auto script = MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
	const auto key = std::make_pair(position, parentEventList);
	if (const auto iter = g_lambdaScriptPosMap.find(key); iter != g_lambdaScriptPosMap.end())
	{
		return iter->second;
	}
	ScopedLock lock(g_lambdaCs);
	auto* scriptLambda = New<Script, 0x5AA0F0>();
	g_lambdaScriptPosMap.emplace(key, scriptLambda);
	scriptLambda->data = g_lastScriptData.scriptData;
	scriptLambda->info.dataLength = g_lastScriptData.size;
	
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
	
	g_lambdaParentEventListMap.emplace(scriptLambda, parentEventList);
	auto& eventListLambdas = g_parentEventListLambdaMap[parentEventList];
	eventListLambdas.push_back(scriptLambda);
	return scriptLambda;
}

ScriptEventList* LambdaManager::GetParentEventList(Script* scriptLambda)
{
	const auto iter = g_lambdaParentEventListMap.find(scriptLambda);
	if (iter != g_lambdaParentEventListMap.end())
		return iter->second;
	return nullptr;
}

void LambdaManager::MarkParentAsDeleted(ScriptEventList* parentEventList)
{
	const auto iterScripts = g_parentEventListLambdaMap.find(parentEventList);
	if (iterScripts == g_parentEventListLambdaMap.end()) return;
	ScopedLock lock(g_lambdaCs);
	for (auto* scriptLambda : iterScripts->second)
	{
		g_lambdaParentEventListMap.erase(scriptLambda);
	}
	g_parentEventListLambdaMap.erase(iterScripts);
}

bool LambdaManager::IsScriptLambda(Script* script)
{
	return g_lambdaParentEventListMap.find(script) != g_lambdaParentEventListMap.end();
}

void LambdaManager::ClearCache()
{
	ScopedLock lock(g_lambdaCs);
	g_lambdasCleared = true;
	for (auto& pair: g_lambdaScriptPosMap)
	{
		auto* scriptLambda = pair.second;
		FormHeap_Free(scriptLambda->data); // ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free script data pointer
		scriptLambda->data = nullptr; // script data and tLists will be freed but script won't be since plugins may store pointers to it to call
	}
	g_lambdaScriptPosMap.clear();
	g_lambdaParentEventListMap.clear();
	g_parentEventListLambdaMap.clear();
}

void LambdaManager::DeleteAllForParentScript(Script* script)
{
	ScopedLock lock(g_lambdaCs);
	g_lambdasCleared = true;
	std::vector<Script*> lambdas;
	for (auto& [scriptLambda, parentEventList] : g_lambdaParentEventListMap)
	{
		if (parentEventList->m_script == script)
		{
			FormHeap_Free(scriptLambda->data); // ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free script data pointer
			scriptLambda->data = nullptr; // script data and tLists will be freed but script won't be since plugins may store pointers to it to call
			lambdas.push_back(scriptLambda);
		}
	}
	std::erase_if(g_lambdaScriptPosMap, [&](auto& p) { return std::ranges::find(lambdas, p.second) != lambdas.end(); });
	std::erase_if(g_lambdaParentEventListMap, [&](auto& p) {return p.second->m_script == script;});
	std::erase_if(g_parentEventListLambdaMap, [&](auto& p) {return p.first->m_script == script;});
}