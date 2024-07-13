#include "GameObjects.h"
#include "GameRTTI.h"
#include "GameExtraData.h"
#include "GameTasks.h"
#include "GameUI.h"
#include "SafeWrite.h"
#include "NiObjects.h"

static const UInt32 s_TESObject_REFR_init	= 0x0055A2F0;	// TESObject_REFR initialization routine (first reference to s_TESObject_REFR_vtbl)
static const UInt32	s_Actor_EquipItem		= 0x0088C650;	// maybe, also, would be: 007198E0 for FOSE	4th call from the end of TESObjectREFR::RemoveItem (func5F)
static const UInt32	s_Actor_UnequipItem		= 0x0088C790;	// maybe, also, would be: 007133E0 for FOSE next sub after EquipItem
static const UInt32 s_TESObjectREFR__GetContainer	= 0x0055D310;	// First call in REFR::RemoveItem
static const UInt32 s_TESObjectREFR_Set3D	= 0x005702E0;	// void : (const char*)
static const UInt32 s_PlayerCharacter_GetCurrentQuestTargets	= 0x00952BA0;	// BuildedQuestObjectiveTargets* : (void)
static const UInt32 s_PlayerCharacter_GenerateNiNode	= 0x0094E1D0; // Func0072
static const UInt32 kPlayerUpdate3Dpatch = 0x0094EB7A;
static const UInt32 TESObjectREFR_Set3D = 0x0094EB40;
static const UInt32 ValidBip01Names_Destroy = 0x00418E00;
static const UInt32 ExtraAnimAnimation_Destroy = 0x00418D20;
static const UInt32 RefNiRefObject_ReplaceNiRefObject = 0x0066B0D0;

static const UInt32 kg_Camera1st	= 0x011E07D0;
static const UInt32 kg_Camera3rd	= 0x011E07D4;
static const UInt32 kg_Bip			= 0x011E07D8;
static const UInt8 kPlayerUpdate3DpatchFrom	= 0x0B6;
static const UInt8 kPlayerUpdate3DpatchTo	= 0x0EB;

static NiObject **	g_3rdPersonCameraNode =				(NiObject**)kg_Camera3rd;
static NiObject **	g_1stPersonCameraBipedNode =		(NiObject**)kg_Bip;
static NiObject **	g_1stPersonCameraNode =				(NiObject**)kg_Camera1st;

ExtraScript* TESObjectREFR::GetExtraScript() const
{
	BSExtraData* xData = extraDataList.GetByType(kExtraData_Script);
	if (xData)
	{
		const auto xScript = DYNAMIC_CAST(xData, BSExtraData, ExtraScript);
		if (xScript)
			return xScript;
	}

	return nullptr;
}

ScriptEventList* TESObjectREFR::GetEventList() const
{
	if (auto* extraScript = GetExtraScript())
		return extraScript->eventList;
	return nullptr;
}

PlayerCharacter** g_thePlayer = (PlayerCharacter **)0x011DEA3C;

PlayerCharacter* PlayerCharacter::GetSingleton()
{
	return *g_thePlayer;
}

QuestObjectiveTargets* PlayerCharacter::GetCurrentQuestObjectiveTargets()
{
	return (QuestObjectiveTargets *)ThisStdCall(s_PlayerCharacter_GetCurrentQuestTargets, this);
}

TESContainer* TESObjectREFR::GetContainer()
{
	if (IsActor_Runtime())
		return &((TESActorBase*)baseForm)->container;
	if (baseForm->typeID == kFormType_TESObjectCONT)
		return &((TESObjectCONT*)baseForm)->container;
	return NULL;
}

bool TESObjectREFR::IsMapMarker()
{
	if (baseForm) {
		return baseForm->refID == 0x010;	// Read from the geck. Why is OBSE looking for a pointer ?
	}
	else {
		return false;
	}
}

// Less worse version as used by some modders 
bool PlayerCharacter::SetSkeletonPath_v1c(const char* newPath)
{
	if (!bThirdPerson) {
		// ###TODO: enable in first person
		return false;
	}

	return true;
}

