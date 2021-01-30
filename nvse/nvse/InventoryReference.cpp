#include "InventoryReference.h"
#include "GameObjects.h"
#include "GameAPI.h"
#include "GameRTTI.h"

InventoryReferenceMap s_invRefMap(0x80);

void WriteToExtraDataList(BaseExtraList* from, BaseExtraList* to)
{
	if (from)
	{
		if (to)
			memcpy(&to->m_data, &from->m_data, 0x1C);
	}
	else if (to)
		memset(&to->m_data, 0, 0x1C);
}

InventoryReference::~InventoryReference()
{
	if (m_data.type)
	{
		Release();	
	}

	// note: TESObjectREFR::Destroy() frees up the formID for reuse
	if (m_tempRef)
	{
		m_tempRef->Destroy(true);
	}

	if (m_tempEntry)
	{
		if (m_tempEntry->extendData)
			GameHeapFree(m_tempEntry->extendData);
		GameHeapFree(m_tempEntry);
	}

	// remove unnecessary extra data, consolidate identical stacks
	if (m_containerRef)
	{
		ExtraContainerChanges* xChanges = (ExtraContainerChanges*)m_containerRef->extraDataList.GetByType(kExtraData_ContainerChanges);
		if (xChanges)
		{
			xChanges->Cleanup();
		}
	}
}

void InventoryReference::Release()
{
	DoDeferredActions();
	SetData(Data());
}

bool InventoryReference::SetData(const Data &data)
{
	ASSERT(m_tempRef);

	m_bRemoved = false;
	m_tempRef->baseForm = data.type;
	m_data = data;
	WriteToExtraDataList(data.xData, &m_tempRef->extraDataList);
	return true;
}

bool InventoryReference::WriteRefDataToContainer()
{
	if (!m_tempRef || !Validate())
		return false;
	else if (m_bRemoved)
		return true;

	if (m_data.xData)
		WriteToExtraDataList(&m_tempRef->extraDataList, m_data.xData);

	return true;
}

SInt32 InventoryReference::GetCount()
{
	SInt32 count = m_data.entry->countDelta;

	if (count < 0)
	{
		DEBUG_PRINT("Warning: InventoryReference::GetCount() found an object with a negative count (%d)", count);
	}

	return count;
}

