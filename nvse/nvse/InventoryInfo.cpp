#include "InventoryInfo.h"

void PrintItemType(TESForm * form)
{
	Console_Print("%s (%s)", GetFullName(form), GetObjectClassName(form));
}

TESForm* GetItemByIdx(TESObjectREFR* thisObj, UInt32 objIdx, SInt32* outNumItems)
{
	if (!thisObj) return NULL;
	if(outNumItems) *outNumItems = 0;

	UInt32 count = 0;

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->GetEntryDataList() : NULL);

	TESContainer* pContainer = NULL;
	TESForm* pBaseForm = thisObj->baseForm;
	if (pBaseForm) {
		pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
	}

	// first look in the base container
	if (pContainer) {
		ContainerFindNth finder(info, objIdx);
		TESContainer::FormCount* pFound = pContainer->formCountList.Find(finder);
		if (pFound) {
			if (outNumItems) *outNumItems = pFound->count;
			return pFound->form;
		} else {
			count = finder.GetCurIdx();
		}
	}

	// now walk the remaining items in the map
	ExtraContainerChanges::EntryData* pEntryData = info.GetNth(objIdx, count);
	if (pEntryData) {
		if (outNumItems) *outNumItems = pEntryData->countDelta;
		return pEntryData->type;
	}
	return NULL;
}

TESForm* GetItemByRefID(TESObjectREFR* thisObj, UInt32 refID, SInt32* outNumItems)
{
	if (!thisObj) return NULL;
	if (outNumItems) *outNumItems = 0;

	UInt32 count = 0;

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->GetEntryDataList() : NULL);

	TESContainer* pContainer = NULL;
	TESForm* pBaseForm = thisObj->baseForm;
	if (pBaseForm) {
		pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
	}

	// first look in the base container
	if (pContainer) {
		ContainerFindRefId finder(info, refID);
		TESContainer::FormCount* pFound = pContainer->formCountList.Find(finder);
		if (pFound) {
			if (outNumItems) *outNumItems = pFound->count;
			return pFound->form;
		} else {
			count = finder.GetCurIdx();
		}
	}

	// now walk the remaining items in the map
	ExtraContainerChanges::EntryData* pEntryData = info.GetRefID(refID);
	if (pEntryData) {
		if (outNumItems) *outNumItems = pEntryData->countDelta;
		return pEntryData->type;
	}
	return NULL;
}

TESForm* GetItemWithHealthAndOwnershipByRefID(TESObjectREFR* thisObj, UInt32 refID, float* outHealth, TESForm** outOwner, UInt32* outRank, SInt32* inOutIndex, SInt32* outNumItems)
{
	if (!thisObj) return NULL;
	if (outNumItems) *outNumItems = 0;
	if (outHealth) *outHealth = 0.0;
	if (outOwner) *outOwner = NULL;
	if (outRank) *outRank = 0;
	UInt32 count = 0;
	TESHealthForm* pHealth = NULL;

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->GetEntryDataList() : NULL);

	// First walk the items in the map
	ExtraContainerChanges::EntryData* pEntryData = info.GetRefID(refID);
	SInt32 n = inOutIndex ? *inOutIndex : 0;
	if (pEntryData) {
		if (pEntryData->extendData)
			if (pEntryData->extendData->Count()>n) {
				ExtraDataList* pExtraDataList = pEntryData->extendData->GetNthItem(n);
				if (pExtraDataList) {
					ExtraHealth* pXHealth = (ExtraHealth*)pExtraDataList->GetByType(kExtraData_Health);
					if (pXHealth) {
						if (outHealth)
							*outHealth = pXHealth->health;
					}
					else {
						pHealth = DYNAMIC_CAST(pEntryData->type, TESForm, TESHealthForm);
						if (pHealth && outHealth)
							*outHealth = pHealth->health;
					}
					ExtraOwnership* pXOwner = (ExtraOwnership*)pExtraDataList->GetByType(kExtraData_Ownership);
					ExtraRank* pXRank = (ExtraRank*)pExtraDataList->GetByType(kExtraData_Rank);
					if (pXOwner) {
						if (outOwner)
							*outOwner = pXOwner->owner;
						if (pXRank)
							if (outRank)
								*outRank = pXRank->rank;
					}
					if (outNumItems) {
						ExtraCount* pXCount = (ExtraCount*)pExtraDataList->GetByType(kExtraData_Count);
						if (pXCount)
							*outNumItems = pXCount->count;
						else
							*outNumItems = 1;
					}
					if (inOutIndex)
						*inOutIndex += 1;
				}
				return pEntryData->type;
			}
			else
				n -= pEntryData->extendData->Count();
	}

	// Then look in the base container
	TESContainer* pContainer = NULL;
	TESForm* pBaseForm = thisObj->baseForm;
	if (pBaseForm)
		pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);

	if (pContainer) {
		ContainerFindRefId finder(info, refID);
		TESContainer::FormCount* pFound = pContainer->formCountList.Find(finder);
		if (pFound)
			count = finder.GetCurIdx() + n; // Get the nth item
		else
			count = pContainer->formCountList.Count(); // so we wont find it
		pFound = pContainer->formCountList.GetNthItem(count);
		if (pFound) {
			if (pFound->form && (pFound->form->refID==refID)) {
				pHealth = DYNAMIC_CAST(pEntryData->type, TESForm, TESHealthForm);
				if (pHealth && outHealth) *outHealth = pHealth->health;
				if (outNumItems) *outNumItems = pFound->count;
				*inOutIndex += 1;
				return pFound->form;
			}
		}
	}

	return NULL;
}

