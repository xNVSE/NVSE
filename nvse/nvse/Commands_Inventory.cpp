#include "Commands_Inventory.h"

#include <unordered_set>

#include "InventoryInfo.h"
#include "InventoryReference.h"

#include "GameObjects.h"
#include "GameProcess.h"

static const Cmd_Execute Cmd_EquipItem_Execute		= (Cmd_Execute)0x005D0060;
static const Cmd_Execute Cmd_UnequipItem_Execute	= (Cmd_Execute)0x005D0300;


void GetWeight_Call(TESForm* form, double *result)
{
	TESWeightForm* weightForm = DYNAMIC_CAST(form, TESForm, TESWeightForm);
	if (weightForm)
	{
		*result = weightForm->weight;
	}
	else 
	{
		TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
		if (pAmmo) 
		{
			*result = pAmmo->weight;
		}
	}
}

// testing conditionals with this
bool Cmd_GetWeight_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (arg1)
	{
		TESForm* form = (TESForm*)arg1;
		GetWeight_Call(form, result);
	}
	else if (thisObj)  //evaluates if the condition has an INVALID (default) parameter
	{
		TESForm* form = thisObj->baseForm;
		GetWeight_Call(form, result);
	}
	if (IsConsoleMode())
		Console_Print("GetWeight >> %.2f", *result);
	return true;
}

bool Cmd_GetWeight_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form))
	{
		form = form->TryGetREFRParent();
		if (!form && thisObj)
			form = thisObj->baseForm;

		if (form)
			return Cmd_GetWeight_Eval(thisObj, form, 0, result);
	}

	return true;
}

bool Cmd_GetHealth_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}

		TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
		if (pHealth) {
			*result = pHealth->health;
			if (IsConsoleMode())
				Console_Print("GetHealth >> %.2f", *result);
		}
	}
	return true;
}
bool Cmd_GetHealth_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (thisObj)  //evaluates if the condition has an INVALID (default) parameter. 
				//With kParams_OneOptionalForm, arg1 will always be INVALID (no way to select a form).
	{
		TESForm* pForm = thisObj->baseForm;
		TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
		if (pHealth)
		{
			*result = pHealth->health;
#if _DEBUG
			Console_Print("GetHealth >> %.2f", *result);
#endif
		}
	}

	return true;
}

bool Cmd_GetValue_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	TESValueForm* pValue = DYNAMIC_CAST(pForm, TESForm, TESValueForm);
	if (pValue) {
		*result = pValue->value;
		if (IsConsoleMode())
			Console_Print("GetValue >> %.2f", *result);
	}
	return true;
}
bool Cmd_GetValue_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm;

	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;
	
	TESValueForm* pValue = DYNAMIC_CAST(pForm, TESForm, TESValueForm);
	if (pValue) 
	{
		*result = pValue->value;
#if _DEBUG
		Console_Print("GetValue >> %.2f", *result);
#endif
	}
	return true;
}

TESForm* Extract_IntAndForm(COMMAND_ARGS, UInt32& intVal)
{
	TESForm* pForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &intVal, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (thisObj) {
				pForm = thisObj->baseForm;
			}
		}
	}
	return pForm;
}

TESForm* Extract_FloatAndForm(COMMAND_ARGS, float& floatVal)
{
	TESForm* pForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &floatVal, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (thisObj) {
				pForm = thisObj->baseForm;
			}
		}
	}
	return pForm;
}

TESObjectWEAP* Extract_IntAndWeapon(COMMAND_ARGS, UInt32& intVal) {
	TESObjectWEAP* pWeapon = NULL;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm) {
		pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);	
	}
	return pWeapon;
}


TESObjectWEAP* Extract_FloatAndWeapon(COMMAND_ARGS, float& floatVal) {
	TESObjectWEAP* pWeapon = NULL;
	TESForm* pForm = Extract_FloatAndForm(PASS_COMMAND_ARGS, floatVal);
	if (pForm) {
		pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);	
	}
	return pWeapon;
}

bool Cmd_SetWeight_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESForm* pForm = Extract_FloatAndForm(PASS_COMMAND_ARGS, floatVal);
	if (pForm) {
		TESWeightForm* pWeightForm = DYNAMIC_CAST(pForm, TESForm, TESWeightForm);
		if (pWeightForm) {
			pWeightForm->weight = floatVal;
		} else {
			TESAmmo* pAmmo = DYNAMIC_CAST(pForm, TESForm, TESAmmo);
			if (pAmmo) {
				pAmmo->weight = floatVal;
			}
		}
	}
	return true;
}

bool Cmd_SetBaseItemValue_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	UInt32 newVal = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form, &newVal))
	{
		TESValueForm* valForm = DYNAMIC_CAST(form, TESForm, TESValueForm);
		if (valForm) {
			valForm->value = newVal;
			//CALL_MEMBER_FN(valForm, SetValue)(newVal);
		}
	}

	return true;
}

bool Cmd_SetHealth_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 health = 0;
	TESForm* pForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &health, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}

		TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
		if (pHealth) {
			pHealth->health = health;
		}
	}
	return true;
}

bool Cmd_GetType_Execute(COMMAND_ARGS)
{
	*result= 0;
	TESForm* form = 0;

	if(!ExtractArgsEx(EXTRACT_ARGS_EX, &form)) return true;
	form = form->TryGetREFRParent();
	if (!form) {
		if (!thisObj || !thisObj->baseForm) return true;
		form = thisObj->baseForm;
	}

#if _DEBUG
	bool bDump = false;
	if (bDump) {
		DumpClass(form);		
	}
#endif
	*result = form->typeID;	
	if (IsConsoleMode()) {
		Console_Print("Type: %d", form->typeID);
	}
	return true;
}
bool Cmd_GetType_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (thisObj)
	{
		TESForm* form = thisObj->baseForm;
		*result = form->typeID;
#if _DEBUG
		Console_Print("Type of %s: %d", form->GetName(), form->typeID);
#endif
	}
	return true;
}

bool Cmd_GetRepairList_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	BGSRepairItemList* pRepairList = DYNAMIC_CAST(pForm, TESForm, BGSRepairItemList);
	if (pRepairList && pRepairList->listForm) {
		*((UInt32*)result) = pRepairList->listForm->refID;
#if _DEBUG
		Console_Print("repair list: %X", pRepairList->listForm->refID);
#endif
	}
	return true;
}

bool Cmd_GetEquipType_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;
	
	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	BGSEquipType* pEquipType = DYNAMIC_CAST(pForm, TESForm, BGSEquipType);
	if (pEquipType) {
		*result = pEquipType->equipType;
	}
	return true;
}
bool Cmd_GetEquipType_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = 0;

	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	BGSEquipType* pEquipType = DYNAMIC_CAST(pForm, TESForm, BGSEquipType);
	if (pEquipType) 
	{
		*result = pEquipType->equipType;
#if _DEBUG
		Console_Print("GetEquipType >> %f", *result);
#endif
	}
	return true;
}

bool Cmd_GetWeaponAmmo_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;
	
	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	BGSAmmoForm* pAmmoForm = DYNAMIC_CAST(pForm, TESForm, BGSAmmoForm);
	if (pAmmoForm && pAmmoForm->ammo) {
		*((UInt32*)result) = pAmmoForm->ammo->refID;
#if _DEBUG
		TESFullName* ammoName = DYNAMIC_CAST(pAmmoForm->ammo, TESAmmo, TESFullName);
		Console_Print("ammo: %X (%s)", pAmmoForm->ammo->refID, ammoName ? ammoName->name.m_data : "unknown ammo");
#endif
	}
	return true;
}

bool Cmd_GetWeaponClipRounds_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;
	
	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}
	BGSClipRoundsForm* pClipRounds = DYNAMIC_CAST(pForm, TESForm, BGSClipRoundsForm);
	if (pClipRounds) {
		*result = pClipRounds->clipRounds;
#if _DEBUG
		Console_Print("clipRounds: %d", pClipRounds->clipRounds);
#endif
	}
	return true;
}
bool Cmd_GetWeaponClipRounds_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;
	
	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;
	
	BGSClipRoundsForm* pClipRounds = DYNAMIC_CAST(pForm, TESForm, BGSClipRoundsForm);
	if (pClipRounds) 
	{
		*result = pClipRounds->clipRounds;
#if _DEBUG
		Console_Print("clipRounds: %d", pClipRounds->clipRounds);
#endif
	}
	return true;
}

bool Cmd_GetAttackDamage_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = 0;
	
	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	TESAttackDamageForm* pDamage = DYNAMIC_CAST(pForm, TESForm, TESAttackDamageForm);
	if (pDamage) {
		*result = pDamage->damage;
#if _DEBUG
		Console_Print("damage: %d", pDamage->damage);
#endif
	}
	return true;
}
bool Cmd_GetAttackDamage_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;

	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	TESAttackDamageForm* pDamage = DYNAMIC_CAST(pForm, TESForm, TESAttackDamageForm);
	if (pDamage) {
		*result = pDamage->damage;
#if _DEBUG
		Console_Print("damage: %d", pDamage->damage);
#endif
	}
	return true;
}

enum EWeapValues
{
	eWeap_Type = 0,
	eWeap_MinSpread,
	eWeap_Spread,
	eWeap_Proj,
	eWeap_SightFOV,
	eWeap_MinRange,
	eWeap_Range,
	eWeap_AmmoUse,
	eWeap_APCost,
	eWeap_CritDam,
	eWeap_CritChance,
	eWeap_CritEffect,
	eWeap_FireRate,
	eWeap_AnimAttackMult,
	eWeap_RumbleLeft,
	eWeap_RumbleRight,
	eWeap_RumbleDuration,
	eWeap_RumbleWaveLength,
	eWeap_AnimShotsPerSec,
	eWeap_AnimReloadTime,
	eWeap_AnimJamTime,
	eWeap_Skill,
	eWeap_ResistType,
	eWeap_FireDelayMin,
	eWeap_FireDelayMax,
	eWeap_AnimMult,
	eWeap_Reach,
	eWeap_IsAutomatic,
	eWeap_HandGrip,
	eWeap_ReloadAnim,
	eWeap_VATSChance,
	eWeap_AttackAnim,
	eWeap_NumProj,
	eWeap_AimArc,
	eWeap_LimbDamageMult,
	eWeap_SightUsage,
	eWeap_ReqStr,
	eWeap_ReqSkill,
	eWeap_LongBursts,
	eWeap_Flags1,
	eWeap_Flags2,
	eWeap_RegenRate,
};

