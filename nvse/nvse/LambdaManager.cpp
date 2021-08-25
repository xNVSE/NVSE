#include "LambdaManager.h"
#include "common/ICriticalSection.h"
#include <ranges>

#include "Commands_Scripting.h"
#include "GameAPI.h"
#include "GameData.h"
#include "GameObjects.h"
#include "Hooks_Other.h"

using ScriptLambda = Script;
using FormID = UInt32;

struct VariableListContext
{
	ScriptEventList* eventListCopy = nullptr;
	std::unordered_map<ScriptLambda*, std::size_t> lambdas;
};

// used for memoizing scripts and prevent endless allocation of scripts
static std::map<std::pair<UInt8*, FormID>, ScriptLambda*> g_lambdaScriptPosMap;
std::unordered_map<ScriptEventList*, VariableListContext> g_savedVarLists;

struct LambdaContext
{
	Script* parentScript = nullptr;
	ScriptEventList* parentEventList = nullptr;
	// any lambdas that are referenced within this lambda
	std::unique_ptr<std::vector<ScriptLambda*>> capturedLambdaVariableScripts;

#if _DEBUG
	std::string name;
	TESForm* ref = nullptr;
#endif
};

// contains all lambdas
static std::unordered_map<ScriptLambda*, LambdaContext> g_lambdas;

// avoid concurrency issues
ICriticalSection LambdaManager::g_lambdaCs;

LambdaContext* GetLambdaContext(ScriptLambda* scriptLambda)
{
	if (auto iter = g_lambdas.find(scriptLambda); iter != g_lambdas.end())
		return &iter->second;
	return nullptr;
}

void SetLambdaParent(LambdaContext& ctx, ScriptEventList* parentEventList)
{
	ctx.parentEventList = parentEventList;
}

void SetLambdaParent(ScriptLambda* scriptLambda, ScriptEventList* parentEventList)
{
	SetLambdaParent(g_lambdas[scriptLambda], parentEventList);
}

