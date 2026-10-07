#include "GameExtraData.h"
#include "GameBSExtraData.h"
#include "GameAPI.h"
#include "GameObjects.h"
#include "GameRTTI.h"
#include "GameScript.h"

struct GetMatchingEquipped {
	FormMatcher& m_matcher;
	EquipData m_found;

	GetMatchingEquipped(FormMatcher& matcher) : m_matcher(matcher) {
		m_found.pForm = NULL;
		m_found.pExtraData = NULL;
	}

	bool Accept(ExtraContainerChanges::EntryData* pEntryData) {
		if (pEntryData) {
			// quick check - needs an extendData or can't be equipped
			ExtraContainerChanges::ExtendDataList* pExtendList = pEntryData->extendData;
			if (pExtendList && m_matcher.Matches(pEntryData->type)) { 
				SInt32 n = 0;
				ExtraDataList* pExtraDataList = pExtendList->GetNthItem(n);
				while (pExtraDataList) {
					if (pExtraDataList->HasType(kExtraData_Worn) || pExtraDataList->HasType(kExtraData_WornLeft)) {
						m_found.pForm = pEntryData->type;
						m_found.pExtraData = pExtraDataList;
						return false;
					}
					n++;
					pExtraDataList = pExtendList->GetNthItem(n);
				}
			}
		}
		return true;
	}

	EquipData Found() {
		return m_found;
	}
};


EquipData ExtraContainerChanges::FindEquipped(FormMatcher& matcher) const
{
	FoundEquipData equipData;
	if (data && data->objList) {
		GetMatchingEquipped getEquipped(matcher);
		data->objList->Visit(getEquipped);
		equipData = getEquipped.Found();
	}
	return equipData;
};

STATIC_ASSERT(sizeof(ExtraHealth) == 0x10);
STATIC_ASSERT(sizeof(ExtraLock) == 0x10);
STATIC_ASSERT(sizeof(ExtraCount) == 0x10);
STATIC_ASSERT(sizeof(ExtraTeleport) == 0x10);

STATIC_ASSERT(sizeof(ExtraWorn) == 0x0C);
STATIC_ASSERT(sizeof(ExtraWornLeft) == 0x0C);
STATIC_ASSERT(sizeof(ExtraCannotWear) == 0x0C);
STATIC_ASSERT(sizeof(ExtraContainerChanges::EntryData) == 0x0C);

#if RUNTIME
static const UInt32 s_ExtraContainerChangesVtbl					= 0x01015BB8;	//	0x0100fb78;
static const UInt32 s_ExtraWornVtbl								= 0x01015BDC;
//static const UInt32 s_ExtraWornLeftVtbl							= 0x01015BE8;
static const UInt32 s_ExtraCannotWearVtbl						= 0x01015BF4;

static const UInt32 s_ExtraOwnershipVtbl						= 0x010158B4;	//	0x0100f874;
static const UInt32 s_ExtraRankVtbl								= 0x010158CC;	//	0x0100f88c;
static const UInt32 s_ExtraActionVtbl							= 0x01015BAC;	
static const UInt32 s_ExtraFactionChangesVtbl					= 0x01015F30;
static const UInt32 s_ExtraScriptVtbl							= 0X1015914;

//static const UInt32 s_ExtraScript_init							= 0x0042C760;

static const UInt32 s_ExtraHealthVtbl = 0x010158E4;
static const UInt32 s_ExtraLockVtbl = 0x0101589C;
static const UInt32 s_ExtraCountVtbl = 0x010158D8;
static const UInt32 s_ExtraTeleportVtbl = 0x010158A8;
static const UInt32 s_ExtraWeaponModFlagsVtbl = 0x010159A4;

static const UInt32 s_ExtraHotkeyVtbl = 0x0101592C;

static const UInt32 s_ExtraSemaphore	= 0x011C3920;
static const UInt32 s_SemaphoreWait		= 0x0040FBF0;
static const UInt32 s_SemaphoreLeave	= 0x0040FBA0;

#endif

static void** g_ExtraSemaphore = (void **)s_ExtraSemaphore;

void* GetExtraSemaphore()
{
	return *g_ExtraSemaphore;
};

void CallSemaphore(void * Semaphore, UInt32 SemaphoreFunc)
{
	_asm pushad
	_asm mov ecx, Semaphore
	_asm call SemaphoreFunc
	_asm popad
};

void CallSemaphore4(void * Semaphore, UInt32 SemaphoreFunc)
{
	_asm pushad
	_asm push ecx	;	does not seem to be used at all
	_asm mov ecx, Semaphore
	_asm call SemaphoreFunc
	_asm popad
};

SInt32 ExtraContainerChanges::ExtendDataList::AddAt(ExtraDataList* item, SInt32 index)
{
	//CallSemaphore4(GetExtraSemaphore(), s_SemaphoreWait);
	SInt32 Result = tList<ExtraDataList>::AddAt(item, index);
	//CallSemaphore(GetExtraSemaphore(), s_SemaphoreLeave);
	return Result;
};