bool InventoryReference::Validate()
{
	// it is possible that an inventory reference is created, and then commands like RemoveItem are used which modify the 
	// ExtraContainerChanges, potentially invalidating the InventoryReference::Data pointers
	// if m_bValidate is true, check for this occurrence before doing stuff with the temp ref
	// doing this in foreach loops is going to make them really slooooow.

	if (!m_bDoValidation)
		return true;

	ExtraContainerChanges* xChanges = (ExtraContainerChanges*)m_containerRef->extraDataList.GetByType(kExtraData_ContainerChanges);
	if (xChanges && xChanges->data)
	{
		for (ExtraContainerChanges::EntryDataList::Iterator cur = xChanges->data->objList->Begin(); !cur.End(); ++cur)
		{
			if (cur.Get() && cur.Get()->type == m_data.entry->type)
			{
				for (ExtraContainerChanges::ExtendDataList::Iterator ed = cur.Get()->extendData->Begin(); !ed.End(); ++ed)
				{
					if (ed.Get() == m_data.xData)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

InventoryReference* InventoryReference::GetForRefID(UInt32 refID)
{
	InventoryReference *invRefr = s_invRefMap.GetPtr(refID);
	if (invRefr && invRefr->Validate())
		return invRefr;
	return NULL;
}

bool InventoryReference::RemoveFromContainer()
{
	if (Validate() && m_tempRef && m_containerRef)
	{
		if (m_data.xData && m_data.xData->IsWorn())
		{
			m_deferredActions.Push(kAction_Remove, m_data);
			return true;
		}
		SetRemoved();
		if (m_data.xData)
			return m_data.entry->Remove(m_data.xData, true);
		else
		{
			ExtraContainerChanges* xcc = (ExtraContainerChanges*)m_containerRef->extraDataList.GetByType(kExtraData_ContainerChanges);
			return xcc->Remove(m_data.type);
		}
	}
	return false;
}

bool InventoryReference::MoveToContainer(TESObjectREFR* dest)
{
	ExtraContainerChanges* xChanges = ExtraContainerChanges::GetForRef(dest);
	if (xChanges && Validate() && m_tempRef && m_containerRef)
	{
		m_deferredActions.Push(kAction_Remove, m_data, dest);
		return true;
	}
	return false;
}

bool InventoryReference::CopyToContainer(TESObjectREFR* dest)
{
	ExtraContainerChanges* xChanges = ExtraContainerChanges::GetForRef(dest);
	if (xChanges && Validate() && m_tempRef)
	{
		ExtraDataList* newDataList = ExtraDataList::Create();
		newDataList->Copy(&m_tempRef->extraDataList);

		// if worn, mark as unequipped
		newDataList->RemoveByType(kExtraData_Worn);

		if (xChanges->Add(m_tempRef->baseForm, newDataList))
			return true;
	}
	return false;
}

bool InventoryReference::SetEquipped(bool bEquipped)
{
	if (m_data.xData)
	{
		if (m_data.xData->IsWorn() == bEquipped)
			return false;
	}
	else if (!bEquipped)
		return false;
	m_deferredActions.Push(kAction_Equip, m_data);
	return true;
}

void InventoryReference::DoDeferredActions()
{
	while (!m_deferredActions.Empty())
	{
		DeferredAction* action = &m_deferredActions.Top();
		SetData(action->Data());
		if (Validate() && GetContainer())
			action->Execute(this);
		m_deferredActions.Pop();
	}
}

bool InventoryReference::DeferredAction::Execute(InventoryReference* iref)
{
	TESObjectREFR* containerRef = iref->GetContainer();
	const InventoryReference::Data& data = Data();
	switch (m_type)
	{
		case kAction_Equip:
		{
			Actor* actor = (Actor*)containerRef;
			if (!actor->IsActor())
				return false;
			if (data.xData && data.xData->IsWorn())
				actor->UnequipItem(data.type, 1, data.xData, 0, false, 0);
			else
				actor->EquipItem(data.type, (data.type->typeID == kFormType_TESAmmo) ? iref->GetCount() : 1, data.xData, 1, false);
			return true;
		}
		case kAction_Remove:
		{
			containerRef->RemoveItem(data.type, data.xData, iref->GetCount(), 0, 0, m_target, 0, 0, 1, 0);
			iref->SetRemoved();
			iref->SetData(InventoryReference::Data());
			return true;
		}
		default:
			return false;
	}
}

void InventoryReference::Data::CreateForUnextendedEntry(ExtraContainerChanges::EntryData* entry, SInt32 totalCount, Vector<Data> &dataOut)
{
	if (totalCount < 1) {
		return;
	}

	ExtraDataList* xDL;

	// create stacks of max 32767 as that's the max ExtraCount::count can hold
	while (totalCount > 32767)
	{
		xDL = ExtraDataList::Create(ExtraCount::Create(32767));
		dataOut.Append(entry->type, entry, xDL);
		totalCount -= 32767;
	}

	if (totalCount > 0)
	{
		if (totalCount > 1) 
			xDL = ExtraDataList::Create(ExtraCount::Create(totalCount));
		else
			xDL = NULL;
		dataOut.Append(entry->type, entry, xDL);
	}
}

InventoryReference *CreateInventoryRef(TESObjectREFR *container, const InventoryReference::Data &data, bool bValidate)
{
	TESObjectREFR *refr = TESObjectREFR::Create(false);
	InventoryReference *invRefr;
	s_invRefMap.Insert(refr->refID, &invRefr);
	invRefr->m_containerRef = container;
	invRefr->m_tempRef = refr;
	invRefr->m_tempEntry = NULL;
	invRefr->m_bDoValidation = bValidate;
	invRefr->m_bRemoved = false;
	invRefr->SetData(data);
	return invRefr;
}

ExtraContainerChanges::EntryData *CreateTempEntry(TESForm *itemForm, SInt32 countDelta, ExtraDataList *xData)
{
	ExtraContainerChanges::EntryData *entry = (ExtraContainerChanges::EntryData*)GameHeapAlloc(sizeof(ExtraContainerChanges::EntryData));
	if (xData)
	{
		entry->extendData = (ExtraContainerChanges::ExtendDataList*)GameHeapAlloc(8);
		entry->extendData->Init(xData);
	}
	else entry->extendData = NULL;
	entry->countDelta = countDelta;
	entry->type = itemForm;
	return entry;
}

TESObjectREFR* CreateInventoryRefEntry(TESObjectREFR *container, TESForm *itemForm, SInt32 countDelta, ExtraDataList *xData)
{
	ExtraContainerChanges::EntryData *entry = CreateTempEntry(itemForm, countDelta, xData);
	InventoryReference *invRef = CreateInventoryRef(container, InventoryReference::Data(itemForm, entry, xData), false);
	invRef->m_tempEntry = entry;
	return invRef->m_tempRef;
}