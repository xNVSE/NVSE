#include "InventoryRef.h"
#include "GameObjects.h"

#if 0
// todo: fix this code copied from JIP LN NVSE. Has some missing functions that would need to be ported over.

ExtraDataList* InventoryRef::CreateExtraData()
{
	ExtraContainerChanges::EntryDataList* entryList = containerRef->GetContainerChangesList();
	if (!entryList) return nullptr;
	ExtraContainerChanges::EntryData* pEntry = entryList->FindForItem(type);
	if (!pEntry)
	{
		pEntry = ExtraContainerChanges::EntryData::Create(type, 0);
		entryList->Prepend(pEntry);
	}
	xData = ExtraDataList::Create();
	if (pEntry->extendData)
		pEntry->extendData->Prepend(xData);
	else
	{
		pEntry->extendData = Game_HeapAlloc<ContChangesExtraList>();
		pEntry->extendData->Init(xData);
	}
	containerRef->MarkAsModified(0x20);
	return xData;
}

ExtraDataList* __fastcall InventoryRef::SplitFromStack(SInt32 maxStack)
{
	if (auto* xCount = (ExtraCount*)xData->GetByType(kExtraData_Count))
	{
		SInt32 delta = xCount->count - maxStack;
		if (delta <= 0)
			return xData;
		ExtraContainerChanges::EntryData* pEntry = containerRef->GetContainerChangesEntry(type);
		if (!pEntry) return xData;
		ExtraDataList* xDataOut = xData->CreateCopy();
		pEntry->extendData->Prepend(xDataOut);
		xDataOut->RemoveByType(kExtraData_Worn);
		xDataOut->RemoveByType(kExtraData_Hotkey);
		if (delta < 2)
		{
			xData->RemoveByType(kExtraData_Count);
			if (!xData->m_data)
			{
				pEntry->extendData->Remove(xData);
				Game_HeapFree(xData);
			}
		}
		else xCount->count = delta;
		xData = xDataOut;
		if (maxStack > 1)
		{
			xCount = (ExtraCount*)xData->GetByType(kExtraData_Count);
			xCount->count = maxStack;
		}
		else xData->RemoveByType(kExtraData_Count);
	}
	else if (maxStack > 1)
		xData->AddExtraCount(maxStack);
	return xData;
}

#endif