bool GetWeaponValue(TESObjectWEAP* pWeapon, UInt32 whichVal, double *result)
{
	if (pWeapon) {
		switch(whichVal) {
			case eWeap_Type:				*result = pWeapon->eWeaponType; break;
			case eWeap_MinSpread:			*result = pWeapon->minSpread; break;
			case eWeap_Spread:				*result = pWeapon->spread; break;
			case eWeap_Proj:
			{
				BGSProjectile* pProj = pWeapon->projectile;
				if (pProj) {
					*((UInt32*)result) = pProj->refID;
				}
			}
			break;
			case eWeap_SightFOV:			*result = pWeapon->sightFOV; break;
			case eWeap_MinRange:			*result = pWeapon->minRange; break;
			case eWeap_Range:				*result = pWeapon->maxRange; break;
			case eWeap_AmmoUse:				*result = pWeapon->ammoUse; break;
			case eWeap_APCost:				*result = pWeapon->AP; break;
			case eWeap_CritDam:				*result = pWeapon->criticalDamage; break;
			case eWeap_CritChance:			*result = pWeapon->criticalPercent; break;
			case eWeap_CritEffect:
			{
				SpellItem* pSpell = pWeapon->criticalEffect;
				if (pSpell) {
					*((UInt32*)result) = pSpell->refID;
				}
			}
			break;
			case eWeap_FireRate:			*result = pWeapon->fireRate; break;
			case eWeap_AnimAttackMult:		*result = pWeapon->animAttackMult; break;
			case eWeap_RumbleLeft:			*result = pWeapon->rumbleLeftMotor; break;
			case eWeap_RumbleRight:			*result = pWeapon->rumbleRightMotor; break;
			case eWeap_RumbleDuration:		*result = pWeapon->rumbleDuration; break;
			case eWeap_RumbleWaveLength:	*result = pWeapon->rumbleWavelength; break;
			case eWeap_AnimShotsPerSec:		*result = pWeapon->animShotsPerSec; break;
			case eWeap_AnimReloadTime:		*result = pWeapon->animReloadTime; break;
			case eWeap_AnimJamTime:			*result = pWeapon->animJamTime; break;
			case eWeap_Skill:				*result = pWeapon->weaponSkill; break;
			case eWeap_ResistType:			*result = pWeapon->resistType; break;
			case eWeap_FireDelayMin:		*result = pWeapon->semiAutoFireDelayMin; break;
			case eWeap_FireDelayMax:		*result = pWeapon->semiAutoFireDelayMax; break;
			case eWeap_AnimMult:			*result = pWeapon->animMult; break;
			case eWeap_Reach:				*result = pWeapon->reach; break;
			case eWeap_IsAutomatic:			*result = pWeapon->IsAutomatic(); break;
			case eWeap_HandGrip:			*result = pWeapon->HandGrip(); break;
			case eWeap_ReloadAnim:			*result = pWeapon->reloadAnim; break;
			case eWeap_VATSChance:			*result = pWeapon->baseVATSChance; break;
			case eWeap_AttackAnim:			*result = pWeapon->AttackAnimation(); break;
			case eWeap_NumProj:				*result = pWeapon->numProjectiles; break;
			case eWeap_AimArc:				*result = pWeapon->aimArc; break;
			case eWeap_LimbDamageMult:		*result = pWeapon->limbDamageMult; break;
			case eWeap_SightUsage:			*result = pWeapon->sightUsage; break;
			case eWeap_ReqStr:				*result = pWeapon->strRequired; break;
			case eWeap_ReqSkill:			*result = pWeapon->skillRequirement; break;
			case eWeap_LongBursts:			*result = pWeapon->weaponFlags2.IsSet(TESObjectWEAP::eFlag_LongBurst); break;
			case eWeap_Flags1:				*result = pWeapon->weaponFlags1.Get(); break;
			case eWeap_Flags2:				*result = pWeapon->weaponFlags2.Get(); break;
			case eWeap_RegenRate:			*result = pWeapon->regenRate; break;
			default: HALT("unknown weapon value"); break;
		}
	}
	return true;
}

bool GetWeaponValue_Execute(COMMAND_ARGS, UInt32 whichVal)
{
	*result = 0;
	TESForm* pForm = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}
	TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
	if (!pWeapon)
		return true;
	return GetWeaponValue(pWeapon, whichVal, result);
}

bool GetWeaponValue_Eval(COMMAND_ARGS_EVAL, UInt32 whichVal)
{
	*result = 0;
	TESForm* pForm = 0;
	
	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;
	
	TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);	
	return GetWeaponValue(pWeapon, whichVal, result);
}

bool Cmd_GetWeaponType_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Type);
}
bool Cmd_GetWeaponType_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Type);
}

bool Cmd_GetWeaponMinSpread_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_MinSpread);
}
bool Cmd_GetWeaponMinSpread_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_MinSpread);
}

bool Cmd_GetWeaponSpread_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Spread);
}
bool Cmd_GetWeaponSpread_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Spread);
}

bool Cmd_GetWeaponProjectile_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Proj);
}
//No Eval since it returns a ref.

bool Cmd_GetWeaponSightFOV_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_SightFOV);
}
bool Cmd_GetWeaponSightFOV_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_SightFOV);
}

bool Cmd_GetWeaponMinRange_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_MinRange);
}
bool Cmd_GetWeaponMinRange_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_MinRange);
}

bool Cmd_GetWeaponMaxRange_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Range);
}
bool Cmd_GetWeaponMaxRange_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Range);
}

bool Cmd_GetWeaponAmmoUse_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AmmoUse);
}
bool Cmd_GetWeaponAmmoUse_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AmmoUse);
}

bool Cmd_GetWeaponActionPoints_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_APCost);
}
bool Cmd_GetWeaponActionPoints_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_APCost);
}

bool Cmd_GetWeaponCritDamage_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_CritDam);
}
bool Cmd_GetWeaponCritDamage_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_CritDam);
}

bool Cmd_GetWeaponCritChance_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_CritChance);
}
bool Cmd_GetWeaponCritChance_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_CritChance);
}

bool Cmd_GetWeaponCritEffect_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_CritEffect);
}
//No Eval since it returns a ref.

bool Cmd_GetWeaponFireRate_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_FireRate);
}
bool Cmd_GetWeaponFireRate_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_FireRate);
}

bool Cmd_GetWeaponAnimAttackMult_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AnimAttackMult);
}
bool Cmd_GetWeaponAnimAttackMult_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AnimAttackMult);
}

bool Cmd_GetWeaponRumbleLeftMotor_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_RumbleLeft);
}
bool Cmd_GetWeaponRumbleLeftMotor_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_RumbleLeft);
}

bool Cmd_GetWeaponRumbleRightMotor_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_RumbleRight);
}
bool Cmd_GetWeaponRumbleRightMotor_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_RumbleRight);
}

bool Cmd_GetWeaponRumbleDuration_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_RumbleDuration);
}
bool Cmd_GetWeaponRumbleDuration_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_RumbleDuration);
}

bool Cmd_GetWeaponRumbleWavelength_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_RumbleWaveLength);
}
bool Cmd_GetWeaponRumbleWavelength_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_RumbleWaveLength);
}

bool Cmd_GetWeaponAnimShotsPerSec_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AnimShotsPerSec);
}
bool Cmd_GetWeaponAnimShotsPerSec_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AnimShotsPerSec);
}

bool Cmd_GetWeaponAnimReloadTime_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AnimReloadTime);
}
bool Cmd_GetWeaponAnimReloadTime_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AnimReloadTime);
}

bool Cmd_GetWeaponAnimJamTime_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AnimJamTime);
}
bool Cmd_GetWeaponAnimJamTime_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AnimJamTime);
}

bool Cmd_GetWeaponSkill_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Skill);
}
bool Cmd_GetWeaponSkill_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Skill);
}

bool Cmd_GetWeaponResistType_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_ResistType);
}
bool Cmd_GetWeaponResistType_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_ResistType);
}

bool Cmd_GetWeaponFireDelayMin_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_FireDelayMin);
}
bool Cmd_GetWeaponFireDelayMin_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_FireDelayMin);
}

bool Cmd_GetWeaponFireDelayMax_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_FireDelayMax);
}
bool Cmd_GetWeaponFireDelayMax_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_FireDelayMax);
}

bool Cmd_GetWeaponAnimMult_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AnimMult);
}
bool Cmd_GetWeaponAnimMult_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AnimMult);
}

bool Cmd_GetWeaponReach_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Reach);
}
bool Cmd_GetWeaponReach_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Reach);
}

bool Cmd_GetWeaponIsAutomatic_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_IsAutomatic);
}
bool Cmd_GetWeaponIsAutomatic_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_IsAutomatic);
}

bool Cmd_GetWeaponHandGrip_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_HandGrip);
}
bool Cmd_GetWeaponHandGrip_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_HandGrip);
}

bool Cmd_GetWeaponReloadAnim_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_ReloadAnim);
}
bool Cmd_GetWeaponReloadAnim_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_ReloadAnim);
}

bool Cmd_GetWeaponBaseVATSChance_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_VATSChance);
}
bool Cmd_GetWeaponBaseVATSChance_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_VATSChance);
}

bool Cmd_GetWeaponAttackAnimation_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AttackAnim);
}
bool Cmd_GetWeaponAttackAnimation_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AttackAnim);
}

bool Cmd_GetWeaponNumProjectiles_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_NumProj);
}
bool Cmd_GetWeaponNumProjectiles_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_NumProj);
}

bool Cmd_GetWeaponAimArc_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_AimArc);
}
bool Cmd_GetWeaponAimArc_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_AimArc);
}

bool Cmd_GetWeaponLimbDamageMult_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_LimbDamageMult);
}
bool Cmd_GetWeaponLimbDamageMult_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_LimbDamageMult);
}

bool Cmd_GetWeaponSightUsage_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_SightUsage);
}
bool Cmd_GetWeaponSightUsage_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_SightUsage);
}

bool Cmd_GetWeaponRequiredStrength_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_ReqStr);
}
bool Cmd_GetWeaponRequiredStrength_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_ReqStr);
}

bool Cmd_GetWeaponRequiredSkill_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_ReqSkill);
}
bool Cmd_GetWeaponRequiredSkill_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_ReqSkill);
}

bool Cmd_GetWeaponLongBursts_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_LongBursts);
}
bool Cmd_GetWeaponLongBursts_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_LongBursts);
}

bool Cmd_GetWeaponFlags1_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Flags1);
}
bool Cmd_GetWeaponFlags1_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Flags1);
}

bool Cmd_GetWeaponFlags2_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_Flags2);
}
bool Cmd_GetWeaponFlags2_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_Flags2);
}

// testing conditionals with this
bool Cmd_GetWeaponHasScope_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* form = (TESForm*)arg1;
	if (form)
	{
		TESObjectWEAP* weapon = DYNAMIC_CAST(form, TESForm, TESObjectWEAP);
		if (weapon)
		{
			*result = weapon->HasScope() ? 1 : 0;
		}
	}
	else if (thisObj)
	{
		form = thisObj->baseForm;
		TESObjectWEAP* weapon = DYNAMIC_CAST(form, TESForm, TESObjectWEAP);
		if (weapon)
		{
			*result = weapon->HasScope();
		}
#if _DEBUG
		Console_Print("GetWeaponHasScope >> %f", *result);
#endif
	}

	return true;
}

