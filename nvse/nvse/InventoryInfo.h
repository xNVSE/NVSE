#pragma once

#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "GameExtraData.h"

// Introduced to be available to plugin that needs to manipulate inventory

typedef Vector<ExtraContainerChanges::EntryData*> ExtraDataVec;
typedef UnorderedMap<TESForm*, UInt32> ExtraContainerMap;

void PrintItemType(TESForm * form);
TESForm* GetItemByIdx(TESObjectREFR* thisObj, UInt32 objIdx, SInt32* outNumItems);
TESForm* GetItemByRefID(TESObjectREFR* thisObj, UInt32 refID, SInt32* outNumItems = NULL);
TESForm* GetItemWithHealthAndOwnershipByRefID(TESObjectREFR* thisObj, UInt32 refID, float* outHealth, TESForm** outOwner, UInt32* outRank, SInt32* inOutIndex = NULL,
	SInt32* outNumItems = NULL);	// returns the inOutIndex stack, or the first if it is NULL
TESForm * SetFirstItemWithHealthAndOwnershipByRefID(TESObjectREFR* thisObj, UInt32 refID, SInt32 NumItems = 1, float Health = -1.0, TESForm* pOwner = NULL, UInt32 Rank = 0);

class ExtraContainerInfo
{
	ExtraDataVec		m_vec;
	ExtraContainerMap	m_map;
public:
	ExtraContainerInfo(ExtraContainerChanges::EntryDataList * entryList) : m_vec(128)
	{
		if (entryList) {
			entryList->Visit(*this);
		}
	}

	bool Accept(ExtraContainerChanges::EntryData* data) 
	{
		if (data)
		{
			m_map[data->type] = m_vec.Size();
			m_vec.Append(data);
		}
		return true;
	}

	bool IsValidFormCount(TESContainer::FormCount* formCount, SInt32& numObjects)
	{
		if (formCount)
		{
			numObjects = formCount->count;
			TESForm* pForm = formCount->form;

			if (DYNAMIC_CAST(pForm, TESForm, TESLevItem))
				return false;

			UInt32 *index = m_map.GetPtr(pForm);
			if (index)
			{
				ExtraContainerChanges::EntryData* pXData = m_vec[*index];
				if (pXData) {
					numObjects += pXData->countDelta;
				}
				// clear the object from the vector so we don't bother to look for it
				// in the second step
				m_vec[*index] = NULL;
			}

			if (numObjects > 0) {
				if (IsConsoleMode()) {
					PrintItemType(pForm);
				}
				return true;
			}
		}
		return false;
	}

	// returns the count of items left in the vector
	UInt32 CountItems()
	{
		UInt32 count = 0;
		ExtraContainerChanges::EntryData *extraData;
		for (auto iter = m_vec.Begin(); !iter.End(); ++iter)
		{
			extraData = *iter;
			if (extraData && (extraData->countDelta > 0))
			{
				count++;
				if (IsConsoleMode())
					PrintItemType(extraData->type);
			}
		}
		return count;
	}

	ExtraContainerChanges::EntryData* GetNth(UInt32 n, UInt32 count)
	{
		ExtraContainerChanges::EntryData *extraData;
		for (auto iter = m_vec.Begin(); !iter.End(); ++iter)
		{
			extraData = *iter;
			if (extraData && (extraData->countDelta > 0))
			{
				if (count == n)
					return extraData;
				count++;
			}
		}
		return NULL;
	}

	ExtraContainerChanges::EntryData* GetRefID(SInt32 refID)
	{
		ExtraContainerChanges::EntryData *extraData;
		for (auto iter = m_vec.Begin(); !iter.End(); ++iter)
		{
			extraData = *iter;
			if (extraData && (extraData->countDelta > 0) && extraData->type && (extraData->type->refID == refID))
				return extraData;
		}
		return NULL;
	}

};

class ContainerCountIf
{
	ExtraContainerInfo& m_info;
public:
	ContainerCountIf(ExtraContainerInfo& info) : m_info(info) { }

	bool Accept(TESContainer::FormCount* formCount) const
	{
		SInt32 numObjects = 0; // not needed in this count
		return m_info.IsValidFormCount(formCount, numObjects);
	}
};

class ContainerFindNth
{
	ExtraContainerInfo& m_info;
	UInt32 m_findIndex;
	UInt32 m_curIndex;
public:
	ContainerFindNth(ExtraContainerInfo& info, UInt32 findIndex) : m_info(info), m_findIndex(findIndex), m_curIndex(0) { }

	bool Accept(TESContainer::FormCount* formCount)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidFormCount(formCount, numObjects)) {
			if (m_curIndex == m_findIndex) {
				return true;
			}
			m_curIndex++;
		}
		return false;
	}

	UInt32 GetCurIdx() { return m_curIndex; }
};

class ContainerFindType
{
	TESForm* m_findType;
public:
	ContainerFindType(TESForm* type) : m_findType(type) { }

	bool Accept(TESContainer::FormCount* formCount)
	{
		return (formCount && formCount->form == m_findType);
	}

};

class ContainerFindRefId
{
	ExtraContainerInfo& m_info;
	UInt32 m_findRefId;
	UInt32 m_curIndex;
public:
	ContainerFindRefId(ExtraContainerInfo& info, SInt32 findRefId) : m_info(info), m_findRefId(findRefId), m_curIndex(0) { }

	bool Accept(TESContainer::FormCount* formCount)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidFormCount(formCount, numObjects)) {
			if (formCount->form->refID == m_findRefId) {
				return true;
			}
			m_curIndex++;
		}
		return false;
	}

	UInt32 GetCurIdx() { return m_curIndex; }
};

bool SameHealth(ExtraHealth* pXHealth, TESHealthForm* pHealth, float Health);
bool SameOwner(ExtraOwnership* pXOwner, ExtraRank* pXRank, TESForm* pOwner, UInt32 Rank);

TESForm * AddItemHealthPercentOwner(TESObjectREFR* thisObj, UInt32 refID, SInt32 NumItems = 1, float Health = 100.0, TESForm* pOwner = NULL, UInt32 Rank = 0);
