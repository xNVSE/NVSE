#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

// for use with Inventory Objects
#define DEFINE_GET_INV(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 1, kParams_OneOptionalObjectID);
#define DEFINE_GET_INV_COND(name, alt, desc) DEFINE_CMD_ALT_COND(name, alt, desc, false, kParams_OneOptionalObjectID);
#define DEFINE_SET_INV_INT(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 2, kParams_OneInt_OneOptionalObjectID);
#define DEFINE_SET_INV_FLOAT(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 2, kParams_OneFloat_OneOptionalObjectID);
#define DEFINE_SET_INV_MAGIC(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 2, kParams_OneMagicItem_OneOptionalObjectID);
// for use with generic objects (anything with 3D) and general Base Forms that could be other types. Is more inclusive.
#define DEFINE_GET_OBJ(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 1, kParams_OneOptionalObject);

#define DEFINE_GET_FORM(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, false, 1, kParams_OneOptionalForm);
#define DEFINE_GET_FORM_COND(name, alt, desc) DEFINE_CMD_ALT_COND(name, alt, desc, false, kParams_OneOptionalForm);  //arg1/2 will always be INVALID

#define DEFINE_SET_CUR_FLOAT(name, alt, desc) DEFINE_CMD_ALT(name, alt, desc, true, 2, kParams_SetEquippedFloat);


DEFINE_CMD_COND(GetWeight, returns the weight of the specified base form, 0, kParams_OneOptionalObjectID);
DEFINE_GET_FORM_COND(GetHealth, GetBaseHealth, returns the base health of the object or calling reference);
DEFINE_GET_INV_COND(GetValue, GetItemValue, returns the base value of the object or calling reference); 
DEFINE_SET_INV_FLOAT(SetWeight, , sets the weight of the object); 
DEFINE_CMD_ALT(SetHealth, SetObjectHealth, sets the base health of the specified base form, 0, 2, kParams_OneInt_OneOptionalObject);
DEFINE_CMD_ALT(SetBaseItemValue, SetValue, sets the monetary value of a base object, 0, 2, kParams_OneObjectID_OneInt);
DEFINE_GET_FORM_COND(GetType, GetObjectType, returns the type of the specified base form);
DEFINE_GET_INV(GetRepairList, grl, returns the repair list for the inventory object);
DEFINE_GET_INV_COND(GetEquipType,  , returns the equipment type of the inventory object);
DEFINE_CMD_ALT(CompareNames,  , compares one name to another, 0, 2, kParams_OneObject_OneOptionalObject);
DEFINE_CMD_ALT(SetName,  , sets the name of the object., 0, 2, kParams_OneString_OneOptionalObject);
DEFINE_CMD_COND(GetCurrentHealth, returns the current health of the calling ref, 1, NULL);
DEFINE_COMMAND(SetCurrentHealth, sets the current health of the calling ref, 1, 1, kParams_OneFloat);
DEFINE_CMD_ALT(SetRepairList,  , sets the repair list for the specified item., 0, 2, kParams_OneFormList_OneOptionalObjectID);
DEFINE_GET_INV_COND(IsQuestItem, , returns 1 if the object or calling reference is a quest item);
DEFINE_SET_INV_INT(SetQuestItem, , Sets whether the specified object or object reference is a quest item);
DEFINE_GET_INV(GetObjectEffect, GetEnchantment, returns the object effect of the inventory object.);

DEFINE_COMMAND(GetHotkeyItem, returns the item assigned to the specified hotkey from 1 to 8, 0, 1, kParams_OneInt);
static ParamInfo kParams_SetHotkeyItem[2] =
{
	{	"hotkey",	kParamType_Integer,		0	},
	{	"item",		kParamType_ObjectID,	0	},
};
DEFINE_COMMAND(SetHotkeyItem, sets the item associated with a hotkey., 0, 2, kParams_SetHotkeyItem);
DEFINE_COMMAND(ClearHotkey, "clears the item associated with the specified hotkey.", 0, 1, kParams_OneInt);

