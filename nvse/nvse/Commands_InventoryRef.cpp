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

bool Cmd_CopyIRAlt_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	if (thisObj) {
		InventoryReference* iref = InventoryReference::GetForRefID(thisObj->refID);
		if (iref) {
			TESObjectREFR* dest = NULL;
			if (ExtractArgs(EXTRACT_ARGS, &dest) && dest) {
				InventoryReference* invRefResult;
				iref->CopyToContainer(dest, &invRefResult);
				*refResult = invRefResult ? invRefResult->GetRef()->refID : 0;
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
		TESContainer *cont = thisObj->GetContainer();
		if (cont)
		{
			ExtraContainerChanges *xChanges = (ExtraContainerChanges*)thisObj->extraDataList.GetByType(kExtraData_ContainerChanges);
			ExtraContainerChanges::EntryDataList *entryList = (xChanges && xChanges->data) ? xChanges->data->objList : NULL;
			if (entryList)
			{
				SInt32 baseCount = cont->GetCountForForm(item);
				ExtraContainerChanges::EntryData *entry = entryList->FindForItem(item);
				TESObjectREFR *invRef;
				if (entry)
				{
					SInt32 xCount = entry->countDelta;
					if (baseCount)
					{
						if (entry->HasExtraLeveledItem())
							baseCount = xCount;
						else baseCount += xCount;
					}
					else baseCount = xCount;

					if ((baseCount > 0) && entry->extendData)
					{
						ExtraDataList *xData;
						for (auto xdlIter = entry->extendData->Begin(); !xdlIter.End(); ++xdlIter)
						{
							xData = xdlIter.Get();
							xCount = GetCountForExtraDataList(xData);
							if (xCount < 1) continue;
							if (xCount > baseCount)
								xCount = baseCount;
							baseCount -= xCount;
							invRef = CreateInventoryRefEntry(thisObj, item, xCount, xData);
							invRefArr->refIDs.Append(invRef->refID);
							if (!baseCount) break;
						}
					}
				}

				if (baseCount > 0)
				{
					invRef = CreateInventoryRefEntry(thisObj, item, baseCount, NULL);
					invRefArr->refIDs.Append(invRef->refID);
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
		TESContainer *cont = thisObj->GetContainer();
		if (!cont) return true;
		ExtraContainerChanges *xChanges = (ExtraContainerChanges*)thisObj->extraDataList.GetByType(kExtraData_ContainerChanges);
		ExtraContainerChanges::EntryDataList *entryList = (xChanges && xChanges->data) ? xChanges->data->objList : NULL;
		if (!entryList) return true;

		double arrIndex = 0;
		SInt32 baseCount = cont->GetCountForForm(item);
		ExtraContainerChanges::EntryData *entry = entryList->FindForItem(item);
		TESObjectREFR *invRef;
		if (entry)
		{
			SInt32 xCount = entry->countDelta;
			if (baseCount)
			{
				if (entry->HasExtraLeveledItem())
					baseCount = xCount;
				else baseCount += xCount;
			}
			else baseCount = xCount;

			if ((baseCount > 0) && entry->extendData)
			{
				ExtraDataList *xData;
				for (auto xdlIter = entry->extendData->Begin(); !xdlIter.End(); ++xdlIter)
				{
					xData = xdlIter.Get();
					xCount = GetCountForExtraDataList(xData);
					if (xCount < 1) continue;
					if (xCount > baseCount)
						xCount = baseCount;
					baseCount -= xCount;
					invRef = CreateInventoryRefEntry(thisObj, item, xCount, xData);
					arr->SetElementFormID(arrIndex, invRef->refID);
					arrIndex += 1;
					if (!baseCount) break;
				}
			}
		}

		if (baseCount > 0)
		{
			invRef = CreateInventoryRefEntry(thisObj, item, baseCount, NULL);
			arr->SetElementFormID(arrIndex, invRef->refID);
		}
	}

	return true;
}

bool Cmd_IsInventoryRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (thisObj) {
		*result = InventoryReference::GetForRefID(thisObj->refID) != nullptr;
	}
	return true;
}

#endif