bool Cmd_GetWeaponHasScope_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form))
	{
		form = form->TryGetREFRParent();
		if (!form && thisObj)
			form = thisObj->baseForm;

		if (form)
			return Cmd_GetWeaponHasScope_Eval(thisObj, form, 0, result);
	}

	return true;
}

bool Cmd_GetWeaponItemMod_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 whichMod = 0;
	TESObjectWEAP* pWeap = Extract_IntAndWeapon(PASS_COMMAND_ARGS, whichMod);
	if (pWeap) {
		TESObjectIMOD* pItemMod = pWeap->GetItemMod(whichMod);
		if (pItemMod) {
			*((UInt32*)result) = pItemMod->refID;
		}
	}
	return true;
}

bool Cmd_SetWeaponClipRounds_Execute(COMMAND_ARGS) 
{
	*result = 0;
	UInt32 clipRounds = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, clipRounds);
	if (pForm) {
		BGSClipRoundsForm* pClipRounds = DYNAMIC_CAST(pForm, TESForm, BGSClipRoundsForm);
		if (pClipRounds) {
			pClipRounds->clipRounds = clipRounds;
#if _DEBUG
			Console_Print("clipRounds: %d", pClipRounds->clipRounds);
#endif
		}
	}
	return true;
}

bool Cmd_SetWeaponMinSpread_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->minSpread = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponSpread_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->spread = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponSightFOV_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->sightFOV = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponMinRange_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->minRange = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponMaxRange_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->maxRange = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponAmmoUse_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->ammoUse = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponActionPoints_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->AP = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponCritDamage_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->criticalDamage = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponType_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal < TESObjectWEAP::kWeapType_Last) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->eWeaponType = intVal;
		}
	}
	return true;
}


bool Cmd_SetWeaponCritChance_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->criticalPercent = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponCritEffect_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	MagicItem* pMagicItem = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pMagicItem, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}
		SpellItem* pSpell = DYNAMIC_CAST(pMagicItem, MagicItem, SpellItem);
		if (pForm && pSpell) {
			TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
			if (pWeapon) {
				pWeapon->criticalEffect = pSpell;
			}
		}
	}
	return true;
}

bool Cmd_SetWeaponFireRate_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->fireRate = floatVal;
	}
	return true;
}

bool Cmd_SetAttackDamage_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm) {
		TESAttackDamageForm* pAttackDmg = DYNAMIC_CAST(pForm, TESForm, TESAttackDamageForm);
		if (pAttackDmg) {
			pAttackDmg->damage = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponAmmo_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	TESForm* pAmmoForm = NULL;
	bool bExtracted = ExtractArgsEx(EXTRACT_ARGS_EX, &pAmmoForm, &pForm);
	if (bExtracted) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}
		
		if (pForm) {
			TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
			if (pWeapon) {
				TESAmmo* pAmmo = DYNAMIC_CAST(pAmmoForm, TESForm, TESAmmo);
				BGSListForm* pAmmoList = DYNAMIC_CAST(pAmmoForm, TESForm, BGSListForm);
				
				if (pAmmo) {
					pWeapon->ammo.ammo = pAmmo;
				} else if (pAmmoList) {
					pWeapon->ammo.ammo = pAmmoList;
				}
			}
		}
	}
	return true;
}

bool Cmd_SetWeaponProjectile_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	TESForm* pProjectileForm = NULL;
	bool bExtracted = ExtractArgsEx(EXTRACT_ARGS_EX, &pProjectileForm, &pForm);
	if (bExtracted) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}
		BGSProjectile* pProjectile = DYNAMIC_CAST(pProjectileForm, TESForm, BGSProjectile);
		if (pForm && pProjectile) {
			TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
			if (pWeapon) {
				pWeapon->projectile = pProjectile;
			}
		}
	}
	return true;
}

bool Cmd_SetRepairList_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	BGSListForm* pListForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}

		BGSRepairItemList* pRepairList = DYNAMIC_CAST(pForm, TESForm, BGSRepairItemList);
		if (pRepairList && pListForm) {
			pRepairList->listForm = pListForm;
		}
	}
	return true;
}


bool Cmd_SetWeaponAnimMult_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->animMult = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponAnimAttackMult_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->animAttackMult = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponReach_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->reach = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponIsAutomatic_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->SetIsAutomatic(intVal == 1);
		}
	}
	return true;
}

bool Cmd_SetWeaponHandGrip_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal < TESObjectWEAP::eHandGrip_Count) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->SetHandGrip(intVal);
		}
	}
	return true;
}

bool Cmd_SetWeaponReloadAnim_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal < TESObjectWEAP::eReload_Count) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->reloadAnim = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponBaseVATSChance_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal <= 100) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->baseVATSChance = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponAttackAnimation_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal < TESObjectWEAP::eAttackAnim_Count) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->SetAttackAnimation(intVal);
		}
	}
	return true;
}

bool Cmd_SetWeaponNumProjectiles_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, intVal);
	if (pForm && intVal <= 255) {
		TESObjectWEAP* pWeapon = DYNAMIC_CAST(pForm, TESForm, TESObjectWEAP);
		if (pWeapon) {
			pWeapon->numProjectiles = intVal;
		}
	}
	return true;
}

bool Cmd_SetWeaponAimArc_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->aimArc = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponLimbDamageMult_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->limbDamageMult = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponSightUsage_Execute(COMMAND_ARGS)
{
	*result = 0;
	float floatVal = 0.0;
	TESObjectWEAP* pWeapon = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, floatVal);
	if (pWeapon) {
		pWeapon->sightUsage = floatVal;
	}
	return true;
}

bool Cmd_SetWeaponRequiredStrength_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if (pWeapon) {
		pWeapon->strRequired = intVal;
	}
	return true;	
}

bool Cmd_SetWeaponRequiredSkill_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if (pWeapon) {
		pWeapon->skillRequirement = intVal;
	}
	return true;	
}

bool Cmd_SetWeaponSkill_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if (pWeapon && intVal >= eActorVal_SkillsStart && intVal <= eActorVal_SkillsEnd) {
		pWeapon->weaponSkill = intVal;
	}
	return true;	
}

bool Cmd_SetWeaponResistType_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if (pWeapon) {
		pWeapon->resistType = intVal;
	}
	return true;	
}

bool Cmd_SetWeaponLongBursts_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if(pWeapon)
		pWeapon->weaponFlags2.Write(TESObjectWEAP::eFlag_LongBurst, intVal != 0);

	return true;	
}

bool Cmd_SetWeaponFlags1_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if(pWeapon)
		pWeapon->weaponFlags1.RawSet(intVal);

	return true;	
}

bool Cmd_SetWeaponFlags2_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 intVal = 0;
	TESObjectWEAP* pWeapon = Extract_IntAndWeapon(PASS_COMMAND_ARGS, intVal);
	if(pWeapon)
		pWeapon->weaponFlags2.RawSet(intVal);

	return true;	
}

bool Cmd_GetWeaponRegenRate_Execute(COMMAND_ARGS)
{
	return GetWeaponValue_Execute(PASS_COMMAND_ARGS, eWeap_RegenRate);
}
bool Cmd_GetWeaponRegenRate_Eval(COMMAND_ARGS_EVAL)
{
	return GetWeaponValue_Eval(PASS_CMD_ARGS_EVAL, eWeap_RegenRate);
}

bool Cmd_SetWeaponRegenRate_Execute(COMMAND_ARGS)
{
	*result = 0;
	float regenRate;
	auto pWeap = Extract_FloatAndWeapon(PASS_COMMAND_ARGS, regenRate);
	if (!pWeap) return true;
	pWeap->regenRate = regenRate;
	*result = 1;
	return true;
}


// Get Equipped Object utility functions
class MatchBySlot : public FormMatcher
{
	UInt32 m_slotMask;
public:
	MatchBySlot(UInt32 slot) : m_slotMask(TESBipedModelForm::MaskForSlot(slot)) {}
	bool Matches(TESForm* pForm) const {
		UInt32 formMask = 0;
		if (pForm) {
			if (pForm->IsWeapon()) {
				formMask = TESBipedModelForm::eSlot_Weapon;
			} else {
				TESBipedModelForm* pBip = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
				if (pBip) {
					formMask = pBip->partMask;
				}
			}
		}
		return (formMask & m_slotMask) != 0;
	}
};

class MatchBySlotMask : public FormMatcher
{
	UInt32 m_targetMask;
	UInt32 m_targetData;
public:
	MatchBySlotMask(UInt32 targetMask, UInt32 targetData) : m_targetMask(targetMask) {}
	bool Matches(TESForm* pForm) const {
		UInt32 slotMask = 0;
		if (pForm) {
			if (pForm->IsWeapon()) {
				slotMask = TESBipedModelForm::ePart_Weapon;
			} else {
				TESBipedModelForm* pBip = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
				if (pBip) {
					slotMask = pBip->partMask;
				}
			}
		}
		return ((slotMask & m_targetMask) == m_targetData); 
	}
};


static EquipData FindEquipped(TESObjectREFR* thisObj, FormMatcher& matcher) {
	ExtraContainerChanges* pContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	return (pContainerChanges) ? pContainerChanges->FindEquipped(matcher) : EquipData();
}



bool Cmd_GetEquippedObject_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32* refResult = ((UInt32*)result);

	if (thisObj) {
		UInt32 slotIdx = 0;
		if (ExtractArgs(EXTRACT_ARGS, &slotIdx)) {
			MatchBySlot matcher(slotIdx);
			EquipData equipD = FindEquipped(thisObj, matcher); 
			TESForm* pFound = equipD.pForm;
			if (pFound) {
				*refResult = pFound->refID;
				if (IsConsoleMode()) {
					Console_Print("At Slot %d: (%X)%s", slotIdx, pFound->refID, pFound->GetFullName()->name.m_data);
				}
			}
		}
	}
	return true;
}

bool Cmd_GetEquippedCurrentHealth_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (thisObj) {
		UInt32 slotIdx = 0;
		if (ExtractArgs(EXTRACT_ARGS, &slotIdx)) {
			MatchBySlot matcher(slotIdx);
			EquipData equipD = FindEquipped(thisObj, matcher);
			if (equipD.pForm) {
				ExtraHealth* pXHealth = equipD.pExtraData ? (ExtraHealth*)equipD.pExtraData->GetByType(kExtraData_Health) : NULL;
				if (pXHealth) {
					*result = pXHealth->health;
					if (IsConsoleMode()) {
						Console_Print("GetEquippedCurrentHealth: %.2f", pXHealth->health);
					}
				} else {
					TESHealthForm* pHealth = DYNAMIC_CAST(equipD.pForm, TESForm, TESHealthForm);
					if (pHealth) {
						*result = pHealth->health;
						if (IsConsoleMode()) {
							Console_Print("GetEquippedCurrentHealth: baseHealth: %d", pHealth->health);
						}
					}
				}
			}
		}
	}
	return true;
}
bool Cmd_GetEquippedCurrentHealth_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;

	if (thisObj) 
	{
		UInt32 slotIdx = (UInt32)arg1;
		MatchBySlot matcher(slotIdx);
		EquipData equipD = FindEquipped(thisObj, matcher);
		if (equipD.pForm) 
		{
			ExtraHealth* pXHealth = equipD.pExtraData ? (ExtraHealth*)equipD.pExtraData->GetByType(kExtraData_Health) : NULL;
			if (pXHealth) 
			{
				*result = pXHealth->health;
			}
			else 
			{
				TESHealthForm* pHealth = DYNAMIC_CAST(equipD.pForm, TESForm, TESHealthForm);
				if (pHealth)
					*result = pHealth->health;
			}
		}
	}
