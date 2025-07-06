#include "FormExtraData.h"
#include <shared_mutex>
#include <ranges>
#include "SafeWrite.h"

namespace 
{
	std::unordered_map<TESForm*, std::vector<NiPointer<FormExtraData>>> g_formExtraDataMap;
	std::shared_mutex g_formExtraDataCS;
	UInt32 g_removeFromAllFormMapsAddr = 0x483C70;
}

bool FormExtraData::Add(TESForm* form, FormExtraData* formExtraData)
{
	if (!form || !formExtraData)
		return false;

	if (!formExtraData->name)
		return false;

	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end())
	{
		auto& dataList = iter->second;
		if (std::ranges::any_of(dataList, [formExtraData](const NiPointer<FormExtraData>& data) {
			return data && data->name == formExtraData->name;
		}))
		{
			return false; // Already exists
		}
	}

	g_formExtraDataMap[form].emplace_back(formExtraData);
	return true;
}

void FormExtraData::RemoveByName(TESForm* form, const char* name)
{
	if (!form || !name)
		return;

	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end())
	{
		auto& dataList = iter->second;

		std::erase_if(dataList, [name](const NiPointer<FormExtraData>& data) {
			return data && data->name == name;
		});

		if (dataList.empty())
			g_formExtraDataMap.erase(iter);
	}
}

void FormExtraData::RemoveByPtr(TESForm* form, FormExtraData* formExtraData) {
	if (!form || !formExtraData)
		return;

	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end())
	{
		auto& dataList = iter->second;

		std::erase_if(dataList, [formExtraData](const NiPointer<FormExtraData>& data) {
			return data == formExtraData;
		});

		if (dataList.empty())
			g_formExtraDataMap.erase(iter);
	}
}

FormExtraData* FormExtraData::Get(const TESForm* form, const char* name)
{
	if (!form || !name)
		return nullptr;

	std::shared_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(const_cast<TESForm*>(form));

	if (iter != g_formExtraDataMap.end())
	{
		for (const auto& data : iter->second) {
			if (data && data->name == name) {
				return data;
			}
		}
	}

	return nullptr;
}

UInt32 FormExtraData::GetAll(const TESForm* form, FormExtraData** outData) {
	if (!form)
		return 0;

	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(const_cast<TESForm*>(form));
	if (iter != g_formExtraDataMap.end())
	{
		const auto& dataList = iter->second;
		UInt32 count = static_cast<UInt32>(dataList.size());
		if (outData) {
			for (UInt32 i = 0; i < count; ++i) {
				outData[i] = dataList[i];
			}
		}
		return count;
	}

	return 0;
}

bool __fastcall RemoveFromAllFormsMapHook(TESForm* form) 
{
	std::unique_lock lock(g_formExtraDataCS);
	auto iter = g_formExtraDataMap.find(form);
	if (iter != g_formExtraDataMap.end())
	{
		for (auto& pair : iter->second) {
			pair->DecRefCount();
		}
		g_formExtraDataMap.erase(iter);
	}
	return ThisStdCall<bool>(g_removeFromAllFormMapsAddr, form);
}

void FormExtraData::WriteHooks()
{
	WriteRelCall(0x483669, &RemoveFromAllFormsMapHook, &g_removeFromAllFormMapsAddr);
}