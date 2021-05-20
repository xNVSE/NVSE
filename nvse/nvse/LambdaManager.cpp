#include "LambdaManager.h"

#include "bimap.h"
#include "GameAPI.h"
#include <unordered_map>

thread_local LambdaManager::ScriptData LambdaManager::g_lastScriptData;

static stde::unordered_map<Script*, ScriptEventList*> g_lambdaParentEventListMap;

// This is required since if the script token cache is cleared and a plugin has a reference to the script it has to be in the same place
// or the plugin will keep a reference to an invalid pointer.
static stde::unordered_map<std::pair<UInt8*, ScriptEventList*>, Script*> g_lambdaHeapMap;
static std::unordered_set<Script*> g_lambdaCache;
static ICriticalSection g_lambdaCs;

Script* LambdaManager::CreateLambdaScript(UInt8* position, ScriptEventList* parentEventList, Script* parentScript)
{
	auto newScript = false;
	// auto script = MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
	Script* scriptLambda;
	if (const auto iter = g_lambdaHeapMap.find(std::make_pair(position, parentEventList)); iter != g_lambdaHeapMap.end())
	{
		scriptLambda = iter->second;
		if (g_lambdaCache.find(scriptLambda) != g_lambdaCache.end())
			return scriptLambda;
		// if a script is being recompiled (i.e. through hot reload) it's important that script pointers in plugins don't get invalidated
		ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free script data pointer and tLists
		ThisStdCall(0x5AA0F0, scriptLambda); // script constructor
	}
	else
	{
		scriptLambda = New<Script, 0x5AA0F0>();
		newScript = true;
	}
	ScopedLock lock(g_lambdaCs);
	if (newScript)
		g_lambdaHeapMap[std::make_pair(position, parentEventList)] = scriptLambda;
	scriptLambda->data = g_lastScriptData.scriptData;
	scriptLambda->info.dataLength = g_lastScriptData.size;
	CdeclCall(0x5AB930, &parentScript->varList, &scriptLambda->varList); // CopyVarList
	CdeclCall(0x5AB7F0, &parentScript->refList, &scriptLambda->refList); // CopyRefList
	scriptLambda->info.numRefs = parentScript->info.numRefs;
	scriptLambda->info.varCount = parentScript->info.varCount;
	scriptLambda->info.lastID = parentScript->info.lastID;
	g_lambdaCache.insert(scriptLambda);
	g_lambdaParentEventListMap[scriptLambda] = parentEventList;
	return scriptLambda;
}

ScriptEventList* LambdaManager::GetParentEventList(Script* scriptLambda)
{
	g_lambdaParentEventListMap[scriptLambda];
	return g_lambdaParentEventListMap[scriptLambda];
}

void LambdaManager::MarkParentAsDeleted(ScriptEventList* parentEventList)
{
#if 0
	if (g_lambdaParentEventListMap.find(parentEventList) == g_lambdaParentEventListMap.end()) return;
	ScopedLock lock(g_lambdaCs);
	g_lambdaParentEventListMap.erase(parentEventList);
#endif
}

bool LambdaManager::IsScriptLambda(Script* script)
{
#if 0
	return g_lambdaHeapMap.to.find(script) != g_lambdaHeapMap.to.end();
#endif
	return true;
}

void LambdaManager::ClearCache()
{
	ScopedLock lock(g_lambdaCs);
	g_lambdaCache.clear();
}