#if _DEBUG
	Console_Print("GetEquippedCurrentHealth: baseHealth >> %f", *result);
#endif

	return true;
}

bool Cmd_CompareNames_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	TESForm* base = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form, &base);
	form = form->TryGetREFRParent();
	base = base->TryGetREFRParent();

	if (!form)
		return true;
	if (!base)
	{
		if (!thisObj)
			return true;
		base = thisObj->baseForm;
	}

	TESFullName* first = DYNAMIC_CAST(base, TESForm, TESFullName);
	TESFullName* second = DYNAMIC_CAST(form, TESForm, TESFullName);
	if (first && second) 
		*result = first->name.Compare(second->name); 

	return true;
}

// SetName
bool Cmd_SetName_Execute(COMMAND_ARGS)
{
	if (!result) return true;

	TESForm* form = NULL;
	char	string[256];

	ExtractArgsEx(EXTRACT_ARGS_EX, &string, &form);
	form = form->TryGetREFRParent();
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESFullName* name = form->GetFullName();
	if (name) {
		name->name.Set(string);
	}
	return true;
}

bool Cmd_GetHotkeyItem_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	UInt32 hotkeyNum;	// passed as 1 - 8, stored by game as 0-7
	if (ExtractArgs(EXTRACT_ARGS, &hotkeyNum) && --hotkeyNum < 8)
	{
		BSExtraData* xData = PlayerCharacter::GetSingleton()->extraDataList.GetByType(kExtraData_ContainerChanges);
		ExtraContainerChanges* xChanges = (ExtraContainerChanges *)xData;
		if (xChanges)
		{
			for (ExtraContainerChanges::EntryDataList::Iterator itemIter = xChanges->data->objList->Begin();
				!itemIter.End();
				++itemIter)
			{
				for (ExtraContainerChanges::ExtendDataList::Iterator iter = itemIter->extendData->Begin();
					!iter.End();
					++iter)
				{
					if (!iter.Get()) {
						continue;
					}

					ExtraHotkey* xHotKey = (ExtraHotkey*)iter->GetByType(kExtraData_Hotkey);
					if (xHotKey && xHotKey->index == hotkeyNum)
					{
						*refResult = itemIter->type->refID;
						if (IsConsoleMode())
							Console_Print("GetHotkeyItem >> %08x (%s)", *refResult, GetFullName(itemIter->type));

						return true;
					}
				}
			}
		}
	}

	// not found
	if (IsConsoleMode())
		Console_Print("GetHotkeyItem >> Hotkey not assigned");

	return true;
}

#if 0
class ExtraContainerChangesEntryFinder {
public:
	TESForm* m_whichForm;

	ExtraContainerChangesEntryFinder ( TESForm* whichForm ) : m_whichForm(whichForm)
	{ }

	bool Accept(ExtraContainerChanges::EntryData* entryData)
	{
		return entryData->type == m_whichForm;
	}
};
class ExtraQuickKeyFinder
{
public:
	UInt32 m_whichKey;
	TESForm* m_whichForm;

	ExtraQuickKeyFinder(UInt32 whichKey, TESForm* whichForm = NULL) : m_whichKey(whichKey), m_whichForm(whichForm)
	{ }

	bool Accept(ExtraContainerChanges::EntryData* entryData)
	{
		//check if TESForm matches
		if (m_whichForm && entryData->type != m_whichForm)
			return false;
		//		if (m_whichForm && entryData->type == m_whichForm)
		//			return true;

		//check if it looks like a hotkey
		if (!entryData->extendData || !entryData->extendData->Head())
			return false;
		ExtraHotkey* qKey = GetByTypeCast(entryData->extendData, ExtraHotkey)
		if (!qKey)
			return false;

		//check if hotkey # matches
		if (m_whichKey != UInt32(-1) && qKey->index != m_whichKey)
			return false;

		//passed all checks, return true:
		return true;
	}
};

static void _ClearHotKey ( UInt32 whichKey ) {
	if (whichKey > 7)
		return;

	//remove ExtraQuickKey from container changes
	ExtraContainerChanges* xChanges = GetByTypeCast(PlayerCharacter::GetSingleton()->extraDataList, kExtraData_ContainerChanges);
	if (xChanges)
	{
		ExtraQuickKeyFinder finder(whichKey);
		ExtraEntryVisitor visitor(xChanges->data->objList);
		const ExtraContainerChanges::Entry* xEntry;
		while (xEntry = visitor.Find(finder))
		{
			BSExtraData* toRemove = xEntry->data->extendData->data->GetByType(kExtraData_QuickKey);
			if (toRemove)
			{
				xEntry->data->extendData->data->Remove(toRemove);
				FormHeap_Free(toRemove);
			}
		}
	}
	//clear quickkey
	NiTPointerList <TESForm> * quickKey = &g_quickKeyList[whichKey];
	if (quickKey->start && quickKey->start->data)
	{
		if (xChanges)
		{
			ExtraQuickKeyFinder finder(UInt32(-1), quickKey->start->data);
			ExtraEntryVisitor visitor(xChanges->data->objList);
			const ExtraContainerChanges::Entry* xEntry = NULL;
			while (xEntry = visitor.Find(finder, xEntry))
			{
				ExtraQuickKey* toRemove = static_cast<ExtraQuickKey*>(xEntry->data->extendData->data->GetByType(kExtraData_QuickKey));
				//if (toRemove && (quickKey->start->data->typeID != kFormType_SpellItem || toRemove->keyID > 7))
				if (toRemove && toRemove->keyID > 7)
				{
					xEntry->data->extendData->data->Remove(toRemove);
					FormHeap_Free(toRemove);
					xEntry = NULL;
				}
			}
		}

		quickKey->FreeNode(quickKey->start);
		quickKey->numItems = 0;
		quickKey->start = NULL;
		quickKey->end = NULL;
	}
}
static bool Cmd_ClearHotKey_Execute(COMMAND_ARGS)
{
	UInt32 whichKey = 0;
	*result = 0;

	if (!ExtractArgs(paramInfo, arg1, opcodeOffsetPtr, thisObj, arg3, scriptObj, eventList, &whichKey))
		return true;

	whichKey -= 1;
	_ClearHotKey ( whichKey );
	return true;
}

class InvDumper
{
public:
	bool Accept(ExtraContainerChanges::EntryData* entryData)
	{
		_MESSAGE("%08x -> %s", entryData->type, GetFullName(entryData->type));
		return true;
	}
};


bool Cmd_SetHotKeyItem_Execute(COMMAND_ARGS)
{
	TESForm* qkForm = NULL;
	UInt32 whichKey = 0;
	*result = 0;

	if(!ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &whichKey, &qkForm))
		return true;

	whichKey -= 1;
	if (whichKey > 7)
		return true;
	_ClearHotKey ( whichKey );

	if (!qkForm)
		return true;

	ExtraContainerChanges* xChanges = static_cast <ExtraContainerChanges *>((*g_thePlayer)->baseExtraList.GetByType(kExtraData_ContainerChanges));
	if (xChanges)
	{
		ExtraQuickKeyFinder finder(UInt32(-1), qkForm);
		ExtraEntryVisitor visitor(xChanges->data->objList);
		const ExtraContainerChanges::Entry* xEntry;
		if (xEntry = visitor.Find(finder))
		{
			BSExtraData* toRemove = xEntry->data->extendData->data->GetByType(kExtraData_QuickKey);
			if (toRemove)
			{
				xEntry->data->extendData->data->Remove(toRemove);
				FormHeap_Free(toRemove);
			}
		}
	}


	NiTPointerList <TESForm> * quickKey = &g_quickKeyList[whichKey];
	NiTPointerList <TESForm>::Node * qkNode = quickKey->start;
	if (!quickKey->numItems || !qkNode)	// quick key not yet assigned
	{
		qkNode = quickKey->AllocateNode();
		quickKey->start = qkNode;
		quickKey->end = qkNode;
		quickKey->numItems = 1;
	}

	qkNode->next = NULL;
	qkNode->prev = NULL;
	qkNode->data = qkForm;

	ExtraQuickKey* xQKey = NULL;

	if (qkForm->typeID != kFormType_SpellItem)	//add ExtraQuickKey to new item's extra data list
	{
		ExtraContainerChanges* xChanges = static_cast <ExtraContainerChanges *>((*g_thePlayer)->baseExtraList.GetByType(kExtraData_ContainerChanges));
		if (xChanges)		//look up form in player's inventory
		{
			ExtraContainerChangesEntryFinder finder(qkForm);
			ExtraEntryVisitor visitor(xChanges->data->objList);
			const ExtraContainerChanges::Entry* xEntry = visitor.Find(finder);
			if (xEntry)
			{
				if (!xEntry->data->extendData)
				{
					xEntry->data->extendData = 
						(ExtraContainerChanges::EntryExtendData*)(FormHeap_Allocate(sizeof(ExtraContainerChanges::EntryExtendData)));
					xEntry->data->extendData->next = NULL;
					xEntry->data->extendData->data = NULL;
				}
				if (!xEntry->data->extendData->data)
					xEntry->data->extendData->data = ExtraDataList::Create();
				if (!xQKey) {
					xQKey = ExtraQuickKey::Create();
					xQKey->keyID = whichKey;
				}

				xEntry->data->extendData->data->Add(xQKey);
			}
			else
				Console_Print("SetHotKeyItem >> Item not found in inventory");
		}
	}

	return true;
}	
#endif