// Inventory functions
DEFINE_CMD_ALT(GetEquippedObject, GetEqObj, returns the base object of the item in the specified slot, 1, 1, kParams_OneInt);
DEFINE_CMD_ALT_COND(GetEquippedCurrentHealth, GetEqCurHealth, returns the current health of the object equipped in the specified slot, 1, kParams_OneInt);
DEFINE_SET_CUR_FLOAT(SetEquippedCurrentHealth, SetEqCurHealth, sets the current health of the equipped object at the given slot.);
DEFINE_CMD_ALT_COND(GetNumItems, GetNumObjects, returns the number of items contained by the calling ref, 1, NULL);
DEFINE_CMD_ALT(GetInventoryObject, GetNthObject, returns the base object of the specified index contained in the calling ref, 1, 1, kParams_OneInt);


// Weapon functions
DEFINE_GET_INV(GetWeaponAmmo, GetAmmo, returns the ammo of a weapon);
DEFINE_GET_INV_COND(GetWeaponClipRounds, GetClipSize, returns the clip size for the weapon);
DEFINE_GET_INV_COND(GetAttackDamage, GetDamage, returns the attack damage for the weapon);
DEFINE_GET_INV_COND(GetWeaponType, GetWeapType, returns the weapon type);
DEFINE_GET_INV_COND(GetWeaponMinSpread, GetMinSpread, returns the minimum spread of the weapon);
DEFINE_GET_INV_COND(GetWeaponSpread, GetSpread, returns the spread of the weapon);
DEFINE_GET_INV(GetWeaponProjectile, GetWeapProj, returns the weapon projectile info);
DEFINE_GET_INV_COND(GetWeaponSightFOV, GetSightFOV, returns the zoom field of view for the weapon);
DEFINE_GET_INV_COND(GetWeaponMinRange, GetMinRange, returns the min range of the weapon);
DEFINE_GET_INV_COND(GetWeaponMaxRange, GetMaxRange, returns the max range of the weapon);
DEFINE_GET_INV_COND(GetWeaponAmmoUse, GetAmmoUse, returns the ammo used per shot of the weapon);
DEFINE_GET_INV_COND(GetWeaponActionPoints, GetAP, returns the number of action points per shot of the weapon);
DEFINE_GET_INV_COND(GetWeaponCritDamage, GetCritDam, returns the critical damage of the weapon);
DEFINE_GET_INV_COND(GetWeaponCritChance, GetCritPerc, returns the chance of a critical shot for the weapon);
DEFINE_GET_INV(GetWeaponCritEffect, GetCritEffect, returns the spell for the critical effect for the weapon);
DEFINE_GET_INV_COND(GetWeaponFireRate, GetFireRate, returns the fire rate of the weapon.);
DEFINE_GET_INV_COND(GetWeaponAnimAttackMult, GetAnimAttackMult, returns the animation attack multiplier of the weapon.);
DEFINE_GET_INV_COND(GetWeaponRumbleLeftMotor, GetRumbleLeft, returns the rumble left motor of the weapon.);
DEFINE_GET_INV_COND(GetWeaponRumbleRightMotor, GetRumbleRight, returns the rumble right motor of the weapon.);
DEFINE_GET_INV_COND(GetWeaponRumbleDuration, GetRumbleDuration, returns the rumble duration of the weapon.);
DEFINE_GET_INV_COND(GetWeaponRumbleWavelength, GetRumbleWavelen, returns the rumble wavelegnth for the weapon.);
DEFINE_GET_INV_COND(GetWeaponAnimShotsPerSec, GetAnimShotsPerSec, returns the animation shots per second of the weapon.);
DEFINE_GET_INV_COND(GetWeaponAnimReloadTime, GetAnimReloadTime, retuns the animation reload time for the weapon.);
DEFINE_GET_INV_COND(GetWeaponAnimJamTime, GetAnimJamTime, returns the animation jam time of the weapon.);
DEFINE_GET_INV_COND(GetWeaponSkill,  , returns the skill for the weapon.);
DEFINE_GET_INV_COND(GetWeaponResistType, GetWeaponResist, returns the resist type for the weapon.);
DEFINE_GET_INV_COND(GetWeaponFireDelayMin, GetFireDelayMin, returns the semi-auto min fire delay for the weapon.);
DEFINE_GET_INV_COND(GetWeaponFireDelayMax, GetFireDelayMax, returns the semi-auto max fire delay for the weapon.);
DEFINE_GET_INV_COND(GetWeaponAnimMult, GetAnimMult, returns the animation multiplier for the weapon);
DEFINE_GET_INV_COND(GetWeaponReach, GetReach,returns the reach of the weapon);
DEFINE_GET_INV_COND(GetWeaponIsAutomatic, GetIsAutomatic, returns 1 if the weapon is an automatic weapon);
DEFINE_GET_INV_COND(GetWeaponHandGrip, GetHandGrip, returns the hand grip of the weapon);
DEFINE_GET_INV_COND(GetWeaponReloadAnim, GetReloadAnim, returns the reload animation of the weapon);
DEFINE_GET_INV_COND(GetWeaponBaseVATSChance, GetVATSChance, returns the base VATS chance of the weapon);
DEFINE_GET_INV_COND(GetWeaponAttackAnimation, GetAttackAnim, returns the attack animation of the weapon);
DEFINE_GET_INV_COND(GetWeaponNumProjectiles, GetNumProj, returns the number of projectiles used in a single shot by the weapon.);
DEFINE_GET_INV_COND(GetWeaponAimArc, GetAimArc, returns the aim arc of the weapon.);
DEFINE_GET_INV_COND(GetWeaponLimbDamageMult, GetLimbDamageMult, returns the limb damage multiplier of the weapon.);
DEFINE_GET_INV_COND(GetWeaponSightUsage, GetSightUsage, returns the sight usage of the weapon.);
DEFINE_CMD_COND(GetWeaponHasScope, returns whether the weapon has a scope or not., 0, kParams_OneOptionalObjectID);
DEFINE_COMMAND(GetWeaponItemMod, returns the specified item mod of the weapon, 0, 2, kParams_OneInt_OneOptionalObjectID);
DEFINE_GET_INV_COND(GetWeaponRequiredStrength, GetReqStr, returns the required strength for the weapon.);
DEFINE_GET_INV_COND(GetWeaponRequiredSkill, GetReqSkill, returns the required strength for the weapon.);
DEFINE_GET_INV_COND(GetWeaponLongBursts, GetLongBursts, returns if a weapon uses long bursts);
DEFINE_GET_INV_COND(GetWeaponFlags1, , returns weapon flags bitfield 1);
DEFINE_GET_INV_COND(GetWeaponFlags2, , returns weapon flags bitfield 2);
DEFINE_GET_INV_COND(GetWeaponRegenRate, , "");