Script* LambdaManager::CreateLambdaScript(const LambdaManager::ScriptData& scriptData, const Script* parentScript)
{
	DataHandler::Get()->DisableAssignFormIDs(true);
	auto* scriptLambda = New<Script, 0x5AA0F0>();
	DataHandler::Get()->DisableAssignFormIDs(false);
	scriptLambda->data = scriptData.scriptData;
	scriptLambda->info.dataLength = scriptData.size;
	
	//scriptLambda->varList = parentScript->varList; // CdeclCall(0x5AB930, &parentScript->varList, &scriptLambda->varList); // CopyVarList
	//scriptLambda->refList = parentScript->refList; // CdeclCall(0x5AB7F0, &parentScript->refList, &scriptLambda->refList); // CopyRefList
	CdeclCall(0x5AB930, &parentScript->varList, &scriptLambda->varList);
	CdeclCall(0x5AB7F0, &parentScript->refList, &scriptLambda->refList);

	scriptLambda->info.numRefs = parentScript->info.numRefs;
	scriptLambda->info.varCount = parentScript->info.varCount;
	scriptLambda->info.unusedVariableCount = parentScript->info.unusedVariableCount;

	parentScript->mods.ForEach(_L(ModInfo* info, scriptLambda->mods.Append(info)));
	return scriptLambda;
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
	else if (auto* ownerRef = OtherHooks::GetExecutingScriptContext()->scriptOwnerRef)
		ownerRefID = ownerRef->refID;
	else
		ownerRefID = parentScript->refID;
	
	// auto script = MakeUnique<Script, 0x5AA0F0, 0x5AA1A0>();
	const auto key = std::make_pair(position, ownerRefID);
	if (const auto iter = g_lambdaScriptPosMap.find(key); iter != g_lambdaScriptPosMap.end())
	{
		auto* scriptLambda = iter->second;
		SetLambdaParent(scriptLambda, parentEventList);
		return scriptLambda;
	}
	auto* scriptLambda = CreateLambdaScript(scriptData, parentScript);
	g_lambdaScriptPosMap[key] = scriptLambda;
	const auto nextFormId = GetNextFreeFormID(parentScript->refID);
	if (nextFormId >> 24 == parentScript->GetModIndex())
	{
		scriptLambda->SetRefID(nextFormId, true);
	}
	auto iter = g_lambdas.emplace(scriptLambda, LambdaContext());
	auto& ctx = iter.first->second;
	SetLambdaParent(ctx, parentEventList);
	ctx.parentScript = parentScript;
#if _DEBUG
	ctx.name = std::string(parentScript->GetName()) + "LambdaAt" + std::to_string(*exprEval.m_opcodeOffsetPtr);
	scriptLambda->SetEditorID(ctx.name.c_str());
	ctx.ref = LookupFormByID(ownerRefID);
#endif
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

// Hot reload function
void LambdaManager::DeleteAllForParentScript(Script* parentScript)
{
	ScopedLock lock(g_lambdaCs);
	std::vector<Script*> lambdas;
	int numLambdas = 0;
	for (auto& [scriptLambda, ctx] : g_lambdas)
	{
		if (ctx.parentScript == parentScript)
		{
			FormHeap_Free(scriptLambda->data); // ThisStdCall(0x5AA1A0, scriptLambda); // call destructor to free parentScript data pointer
			lambdas.push_back(scriptLambda);
			ctx.parentEventList = nullptr;
			scriptLambda->data = nullptr; // parentScript data and tLists will be freed but parentScript won't be since plugins may store pointers to it to call
			scriptLambda->varList = {};
			scriptLambda->refList = {};
			++numLambdas;
		}
	}
	for (auto& ctx : g_lambdas | std::views::values)
	{
		if (!ctx.capturedLambdaVariableScripts)
			continue;
		auto iter = std::ranges::find_if(*ctx.capturedLambdaVariableScripts, [&](Script* script)
		{
			return std::ranges::find(lambdas, script) != lambdas.end();
		});
		if (iter != ctx.capturedLambdaVariableScripts->end())
			ctx.capturedLambdaVariableScripts = nullptr;
	}
	std::erase_if(g_lambdaScriptPosMap, [&](auto& p) { return std::ranges::find(lambdas, p.second) != lambdas.end(); });
	std::erase_if(g_lambdas, [&](auto& p)
	{
		return std::ranges::find(lambdas, p.first) != lambdas.end();
	});
	g_savedVarLists.clear();

	if (numLambdas)
	{
		Console_Print("Invalidated %d lambda(s) which need to be re-initialized", numLambdas);
	}
}

void LambdaManager::ClearSavedDeletedEventLists()
{
	// reserved for future use
}

auto GetVarListContextIter(Script* scriptLambda)
{
	return std::ranges::find_if(g_savedVarLists, [&](auto& p)
	{
		return p.second.lambdas.find(scriptLambda) != p.second.lambdas.end();
	});
	
}

VariableListContext* GetVarListContext(Script* scriptLambda)
{
	const auto varListCopyIter = GetVarListContextIter(scriptLambda);
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
	for (auto& ctx : g_lambdas | std::views::values)
	{
		if (ctx.parentEventList == eventList)
			ctx.parentEventList = nullptr;
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

void CaptureChildLambdas(ScriptLambda* scriptLambda, LambdaContext& ctx)
{
	// save any children lambda's variable lists
	auto* eventList = ctx.parentEventList;
	if (ctx.capturedLambdaVariableScripts)
		return;
	
	// cached since O(n^2)
	auto varLambdas = std::make_unique<std::vector<ScriptLambda*>>();
	for (auto* ref : scriptLambda->refList)
	{
		if (!ref || !ref->varIdx)
			continue;
		ref->Resolve(eventList);
		auto* form = ref->form;
		if (form && IS_ID(form, Script) && LambdaManager::IsScriptLambda(static_cast<ScriptLambda*>(form)) && form != scriptLambda)
		{
			auto* childLambda = static_cast<ScriptLambda*>(form);
			if (LambdaManager::GetParentEventList(scriptLambda) != ctx.parentEventList) // do not capture the variables of a lambda which you share variables with
				varLambdas->push_back(childLambda);
		}
	}
	ctx.capturedLambdaVariableScripts = std::move(varLambdas);
}

void SaveLambdaVariables(Script* scriptLambda, Script* parentLambda)
{
	auto* ctx = GetLambdaContext(scriptLambda);
	if (!ctx)
		return;
	auto* eventList = ctx->parentEventList;
	if (!eventList)
		return;
	auto& varCtx = g_savedVarLists[eventList];
	++varCtx.lambdas[scriptLambda];

	// save any child lambda variables
	CaptureChildLambdas(scriptLambda, *ctx);
	for (auto* childLambda : *ctx->capturedLambdaVariableScripts)
	{
		if (childLambda != parentLambda)
			SaveLambdaVariables(childLambda, scriptLambda);
	}
}

void LambdaManager::SaveLambdaVariables(Script* scriptLambda)
{
	::SaveLambdaVariables(scriptLambda, nullptr);
}


void DeleteLambdaScript(auto& iter)
{
	Script* scriptLambda = iter->first;
	g_lambdas.erase(scriptLambda);
	scriptLambda->Destroy(true);
}

void UnsaveLambdaVariables(Script* scriptLambda, Script* parentScript)
{
	auto* ctx = GetLambdaContext(scriptLambda);
	if (!ctx)
		return;
	auto iter = GetVarListContextIter(scriptLambda);
	if (iter == g_savedVarLists.end())
		return;
	auto& varCtx = iter->second;
	auto refCountIter = varCtx.lambdas.find(scriptLambda);
	auto& refCount = refCountIter->second;
	if (--refCount <= 0)
		varCtx.lambdas.erase(refCountIter);

	const auto eventListCopy = varCtx.eventListCopy;

	// before proceeding check if there were any child lambdas referenced, delete their event list
	for (auto* childLambda : *ctx->capturedLambdaVariableScripts)
	{
		if (childLambda != parentScript)
			UnsaveLambdaVariables(childLambda, scriptLambda);
	}

	if (varCtx.lambdas.empty())
	{
		g_savedVarLists.erase(iter);
		if (eventListCopy)
			OtherHooks::DeleteEventList(eventListCopy);
	}
}

void LambdaManager::UnsaveLambdaVariables(Script* scriptLambda)
{
	UnsaveLambdaVariables(scriptLambda, nullptr);
}

LambdaManager::LambdaVariableContext::LambdaVariableContext(Script* scriptLambda) : scriptLambda(scriptLambda)
{
	if (scriptLambda)
		SaveLambdaVariables(scriptLambda);
}

// prevent reallocation from destroying other lambda context
LambdaManager::LambdaVariableContext::LambdaVariableContext(LambdaVariableContext&& other) noexcept: scriptLambda(
	other.scriptLambda)
{
	other.scriptLambda = nullptr;
}

LambdaManager::LambdaVariableContext& LambdaManager::LambdaVariableContext::operator=(
	LambdaVariableContext&& other) noexcept
{
	if (this == &other)
		return *this;
	if (this->scriptLambda)
		UnsaveLambdaVariables(this->scriptLambda);
	scriptLambda = other.scriptLambda;
	other.scriptLambda = nullptr;
	return *this;
}

LambdaManager::LambdaVariableContext::~LambdaVariableContext()
{
	if (this->scriptLambda)
		UnsaveLambdaVariables(this->scriptLambda);
}