bool Cmd_SetHotkeyItem_Execute(COMMAND_ARGS)
{
    *result = 0;
    int hotkeynum;
    TESForm* itemform = NULL;
	ExtraDataList * found = NULL;
	ExtraHotkey* xHotkey = NULL;
	
    if(!ExtractArgsEx(EXTRACT_ARGS_EX, &hotkeynum, &itemform))
		return true;
    if(--hotkeynum < 8)
    {
        BSExtraData* xData = PlayerCharacter::GetSingleton()->extraDataList.GetByType(kExtraData_ContainerChanges);
        ExtraContainerChanges* xChanges = (ExtraContainerChanges*)xData;
        if(xChanges)
        {
			// Remove the hotkey if it exists on another object.
			for(ExtraContainerChanges::EntryDataList::Iterator itemIter = xChanges->data->objList->Begin(); !itemIter.End(); ++itemIter)
			{
				if(itemIter->type->refID != itemform->refID)
				{
					for (ExtraContainerChanges::ExtendDataList::Iterator iter = itemIter->extendData->Begin(); !iter.End(); ++iter)
					{
						xHotkey = (ExtraHotkey*)iter->GetByType(kExtraData_Hotkey);
						if(xHotkey && xHotkey->index == hotkeynum)
						{
							found = iter.Get();
							break;
						}
					}
					if(found)
						break;
				};
			}
			if(found) {
				found->RemoveByType(kExtraData_Hotkey);
				found = NULL;
			}

			xHotkey = NULL;
			for(ExtraContainerChanges::EntryDataList::Iterator itemIter = xChanges->data->objList->Begin(); !itemIter.End(); ++itemIter)
			{
				if(itemIter->type->refID == itemform->refID)
				{
					for (ExtraContainerChanges::ExtendDataList::Iterator iter = itemIter->extendData->Begin(); !iter.End(); ++iter)
					{
						xHotkey = (ExtraHotkey*)iter->GetByType(kExtraData_Hotkey);
						if(xHotkey) {
							xHotkey->index = hotkeynum; // If the item already has a hotkey, just change the index.
							break;
						}
					}
					if(!xHotkey)
					{
						ExtraHotkey* zHotkey = ExtraHotkey::Create();
						if (zHotkey)
						{
							zHotkey->index = hotkeynum;
							if (!itemIter->extendData)
								itemIter.Get()->Add(new ExtraDataList());
							if (itemIter->extendData)
							{
								if (!itemIter->extendData->Count())
									itemIter->extendData->AddAt(new ExtraDataList(), 0);
								if (itemIter->extendData->Count())
									itemIter->extendData->GetNthItem(0)->Add(zHotkey);
							}
						}
					}
					break;
				}
			}
        }
    }
    return true;
}

bool Cmd_ClearHotkey_Execute(COMMAND_ARGS)
{
    *result = 0;
    int hotkeynum;
    ExtraDataList* found = NULL;
    ExtraHotkey* xHotkey = NULL;
    if(!ExtractArgsEx(EXTRACT_ARGS_EX, &hotkeynum))
		return true;
    if(--hotkeynum < 8)
    {
        BSExtraData* xData = PlayerCharacter::GetSingleton()->extraDataList.GetByType(kExtraData_ContainerChanges);
        ExtraContainerChanges* xChanges = (ExtraContainerChanges*)xData;
        if(xChanges)
        {
            for(ExtraContainerChanges::EntryDataList::Iterator itemIter = xChanges->data->objList->Begin(); !itemIter.End(); ++itemIter)
            {
                for (ExtraContainerChanges::ExtendDataList::Iterator iter = itemIter->extendData->Begin(); !iter.End(); ++iter)
                {
                    xHotkey = (ExtraHotkey*)iter->GetByType(kExtraData_Hotkey);
                    if(xHotkey && xHotkey->index == hotkeynum)
                    {
                        found = iter.Get();
                        break;
                    }
                }
                if(found)
					break;
            }
            if(found)
				found->RemoveByType(kExtraData_Hotkey);
        }
    }
    return true;
}

UInt32 GetNumItems_Call(TESObjectREFR* thisObj)
{
	UInt32 count = 0;

	// handle critical section?

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->GetEntryDataList() : NULL);

	TESContainer* pContainer = NULL;
	TESForm* pBaseForm = thisObj->baseForm;
	if (pBaseForm) {
		pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
	}

	// first walk the base container
	if (pContainer) {
		ContainerCountIf counter(info);
		count = pContainer->formCountList.CountIf(counter);
	}

	// now count the remaining items
	count += info.CountItems();

	// handle leave critical section
	
	return count;
}

bool Cmd_GetNumItems_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (!thisObj) return true;

	*result = GetNumItems_Call(thisObj);

	if(IsConsoleMode())
		Console_Print("item count: %f", *result);

	return true;
}
bool Cmd_GetNumItems_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!thisObj) return true;

	*result = GetNumItems_Call(thisObj);

#if _DEBUG
	Console_Print("GetNumItems >> %f", *result);
#endif

	return true;
}

bool Cmd_GetInventoryObject_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (!thisObj) return true;

	UInt32 objIdx = 0;
	if (!ExtractArgs(EXTRACT_ARGS, &objIdx))
		return true;

	// enter critical section

	TESForm* pForm = GetItemByIdx(thisObj, objIdx, NULL);
	if (pForm) {
		UInt32 id = pForm->refID;
		*((UInt32*)result) = id;
		if (IsConsoleMode()) {
			Console_Print("%d: %s (%s)", objIdx, GetFullName(pForm), GetObjectClassName(pForm));
		}
	}

	// leave critical section

	return true;
}

bool Cmd_GetCurrentHealth_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESHealthForm *healthForm = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESHealthForm);
	if (healthForm)
	{
		ExtraHealth *xHealth = (ExtraHealth*)thisObj->extraDataList.GetByType(kExtraData_Health);
		*result = xHealth ? xHealth->health : (int)healthForm->health;
	}
	else
	{
		BGSDestructibleObjectForm *destructible = DYNAMIC_CAST(thisObj->baseForm, TESForm, BGSDestructibleObjectForm);
		if (destructible && destructible->data)
		{
			ExtraObjectHealth *xObjHealth = (ExtraObjectHealth*)thisObj->extraDataList.GetByType(kExtraData_ObjectHealth);
			*result = xObjHealth ? xObjHealth->health : (int)destructible->data->health;
		}
	}
	if (IsConsoleMode())
		Console_Print("GetCurrentHealth >> %.4f", *result);
	return true;
}
bool Cmd_GetCurrentHealth_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!thisObj) return true;
	TESHealthForm* healthForm = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESHealthForm);
	if (healthForm)
	{
		ExtraHealth* xHealth = (ExtraHealth*)thisObj->extraDataList.GetByType(kExtraData_Health);
		*result = xHealth ? xHealth->health : (int)healthForm->health;
	}
	else
	{
		BGSDestructibleObjectForm* destructible = DYNAMIC_CAST(thisObj->baseForm, TESForm, BGSDestructibleObjectForm);
		if (destructible && destructible->data)
		{
			ExtraObjectHealth* xObjHealth = (ExtraObjectHealth*)thisObj->extraDataList.GetByType(kExtraData_ObjectHealth);
			*result = xObjHealth ? xObjHealth->health : (int)destructible->data->health;
		}
	}
#if _DEBUG
	Console_Print("GetCurrentHealth >> %.4f", *result);
#endif
	return true;
}

static bool AdjustHealth(TESHealthForm* pHealth, ExtraDataList* pExtraData, float nuHealth)
{
	ExtraHealth* pXHealth = (ExtraHealth*)pExtraData->GetByType(kExtraData_Health);
	if (nuHealth < 0) nuHealth = 0;
	if (nuHealth >= pHealth->health) {
		if (pXHealth) {
			pExtraData->Remove(pXHealth, true);
		}
	} else if (pXHealth) {
		pXHealth->health = nuHealth;
	} else {
		// need to create a new pXHealth
		pXHealth = ExtraHealth::Create();
		if (pXHealth) {
			pXHealth->health = nuHealth;
			pExtraData->Add(pXHealth);
		}
	}
	return true;
}

bool Cmd_SetCurrentHealth_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (!thisObj) return true;

	TESHealthForm* pHealth = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESHealthForm);
	if (!pHealth) return true;

	float nuHealth = 0.0;

	if (ExtractArgs(EXTRACT_ARGS, &nuHealth)) {
		if (nuHealth < 0.0) nuHealth = 0.0;
		AdjustHealth(pHealth, &thisObj->extraDataList, nuHealth);
	}
	return true;
}

bool Cmd_SetEquippedCurrentHealth_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (!thisObj) return true;


	UInt32 slotIdx = 0;
	float health = 0;
	if (!ExtractArgs(EXTRACT_ARGS, &health, &slotIdx)) return true;
	MatchBySlot matcher(slotIdx);
	EquipData equipD = FindEquipped(thisObj, matcher);
	if (equipD.pForm && equipD.pExtraData) {
		TESHealthForm* pHealth = DYNAMIC_CAST(equipD.pForm, TESForm, TESHealthForm);
		if (pHealth) {
			float nuHealth = health;
			AdjustHealth(pHealth, equipD.pExtraData, nuHealth);
		}
	}
	return true;
}

bool Cmd_GetArmorAR_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
	if (pArmor) {
		*result = pArmor->armorRating;
		if (IsConsoleMode()) {
			Console_Print("%s armor rating: %d", GetFullName(pArmor), pArmor->armorRating);
		}
	}
	return true;
}
bool Cmd_GetArmorAR_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;

	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
	if (pArmor) 
	{
		*result = pArmor->armorRating;
#if _DEBUG
		Console_Print("%s armor rating: %d", GetFullName(pArmor), pArmor->armorRating);
#endif
	}
	return true;
}

bool Cmd_SetArmorAR_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 nuAR = 0;
	TESForm* pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, nuAR);
	if (pForm) {
		TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
		if (pArmor) {
			pArmor->armorRating = nuAR;
			if (IsConsoleMode()) {
				Console_Print("Setting %s armor rating to %d", GetFullName(pArmor), nuAR);
			}
		}
	}
	return true;
}


bool Cmd_GetArmorDT_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}

	TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
	if (pArmor) {
		*result = pArmor->damageThreshold;
		if (IsConsoleMode()) {
			Console_Print("%s damage threshold: %f", GetFullName(pArmor), pArmor->damageThreshold);
		}
	}
	return true;
}
bool Cmd_GetArmorDT_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;

	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
	if (pArmor) 
	{
		*result = pArmor->damageThreshold;
#if _DEBUG
			Console_Print("%s damage threshold: %f", GetFullName(pArmor), pArmor->damageThreshold);
#endif
	}
	return true;
}

bool Cmd_SetArmorDT_Execute(COMMAND_ARGS)
{
	*result = 0;
	float nuDT = 0.0;
	TESForm* pForm = Extract_FloatAndForm(PASS_COMMAND_ARGS, nuDT);
	if (pForm) {
		TESObjectARMO* pArmor = DYNAMIC_CAST(pForm, TESForm, TESObjectARMO);
		if (pArmor) {
			pArmor->damageThreshold = nuDT;
			if (IsConsoleMode()) {
				Console_Print("Setting %s damage threshold to %f", GetFullName(pArmor), nuDT);
			}
		}
	}
	return true;
}