void ExtraContainerChanges::ExtendDataList::RemoveAll() const
{
	//CallSemaphore4(GetExtraSemaphore(), s_SemaphoreWait);
	tList<ExtraDataList>::RemoveAll();
	//CallSemaphore(GetExtraSemaphore(), s_SemaphoreLeave);
};

ExtraDataList* ExtraContainerChanges::ExtendDataList::RemoveNth(SInt32 n)
{
	//CallSemaphore4(GetExtraSemaphore(), s_SemaphoreWait);
	ExtraDataList* Result = tList<ExtraDataList>::RemoveNth(n);
	//CallSemaphore(GetExtraSemaphore(), s_SemaphoreLeave);
	return Result;
};

ExtraContainerChanges::ExtendDataList* ExtraContainerChangesExtendDataListCreate(ExtraDataList* pExtraDataList)
{
	ExtraContainerChanges::ExtendDataList* xData = (ExtraContainerChanges::ExtendDataList*)FormHeap_Allocate(sizeof(ExtraContainerChanges::ExtendDataList));
	if (xData) {
		memset(xData, 0, sizeof(ExtraContainerChanges::ExtendDataList));
		if (pExtraDataList)
			if (xData->AddAt(pExtraDataList, eListEnd)==eListInvalid) {
				FormHeap_Free(xData);
				xData = NULL;
			}
	}
	return xData;
}

static void ExtraContainerChangesExtendDataListFree(ExtraContainerChanges::ExtendDataList* xData, bool bFreeList)
{
	if (xData)
	{
		if (bFreeList)
		{
			UInt32 i = 0;
			ExtraDataList* pExtraDataList = xData->GetNthItem(i);
			while (pExtraDataList)
			{
				for (UInt32 j = 0 ; j<0xFF ; j++)
					pExtraDataList->RemoveByType(j);
				i++;
				pExtraDataList = xData->GetNthItem(i);
			}
		}
		FormHeap_Free(xData);
	}
}

static bool ExtraContainerChangesExtendDataListAdd(ExtraContainerChanges::ExtendDataList* xData, ExtraDataList* xList) {
	if (xData && xList) {
		xData->AddAt(xList, eListEnd);
		return true;
	}
	return false;
}

static bool ExtraContainerChangesExtendDataListRemove(ExtraContainerChanges::ExtendDataList* xData, ExtraDataList* xList, bool bFreeList) {
	if (xData && xList) {
		UInt32 i = 0;
		ExtraDataList* pExtraDataList = xData->GetNthItem(i);
		while (pExtraDataList && pExtraDataList != xList) {
				i++;
				pExtraDataList = xData->GetNthItem(i);
		}
		if (pExtraDataList == xList) {
			xData->RemoveNth(i);
			if (bFreeList) {
				for (UInt32 j = 0 ; j<0xFF ; j++)
					pExtraDataList->RemoveByType(j);
				FormHeap_Free(xList);
			}
			return true;
		}
	}
	return false;
}

void ExtraContainerChanges::EntryData::Cleanup()
{
	// Didn't find the hook, let's fake it
	if (extendData) {
		ExtraContainerChanges::ExtendDataList::Iterator iter = extendData->Begin();
		while (!iter.End())
			if (iter.Get()) {
				ExtraCount* xCount = (ExtraCount*)iter.Get()->GetByType(kExtraData_Count);
				if (xCount && xCount->count<2)
					iter.Get()->RemoveByType(kExtraData_Count);
				if (countDelta || iter.Get()->m_data)	// There are other extras than count like ExtraWorn :)
					++iter;
				else
					extendData->RemoveIf(ExtraDataListInExtendDataListMatcher(iter.Get()));
			}
			else
				extendData->RemoveIf(ExtraDataListInExtendDataListMatcher(iter.Get()));
	}
}

#ifdef RUNTIME
ExtraContainerChanges::EntryData* ExtraContainerChanges::EntryData::Create(UInt32 refID, UInt32 count, ExtraContainerChanges::ExtendDataList* pExtendDataList)
{
	ExtraContainerChanges::EntryData* xData = (ExtraContainerChanges::EntryData*)FormHeap_Allocate(sizeof(ExtraContainerChanges::EntryData));
	if (xData) {
		memset(xData, 0, sizeof(ExtraContainerChanges::EntryData));
		if (refID) {
			TESForm * pForm = LookupFormByID(refID);
			if (pForm) {
				xData->type = pForm;
				xData->countDelta = count;
				xData->extendData = pExtendDataList;
			}
			else {
				FormHeap_Free(xData);
				xData = NULL;
			}
		}
	}
	return xData;
}
#endif