DEFINE_CMD_ALT(SetWeaponAmmo, SetAmmo, sets the ammo of the weapon, 0, 2, kParams_OneInventoryItem_OneOptionalObjectID);
DEFINE_SET_INV_INT(SetWeaponClipRounds, SetClipSize, sets the weapon clip size);
DEFINE_SET_INV_INT(SetAttackDamage, SetDamage, sets the attack damage of the form);
DEFINE_SET_INV_INT(SetWeaponType, , sets the weapon type of the weapon.);
DEFINE_SET_INV_FLOAT(SetWeaponMinSpread, SetMinSpread, sets the weapon min spread);
DEFINE_SET_INV_FLOAT(SetWeaponSpread, SetSpread, sets the weapon spread);
DEFINE_CMD_ALT(SetWeaponProjectile, SetProjectile, sets the projectile of the weapon, 0, 2, kParams_OneInventoryItem_OneOptionalObjectID);
DEFINE_SET_INV_FLOAT(SetWeaponSightFOV, SetSightFOV, sets the weapon xoom field of view);
DEFINE_SET_INV_FLOAT(SetWeaponMinRange, SetMinRange, sets the weapon min range);
DEFINE_SET_INV_FLOAT(SetWeaponMaxRange, SetMaxRange, sets the weapon max range.);
DEFINE_SET_INV_INT(SetWeaponAmmoUse, SetAmmoUse, sets the weapon ammo use);
DEFINE_SET_INV_FLOAT(SetWeaponActionPoints, SetAP, sets the weapon number of action pointer per shot);
DEFINE_SET_INV_INT(SetWeaponCritDamage, SetCritDam, sets the weapon critical hit damage.);
DEFINE_SET_INV_FLOAT(SetWeaponCritChance, SetCritPerc, sets the weapon critical hit chance);
DEFINE_SET_INV_MAGIC(SetWeaponCritEffect, SetCritEffect, sets the weapon critical hit effect);
DEFINE_SET_INV_FLOAT(SetWeaponFireRate, SetFireRate, sets the weapon fire rate);
DEFINE_SET_INV_FLOAT(SetWeaponAnimAttackMult, SetAnimAttackMult, sets the anim attack multiplier.);
DEFINE_SET_INV_FLOAT(SetWeaponAnimMult, SetAnimMult, sets the animiation multiple of the weapon.);
DEFINE_SET_INV_FLOAT(SetWeaponReach, SetReach, sets the reach of the weapon.);
DEFINE_SET_INV_INT(SetWeaponIsAutomatic, SetIsAutomatic, sets whether the weapon is an automatic weapon or not.);
DEFINE_SET_INV_INT(SetWeaponHandGrip, SetHandGrip, sets the hand grip of the weapon.);
DEFINE_SET_INV_INT(SetWeaponReloadAnim, SetReloadAnim, sets the reload animation of the weapon.);
DEFINE_SET_INV_INT(SetWeaponBaseVATSChance, SetVATSChance, sets the base VATS chance for the weapon.);
DEFINE_SET_INV_INT(SetWeaponAttackAnimation, SetAttackAnim, sets the attack animation for the weapon.);
DEFINE_SET_INV_INT(SetWeaponNumProjectiles, SetNumProj, sets the number of projectiles the weapon fires each time it is shot.);
DEFINE_SET_INV_FLOAT(SetWeaponAimArc, SetAimArc, sets the aim arc of the weapon.);
DEFINE_SET_INV_FLOAT(SetWeaponLimbDamageMult, SetLimbDamageMult, sets the limb damage multiplier for the weapon.);
DEFINE_SET_INV_FLOAT(SetWeaponSightUsage, SetSightUsage, sets the sight usage for the weapon.);
DEFINE_SET_INV_INT(SetWeaponRequiredStrength, SetReqStr, sets the required strength for the weapon);
DEFINE_SET_INV_INT(SetWeaponRequiredSkill, SetReqSkill, sets the required skill for the weapon);
DEFINE_SET_INV_INT(SetWeaponSkill, , sets the skill for the weapon);
DEFINE_SET_INV_INT(SetWeaponResistType, SetWeaponResist, sets the weapon clip size);
DEFINE_SET_INV_INT(SetWeaponLongBursts, SetLongBursts, sets if a weapon uses long bursts);
DEFINE_SET_INV_INT(SetWeaponFlags1, , sets weapon flags bitfield 1);
DEFINE_SET_INV_INT(SetWeaponFlags2, , sets weapon flags bitfield 2);
DEFINE_SET_INV_FLOAT(SetWeaponRegenRate, , "");
DEFINE_CMD_COND(GetEquippedWeaponModFlags, returns equipped weapon mod flags, 1, NULL);
DEFINE_COMMAND(SetEquippedWeaponModFlags, sets equipped weapon mod flags, 1, 1, kParams_OneInt);
DEFINE_COMMAND(GetWeaponItemModEffect, returns the specified item mod effect id of the weapon, 0, 2, kParams_OneInt_OneOptionalObjectID);
DEFINE_COMMAND(GetWeaponItemModValue1, returns the specified item mod value1 of the weapon, 0, 2, kParams_OneInt_OneOptionalObjectID);
DEFINE_COMMAND(GetWeaponItemModValue2, returns the specified item mod value2 of the weapon, 0, 2, kParams_OneInt_OneOptionalObjectID);

