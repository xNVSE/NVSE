#include "FormExtraData.h"
#include <shared_mutex>
#include <ranges>
#include "SafeWrite.h"

namespace 
{
	std::unordered_map<TESForm*, std::vector<std::pair<const char*, FormExtraData*>>> g_formExtraDataMap;
	std::shared_mutex g_formExtraDataCS;
	UInt32 g_removeFromAllFormMapsAddr = 0x483C70;
}

void FormExtraData::Add(TESForm* form, const char* name, FormExtraData* formExtraData)
{
	std::unique_lock lock(g_formExtraDataCS);
	g_formExtraDataMap[form].emplace_back(std::make_pair(name, formExtraData));
}

void FormExtraData::Remove(TESForm* form, const char* name)
{
	std::unique_lock lock(g_formExtraDataCS);
	auto& datum = g_formExtraDataMap[form];
	auto iter = std::ranges::find_if(datum, [&](auto& iter) 
	{
		return iter.first == name;
	});
	if (iter != datum.end())
	{
		auto* data = iter->second;
		data->~FormExtraData();
		FormHeap_Free(data);
		datum.erase(iter);
	}
}

FormExtraData* FormExtraData::Get(TESForm* form, const char* name)
{
	std::shared_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end()) 
	{
		auto it = std::ranges::find_if(iter->second, [&](auto& item) 
		{
			return item.first == name;
		});
		return it->second;
	}
	return nullptr;
}

bool __fastcall RemoveFromAllFormsMapHook(TESForm* form) 
{
	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end())
	{
		for (auto& pair : iter->second)
		{
			auto* data = pair.second;
			data->~FormExtraData();
			FormHeap_Free(data);
		}
		g_formExtraDataMap.erase(iter);
	}
	return ThisStdCall<bool>(g_removeFromAllFormMapsAddr, form);
}

void FormExtraData::WriteHooks()
{
	WriteRelCall(0x483669, &RemoveFromAllFormsMapHook, &g_removeFromAllFormMapsAddr);
}