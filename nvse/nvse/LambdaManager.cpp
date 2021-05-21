#include "LambdaManager.h"
#include "GameAPI.h"

// set in ScriptToken::ReadFrom
thread_local LambdaManager::ScriptData LambdaManager::g_lastScriptData;

// need to look up event list for the lambda to use itself
static std::unordered_map<Script*, ScriptEventList*> g_lambdaParentEventListMap;
// needs to be kept since if an event list is deleted the pointer becomes invalid; clear both maps so that it uses an empty event list instead
static std::multimap<ScriptEventList*, Script*>	g_parentEventListLambdaMap;
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
	scriptLambda->info.lastID = parentScript->info.lastID;
	
	g_lambdaParentEventListMap.emplace(scriptLambda, parentEventList);
	g_parentEventListLambdaMap.emplace(parentEventList, scriptLambda);
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
	const auto iter = g_parentEventListLambdaMap.find(parentEventList);
	if (iter == g_parentEventListLambdaMap.end()) return;
	ScopedLock lock(g_lambdaCs);
	g_lambdaParentEventListMap.erase(iter->second);
	g_parentEventListLambdaMap.erase(iter);
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