// Armor functions
DEFINE_GET_INV_COND(GetArmorAR, GetArmorArmorRating, returns the armor rating of the specified armor.);
DEFINE_GET_INV_COND(IsPowerArmor, IsPA, returns whether the biped form is considered power armor.);
DEFINE_GET_INV_COND(GetArmorDT, GetArmorDamageThreshold, returns the damage threshold of the armor.);
DEFINE_GET_INV(IsPlayable, , returns whether the biped form is usable by the player.);
DEFINE_GET_INV(GetEquipmentSlotsMask, GetESM, returns the slots used by the biped form as a bitmask.);
DEFINE_GET_INV(GetEquipmentBipedMask, GetEBM, gets the flags used by the biped form as a bitmask.);

DEFINE_SET_INV_INT(SetIsPowerArmor, SetIsPA, sets whether the armor is power armor or not.);
DEFINE_SET_INV_INT(SetArmorAR, SetArmorArmorRating, sets the armor rating of the armor.);
DEFINE_SET_INV_FLOAT(SetArmorDT, SetArmorDamageThreshold, sets the damage threshold of the armor);
DEFINE_SET_INV_INT(SetIsPlayable, , sets whether the armor is usable by the player or not.);
DEFINE_SET_INV_INT(SetEquipmentSlotsMask, SetESM, sets the slots used by the biped form from a bitmask.);
DEFINE_SET_INV_INT(SetEquipmentBipedMask, SetEBM, sets the flags used by the biped form from a bitmask.);

