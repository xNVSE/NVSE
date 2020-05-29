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
		InventoryReference* iref = InventoryReference::Create(NULL, InventoryReference::Data(form, NULL, NULL), false);
		if (iref) {
			*refResult = iref->GetRef()->refID;
		}
	}

	return true;
}

#if 0
#error Old Version

struct InvRefArrayItem {
	TESForm* type;
	UInt32 last;
	std::vector<UInt32> refIDs;
};

typedef std::map<UInt32, InvRefArrayItem> InvRefArray;
InvRefArray g_invRefArray;

bool Cmd_GetFirstRefForItem_Execute(COMMAND_ARGS)
{
	// builds an array of inventory references for the specified base object in the calling object's inventory, and returns the first element
	TESForm* item = NULL;
	*result = 0;
	UInt32 idxRef = 0;
	UInt32 idx = scriptObj->refID;
	g_invRefArray.erase(idx);
	g_invRefArray[idx].type = NULL;
	g_invRefArray[idx].last = 0;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item) {
		g_invRefArray[idx].type = item;

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
			if (xChanges && xChanges->data) {
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
							InventoryReference* iref = InventoryReference::Create(thisObj, dataIR, false);
							g_invRefArray[idx].refIDs[idxRef] = iref->GetRef()->refID;
							idxRef += 1.0;
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

				std::vector<InventoryReference::Data> refData;
				InventoryReference::Data::CreateForUnextendedEntry(entry, baseCount, refData);
				for (std::vector<InventoryReference::Data>::iterator iter = refData.begin(); iter != refData.end(); ++iter) {
					InventoryReference* iref = InventoryReference::Create(thisObj, *iter, false);
					g_invRefArray[idx].refIDs[idxRef] = iref->GetRef()->refID;
					idxRef += 1.0;
				}
			}
		}
	}
	if (idxRef>0) {
		g_invRefArray[idx].last = 0;
		*result = g_invRefArray[idx].refIDs[0];
	}
	return true;
}

bool Cmd_GetNextRefForItem_Execute(COMMAND_ARGS)
{
	// returns an inventory references to the next stack of the specified base object in the calling object's inventory
	TESForm* item = NULL;
	*result = 0;
	UInt32 idx = scriptObj->refID;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item) {
		if ((g_invRefArray.find(idx)==g_invRefArray.end()) || (g_invRefArray[idx].type != item))	// we have to rebuild the array
			return Cmd_GetFirstRefForItem_Execute(PASS_COMMAND_ARGS);
		if (g_invRefArray[idx].last<g_invRefArray[idx].refIDs.size()) {
			*result = g_invRefArray[idx].refIDs[g_invRefArray[idx].last];
			g_invRefArray[idx].last += 1.0;
		}
	}
	return true;

}
#endif

struct InvRefArrayItem {
	TESForm* type;
	UInt32 next;
	std::vector<UInt32> refIDs;
};

typedef std::map<UInt32, InvRefArrayItem> InvRefArray;
InvRefArray g_invRefArray;

bool Cmd_GetFirstRefForItem_Execute(COMMAND_ARGS)
{
	// builds an array of inventory references for the specified base object in the calling object's inventory, and returns the first element
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* item = NULL;
	UInt32 idx = scriptObj->refID;
	g_invRefArray.erase(idx);
	g_invRefArray[idx].type = NULL;
	g_invRefArray[idx].next = 0;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item) {
		g_invRefArray[idx].type = item;

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
							InventoryReference* iref = InventoryReference::Create(thisObj, dataIR, false);
							g_invRefArray[idx].refIDs.push_back(iref->GetRef()->refID);
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

				std::vector<InventoryReference::Data> refData;
				InventoryReference::Data::CreateForUnextendedEntry(entry, baseCount, refData);
				for (std::vector<InventoryReference::Data>::iterator iter = refData.begin(); iter != refData.end(); ++iter) {
					InventoryReference* iref = InventoryReference::Create(thisObj, *iter, false);
					g_invRefArray[idx].refIDs.push_back(iref->GetRef()->refID);
				}
			}
		}
	}
	if (g_invRefArray[idx].refIDs.size() > 0) {
		g_invRefArray[idx].next = 1;
		*refResult = g_invRefArray[idx].refIDs[0];
	}
	return true;
}

bool Cmd_GetNextRefForItem_Execute(COMMAND_ARGS)
{
	// returns an inventory references to the next stack of the specified base object in the calling object's inventory
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* item = NULL;
	UInt32 idx = scriptObj->refID;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item) {
		if ((g_invRefArray.find(idx)==g_invRefArray.end()) || (g_invRefArray[idx].type != item))	// we have to rebuild the array
			return Cmd_GetFirstRefForItem_Execute(PASS_COMMAND_ARGS);
		if (g_invRefArray[idx].next < g_invRefArray[idx].refIDs.size()) {
			*refResult = g_invRefArray[idx].refIDs[g_invRefArray[idx].next];
			g_invRefArray[idx].next++;
		}
	}
	return true;
}

bool Cmd_GetInvRefsForItem_Execute(COMMAND_ARGS)
{
	// returns an array of inventory references for the specified base object in the calling object's inventory
	TESForm* item = NULL;
	ArrayID arrID = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arrID;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &item) && item) {
		double arrIndex = 0.0;

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
							InventoryReference* iref = InventoryReference::Create(thisObj, dataIR, false);
							g_ArrayMap.SetElementFormID(arrID, arrIndex, iref->GetRef()->refID);
							arrIndex += 1.0;
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

				std::vector<InventoryReference::Data> refData;
				InventoryReference::Data::CreateForUnextendedEntry(entry, baseCount, refData);
				for (std::vector<InventoryReference::Data>::iterator iter = refData.begin(); iter != refData.end(); ++iter) {
					InventoryReference* iref = InventoryReference::Create(thisObj, *iter, false);
					g_ArrayMap.SetElementFormID(arrID, arrIndex, iref->GetRef()->refID);
					arrIndex += 1.0;
				}
			}
		}
	}

	return true;
}

#endif