ExtraContainerChanges::EntryData* ExtraContainerChanges::EntryData::Create(TESForm* pForm, UInt32 count, ExtraContainerChanges::ExtendDataList* pExtendDataList)
{
	ExtraContainerChanges::EntryData* xData = (ExtraContainerChanges::EntryData*)FormHeap_Allocate(sizeof(ExtraContainerChanges::EntryData));
	if (xData) {
		memset(xData, 0, sizeof(ExtraContainerChanges::EntryData));
		if (pForm) {
			xData->type = pForm;
			xData->countDelta = count;
			xData->extendData = pExtendDataList;
		}
	}
	return xData;
}

ExtraContainerChanges::ExtendDataList* ExtraContainerChanges::EntryData::Add(ExtraDataList* newList)
{
	if (extendData)
		extendData->AddAt(newList, eListEnd);
	else
		/* ExtendDataList* */ extendData = ExtraContainerChangesExtendDataListCreate(newList);
	ExtraCount* xCount = (ExtraCount*)newList->GetByType(kExtraData_Count);
	countDelta += xCount ? xCount->count : 1;
	return extendData;
}

bool ExtraContainerChanges::EntryData::Remove(ExtraDataList* toRemove, bool bFree)
{
	if (extendData && toRemove) {
		SInt32 index = extendData->GetIndexOf(ExtraDataListInExtendDataListMatcher(toRemove));
		if (index >= 0) {
			ExtraCount* xCount = (ExtraCount*)toRemove->GetByType(kExtraData_Count);
			SInt16 count = xCount ? xCount->count : 1;

			extendData->RemoveNth(index);
			countDelta -= count;
			if (bFree) {
				toRemove->RemoveAll(true);
				FormHeap_Free(toRemove);
			}
			return true;
		}

	}
	return false;
}

void ExtraContainerChangesEntryDataFree(ExtraContainerChanges::EntryData* xData, bool bFreeList) {
	if (xData) {
		if (xData->extendData) {
			ExtraContainerChangesExtendDataListFree(xData->extendData, bFreeList);
		}
		FormHeap_Free(xData);
	}
}

ExtraContainerChanges::EntryDataList* ExtraContainerChangesEntryDataListCreate(UInt32 refID, UInt32 count, ExtraContainerChanges::ExtendDataList* pExtendDataList)
{
#ifdef RUNTIME
	ExtraContainerChanges::EntryDataList* xData = (ExtraContainerChanges::EntryDataList*)FormHeap_Allocate(sizeof(ExtraContainerChanges::EntryDataList));
	if (xData) {
		memset(xData, 0, sizeof(ExtraContainerChanges::EntryDataList));
		xData->AddAt(ExtraContainerChanges::EntryData::Create(refID, count, pExtendDataList), eListEnd);
	}
	return xData;
#else
	return NULL;
#endif
}

void ExtraContainerChangesEntryDataListFree(ExtraContainerChanges::EntryDataList* xData, bool bFreeList) {
	if (xData) {
		UInt32 i = 0;
		ExtraContainerChanges::EntryData* pX = xData->GetNthItem(i);
		if (pX) {
			ExtraContainerChangesEntryDataFree(pX, bFreeList);
			i++;
			pX = xData->GetNthItem(i);
		}
		FormHeap_Free(xData);
	}
}

ExtraContainerChanges::ExtendDataList* ExtraContainerChanges::Add(TESForm* form, ExtraDataList* dataList, 
	EntryData** outEntryData)
{
	if (!data) {
		// wtf
		_WARNING("ExtraContainerChanges::Add() encountered ExtraContainerChanges with NULL data");
		return NULL;
	}

	if (!data->objList) {
		data->objList = ExtraContainerChangesEntryDataListCreate();
	}

	// try to locate the form
	EntryData* found = data->objList->Find(ItemInEntryDataListMatcher(form));
	if (!found) {
		// add it to the list with a count delta of 0
		found = EntryData::Create(form, 0);
		data->objList->AddAt(found, eListEnd);
	}
	if (outEntryData)
		*outEntryData = found;

	return found->Add(dataList);
}

ExtraContainerChanges* ExtraContainerChanges::Create()
{
	ExtraContainerChanges* xChanges = (ExtraContainerChanges*)BSExtraData::Create(kExtraData_ContainerChanges, sizeof(ExtraContainerChanges), s_ExtraContainerChangesVtbl);
	xChanges->data = NULL;
	return xChanges;
}

ExtraContainerChanges::Data* ExtraContainerChanges::Data::Create(TESObjectREFR* owner)
{
	Data* data = (Data*)FormHeap_Allocate(sizeof(Data));
	if (data) {
		data->owner = owner;
		data->objList = NULL;
		data->unk2 = 0.0;
		data->unk3 = 0.0;
	}
	return data;
}

ExtraContainerChanges* ExtraContainerChanges::Create(TESObjectREFR* thisObj, UInt32 refID, UInt32 count, ExtraContainerChanges::ExtendDataList* pExtendDataList)
{
	ExtraContainerChanges* xData = (ExtraContainerChanges*)BSExtraData::Create(kExtraData_ContainerChanges, sizeof(ExtraContainerChanges), s_ExtraContainerChangesVtbl);
	if (xData) {
		xData->data = ExtraContainerChanges::Data::Create(thisObj);
		if (refID) {
			xData->data->objList = ExtraContainerChangesEntryDataListCreate(refID, count, pExtendDataList);
		}
	}
	return xData;
}