// Ammo functions
DEFINE_GET_INV_COND(GetAmmoSpeed, , returns the speed of the specified ammo.);
DEFINE_GET_INV_COND(GetAmmoConsumedPercent, , returns the percentage of ammo consumed for the specified ammo.);
DEFINE_CMD(SetAmmoConsumedPercent, "", false, kParams_OneFloat_OneOptionalObjectID);
DEFINE_GET_INV(GetAmmoCasing, , returns the casing of the specified ammo);

DEFINE_COMMAND(GetPlayerCurrentAmmoRounds, returns the current number of rounds in the clip of the player, 0, 0, NULL);
DEFINE_COMMAND(SetPlayerCurrentAmmoRounds, sets the current number of rounds in the clip of the player, 0, 1, kParams_OneInt);
DEFINE_COMMAND(GetPlayerCurrentAmmo, returns the current ammo in the weapon held by the player, 0, 0, NULL);

DEFINE_CMD_ALT_COND(HasAmmoEquipped, IsAmmoEquipped, 
	"returns if a specific ammo is equipped by the calling ref",
	true, kParams_OneObjectID);
DEFINE_CMD_COND(IsEquippedAmmoInList, "returns if any ammo in a list is currently equipped by the calling ref",
	true, kParams_OneFormList);
DEFINE_CMD_COND(GetWeaponCanUseAmmo,
	"returns if the weapon can use the specified ammo",
	false, kParams_OneObjectID_OneOptionalObjectID);
DEFINE_CMD_COND(GetEquippedWeaponCanUseAmmo,
	"returns if the equipped weapon can use the specified ammo",
	true, kParams_OneObjectID);
DEFINE_CMD_COND(GetEquippedWeaponUsesAmmoList,
	"returns if the equipped weapon uses a specific list of ammo",
	true, kParams_OneFormList);

DEFINE_GET_FORM(CloneForm, , clones the specified form and returns a new base form which will not be saved in the save game.);
DEFINE_GET_FORM(IsClonedForm, IsCloned, returns whether the specified form is a created object or not.);

static ParamInfo kParamsTempCloneForm[2] =
{
	{	"form",	kParamType_AnyForm,	0	},
	{	"bInheritModIndex",	kParamType_Integer,	1	},
};

DEFINE_CMD(TempCloneForm, "clones the specified form and returns a new base form which will be saved in the save game.", false, kParamsTempCloneForm);

#undef DEFINE_GET_INV
#undef DEFINE_SET_INV_INT
#undef DEFINE_SET_INV_FLOAT
#undef DEFINE_SET_INV_MAGIC
#undef DEFINE_GET_OBJ
#undef DEFINE_GET_FORM
#undef DEFINE_SET_CUR_FLOAT