// Assumes there is only one such Item in the container
TESForm * SetFirstItemWithHealthAndOwnershipByRefID(TESObjectREFR* thisObj, UInt32 refID, SInt32 NumItems, float Health, TESForm* pOwner, UInt32 Rank)
{
	if (!thisObj) return NULL;
	if ((Health == -1.0) && (pOwner == NULL) && (Rank == 0)) return NULL; // Nothing to modify!

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	if (!pXContainerChanges)
		return AddItemHealthPercentOwner(thisObj, refID, NumItems, Health, pOwner, Rank);

	if (!pXContainerChanges->GetEntryDataList())
		return AddItemHealthPercentOwner(thisObj, refID, NumItems, Health, pOwner, Rank);
	ExtraContainerInfo info(pXContainerChanges->GetEntryDataList());

	ExtraContainerChanges::EntryData* pEntryData = info.GetRefID(refID);
	if (!pEntryData)
		return AddItemHealthPercentOwner(thisObj, refID, NumItems, Health, pOwner, Rank);

	ExtraContainerChanges::ExtendDataList* pExtendList = pEntryData->extendData;
	if (!pExtendList) 
		return AddItemHealthPercentOwner(thisObj, refID, NumItems, Health, pOwner, Rank);

	if (!pEntryData) 
		return AddItemHealthPercentOwner(thisObj, refID, NumItems, Health, pOwner, Rank);

	bool Done = false;
	TESHealthForm* pHealth = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESHealthForm);
	ExtraHealth* pXHealth = NULL;
	ExtraOwnership* pXOwner = NULL;
	ExtraRank* pXRank = NULL;
	ExtraCount* pXCount = NULL;
	ExtraDataList* pExtraDataList = NULL;

	pEntryData->countDelta = NumItems;
	SInt32 n = 0;
	pExtraDataList = pExtendList->GetNthItem(n); // Only one will be set, the first one 
	pXHealth = (ExtraHealth*)pExtraDataList->GetByType(kExtraData_Health);
	pXCount = (ExtraCount*)pExtraDataList->GetByType(kExtraData_Count);
	pXOwner  = (ExtraOwnership*)pExtraDataList->GetByType(kExtraData_Ownership);
	pXRank  = (ExtraRank*)pExtraDataList->GetByType(kExtraData_Rank);
	if (!pXHealth) {
		if (Health != -1.0) {
			pXHealth = ExtraHealth::Create();
			pExtraDataList->Add(pXHealth);
			pXHealth->health = Health;
		}
	}
	else
		if (Health != -1.0)
			if (pHealth && Health==pHealth->health)
				pExtraDataList->Remove(pXHealth, true);
			else
				pXHealth->health = Health;
	if (!pXOwner) {
		if (pOwner) {
			pXOwner = ExtraOwnership::Create();
			pExtraDataList->Add(pXOwner);
			pXOwner->owner = pOwner;
		}
	}
	else
		if (pOwner)
			pXOwner->owner = pOwner;
		else
			pExtraDataList->Remove(pXOwner, true);
	if (!pXRank) {
		if (Rank) {
			pXRank = ExtraRank::Create();
			pExtraDataList->Add(pXRank);
			pXRank->rank = Rank;
		}
	}
	else
		if (Rank)
			pXRank->rank = Rank;
		else
			pExtraDataList->Remove(pXRank, true);

	if (pXCount)
		pXCount->count = NumItems;
	
	return pEntryData->type;
}

