#pragma once

#include "Commands_InventoryRef.h"

#if RUNTIME

#include "GameObjects.h"
#include "GameAPI.h"
#include "GameForms.h"
#include "InventoryReference.h"
#include "GameRTTI.h"
#include "ArrayVar.h"
#include "GameScript.h"

bool Cmd_RemoveMeIR_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (thisObj) {
		InventoryReference* iref = InventoryReference::GetForRefID(thisObj->refID);
		if (iref) {
			TESObjectREFR* dest = NULL;
			if (ExtractArgs(EXTRACT_ARGS, &dest)) {
				if (dest) {
					*result = iref->MoveToContainer(dest) ? 1.0 : 0.0;
				}
				else {
					*result = iref->RemoveFromContainer() ? 1.0 : 0.0;
				}
			}
		}
	}

	return true;
}

bool Cmd_CopyIR_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (thisObj) {
		InventoryReference* iref = InventoryReference::GetForRefID(thisObj->refID);
		if (iref) {
			TESObjectREFR* dest = NULL;
			if (ExtractArgs(EXTRACT_ARGS, &dest) && dest) {
				*result = iref->CopyToContainer(dest) ? 1.0 : 0.0;
			}
		}
	}

	return true;
}

bool Cmd_CreateTempRef_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	TESForm* form = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &form) && form) {
		InventoryReference* iref = CreateInventoryRef(NULL, InventoryReference::Data(form, NULL, NULL), false);
		if (iref) {
			*refResult = iref->GetRef()->refID;
		}
	}

	return true;
}

struct InvRefArrayItem
{
	TESForm			*type;
	UInt32			next;
	Vector<UInt32>	refIDs;

	InvRefArrayItem() : type(NULL), next(0) {}
};

typedef UnorderedMap<UInt32, InvRefArrayItem> InvRefArray;
InvRefArray g_invRefArray;

bool Cmd_GetFirstRefForItem_Execute(COMMAND_ARGS)
{
	// builds an array of inventory references for the specified base object in the calling object's inventory, and returns the first element
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* item = NULL;

	InvRefArrayItem *invRefArr;
	if (!g_invRefArray.Insert(scriptObj->refID, &invRefArr))
	{
		invRefArr->type = NULL;
		invRefArr->next = 0;
		invRefArr->refIDs.Clear();
	}

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item)
	{
		invRefArr->type = item;

		// get count for base container
		TESContainer* cont = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESContainer);
		if (cont)
		{
			TESContainer::FormCount* pFound = cont->formCountList.Find(ContainerFindType(item));
			SInt32 baseCount = 0;
			if (pFound) {
				baseCount = (pFound->count > 0) ? pFound->count : 0;
			}
			
			// get container changes for refr
			ExtraContainerChanges* xChanges = (ExtraContainerChanges*)thisObj->extraDataList.GetByType(kExtraData_ContainerChanges);
			if (xChanges && xChanges->data && xChanges->data->objList)
			{
				// locate entry for this item type
				ExtraContainerChanges::EntryData* entry = xChanges->data->objList->Find(ItemInEntryDataListMatcher(item));

				// create temp refs for each stack
				if (entry)
				{
					ExtraContainerChanges::EntryData* ed = entry;
					baseCount += ed->countDelta;
					if (baseCount)
					{
						ExtraContainerChanges::ExtendDataList* extend = ed->extendData;
						ExtraDataList* xData = NULL;
						for (UInt32 index = 0; extend->Count()>index; index++)
						{
							xData = extend->GetNthItem(index);
							//if (!xData) {
							//	xData = ExtraDataList::Create();
							//}
							InventoryReference::Data dataIR(item, entry, xData);
							InventoryReference* iref = CreateInventoryRef(thisObj, dataIR, false);
							invRefArr->refIDs.Append(iref->GetRef()->refID);
							baseCount -= GetCountForExtraDataList(xData);
						}
					}
				}
			}

			if (baseCount > 0)
			{
				// create temp ref for items in base container not accounted for by container changes
				xChanges = ExtraContainerChanges::GetForRef(thisObj);
				if (!xChanges->data) {
					xChanges->data = ExtraContainerChanges::Data::Create(thisObj);
				}

				if (!xChanges->data->objList) {
					xChanges->data->objList = ExtraContainerChangesEntryDataListCreate();
				}

				// locate entry for this item type
				ExtraContainerChanges::EntryData* entry = xChanges->data->objList->Find(ItemInEntryDataListMatcher(item));

				if (!entry) {
					entry = ExtraContainerChanges::EntryData::Create(item->refID, baseCount);
				}

				Vector<InventoryReference::Data> refData;
				InventoryReference::Data::CreateForUnextendedEntry(entry, baseCount, refData);
				for (auto iter = refData.Begin(); !iter.End(); ++iter)
				{
					InventoryReference* iref = CreateInventoryRef(thisObj, *iter, false);
					invRefArr->refIDs.Append(iref->GetRef()->refID);
				}
			}
		}
	}
	if (!invRefArr->refIDs.Empty())
	{
		invRefArr->next = 1;
		*refResult = invRefArr->refIDs[0];
	}
	return true;
}