bool Cmd_IsPowerArmor_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	if (!ExtractArgs(EXTRACT_ARGS, &pForm)) return true;
	pForm = pForm->TryGetREFRParent();
	if (!pForm) {
		if (!thisObj) return true;
		pForm = thisObj->baseForm;
	}
	
	TESBipedModelForm* pBiped = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
	if (pBiped) {
		*result = pBiped->IsPowerArmor() ? 1 : 0;
	}
	return true;
}
bool Cmd_IsPowerArmor_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;
	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	TESBipedModelForm* pBiped = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
	if (pBiped) 
	{
		*result = pBiped->IsPowerArmor() ? 1 : 0;
	}
	return true;
}

bool Cmd_SetIsPowerArmor_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	UInt32 isPA = 0;

	pForm = Extract_IntAndForm(PASS_COMMAND_ARGS, isPA);
	if (pForm) {
		TESBipedModelForm* pBiped = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
		if (pBiped) {
			pBiped->SetPowerArmor( isPA != 0);
		}
	}
	return true;
}


bool Cmd_IsQuestItem_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}

		*result = pForm->IsQuestItem();
	}
	return true;
}
bool Cmd_IsQuestItem_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* pForm = NULL;
	if (arg1)
	{
		pForm = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		pForm = thisObj->baseForm;
	}
	else return true;

	if (pForm)
		*result = pForm->IsQuestItem();

#if _DEBUG
	Console_Print("IsQuestItem >> %f", *result); 
#endif

	return true;
}

bool Cmd_SetQuestItem_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm = NULL;
	UInt32 nSetQuest = 0;
	if (ExtractArgs(EXTRACT_ARGS, &nSetQuest, &pForm)) {
		pForm = pForm->TryGetREFRParent();
		if (!pForm) {
			if (!thisObj) return true;
			pForm = thisObj->baseForm;
		}

		pForm->SetQuestItem(nSetQuest == 1);
	}
	return true;
}

bool Cmd_GetObjectEffect_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESForm* form = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESEnchantableForm* enchantable = DYNAMIC_CAST(form, TESForm, TESEnchantableForm);

	if (enchantable && enchantable->enchantItem) {
		*refResult = enchantable->enchantItem->refID;
	}

	return true;
}

bool Cmd_GetAmmoSpeed_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo) {
		*result = pAmmo->speed;
		if (IsConsoleMode()) {
			Console_Print("%s ammo speed: %f", GetFullName(pAmmo), pAmmo->speed);
		}
	}
	return true;
}
bool Cmd_GetAmmoSpeed_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* form = NULL;
	if (arg1)
	{
		form = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		form = thisObj->baseForm;
	}
	else return true;

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo) 
	{
		*result = pAmmo->speed;
#if _DEBUG
		Console_Print("%s ammo speed: %f", GetFullName(pAmmo), pAmmo->speed);
#endif
	}
	return true;
}

bool Cmd_GetAmmoConsumedPercent_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo) {
		*result = pAmmo->ammoPercentConsumed;
	}
	return true;
}
bool Cmd_GetAmmoConsumedPercent_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESForm* form;
	if (arg1)
	{
		form = (TESForm*)arg1;
	}
	else if (thisObj)
	{
		form = thisObj->baseForm;
	}
	else return true;

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo) 
	{
		*result = pAmmo->ammoPercentConsumed;
	}
	return true;
}

bool Cmd_SetAmmoConsumedPercent_Execute(COMMAND_ARGS)
{
	float fNewPerc;
	TESForm* form = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &fNewPerc, &form);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo) {
		pAmmo->ammoPercentConsumed = fNewPerc;
	}
	return true;
}

bool Cmd_GetAmmoCasing_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESForm* form = NULL;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESAmmo* pAmmo = DYNAMIC_CAST(form, TESForm, TESAmmo);
	if (pAmmo && pAmmo->casing) {
		*refResult = pAmmo->casing->refID;
	}
	return true;
}

bool Cmd_GetPlayerCurrentAmmoRounds_Execute(COMMAND_ARGS)
{
	*result = 0;
	PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
	if (pPC) {
		BaseProcess* pBaseProc = pPC->baseProcess;
		BaseProcess::AmmoInfo* pAmmoInfo = pBaseProc->GetAmmoInfo();
		if (pAmmoInfo) {
			*result = pAmmoInfo->count;
		}
	}
	return true;
}

bool Cmd_SetPlayerCurrentAmmoRounds_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 nuCount = 0;
	if (ExtractArgs(EXTRACT_ARGS, &nuCount)) {
		PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
		if (pPC) {
			BaseProcess* pBaseProc = pPC->baseProcess;
			BaseProcess::AmmoInfo* pAmmoInfo = pBaseProc->GetAmmoInfo();
			if (pAmmoInfo) {
				pAmmoInfo->count = nuCount;
			}
		}
	}
	return true;
}

bool Cmd_GetPlayerCurrentAmmo_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
	if (pPC) {
		BaseProcess* pBaseProc = pPC->baseProcess;
		BaseProcess::AmmoInfo* pAmmoInfo = pBaseProc->GetAmmoInfo();
		if (pAmmoInfo) {
			*refResult = pAmmoInfo->ammo->refID;
		}
	}
	return true;
}

bool Cmd_HasAmmoEquipped_Eval(COMMAND_ARGS_EVAL) {
	*result = 0;
	if (thisObj && thisObj->IsActor_Runtime()) {
		auto actor = static_cast<Actor*>(thisObj);
		if (auto pBaseProc = actor->baseProcess) {
			if (const auto* pAmmoInfo = pBaseProc->GetAmmoInfo()) {
				if (const auto* pAmmo = DYNAMIC_CAST(arg1, TESForm, TESAmmo))
					*result = pAmmoInfo->ammo == pAmmo;
				else if (auto* pAmmoList = DYNAMIC_CAST(arg1, TESForm, BGSListForm))
					*result = pAmmoList->GetIndexOf(pAmmoInfo->ammo) != eListInvalid;
			}
		}
	}
	return true;
}

bool Cmd_HasAmmoEquipped_Execute(COMMAND_ARGS) {
	*result = 0;
	TESForm* ammoOrList;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &ammoOrList))
		return true;
	return Cmd_HasAmmoEquipped_Eval(thisObj, ammoOrList, nullptr, result);
}

bool Cmd_IsEquippedAmmoInList_Eval(COMMAND_ARGS_EVAL) {
	return Cmd_HasAmmoEquipped_Eval(thisObj, arg1, arg2, result);
}
bool Cmd_IsEquippedAmmoInList_Execute(COMMAND_ARGS) {
	return Cmd_HasAmmoEquipped_Execute(PASS_COMMAND_ARGS);
}

bool Cmd_GetWeaponCanUseAmmo_Eval(COMMAND_ARGS_EVAL) {
	*result = 0;
	TESObjectWEAP* weap = nullptr;
	if (arg2)
		weap = static_cast<TESObjectWEAP*>(arg2);
	else if (thisObj)
		weap = static_cast<TESObjectWEAP*>(thisObj->baseForm);

	if (weap) {
		auto* ammoOrList = static_cast<TESForm*>(arg1);
		*result = weap->ammo.ammo == ammoOrList;
		if (!*result)
		{
			if (auto* pAmmoList = DYNAMIC_CAST(weap->ammo.ammo, TESForm, BGSListForm);
				pAmmoList && ammoOrList->typeID == kFormType_TESAmmo)
			{
				*result = pAmmoList->GetIndexOf(ammoOrList) != eListInvalid;
			}
		}
	}

	return true;
}
bool Cmd_GetWeaponCanUseAmmo_Execute(COMMAND_ARGS) {
	*result = 0;
	TESForm* ammoOrList;
	TESObjectWEAP* baseWeapon = nullptr;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &ammoOrList, &baseWeapon))
		return true;
	return Cmd_GetWeaponCanUseAmmo_Eval(thisObj, ammoOrList, baseWeapon, result);
}

bool Cmd_GetEquippedWeaponCanUseAmmo_Eval(COMMAND_ARGS_EVAL) {
	*result = 0;
	if (thisObj) {
		return Cmd_GetWeaponCanUseAmmo_Eval(nullptr, arg1, static_cast<Actor*>(thisObj)->GetEquippedWeapon(), result);
	}
	return true;
}
bool Cmd_GetEquippedWeaponCanUseAmmo_Execute(COMMAND_ARGS) {
	*result = 0;
	TESForm* ammoOrList;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &ammoOrList))
		return true;
	return Cmd_GetEquippedWeaponCanUseAmmo_Eval(thisObj, ammoOrList, nullptr, result);
}

bool Cmd_GetEquippedWeaponUsesAmmoList_Eval(COMMAND_ARGS_EVAL) {
	return Cmd_GetEquippedWeaponCanUseAmmo_Eval(thisObj, arg1, arg2, result);
}
bool Cmd_GetEquippedWeaponUsesAmmoList_Execute(COMMAND_ARGS) {
	return Cmd_GetEquippedWeaponCanUseAmmo_Execute(PASS_COMMAND_ARGS);
}

bool Cmd_SetNameEx_Execute(COMMAND_ARGS)
{
	TESForm	* form = NULL;
	char	newName[kMaxMessageLength];

	if (!ExtractFormatStringArgs(0, newName, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, kCommandInfo_SetNameEx.numParams, &form))
		return true;

	if (!form && thisObj)
	{
		form = thisObj;

#if 0	// ### MapMenu not decoded in fallout
		// if we are changing the name of a mapmarker and the mapmenu is open, update map menu
		if(thisObj->baseForm && thisObj->baseForm->refID == kFormID_MapMarker)
		{
			MapMenu* mapMenu = (MapMenu*)GetMenuByType(kMenuType_Map);
			if (mapMenu)
				mapMenu->UpdateMarkerName(thisObj, newName);
		}
#endif
	}

	if (!form) return true;

	TESFullName	* name = form->GetFullName();
	if (name) name->name.Set(newName);

	return true;
}

extern std::unordered_set<UInt32> s_clonedFormsWithInheritedModIdx;

bool Cmd_IsClonedForm_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;

	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	form = form->TryGetREFRParent(); 
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	if (form->GetModIndex() == 0xFF || s_clonedFormsWithInheritedModIdx.contains(form->refID))
		*result = 1;
	return true;
}

std::unordered_set<UInt32> s_clonedFormsWithInheritedModIdx;

bool CloneForm_Execute(COMMAND_ARGS, bool bPersist)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESForm* form = NULL;
	int inheritModIndexFromCallingScript = false;
	ExtractArgsEx(EXTRACT_ARGS_EX, &form, &inheritModIndexFromCallingScript);
	if (!form) {
		if (!thisObj) return true;
		form = thisObj->baseForm;
	}

	TESForm* clonedForm = form->CloneForm(bPersist);
	if (clonedForm) 
	{
		if (inheritModIndexFromCallingScript)
		{
			const auto nextFormId = GetNextFreeFormID(scriptObj->refID);
			if (nextFormId >> 24 == scriptObj->GetModIndex())
			{
				clonedForm->SetRefID(nextFormId, true);
				s_clonedFormsWithInheritedModIdx.insert(nextFormId);
			}
		}
		*refResult = clonedForm->refID;
		if (IsConsoleMode())
			Console_Print("Created cloned form: %08x", *refResult);
	}

	return true;
}