bool SameHealth(ExtraHealth* pXHealth, TESHealthForm* pHealth, float Health) {
	if (!pHealth || (Health==-1.0)) // no health possible!
		return true;
	if (!pXHealth && (pHealth->health==Health))
		return true;
	if (pXHealth && (pXHealth->health==Health))
		return true;
	return false;
}

bool SameOwner(ExtraOwnership* pXOwner, ExtraRank* pXRank, TESForm* pOwner, UInt32 Rank) {
	if ((pXOwner && !pOwner) || (!pXOwner && pOwner)) 
		return false;
	if (!pXOwner && !pOwner)
		return true;
	TESFaction* pXFaction = DYNAMIC_CAST(pXOwner->owner, TESForm, TESFaction);
	TESFaction* pFaction = DYNAMIC_CAST(pOwner, TESForm, TESFaction);
	if (pFaction && pXFaction)
		if (pFaction->refID==pXFaction->refID)
			if (pXRank && (pXRank->rank==Rank))
				return true;
			else
				return false;
		else
			return false;
	if (pFaction || pXFaction)
		return false;
	TESObjectREFR* pXRef = DYNAMIC_CAST(pXOwner->owner, TESForm, TESObjectREFR);
	TESObjectREFR* pRef = DYNAMIC_CAST(pOwner, TESForm, TESObjectREFR);
	if ((pXRef && !pRef) || (!pXRef && pRef))
		return false;
	else if (pRef && pXRef)
		return pRef->refID==pXRef->refID;
	else
		return pXOwner->owner->refID==pOwner->refID;
}

TESForm * AddItemHealthPercentOwner(TESObjectREFR* thisObj, UInt32 refID, SInt32 NumItems, float Health, TESForm* pOwner, UInt32 Rank)
{
	if (!thisObj) return NULL;

	TESForm * pForm = LookupFormByID(refID);
	if (!pForm) return NULL;
	TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
	if (!pHealth && (Health != -1.0)) {
		_MESSAGE("\t\tInventoryInfo\t\tAddItemHealthPercentOwner:\tInvalid refID:%#10X, no health attribute", thisObj->refID);
		return NULL;
	}
	TESScriptableForm* pScript = DYNAMIC_CAST(pForm, TESForm, TESScriptableForm);
	if (pScript && !pScript->script) pScript = NULL;  // Only existing scripts matter

	ExtraHealth* pXHealth = NULL;
	ExtraOwnership* pXOwner = NULL;
	ExtraRank* pXRank = NULL;
	ExtraCount* pXCount = NULL;
	ExtraDataList* pExtraDataList = NULL;
	ExtraScript * pXScript = NULL;

	if (!(1.0 == Health) || pOwner || Rank || pScript) {
		pExtraDataList = ExtraDataList::Create();
		if (!(1.0 == Health)) {
			pXHealth = ExtraHealth::Create();
			pExtraDataList->Add(pXHealth);
			pXHealth->health = pHealth->GetHealth() * Health;
		}
		if (pOwner) {
			pXOwner = ExtraOwnership::Create();
			pExtraDataList->Add(pXOwner);
			pXOwner->owner = pOwner;
		}
		if (Rank) {
			pXRank = ExtraRank::Create();
			pExtraDataList->Add(pXRank);
			pXRank->rank = Rank;
		}
		if (pScript) {
			pXScript = ExtraScript::Create(pForm, true);
			pExtraDataList->Add(pXScript);
		}
	}
	thisObj->AddItem(pForm, pExtraDataList, NumItems);
	return pForm;
}