bool TESObjectREFR::Update3D_v1c()
{
	static const UInt32 kPlayerUpdate3Dpatch = 0x0094EB7A;

	UInt8 kPlayerUpdate3DpatchFrom = 0x0B6;
	UInt8 kPlayerUpdate3DpatchTo = 0x0EB;

	if (this == *g_thePlayer) {
		// Lets try to allow unloading the player3D never the less...
		SafeWrite8(kPlayerUpdate3Dpatch, kPlayerUpdate3DpatchTo);
	}

	Set3D(NULL, true);
	ModelLoader::GetSingleton()->QueueReference(this, 1, 0);
	return true;
}

// Current basically not functioning version, but should show some progress in the end.. I hope
bool PlayerCharacter::SetSkeletonPath(const char* newPath)
{
	if (!bThirdPerson) {
		// ###TODO: enable in first person
		return false;
	}

#ifdef NVSE_CORE
#ifdef _DEBUG
	// store parent of current niNode
	/*NiNode* niParent = (NiNode*)(renderState->niNode->m_parent);

	if (renderState->niNode) renderState->niNode->Dump(0, "");

	// set niNode to NULL via BASE CLASS Set3D() method
	ThisStdCall(s_TESObjectREFR_Set3D, this, NULL, true);

	// modify model path
	if (newPath) {
		TESNPC* base = DYNAMIC_CAST(baseForm, TESForm, TESNPC);
		base->model.SetPath(newPath);
	}

	// create new NiNode, add to parent
	//*(g_bUpdatePlayerModel) = 1;

	// s_PlayerCharacter_GenerateNiNode = (MobileObject::Func0053 in Oblivion)
	NiNode* newNode = (NiNode*)ThisStdCall(s_PlayerCharacter_GenerateNiNode, this, false);	// got a body WITHOUT the head :)

	if (newNode) newNode->Dump(0, "");

	if (niParent)
		niParent->AddObject(newNode, 1);	// no complaints
	//*(g_bUpdatePlayerModel) = 0;

	newNode->SetName("Player");	// no complaints

	if (newNode) newNode->Dump(0, "");

	if (playerNode) playerNode->Dump(0, "");

	// get and store camera node
	// ### TODO: pretty this up
	UInt32 vtbl = *((UInt32*)newNode);				// ok
	UInt32 vfunc = *((UInt32*)(vtbl + 0x9C));		// ok
	NiObject* cameraNode = (NiObject*)ThisStdCall(vfunc, newNode, "Camera3rd");				// returns NULL !!!
	*g_3rdPersonCameraNode = cameraNode;

	cameraNode = (NiObject*)ThisStdCall(vfunc, (NiNode*)this->playerNode, "Camera1st");		// returns NULL !!!
	*g_1stPersonCameraNode = cameraNode;

	AnimateNiNode();*/
#endif
#endif
	return true;
}

bool TESObjectREFR::Update3D()
{
	if (this == *g_thePlayer) {
#ifdef _DEBUG
		TESModel* model = DYNAMIC_CAST(baseForm, TESForm, TESModel);
		return (*g_thePlayer)->SetSkeletonPath(model->GetModelPath());
#else
		// Lets try to allow unloading the player3D never the less...
		SafeWrite8(kPlayerUpdate3Dpatch, kPlayerUpdate3DpatchTo);
#endif
	}

	Set3D(NULL, true);
	ModelLoader::GetSingleton()->QueueReference(this, 1, false);
	return true;
}

TESObjectREFR* TESObjectREFR::Create(bool bTemp)
{
	TESObjectREFR* refr = (TESObjectREFR*)FormHeap_Allocate(sizeof(TESObjectREFR));
	ThisStdCall(s_TESObject_REFR_init, refr);
	if (bTemp)
		CALL_MEMBER_FN(refr, MarkAsTemporary)();
	return refr;
}