void ExtraContainerChangesFree(ExtraContainerChanges* xData, bool bFreeList) {
	if (xData) {
		if (xData->data) {
			if (xData->data->objList && bFreeList) {
				ExtraContainerChangesEntryDataListFree(xData->data->objList, true);
			}
			FormHeap_Free(xData->data);
		}
		FormHeap_Free(xData);
	}
}

void ExtraContainerChanges::Cleanup()
{
	if (data && data->objList) {
		for (EntryDataList::Iterator iter = data->objList->Begin(); !iter.End(); ++iter) {
			iter.Get()->Cleanup() ;

			// make sure we don't have any NULL ExtraDataList's in extend data, game will choke when saving
			if (iter->extendData == nullptr)
			{
				continue;
			}

			auto xIter = iter->extendData->Head();
			if (!xIter) continue;
			do
			{
				ExtraDataList* xtendData = xIter->data;
				if (xtendData && !xtendData->m_data)
				{
					FormHeap_Free(xtendData);
					xIter = xIter->RemoveMe();
					if (!xIter) break;
				}

			} while (xIter = xIter->next);
		}
	}
}

ExtraDataList* ExtraContainerChanges::SetEquipped(TESForm* obj, bool bEquipped, bool bForce)
{
	if (data) {
		EntryData* xData = data->objList->Find(ItemInEntryDataListMatcher(obj));
		if (xData) {
			ExtraDataList* Possible = NULL;
			bool atleastOne = xData->extendData && (xData->extendData->Count()>0);
			if (atleastOne)
				for (ExtendDataList::Iterator iter = xData->extendData->Begin(); !iter.End(); ++iter)
					if (iter.Get()->HasType(kExtraData_Worn) || iter.Get()->HasType(kExtraData_WornLeft)) {
						if (!bEquipped)
							if (bForce || !iter.Get()->HasType(kExtraData_WornLeft)) {
								iter.Get()->RemoveByType(kExtraData_Worn);
								iter.Get()->RemoveByType(kExtraData_WornLeft);
								Cleanup();
								return iter.Get(); 
							}

					}
					else
						if (bEquipped)
							if (bForce || !iter.Get()->HasType(kExtraData_CannotWear))
								Possible = iter.Get();
			if (!xData->extendData)
				xData->extendData = ExtraContainerChangesExtendDataListCreate(NULL);
			if (Possible) {
				Possible->Add(ExtraWorn::Create());
				return Possible; 
			}
		}
	}
	return NULL; 
}

bool ExtraContainerChanges::Remove(TESForm* obj, ExtraDataList* dataList, bool bFree)
{
	for (UInt32 i = 0; i < data->objList->Count(); i++)
		if (data->objList->GetNthItem(i)->type == obj) {
			ExtraContainerChanges::EntryData* found = data->objList->GetNthItem(i);
			if (dataList && found->extendData) {
				for (UInt32 j = 0; j < found->extendData->Count(); j++)
					if (found->extendData->GetNthItem(j) == dataList)
						found->extendData->RemoveNth(j);
			}
			else if (!dataList && !found->extendData)
				data->objList->RemoveNth(i);
		}
	return false;
}

#ifdef RUNTIME

void ExtraContainerChanges::DebugDump()
{
	_MESSAGE("Dumping ExtraContainerChanges");
	gLog.Indent();

	if (data && data->objList)
	{
		for (ExtraContainerChanges::EntryDataList::Iterator entry = data->objList->Begin(); !entry.End(); ++entry)
		{
			_MESSAGE("Type: %s CountDelta: %d [%08X]", GetFullName(entry.Get()->type), entry.Get()->countDelta, entry.Get());
			gLog.Indent();
			if (!entry.Get() || !entry.Get()->extendData)
				_MESSAGE("* No extend data *");
			else
			{
				for (ExtraContainerChanges::ExtendDataList::Iterator extendData = entry.Get()->extendData->Begin(); !extendData.End(); ++extendData)
				{
					_MESSAGE("Extend Data: [%08X]", extendData.Get());
					gLog.Indent();
					if (extendData.Get()) {
						extendData.Get()->DebugDump();
						ExtraCount* xCount = (ExtraCount*)extendData.Get()->GetByType(kExtraData_Count);
						if (xCount) {
							_MESSAGE("ExtraCount value : %d", xCount->count);
						}
					}
					else
						_MESSAGE("NULL");

					gLog.Outdent();
				}
			}
			gLog.Outdent();
		}
	}
	gLog.Outdent();
}
#endif