static ParamInfo kParams_SetNameEx[22] =
{
	FORMAT_STRING_PARAMS,
	{"inventory object", kParamType_AnyForm,	1	},
};

DEFINE_COMMAND(SetNameEx, sets the name of the object based on the format string, 0, 22, kParams_SetNameEx);

static ParamInfo kParams_OneObjectID_TwoFloat_OneObject_OneOptionalRank[5] =
{
	{	"item",		kParamType_ObjectID,	0	},
	{	"count",	kParamType_Float,		0	},
	{	"health",	kParamType_Float,		0	},
 	{	"owner",	kParamType_AnyForm,	0	},
 	{	"rank",		kParamType_Integer,		1	},
};

static ParamInfo kParams_OneObjectID_OneFloat_OneObject_OneOptionalRank[4] =
{
	{	"item",		kParamType_ObjectID,	0	},
	{	"count",	kParamType_Float,		0	},
 	{	"owner",	kParamType_AnyForm,	0	},
 	{	"rank",		kParamType_Integer,		1	},
};

DEFINE_COMMAND(AddItemOwnership, adds an item to a container with a specified ownership., 1, 4, kParams_OneObjectID_OneFloat_OneObject_OneOptionalRank); 
DEFINE_COMMAND(AddItemHealthPercentOwner, adds an item to a container at a specified health percentage and with an ownership., 1, 5, kParams_OneObjectID_TwoFloat_OneObject_OneOptionalRank); 

static ParamInfo kParams_OneObjectID_OneObject[2] =
{
	{	"item",			kParamType_ObjectID,	0	},
 	{	"reference",	kParamType_AnyForm,	0	},
};
static ParamInfo kParams_OneObjectID_OneActor[2] =
{
	{	"item",			kParamType_ObjectID,	0	},
 	{	"actor",	kParamType_Actor,		0	},
};
static ParamInfo kParams_OneObjectID_OneFloat[2] =
{
	{	"item",		kParamType_ObjectID,	0	},
	{	"value",	kParamType_Float,		0	},
};
static ParamInfo kParams_OneObjectID_OneFloat_OneObject[3] =
{
	{	"item",			kParamType_ObjectID,	0	},
	{	"value",		kParamType_Float,		0	},
 	{	"reference",	kParamType_AnyForm,	0	},
};

// Token: A token is an Item that can only be present once in inventory, and must allow Health and Ownership.
//	Most likely created from an Armor with no biped slot so it can't be equipped.
DEFINE_CMD_ALT_COND(GetTokenValue, GetTV, get a token value., 0, kParams_OneForm);
DEFINE_CMD_ALT(SetTokenValue, SetTV, set a token value or create a token with value., 1, 2, kParams_OneObjectID_OneFloat);

DEFINE_CMD_ALT_COND(GetTokenRef, GetTR, get a token reference., 0, kParams_OneForm);
DEFINE_CMD_ALT(SetTokenRef, SetTR, Add or modify a token reference., 1, 2, kParams_OneObjectID_OneObject);

DEFINE_CMD_ALT(SetTokenValueAndRef, SetTVR, Add or modify a token with value and reference., 1, 3, kParams_OneObjectID_OneFloat_OneObject);

DEFINE_CMD_COND(GetPaired, Detects if ref and actor are crossreferencing each others, 1, kParams_OneObjectID_OneActor);

DEFINE_CMD(PickOneOf, Selects a random item present in the formList, 1, kParams_FormList);

DEFINE_CMD(EquipItem2, equips and runs onequip block, 1, kParams_EquipItem);
DEFINE_CMD(EquipMe, equips the calling object on its owner, 1, NULL);
DEFINE_CMD(UnequipMe, unequips the calling object on its owner, 1, NULL);

DEFINE_COMMAND(IsEquipped, returns 1 if the calling object is currently being worn, 1, 0, NULL);
//Does not work as a condition function.

DEFINE_CMD(GetMe, returns the current object reference for items that do not accept GetSelf. For most of those the value will not survive the current frame!!!, 1, NULL);