TESForm* GetPermanentBaseForm(TESObjectREFR* thisObj)	// For LevelledForm, find real baseForm, not temporary one.
{
	if (!thisObj)
		return nullptr;
	TESForm *baseForm = thisObj->baseForm;
	if (baseForm && (baseForm->GetModIndex() == 0xFF))
	{
		if (BGSPlaceableWater *plcWater = DYNAMIC_CAST(baseForm, TESForm, BGSPlaceableWater))
			return plcWater->water;
		if (ExtraLeveledCreature *pXCreatureData = GetByTypeCast(thisObj->extraDataList, LeveledCreature); pXCreatureData && pXCreatureData->baseForm)
			return pXCreatureData->baseForm;
	}
	return baseForm;
}

// Taken from JIP LN NVSE.
__declspec(naked) float __vectorcall GetDistance3D(const TESObjectREFR* ref1, const TESObjectREFR* ref2)
{
	__asm
	{
		movups	xmm0, [ecx + 0x30]
		movups	xmm1, [edx + 0x30]
		subps	xmm0, xmm1
		mulps	xmm0, xmm0
		movhlps	xmm1, xmm0
		addss	xmm1, xmm0
		psrlq	xmm0, 0x20
		addss	xmm1, xmm0
		sqrtss	xmm0, xmm1
		retn
	}
}

void Actor::EquipItem(TESForm * objType, UInt32 equipCount, ExtraDataList* itemExtraList, UInt32 unk3, bool lockEquip, UInt32 unk5)
{
	ThisStdCall(s_Actor_EquipItem, this, objType, equipCount, itemExtraList, unk3, lockEquip, unk5);
}

void Actor::UnequipItem(TESForm* objType, UInt32 unk1, ExtraDataList* itemExtraList, UInt32 unk3, bool lockUnequip, UInt32 unk5)
{
	ThisStdCall(s_Actor_UnequipItem, this, objType, unk1, itemExtraList, unk3, lockUnequip, unk5);
}

EquippedItemsList Actor::GetEquippedItems()
{
	EquippedItemsList itemList;
	ExtraContainerDataArray outEntryData;
	ExtraContainerExtendDataArray outExtendData;

	ExtraContainerChanges	* xChanges = static_cast <ExtraContainerChanges *>(extraDataList.GetByType(kExtraData_ContainerChanges));
	if(xChanges) {
		UInt32 count = xChanges->GetAllEquipped(outEntryData, outExtendData);
		for (UInt32 i = 0; i < count ; i++)
			itemList.push_back(outEntryData[i]->type);

	}

	return itemList;
}

ExtraContainerDataArray	Actor::GetEquippedEntryDataList()
{
	ExtraContainerDataArray itemArray;
	ExtraContainerExtendDataArray outExtendData;

	ExtraContainerChanges	* xChanges = static_cast <ExtraContainerChanges *>(extraDataList.GetByType(kExtraData_ContainerChanges));
	if(xChanges)
		xChanges->GetAllEquipped(itemArray, outExtendData);

	return itemArray;
}

ExtraContainerExtendDataArray	Actor::GetEquippedExtendDataList()
{
	ExtraContainerDataArray itemArray;
	ExtraContainerExtendDataArray outExtendData;

	ExtraContainerChanges* xChanges = static_cast<ExtraContainerChanges*>(extraDataList.GetByType(kExtraData_ContainerChanges));
	if(xChanges)
		xChanges->GetAllEquipped(itemArray, outExtendData);

	return outExtendData;
}

// Copied from JIP
TESObjectWEAP* Actor::GetEquippedWeapon() const
{
	if (baseProcess) {
		ExtraContainerChanges::EntryData* weaponInfo = baseProcess->GetWeaponInfo();
		if (weaponInfo) return (TESObjectWEAP*)weaponInfo->type;
	}
	return NULL;
}

void Actor::AimWeapon(bool shouldAim, bool hasQueuedIdleFlags10000)
{
	ThisStdCall(0x8BB650, this, static_cast<UInt8>(shouldAim), hasQueuedIdleFlags10000, static_cast<UInt8>(false));
}

bool Actor::SetBlocking(bool shouldBlock)
{
	return ThisStdCall<bool>(0x894CC0, this, static_cast<UInt8>(shouldBlock));
}