ExtraContainerChanges* ExtraContainerChanges::GetForRef(TESObjectREFR* refr)
{
	ExtraContainerChanges* xChanges = (ExtraContainerChanges*)refr->extraDataList.GetByType(kExtraData_ContainerChanges);
	if (!xChanges) {
		xChanges = ExtraContainerChanges::Create();
		refr->extraDataList.Add(xChanges);
	}
	return xChanges;
}

UInt32 ExtraContainerChanges::GetAllEquipped(std::vector<EntryData*>& outEntryData, std::vector<ExtendDataList*>& outExtendData)
{
	if (data && data->objList)
	{
		for (ExtraContainerChanges::EntryDataList::Iterator entry = data->objList->Begin(); !entry.End(); ++entry)
		{
			if (entry.Get() && entry.Get()->extendData)
			{
				for (ExtraContainerChanges::ExtendDataList::Iterator extendData = entry.Get()->extendData->Begin(); !extendData.End(); ++extendData)
				{
					if (extendData.Get()->IsWorn()) {
						outEntryData.push_back(entry.Get());
						outExtendData.push_back(entry.Get()->extendData);
					}
				}
			}
		}
	}
	return outEntryData.size();
}

// static
BSExtraData* BSExtraData::Create(UInt8 xType, UInt32 size, UInt32 vtbl)
{
	void* memory = FormHeap_Allocate(size);
	memset(memory, 0, size);
	((UInt32*)memory)[0] = vtbl;
	BSExtraData* xData = (BSExtraData*)memory;
	xData->type = xType;
	return xData;
}

ExtraHealth* ExtraHealth::Create() 
{
	ExtraHealth* xHealth = (ExtraHealth*)BSExtraData::Create(kExtraData_Health, sizeof(ExtraHealth), s_ExtraHealthVtbl);
	return xHealth;
}

ExtraWorn* ExtraWorn::Create() 
{
	ExtraWorn* xWorn = (ExtraWorn*)BSExtraData::Create(kExtraData_Worn, sizeof(ExtraWorn), s_ExtraWornVtbl);
	return xWorn;
}

//ExtraWornLeft* ExtraWornLeft::Create() 
//{
//	ExtraWornLeft* xWornLeft = (ExtraWornLeft*)BSExtraData::Create(kExtraData_WornLeft, sizeof(ExtraWornLeft), s_ExtraWornLeftVtbl);
//	return xWornLeft;
//}

ExtraCannotWear* ExtraCannotWear::Create() 
{
	ExtraCannotWear* xCannotWear = (ExtraCannotWear*)BSExtraData::Create(kExtraData_CannotWear, sizeof(ExtraCannotWear), s_ExtraCannotWearVtbl);
	return xCannotWear;
}

ExtraLock* ExtraLock::Create()
{
	ExtraLock* xLock = (ExtraLock*)BSExtraData::Create(kExtraData_Lock, sizeof(ExtraLock), s_ExtraLockVtbl);
	ExtraLock::Data* lockData = (ExtraLock::Data*)FormHeap_Allocate(sizeof(ExtraLock::Data));
	memset(lockData, 0, sizeof(ExtraLock::Data));
	xLock->data = lockData;
	return xLock;
}

ExtraCount* ExtraCount::Create(UInt32 count)
{
	ExtraCount* xCount = (ExtraCount*)BSExtraData::Create(kExtraData_Count, sizeof(ExtraCount), s_ExtraCountVtbl);
	xCount->count = count;
	return xCount;
}

ExtraTeleport* ExtraTeleport::Create()
{
	ExtraTeleport* tele = (ExtraTeleport*)BSExtraData::Create(kExtraData_Teleport, sizeof(ExtraTeleport), s_ExtraTeleportVtbl);
	
	// create data
	ExtraTeleport::Data* data = (ExtraTeleport::Data*)FormHeap_Allocate(sizeof(ExtraTeleport::Data));
	data->linkedDoor = NULL;
	data->yRot = -0.0;
	data->xRot = 0.0;
	data->x = 0.0;
	data->y = 0.0;
	data->z = 0.0;
	data->zRot = 0.0;

	tele->data = data;
	return tele;
}

ExtraWeaponModFlags* ExtraWeaponModFlags::Create()
{
	ExtraWeaponModFlags* xWeaponModFlags = (ExtraWeaponModFlags*)BSExtraData::Create(kExtraData_WeaponModFlags, sizeof(ExtraWeaponModFlags), s_ExtraWeaponModFlagsVtbl);

	xWeaponModFlags->flags = 0;

	return xWeaponModFlags;
}

SInt32 GetCountForExtraDataList(ExtraDataList* list)
{
	if (!list)
		return 1;

	ExtraCount* xCount = (ExtraCount*)list->GetByType(kExtraData_Count);
	return xCount ? xCount->count : 1;
}

ExtraOwnership* ExtraOwnership::Create() 
{
	ExtraOwnership* xOwner = (ExtraOwnership*)BSExtraData::Create(kExtraData_Ownership, sizeof(ExtraOwnership), s_ExtraOwnershipVtbl);
	return xOwner;
}