bool Cmd_GetNextRefForItem_Execute(COMMAND_ARGS)
{
	// returns an inventory references to the next stack of the specified base object in the calling object's inventory
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* item = NULL;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item)
	{
		InvRefArrayItem *invRefArr = g_invRefArray.GetPtr(scriptObj->refID);
		if (!invRefArr || (invRefArr->type != item))
			return Cmd_GetFirstRefForItem_Execute(PASS_COMMAND_ARGS);
		if (invRefArr->next < invRefArr->refIDs.Size())
		{
			*refResult = invRefArr->refIDs[invRefArr->next];
			invRefArr->next++;
		}
	}
	return true;
}

bool Cmd_GetInvRefsForItem_Execute(COMMAND_ARGS)
{
	// returns an array of inventory references for the specified base object in the calling object's inventory
	TESForm* item = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item)
	{
		double arrIndex = 0;

		// get count for base container
		TESContainer* cont = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESContainer);
		if (cont) {
			TESContainer::FormCount* pFound = cont->formCountList.Find(ContainerFindType(item));
			SInt32 baseCount = 0;
			if (pFound) {
				baseCount = (pFound->count > 0) ? pFound->count : 0;
			}
			
			// get container changes for refr
			ExtraContainerChanges* xChanges = (ExtraContainerChanges*)thisObj->extraDataList.GetByType(kExtraData_ContainerChanges);
			if (xChanges && xChanges->data && xChanges->data->objList) {
				// locate entry for this item type
				ExtraContainerChanges::EntryData* entry = xChanges->data->objList->Find(ItemInEntryDataListMatcher(item));

				// create temp refs for each stack
				if (entry) {
					ExtraContainerChanges::EntryData* ed = entry;
					baseCount += ed->countDelta;
					if (baseCount) {
						ExtraContainerChanges::ExtendDataList* extend = ed->extendData;
						ExtraDataList* xData = NULL;
						for (UInt32 index = 0; extend->Count()>index; index++) {
							xData = extend->GetNthItem(index);
							//if (!xData) {
							//	xData = ExtraDataList::Create();
							//}
							InventoryReference::Data dataIR(item, entry, xData);
							InventoryReference* iref = CreateInventoryRef(thisObj, dataIR, false);
							arr->SetElementFormID(arrIndex, iref->GetRef()->refID);
							arrIndex += 1;
							baseCount -= GetCountForExtraDataList(xData);
						}
					}
				}
			}

			if (baseCount > 0) {
				// create temp ref for items in base container not accounted for by container changes
				xChanges = ExtraContainerChanges::GetForRef(thisObj);
				if (!xChanges->data) {
					xChanges->data = ExtraContainerChanges::Data::Create(thisObj);
				}

				if (!xChanges->data->objList) {
					xChanges->data->objList = ExtraContainerChangesEntryDataListCreate();
				}

				// locate entry for this item type
				ExtraContainerChanges::EntryData* entry = xChanges->data->objList->Find(ItemInEntryDataListMatcher(item));

				if (!entry) {
					entry = ExtraContainerChanges::EntryData::Create(item->refID, baseCount);
				}

				Vector<InventoryReference::Data> refData;
				InventoryReference::Data::CreateForUnextendedEntry(entry, baseCount, refData);
				for (auto iter = refData.Begin(); !iter.End(); ++iter)
				{
					InventoryReference* iref = CreateInventoryRef(thisObj, *iter, false);
					arr->SetElementFormID(arrIndex, iref->GetRef()->refID);
					arrIndex += 1;
				}
			}
		}
	}

	return true;
}

#endif