bool Cmd_TempCloneForm_Execute(COMMAND_ARGS)
{
	return CloneForm_Execute(PASS_COMMAND_ARGS, false);
}

bool Cmd_CloneForm_Execute(COMMAND_ARGS)
{
	return CloneForm_Execute(PASS_COMMAND_ARGS, true);
}

void GetEquippedWeaponModFlags_Call(TESObjectREFR* thisObj, double *result)
{
	MatchBySlot matcher(5);
	EquipData equipD = FindEquipped(thisObj, matcher);

	if (!equipD.pForm) return;
	if (!equipD.pExtraData) return;

	ExtraWeaponModFlags* pXWeaponModFlags = (ExtraWeaponModFlags*)equipD.pExtraData->GetByType(kExtraData_WeaponModFlags);
	if (pXWeaponModFlags)
		*result = pXWeaponModFlags->flags;
}

bool Cmd_GetEquippedWeaponModFlags_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (!thisObj) return true;

	GetEquippedWeaponModFlags_Call(thisObj, result);

	if (IsConsoleMode())
		Console_Print("Weapon Mod Flags: %f", *result);

	return true;
}
bool Cmd_GetEquippedWeaponModFlags_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!thisObj) return true;

	GetEquippedWeaponModFlags_Call(thisObj, result);

#if _DEBUG
	Console_Print("Weapon Mod Flags: %f", *result);
#endif

	return true;
}

bool Cmd_SetEquippedWeaponModFlags_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (!thisObj)
		return true;

	UInt32 flags = 0;
	if (!ExtractArgs(EXTRACT_ARGS, &flags))
		return true;

	if (flags < 0 || flags > 7)
		return true;

	MatchBySlot matcher(5);
	EquipData equipD = FindEquipped(thisObj, matcher);

	if (!equipD.pForm)
		return true;

	if (!equipD.pExtraData)
		return true;

	ExtraWeaponModFlags* pXWeaponModFlags = (ExtraWeaponModFlags*)equipD.pExtraData->GetByType(kExtraData_WeaponModFlags);

	// Modify existing flags
	if (pXWeaponModFlags) {
		if (flags) {
			pXWeaponModFlags->flags = (UInt8)flags;
		} else {
			equipD.pExtraData->Remove(pXWeaponModFlags, true);
		}

		// Create new extra data
	} else if (flags) {
		pXWeaponModFlags = ExtraWeaponModFlags::Create();
		if (pXWeaponModFlags) {
			pXWeaponModFlags->flags = (UInt8)flags;
			equipD.pExtraData->Add(pXWeaponModFlags);
		}
	}

	return true;
}

bool Cmd_GetWeaponItemModEffect_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 whichMod = 0;
	TESObjectWEAP* pWeap = Extract_IntAndWeapon(PASS_COMMAND_ARGS, whichMod);
	if (!pWeap)
		return true;

	*result = pWeap->GetItemModEffect(whichMod);
	if(IsConsoleMode())
		Console_Print("Item Mod %d Effect: %d", whichMod, pWeap->GetItemModEffect(whichMod));

	return true;
}

bool Cmd_GetWeaponItemModValue1_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 whichMod = 0;
	TESObjectWEAP* pWeap = Extract_IntAndWeapon(PASS_COMMAND_ARGS, whichMod);
	if (!pWeap)
		return true;

	*result = pWeap->GetItemModValue1(whichMod);
	if(IsConsoleMode())
		Console_Print("Item Mod %d Value1: %f", whichMod, pWeap->GetItemModValue1(whichMod));

	return true;
}

bool Cmd_GetWeaponItemModValue2_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 whichMod = 0;
	TESObjectWEAP* pWeap = Extract_IntAndWeapon(PASS_COMMAND_ARGS, whichMod);
	if (!pWeap)
		return true;

	*result = pWeap->GetItemModValue2(whichMod);
	if(IsConsoleMode())
		Console_Print("Item Mod %d Value2: %f", whichMod, pWeap->GetItemModValue2(whichMod));

	return true;
}

bool Cmd_AddItemHealthPercentOwner_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	float NumItems = 1;
	float Health = 1.0;
	TESForm* pOwner = NULL;
	UInt32 Rank = 0;
	TESForm* rcItem = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pItem, &NumItems, &Health, &pOwner, &Rank)) {
		rcItem = AddItemHealthPercentOwner(thisObj, pItem->refID, NumItems, Health, pOwner, Rank);
	}
	return true;
}

bool Cmd_AddItemOwnership_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	float NumItems = 1;
	TESForm* pOwner = NULL;
	UInt32 Rank = 0;
	TESForm* rcItem = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pItem, &NumItems, &pOwner, &Rank)) {
		rcItem = AddItemHealthPercentOwner(thisObj, pItem->refID, NumItems, -1.0, pOwner, Rank);
	}
	return true;
}

bool Cmd_GetTokenValue_Eval(COMMAND_ARGS_EVAL)
{
	//DEBUG_MESSAGE("\t\tCI GetTokenValue Evaluated");
	*result = 0;
	float value = 0;
	TESForm* pItem = (TESForm*)arg1;
	UInt32 refID = pItem->refID;
	TESForm * pForm = GetItemWithHealthAndOwnershipByRefID(thisObj, refID, &value, NULL, NULL);

	//DEBUG_MESSAGE("\t\tCI GetTokenValue Searched");
	if (pForm) {
		//DEBUG_MESSAGE("\t\tCI GetTokenValue Exists");
		*result = value;
		if (IsConsoleMode()) {
			if (pForm->GetFullName())
				Console_Print("GetTokenValue: >> %f (%s)", *result, pForm->GetFullName()->name);
			else
				Console_Print("GetTokenValue: >> %f", *result);
		}
	}
	//DEBUG_MESSAGE("\t\tCI GetTokenValue Done");
	return true;
}

bool Cmd_GetTokenValue_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	bool rc;

	//DEBUG_MESSAGE("\n\tCI GetTokenValue Called");
	if (ExtractArgs(EXTRACT_ARGS, &pItem)) {
		//DEBUG_MESSAGE("\t\tGTV thisObj:%#10X refID:[%x]", thisObj, pItem->refID);
		rc = Cmd_GetTokenValue_Eval(thisObj, (void*)pItem, 0, result);
		//if (rc)
		//	DEBUG_MESSAGE("\t\tGTV thisObj:%#10X refID:[%x] value:[%f]\n", thisObj, pItem->refID, *result);
		return rc;
	}
	return true;
}

bool Cmd_GetTokenRef_Eval(COMMAND_ARGS_EVAL)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* pItem = (TESForm*)arg1;
	UInt32 refID = pItem->refID;
	//DEBUG_MESSAGE("\t\t\tGTR thisObj:%#10X token:[%x]\n", thisObj, pItem->refID);
	TESForm* owner = NULL;
	GetItemWithHealthAndOwnershipByRefID(thisObj, refID, NULL, &owner, NULL);;
	if (owner) {
		*refResult = owner->refID;
		//DEBUG_MESSAGE("\t\t\tGTR thisObj:%#10X token:[%x] ref:[%x]\n", thisObj, pItem->refID, *refResult);
		if (IsConsoleMode()) {
			if (owner->GetFullName())
				Console_Print("TokenReference: >> %#10X (%s)", *result, owner->GetFullName()->name);
			else
				Console_Print("TokenReference: >> %#10X", *result);
		}
	}
	return true;
}

bool Cmd_GetTokenRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* pItem = NULL;
	bool rc;

	//DEBUG_MESSAGE("\n\tGTR GetTokenRef called");
	if (ExtractArgs(EXTRACT_ARGS, &pItem)) {
		//DEBUG_MESSAGE("\t\tGTR thisObj:%#10X token:[%x]", thisObj, pItem->refID);
		rc = Cmd_GetTokenRef_Eval(thisObj, (void*)pItem, 0, result);
		if (rc && *result)
			//DEBUG_MESSAGE("\t\tGTR thisObj:%#10X token:[%x] ref:[%x]\n", thisObj, pItem->refID, *refResult);
		return rc;
	}
	return true;
}

bool SetTokenValueOrRef(TESObjectREFR * thisObj, TESForm* pItem, float value = 100.0, TESForm* ref = NULL)
{
	//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef Evaluated");
	UInt32 refID = pItem->refID;
	float currHealth = -1.0;
	TESForm* currOwner = NULL;
	UInt32 currRank = 0;
	TESForm * pForm = GetItemWithHealthAndOwnershipByRefID(thisObj, refID, &currHealth, &currOwner, &currRank);

	if (!pForm) {
		//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef To be created");
		pForm = LookupFormByID(refID);
		if (pForm) {
			//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef LookedUp");
			TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
			if (pHealth) {
				//DEBUG_MESSAGE("\t\tCI\tSetting: value=%f", value);
				pForm = AddItemHealthPercentOwner(thisObj, refID, 1, value, ref);
				thisObj->MarkAsModified(TESObjectREFR::kChanged_Inventory);	// Makes the change permanent
			}
			else
				pForm = NULL;
		}
	}
	//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef Searched");
	if (pForm) {
		//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef Exists");
		TESHealthForm* pHealth = DYNAMIC_CAST(pForm, TESForm, TESHealthForm);
		if (pHealth) {
			if (value == -1.0)
				value = currHealth;
			if (ref == NULL)
				ref = currOwner;
			else
				currRank = 0;
			pForm = SetFirstItemWithHealthAndOwnershipByRefID(thisObj, refID, 1, value, ref, currRank);
			thisObj->MarkAsModified(TESObjectREFR::kChanged_Inventory);	// Makes the change permanent
			if (IsConsoleMode() && ref != nullptr) {
				if (pForm->GetFullName())
					Console_Print("SetTokenValueOrRef: >> %f [%x] (%s)", value, ref->refID, pForm->GetFullName()->name);
				else
					Console_Print("SetTokenValueOrRef: >> %f [%x]", value, ref->refID);
			}
		}
	}
	//DEBUG_MESSAGE("\t\tCI SetTokenValueOrRef Done");
	return true;
}

bool Cmd_SetTokenValue_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	float value = 0;
	bool rc;

	//DEBUG_MESSAGE("\n\tCI SetTokenValue Called");
	if (ExtractArgs(EXTRACT_ARGS, &pItem, &value)) {
		//DEBUG_MESSAGE("\t\tSTV thisObj:%#10X refID:[%x] value:[%f]", thisObj, pItem->refID, value);
		rc = SetTokenValueOrRef(thisObj, pItem, value);
		//if (rc)
		//	DEBUG_MESSAGE("\t\tSTV thisObj:%#10X refID:[%x] value:[%f]\n", thisObj, pItem->refID, value);
		return rc;
	}
	return true;
}