ExtraRank* ExtraRank::Create()
{
	ExtraRank* xRank = (ExtraRank*)BSExtraData::Create(kExtraData_Rank, sizeof(ExtraRank), s_ExtraRankVtbl);
	return xRank;
}

ExtraAction* ExtraAction::Create()
{
	ExtraAction* xAction = (ExtraAction*)BSExtraData::Create(kExtraData_Action, sizeof(ExtraAction), s_ExtraActionVtbl);
	return xAction;
}

const char* GetExtraDataName(UInt8 ExtraDataType) {
	switch (ExtraDataType) {			
		case	kExtraData_Havok                    	: return "Havok"; break;
		case	kExtraData_Cell3D                   	: return "Cell3D"; break;
		case	kExtraData_CellWaterType            	: return "CellWaterType"; break;
		case	kExtraData_RegionList               	: return "RegionList"; break;
		case	kExtraData_SeenData                 	: return "SeenData"; break;
		case	kExtraData_CellMusicType            	: return "CellMusicType"; break;
		case	kExtraData_CellClimate              	: return "CellClimate"; break;
		case	kExtraData_ProcessMiddleLow         	: return "ProcessMiddleLow"; break;
		case	kExtraData_CellCanopyShadowMask     	: return "CellCanopyShadowMask"; break;
		case	kExtraData_DetachTime               	: return "DetachTime"; break;
		case	kExtraData_PersistentCell           	: return "PersistentCell"; break;
		case	kExtraData_Script                   	: return "Script"; break;
		case	kExtraData_Action                   	: return "Action"; break;
		case	kExtraData_StartingPosition         	: return "StartingPosition"; break;
		case	kExtraData_Anim                     	: return "Anim"; break;
		case	kExtraData_UsedMarkers              	: return "UsedMarkers"; break;
		case	kExtraData_DistantData              	: return "DistantData"; break;
		case	kExtraData_RagdollData              	: return "RagdollData"; break;
		case	kExtraData_ContainerChanges         	: return "ContainerChanges"; break;
		case	kExtraData_Worn                     	: return "Worn"; break;
		case	kExtraData_WornLeft                 	: return "WornLeft"; break;
		case	kExtraData_PackageStartLocation     	: return "PackageStartLocation"; break;
		case	kExtraData_Package                  	: return "Package"; break;
		case	kExtraData_TrespassPackage          	: return "TrespassPackage"; break;
		case	kExtraData_RunOncePacks             	: return "RunOncePacks"; break;
		case	kExtraData_ReferencePointer         	: return "ReferencePointer"; break;
		case	kExtraData_Follower                 	: return "Follower"; break;
		case	kExtraData_LevCreaModifier          	: return "LevCreaModifier"; break;
		case	kExtraData_Ghost                    	: return "Ghost"; break;
		case	kExtraData_OriginalReference        	: return "OriginalReference"; break;
		case	kExtraData_Ownership                	: return "Ownership"; break;
		case	kExtraData_Global                   	: return "Global"; break;
		case	kExtraData_Rank                     	: return "Rank"; break;
		case	kExtraData_Count                    	: return "Count"; break;
		case	kExtraData_Health                   	: return "Health"; break;
		case	kExtraData_Uses                     	: return "Uses"; break;
		case	kExtraData_TimeLeft                 	: return "TimeLeft"; break;
		case	kExtraData_Charge                   	: return "Charge"; break;
		case	kExtraData_Light                    	: return "Light"; break;
		case	kExtraData_Lock                     	: return "Lock"; break;
		case	kExtraData_Teleport                 	: return "Teleport"; break;
		case	kExtraData_MapMarker                	: return "MapMarker"; break;
		case	kExtraData_LeveledCreature          	: return "LeveledCreature"; break;
		case	kExtraData_LeveledItem              	: return "LeveledItem"; break;
		case	kExtraData_Scale                    	: return "Scale"; break;
		case	kExtraData_Seed                     	: return "Seed"; break;
		case	kExtraData_PlayerCrimeList          	: return "PlayerCrimeList"; break;
		case	kExtraData_EnableStateParent        	: return "EnableStateParent"; break;
		case	kExtraData_EnableStateChildren      	: return "EnableStateChildren"; break;
		case	kExtraData_ItemDropper              	: return "ItemDropper"; break;
		case	kExtraData_DroppedItemList          	: return "DroppedItemList"; break;
		case	kExtraData_RandomTeleportMarker     	: return "RandomTeleportMarker"; break;
		case	kExtraData_MerchantContainer        	: return "MerchantContainer"; break;
		case	kExtraData_SavedHavokData           	: return "SavedHavokData"; break;
		case	kExtraData_CannotWear               	: return "CannotWear"; break;
		case	kExtraData_Poison                   	: return "Poison"; break;
		case	kExtraData_LastFinishedSequence     	: return "LastFinishedSequence"; break;
		case	kExtraData_SavedAnimation           	: return "SavedAnimation"; break;
		case	kExtraData_NorthRotation            	: return "NorthRotation"; break;
		case	kExtraData_XTarget                  	: return "XTarget"; break;
		case	kExtraData_FriendHits               	: return "FriendHits"; break;
		case	kExtraData_HeadingTarget            	: return "HeadingTarget"; break;
		case	kExtraData_RefractionProperty       	: return "RefractionProperty"; break;
		case	kExtraData_StartingWorldOrCell      	: return "StartingWorldOrCell"; break;
		case	kExtraData_Hotkey                   	: return "Hotkey"; break;
		case	kExtraData_EditorRefMovedData       	: return "EditorRefMovedData"; break;
		case	kExtraData_InfoGeneralTopic         	: return "InfoGeneralTopic"; break;
		case	kExtraData_HasNoRumors              	: return "HasNoRumors"; break;
		case	kExtraData_Sound                    	: return "Sound"; break;
		case	kExtraData_TerminalState            	: return "TerminalState"; break;
		case	kExtraData_LinkedRef                	: return "LinkedRef"; break;
		case	kExtraData_LinkedRefChildren        	: return "LinkedRefChildren"; break;
		case	kExtraData_ActivateRef              	: return "ActivateRef"; break;
		case	kExtraData_ActivateRefChildren      	: return "ActivateRefChildren"; break;
		case	kExtraData_TalkingActor             	: return "TalkingActor"; break;
		case	kExtraData_ObjectHealth             	: return "ObjectHealth"; break;
		case	kExtraData_DecalRefs                	: return "DecalRefs"; break;
		case	kExtraData_CellImageSpace           	: return "CellImageSpace"; break;
		case	kExtraData_NavMeshPortal            	: return "NavMeshPortal"; break;
		case	kExtraData_ModelSwap                	: return "ModelSwap"; break;
		case	kExtraData_Radius                   	: return "Radius"; break;
		case	kExtraData_Radiation                	: return "Radiation"; break;
		case	kExtraData_FactionChanges           	: return "FactionChanges"; break;
		case	kExtraData_DismemberedLimbs         	: return "DismemberedLimbs"; break;
		case	kExtraData_MultiBound               	: return "MultiBound"; break;
		case	kExtraData_MultiBoundData           	: return "MultiBoundData"; break;
		case	kExtraData_MultiBoundRef            	: return "MultiBoundRef"; break;
		case	kExtraData_ReflectedRefs            	: return "ReflectedRefs"; break;
		case	kExtraData_ReflectorRefs            	: return "ReflectorRefs"; break;
		case	kExtraData_EmittanceSource          	: return "EmittanceSource"; break;
		case	kExtraData_RadioData                	: return "RadioData"; break;
		case	kExtraData_CombatStyle              	: return "CombatStyle"; break;
		case	kExtraData_Primitive                	: return "Primitive"; break;
		case	kExtraData_OpenCloseActivateRef     	: return "OpenCloseActivateRef"; break;
		case	kExtraData_AnimNoteReciever				: return "AnimNoteReciever"; break;
		case	kExtraData_Ammo                     	: return "Ammo"; break;
		case	kExtraData_PatrolRefData            	: return "PatrolRefData"; break;
		case	kExtraData_PackageData              	: return "PackageData"; break;
		case	kExtraData_OcclusionPlane           	: return "OcclusionPlane"; break;
		case	kExtraData_CollisionData            	: return "CollisionData"; break;
		case	kExtraData_SayTopicInfoOnceADay     	: return "SayTopicInfoOnceADay"; break;
		case	kExtraData_EncounterZone            	: return "EncounterZone"; break;
		case	kExtraData_SayToTopicInfo           	: return "SayToTopicInfo"; break;
		case	kExtraData_OcclusionPlaneRefData    	: return "OcclusionPlaneRefData"; break;
		case	kExtraData_PortalRefData            	: return "PortalRefData"; break;
		case	kExtraData_Portal                   	: return "Portal"; break;
		case	kExtraData_Room                     	: return "Room"; break;
		case	kExtraData_HealthPerc               	: return "HealthPerc"; break;
		case	kExtraData_RoomRefData              	: return "RoomRefData"; break;
		case	kExtraData_GuardedRefData           	: return "GuardedRefData"; break;
		case	kExtraData_CreatureAwakeSound       	: return "CreatureAwakeSound"; break;
		case	kExtraData_WaterZoneMap             	: return "WaterZoneMap"; break;
		case	kExtraData_IgnoredBySandbox         	: return "IgnoredBySandbox"; break;
		case	kExtraData_CellAcousticSpace        	: return "CellAcousticSpace"; break;
		case	kExtraData_ReservedMarkers          	: return "ReservedMarkers"; break;
		case	kExtraData_WeaponIdleSound          	: return "WeaponIdleSound"; break;
		case	kExtraData_WaterLightRefs           	: return "WaterLightRefs"; break;
		case	kExtraData_LitWaterRefs             	: return "LitWaterRefs"; break;
		case	kExtraData_WeaponAttackSound        	: return "WeaponAttackSound"; break;
		case	kExtraData_ActivateLoopSound        	: return "ActivateLoopSound"; break;
		case	kExtraData_PatrolRefInUseData       	: return "PatrolRefInUseData"; break;
		case	kExtraData_AshPileRef               	: return "AshPileRef"; break;
		case	kExtraData_CreatureMovementSound    	: return "CreatureMovementSound"; break;
		case	kExtraData_FollowerSwimBreadcrumbs  	: return "FollowerSwimBreadcrumbs"; break;
	};
	return "unknown";
}