bool TESObjectREFR::GetInventoryItems(InventoryItemsMap &invItems)
{
	TESContainer *container = GetContainer();
	if (!container) return false;
	ExtraContainerChanges *xChanges = (ExtraContainerChanges*)extraDataList.GetByType(kExtraData_ContainerChanges);
	ExtraContainerChanges::EntryDataList *entryList = (xChanges && xChanges->data) ? xChanges->data->objList : NULL;
	if (!entryList) return false;

	TESForm *item;
	SInt32 contCount, countDelta;
	ExtraContainerChanges::EntryData *entry;

	for (auto contIter = container->formCountList.Begin(); !contIter.End(); ++contIter)
	{
		item = contIter->form;
		if ((item->typeID == kFormType_TESLevItem) || invItems.HasKey(item))
			continue;
		contCount = container->GetCountForForm(item);
		if (entry = entryList->FindForItem(item))
		{
			countDelta = entry->countDelta;
			if (entry->HasExtraLeveledItem())
				contCount = countDelta;
			else contCount += countDelta;
		}
		if (contCount > 0)
			invItems.Emplace(item, contCount, entry);
	}

	for (auto xtraIter = entryList->Begin(); !xtraIter.End(); ++xtraIter)
	{
		entry = xtraIter.Get();
		item = entry->type;
		if (invItems.HasKey(item))
			continue;
		countDelta = entry->countDelta;
		if (countDelta > 0)
			invItems.Emplace(item, countDelta, entry);
	}

	return !invItems.Empty();
}

ExtraDroppedItemList* TESObjectREFR::GetDroppedItems()
{
	return static_cast<ExtraDroppedItemList*>(extraDataList.GetByType(kExtraData_DroppedItemList));
}

// Code by JIP
double TESObjectREFR::GetHeadingAngle(const TESObjectREFR* to) const
{
	auto const from = this;
	double result = (atan2(to->posX - from->posX, to->posY - from->posY) - from->rotZ) * 57.29577951308232;
	if (result < -180)
		result += 360;
	else if (result > 180)
		result -= 360;
	return result;
}

// Code by JIP
__declspec(naked) bool __fastcall TESObjectREFR::GetInSameCellOrWorld(TESObjectREFR* target) const
{
	__asm
	{
		mov		eax, [ecx + 0x40]
		test	eax, eax
		jnz		hasCell1
		push	edx
		push	kExtraData_PersistentCell
		add		ecx, 0x44
		call	BaseExtraList::GetByType
		pop		edx
		test	eax, eax
		jz		done
		mov		eax, [eax + 0xC]
	hasCell1:
		mov		ecx, [edx + 0x40]
		test	ecx, ecx
		jnz		hasCell2
		push	eax
		push	kExtraData_PersistentCell
		lea		ecx, [edx + 0x44]
		call	BaseExtraList::GetByType
		pop		edx
		test	eax, eax
		jz		done
		mov		ecx, [eax + 0xC]
		mov		eax, edx
	hasCell2 :
		cmp		eax, ecx
		jz		retnTrue
		mov		eax, [eax + 0xC0]
		test	eax, eax
		jz		done
		cmp		eax, [ecx + 0xC0]
	retnTrue:
		setz	al
	done :
		retn
	}
}

// Code by JIP
__declspec(naked) float __vectorcall TESObjectREFR::GetDistance(TESObjectREFR* target) const
{
	__asm
	{
		push	ecx
		push	edx
		call	TESObjectREFR::GetInSameCellOrWorld
		pop		edx
		pop		ecx
		test	al, al
		jz		fltMax
		add		ecx, 0x30
		add		edx, 0x30
		jmp		Point3Distance
	fltMax :
		mov		eax, 0x7F7FFFFF
		movd	xmm0, eax
		retn
	}
}

void Actor::SetWantsWeaponOut(bool wantsWeaponOut)
{
	ThisStdCall(0x8A6840, this, (UInt8)wantsWeaponOut);
}

bool Actor::IsWeaponOut()
{
	return baseProcess && baseProcess->IsWeaponOut();
}

void PlayerCharacter::UpdateCamera(bool isCalledFromFunc21, bool _zero_skipUpdateLOD)
{
	ThisStdCall(0x94AE40, this, (UInt8)isCalledFromFunc21, (UInt8)_zero_skipUpdateLOD);
}