bool Cmd_SetTokenRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	TESForm* ref = NULL;
	bool rc;

	//DEBUG_MESSAGE("\n\tCI SetTokenRef Called");
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pItem, &ref)) {
		//DEBUG_MESSAGE("\t\tSTR thisObj:%#10X refID:[%x] ref:[%x]", thisObj, pItem->refID, ref);
		rc = SetTokenValueOrRef(thisObj, pItem, -1.0, ref);
		//if (rc)
			//DEBUG_MESSAGE("\t\tSTR thisObj:%#10X refID:[%x] ref:[%x]\n", thisObj, pItem->refID, ref);
		return rc;
	}
	return true;
}

bool Cmd_SetTokenValueAndRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pItem = NULL;
	float value = 0;
	TESForm* ref = NULL;
	bool rc;

	//DEBUG_MESSAGE("\n\tCI SetTokenValueAndRef Called");
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pItem, &value, &ref)) {
		//DEBUG_MESSAGE("\t\tSTVR thisObj:%#10X refID:[%x] value:[%f]", thisObj, pItem->refID, value);
		rc = SetTokenValueOrRef(thisObj, pItem, value, ref);
		//if (rc)
		//	DEBUG_MESSAGE("\t\tSTVR thisObj:%#10X refID:[%x] value:[%f]\n", thisObj, pItem->refID, value);
		return rc;
	}
	return true;
}

bool Cmd_GetPaired_Eval(COMMAND_ARGS_EVAL) {
	//DEBUG_MESSAGE("\n\tCI GetPaired Evaluated");
	*result = false;
	SInt32 inOutIndex = 0;
	TESForm* pOwner;
	TESForm* pItem = (TESForm*)arg1;
	Actor* pActor = (Actor*)arg2;
	if (!pItem || !pActor) return true;

	//DEBUG_MESSAGE("\t\tGP thisObj:%x refID:[%x] Actor:[%x]", thisObj, pItem->refID, pActor->refID);
	while (GetItemWithHealthAndOwnershipByRefID(thisObj, pItem->refID, NULL, &pOwner, NULL, &inOutIndex) && (pOwner != pActor));
	//DEBUG_MESSAGE("\t\tGP thisObj:%x refID:[%x] Actor:[%x] Owner:[%x] %d", thisObj, pItem->refID, pActor, pOwner, inOutIndex);
	if (pOwner != pActor) return true;
	inOutIndex = 0;
	while (GetItemWithHealthAndOwnershipByRefID(pActor, pItem->refID, NULL, &pOwner, NULL, &inOutIndex) && (pOwner != thisObj));
	//DEBUG_MESSAGE("\t\tGP thisObj:%x refID:[%x] Actor:[%x] Owner:[%x] %d", thisObj, pItem->refID, pActor, pOwner, inOutIndex);
	*result = (pOwner == thisObj);
	return true;
}

bool Cmd_GetPaired_Execute(COMMAND_ARGS) {
	*result = false;
	TESForm* pItem = NULL;
	Actor* pActor = NULL;
	bool rc;

	//DEBUG_MESSAGE("\n\tCI GetPaired Called");
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &pItem, &pActor)) {
		//DEBUG_MESSAGE("\t\tGP thisObj:%#10X refID:[%x] Actor:[%x]", thisObj, pItem->refID, pActor->refID);
		rc = Cmd_GetPaired_Eval(thisObj, pItem, pActor, result);
		//if (rc)
		//	DEBUG_MESSAGE("\t\tGP thisObj:%x refID:[%x] Actor:[%x] result:%s\n", thisObj, pItem->refID, pActor->refID, *result ? "paired" : "unpaired");
		return rc;
	}
	return true;
}

bool Cmd_PickOneOf_Execute(COMMAND_ARGS) {
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	BGSListForm* pFormList = NULL;
	Actor* pActor = NULL;
	SInt32 count;
	UInt32 random;

	//DEBUG_MESSAGE("\n\tCI PickOneOf Called");
	pActor = DYNAMIC_CAST(thisObj, TESForm, Actor);
	if (!pActor)
		return true;
	if (ExtractArgs(EXTRACT_ARGS, &pFormList)) {
		std::vector<TESForm*> present;
		for (int i = 0; i < pFormList->Count(); i++) {
			GetItemByRefID(thisObj, pFormList->GetNthForm(i)->refID, &count);
			if (count > 0)
				present.push_back(pFormList->GetNthForm(i));
		}
		switch (present.size())
		{
			case 0:
				break;
			case 1:
				*refResult = present.at(0)->refID;
				break;
			default:
				random = MersenneTwister::genrand_real2() * (present.size());
				*refResult = present.at(random)->refID;
		}
	}
	return true;
}

bool Cmd_IsPlayable_Execute(COMMAND_ARGS)
{
	//version which only returns false if "unplayable" flag is present and set
	*result = 1;
	TESForm* form = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			*result = biped->IsPlayable() ? 1 : 0;
		else {
			TESObjectWEAP* weap = DYNAMIC_CAST(form, TESForm, TESObjectWEAP);
			if (weap)
				*result = weap->IsPlayable() ? 1 : 0;
			else {
				TESAmmo* ammo = DYNAMIC_CAST(form, TESForm, TESAmmo);
				if (ammo)
					*result = ammo->IsPlayable() ? 1 : 0;
				else {
					TESRace* race = DYNAMIC_CAST(form, TESForm, TESRace);
					if (race)
						*result = race->IsPlayable() ? 1 : 0;
				}
			}
		}
	}
	return true;
}

// SetIsPlayable
bool Cmd_SetIsPlayable_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	float doSet;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &doSet, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			biped->SetNonPlayable(0.0 == doSet);
	}
	return true;
}

bool Cmd_GetEquipmentSlotsMask_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			*result = biped->GetSlotsMask();
	}
	return true;
}

// SetEquipmentSlotMask
bool Cmd_SetEquipmentSlotsMask_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	UInt32 mask;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &mask, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			biped->SetSlotsMask(mask);
	}
	return true;
}

bool Cmd_GetEquipmentBipedMask_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			*result = biped->GetBipedMask();
	}
	return true;
}

// SetEquipmentSlotMask
bool Cmd_SetEquipmentBipedMask_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form = NULL;
	UInt32 mask;

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &mask, &form))
	{
		form = form->TryGetREFRParent();
		if (!form)
			if (thisObj)
				form = thisObj->baseForm;
	}
	if (form)
	{
		TESBipedModelForm* biped = DYNAMIC_CAST(form, TESForm, TESBipedModelForm);
		if (biped)
			biped->SetBipedMask(mask);
	}
	return true;
}

bool Cmd_EquipItem2_Execute_OBSE(COMMAND_ARGS)
{
	// forces onEquip block to run

	Actor* actor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);
	if (actor) {
		ExtraContainerExtendDataArray  preList = actor->GetEquippedExtendDataList();
		Cmd_EquipItem_Execute(PASS_COMMAND_ARGS);
		ExtraContainerExtendDataArray postList = actor->GetEquippedExtendDataList();

		// what was equipped (if anything)?
		bool found;
		for (UInt32 i = 0; i < postList.size(); i++) {
			found = false;
			for (UInt32 j = 0; j < preList.size(); j++)
				if (preList[j] = postList[i]) {
					found = true;
					break;
				}
			if (!found) {
				ExtraContainerChanges::ExtendDataList* data = postList[i];
				if (data->GetNthItem(0)) {
					// mark the event
					data->GetNthItem(0)->MarkScriptEvent(ScriptEventList::kEvent_OnEquip, actor);
				}
			}
		}
	}

	return true;
}

bool Cmd_EquipItem2_Execute(COMMAND_ARGS)
{
	TESForm *item = NULL;
	UInt32 noUnequip = 0, noMessage = 1;

	if (!thisObj || !ExtractArgs(EXTRACT_ARGS, &item, &noUnequip, &noMessage)) return true;

	Actor *actor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);
	if (!actor) return true;

	UInt8 itemType = item->typeID;
	// Those following are the only equip-able types.
	if ((itemType != kFormType_TESObjectARMO) && (itemType != kFormType_TESObjectBOOK) && (itemType != kFormType_TESObjectWEAP) && 
		(itemType != kFormType_TESAmmo) && (itemType != kFormType_AlchemyItem)) return true;

	ExtraContainerChanges *xChanges = (ExtraContainerChanges*)actor->extraDataList.GetByType(kExtraData_ContainerChanges);
	if (!xChanges || !xChanges->data || !xChanges->data->objList) return true;

	ExtraContainerChanges::EntryData *entry = xChanges->data->objList->Find(ItemInEntryDataListMatcher(item));
	if (!entry) return true;

	UInt32 eqpCount = 1;
	if (itemType == kFormType_TESObjectWEAP)
	{
		TESObjectWEAP *weapon = DYNAMIC_CAST(item, TESForm, TESObjectWEAP);
		// If the weapon is stack-able, equip whole stack.
		if (weapon && (weapon->eWeaponType > 9)) eqpCount = entry->countDelta;
	}
	else if (itemType == kFormType_TESAmmo) eqpCount = entry->countDelta;

	ExtraDataList *xData = entry->extendData ? entry->extendData->GetNthItem(0) : NULL;

	actor->EquipItem(item, eqpCount, xData, 1, noUnequip != 0, noMessage != 0);

	return true;
}

bool Cmd_EquipMe_Execute(COMMAND_ARGS)
{
	Actor* owner = DYNAMIC_CAST(containingObj, TESObjectREFR, Actor);
	if (thisObj) {
		if (owner) {
			owner->EquipItem(thisObj->baseForm, 1, &thisObj->extraDataList, 1, false);
		}
		else {	// inventory reference?
			InventoryReference* iref = InventoryReference::GetForRefID(thisObj->refID);
			if (iref) {
				iref->SetEquipped(true);
			}
		}
	}

	return true;
}

bool Cmd_UnequipMe_Execute(COMMAND_ARGS)
{
	Actor* owner = DYNAMIC_CAST(containingObj, TESObjectREFR, Actor);
	if (thisObj) {
		if (owner) {
			//bool worn = thisObj->extraDataList.HasType(kExtraData_Worn) || thisObj->extraDataList.HasType(kExtraData_WornLeft);
			owner->UnequipItem(thisObj->baseForm, 1,  NULL, 1, false, 1);
		}
		else {
			InventoryReference* iref = InventoryReference::GetForRefID(thisObj->refID);
			if (iref) {
				iref->SetEquipped(false);
			}
		}
	}

	return true;
}

bool Cmd_IsEquipped_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (thisObj && (thisObj->extraDataList.HasType(kExtraData_Worn) || thisObj->extraDataList.HasType(kExtraData_WornLeft))) {
		*result = 1.0;
	}

	return true;
}

bool Cmd_GetMe_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = ((UInt32*)result);
	*refResult = 0;

	if (thisObj) {	// on temporary references, looks like most properties are invalid, starting with extraDataList, ParentCell looks good.
		*result = thisObj->refID;
	}

	return true;
}