class TESScript;
class TESScriptableForm;

ExtraScript* ExtraScript::Create(TESForm* baseForm, bool create, TESObjectREFR* container) {
	ExtraScript* xScript = (ExtraScript*)BSExtraData::Create(kExtraData_Script, sizeof(ExtraScript), s_ExtraScriptVtbl);
	if (xScript && baseForm) {
		TESScriptableForm* pScript = DYNAMIC_CAST(baseForm, TESForm, TESScriptableForm);
		if (pScript && pScript->script) {
			xScript->script = pScript->script;
			if (create) {
				xScript->eventList = xScript->script->CreateEventList();
				if (container)
					xScript->EventCreate(ScriptEventList::kEvent_OnAdd, container);
			}
		}
	}
	return xScript;
}

void ExtraScript::EventCreate(UInt32 eventCode, TESObjectREFR* container) {
	if (eventList) {
		// create Event struct
		ScriptEventList::Event * pEvent = (ScriptEventList::Event*)FormHeap_Allocate(sizeof(ScriptEventList::Event));
		if (pEvent) {
			pEvent->eventMask = eventCode;
			pEvent->object = container;
		}

		if (!eventList->m_eventList) {
			eventList->m_eventList = (ScriptEventList::EventList*)FormHeap_Allocate(sizeof(ScriptEventList::EventList));
			eventList->m_eventList->Init();
		}
		if (eventList->m_eventList && pEvent)
			eventList->m_eventList->AddAt(pEvent, 0);
	}
}

