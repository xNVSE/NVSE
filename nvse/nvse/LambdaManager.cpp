#include "LambdaManager.h"
#include "common/ICriticalSection.h"
#include <ranges>

#include "Commands_Scripting.h"
#include "GameAPI.h"
#include "GameObjects.h"
#include "Hooks_Other.h"

using ScriptLambda = Script;
using FormID = UInt32;

struct VariableListContext
{
	UInt32 refCount = 0;
	ScriptEventList* eventListCopy = nullptr;
	std::vector<ScriptLambda*> lambdas;
};

// used for memoizing scripts and prevent endless allocation of scripts
static std::map<std::pair<UInt8*, FormID>, ScriptLambda*> g_lambdaScriptPosMap;
std::unordered_map<ScriptEventList*, VariableListContext> g_savedVarLists;

struct LambdaContext
{
	ScriptEventList* parentEventList = nullptr;
};

// contains all lambdas
static std::unordered_map<ScriptLambda*, LambdaContext> g_lambdas;

// avoid concurrency issues
ICriticalSection LambdaManager::g_lambdaCs;

void SetLambdaParent(LambdaContext& ctx, ScriptEventList* parentEventList)
{
	ctx.parentEventList = parentEventList;
}

void SetLambdaParent(ScriptLambda* scriptLambda, ScriptEventList* parentEventList)
{
	SetLambdaParent(g_lambdas[scriptLambda], parentEventList);
}

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
		auto* scriptLambda = iter->second;
		SetLambdaParent(scriptLambda, parentEventList);
		return scriptLambda;
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
	auto iter = g_lambdas.emplace(scriptLambda, LambdaContext());
	auto& ctx = iter.first->second;
	SetLambdaParent(ctx, parentEventList);
	
	return scriptLambda;
}

FormID GetFormIDForLambda(ScriptLambda* scriptLambda)
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

auto GetLambdaForParentIter(ScriptEventList* parentEventList)
{
	return std::ranges::find_if(g_lambdas, [&](auto& p)
	{
		return p.second.parentEventList == parentEventList;
	});
}

Script* GetLambdaForParent(ScriptEventList* parentEventList)
{
	const auto lpIt = GetLambdaForParentIter(parentEventList);
	if (lpIt == g_lambdas.end()) 
		return nullptr;
	return lpIt->first;
}

FormID GetFormIDForLambda(ScriptEventList* parentEventList)
{
	auto* scriptLambda = GetLambdaForParent(parentEventList);
	if (!scriptLambda) 
		return 0;
	return GetFormIDForLambda(scriptLambda);
}

bool LambdaManager::IsScriptLambda(Script* scriptLambda)
{
	return g_lambdas.contains(scriptLambda);
}

void LambdaManager::DeleteAllForParentScript(Script* parentScript)
{
	ScopedLock lock(g_lambdaCs);
	std::vector<Script*> lambdas;
	for (auto& [scriptLambda, ctx] : g_lambdas)
	{
		if (ctx.parentEventList && ctx.parentEventList->m_script == parentScript)
		{
			FormHeap_Free(scriptLambda->data); // ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free parentScript data pointer
			lambdas.push_back(scriptLambda);
			ctx.parentEventList = nullptr;
			scriptLambda->data = nullptr; // parentScript data and tLists will be freed but parentScript won't be since plugins may store pointers to it to call
			scriptLambda->varList = {};
			scriptLambda->refList = {};
		}
	}
	std::erase_if(g_lambdaScriptPosMap, [&](auto& p) { return std::ranges::find(lambdas, p.second) != lambdas.end(); });
	g_savedVarLists.clear();
}

void LambdaManager::ClearSavedDeletedEventLists()
{
	// reserved for future use
}

VariableListContext* GetVarListContext(Script* scriptLambda)
{
	const auto varListCopyIter = std::ranges::find_if(g_savedVarLists, [&](auto& p)
	{
		return std::ranges::find(p.second.lambdas, scriptLambda) != p.second.lambdas.end();
	});
	if (varListCopyIter != g_savedVarLists.end())
		return &varListCopyIter->second;
	return nullptr;
}

ScriptEventList* LambdaManager::GetParentEventList(Script* scriptLambda)
{
	const auto iter = g_lambdas.find(scriptLambda);
	if (iter == g_lambdas.end())
		return nullptr;
	if (auto* eventList = iter->second.parentEventList)
		return eventList;

	// saved variable lists
	auto* varListContext = GetVarListContext(scriptLambda);
	if (varListContext && varListContext->eventListCopy)
		return varListContext->eventListCopy;
	return nullptr;
}

void RemoveEventList(ScriptEventList* eventList)
{
	for (auto& [parentEventList] : g_lambdas | std::views::values)
	{
		if (parentEventList == eventList)
			parentEventList = nullptr;
	}
}

void LambdaManager::MarkParentAsDeleted(ScriptEventList* parentEventList)
{
	ScopedLock lock(g_lambdaCs);
	RemoveEventList(parentEventList);
	if (auto iter = g_savedVarLists.find(parentEventList); iter != g_savedVarLists.end())
	{
		auto& ctx = iter->second;
		ctx.eventListCopy = parentEventList->Copy();
	}
}

void LambdaManager::SaveLambdaVariables(Script* scriptLambda)
{
	auto* eventList = GetParentEventList(scriptLambda);
	if (!eventList)
		return;
	auto& [refCount, eventListCopy, lambdas] = g_savedVarLists[eventList];
	++refCount;
	lambdas.push_back(scriptLambda);
}

void DeleteLambdaScript(auto& iter)
{
	Script* scriptLambda = iter->first;
	g_lambdas.erase(scriptLambda);
	scriptLambda->Destroy(true);
}


void LambdaManager::UnsaveLambdaVariables(Script* scriptLambda)
{
	auto* eventList = GetParentEventList(scriptLambda);
	const auto iter = g_savedVarLists.find(eventList);
	if (iter == g_savedVarLists.end())
		return;
	auto& refCount = iter->second.refCount;
	const auto eventListCopy = iter->second.eventListCopy;
	--refCount;
	if (refCount == 0)
	{
		g_savedVarLists.erase(iter);
		if (eventListCopy)
			OtherHooks::DeleteEventList(eventListCopy);
	}
}

LambdaManager::LambdaVariableContext::LambdaVariableContext(Script* scriptLambda) : scriptLambda(scriptLambda)
{
	SaveLambdaVariables(scriptLambda);
}

LambdaManager::LambdaVariableContext::~LambdaVariableContext()
{
	UnsaveLambdaVariables(this->scriptLambda);
}