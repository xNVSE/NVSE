#include "nvse/GameObjects.h"
#include "nvse/GameExtraData.h"
#include "nvse/GameTasks.h"

TESForm* GetPermanentBaseForm(TESObjectREFR* thisObj)	// For LevelledForm, find real baseForm, not temporary one.
{
	ExtraLeveledCreature * pXCreatureData = NULL;

	if (thisObj) {
		pXCreatureData = GetByTypeCast(thisObj->extraDataList, LeveledCreature);
		if (pXCreatureData && pXCreatureData->baseForm) {
			return pXCreatureData->baseForm;
		}
	}
	if (thisObj && thisObj->baseForm) {
		return thisObj->baseForm;
	}
	return NULL;
}

QuestObjectiveTargets* PlayerCharacter::GetCurrentQuestObjectiveTargets()
{
	return (QuestObjectiveTargets *)ThisStdCall(s_PlayerCharacter_GetCurrentQuestTargets, this);
}

ScriptEventList *TESObjectREFR::GetEventList() const
{
	ExtraScript *xScript = GetExtraType(extraDataList, Script);
	return xScript ? xScript->eventList : NULL;
}

PlayerCharacter** g_thePlayer = (PlayerCharacter**)(0x11DEA3C);
PlayerCharacter *PlayerCharacter::GetSingleton()
{
	return *g_thePlayer;
}

__declspec(naked) TESContainer *TESObjectREFR::GetContainer()
{
	__asm
	{
		mov		eax, [ecx]
		mov		eax, [eax+0x100]
		call	eax
		test	al, al
		mov		eax, [ecx+0x20]
		jz		notActor
		add		eax, 0x64
		retn
	notActor:
		cmp		dword ptr [eax], kVtbl_TESObjectCONT
		jnz		notCONT
		add		eax, 0x30
		retn
	notCONT:
		xor		eax, eax
		retn
	}
}

bool TESObjectREFR::IsMapMarker()
{
	return baseForm->refID == 0x10;
}

extern ModelLoader *g_modelLoader;

void TESObjectREFR::Update3D()
{
	if (this == PlayerCharacter::GetSingleton())
	{
		ThisStdCall(kUpdateAppearanceAddr, this);
	}
	else
	{
		Set3D(NULL, true);
		ModelLoader::GetSingleton()->QueueReference(this, 1, 0);
	}
}

TESObjectREFR *TESObjectREFR::Create(bool bTemp)
{
	TESObjectREFR *refr = (TESObjectREFR*)GameHeapAlloc(sizeof(TESObjectREFR));
	ThisStdCall(s_TESObject_REFR_init, refr);
	if (bTemp) ThisStdCall(0x484490, refr);
	return refr;
}

void Actor::EquipItem(TESForm* objType, UInt32 unequipCount, ExtraDataList* itemExtraList, bool arg4, bool lockUnequip, bool noMessage)
{
	ThisStdCall(s_Actor_EquipItem, this, objType, unequipCount, itemExtraList, arg4, lockUnequip, noMessage);
}

void Actor::UnequipItem(TESForm* objType, UInt32 unequipCount, ExtraDataList* itemExtraList, bool arg4, bool lockUnequip, bool noMessage)
{
	ThisStdCall(s_Actor_UnequipItem, this, objType, unequipCount, itemExtraList, arg4, lockUnequip, noMessage);
}