ExtraFactionChanges* ExtraFactionChanges::Create()
{
	ExtraFactionChanges* xFactionChanges = (ExtraFactionChanges*)BSExtraData::Create(kExtraData_FactionChanges, sizeof(ExtraFactionChanges), s_ExtraFactionChangesVtbl);
	ExtraFactionChanges::FactionListEntry* FactionChangesData = (FactionListEntry*)FormHeap_Allocate(sizeof(FactionListEntry));
	memset(FactionChangesData, 0, sizeof(FactionListEntry));
	xFactionChanges->data = FactionChangesData;
	return xFactionChanges;
}

ExtraFactionChanges::FactionListEntry* GetExtraFactionList(BaseExtraList& xDataList)
{
	ExtraFactionChanges* xFactionChanges = GetByTypeCast(xDataList, FactionChanges);
	if (xFactionChanges)
		return xFactionChanges->data;
	return NULL;
}

SInt8 GetExtraFactionRank(BaseExtraList& xDataList, TESFaction * faction)
{
	ExtraFactionChanges* xFactionChanges = GetByTypeCast(xDataList, FactionChanges);
	if (xFactionChanges && xFactionChanges->data) {
		ExtraFactionChangesMatcher matcher(faction, xFactionChanges);
		ExtraFactionChanges::FactionListData* pData = xFactionChanges->data->Find(matcher);
		return (pData) ? pData->rank : -1;
	}
	return -1;
}

void SetExtraFactionRank(BaseExtraList& xDataList, TESFaction * faction, SInt8 rank)
{
	ExtraFactionChanges::FactionListData* pData = NULL;
	ExtraFactionChanges* xFactionChanges = GetByTypeCast(xDataList, FactionChanges);
	if (xFactionChanges && xFactionChanges->data) {
		ExtraFactionChangesMatcher matcher(faction, xFactionChanges);
		pData = xFactionChanges->data->Find(matcher);
		if (pData)
			pData->rank = rank;
	}
	if (!pData) {
		if (!xFactionChanges) {
			xFactionChanges = ExtraFactionChanges::Create();
			xDataList.Add(xFactionChanges);
		}
		pData = (ExtraFactionChanges::FactionListData*)FormHeap_Allocate(sizeof(ExtraFactionChanges::FactionListData));
		if (pData) {
			pData->faction = faction;
			pData->rank = rank;
			xFactionChanges->data->AddAt(pData, -2);
		}
	}
}

ExtraHotkey* ExtraHotkey::Create()
{
	ExtraHotkey* xHotkey = (ExtraHotkey*)BSExtraData::Create(kExtraData_Hotkey, sizeof(ExtraHotkey), s_ExtraHotkeyVtbl);
	xHotkey->index  = 0;
	return xHotkey;
}

