#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "ScriptUtils.h"

static ParamInfo kParams_OneOptionalActor[1] =
{
	{	"actor",	kParamType_Actor,	1	},
};

static ParamInfo kParams_OnePerkOneOptionalActor[2] =
{
	{	"perk", 	kParamType_Perk,	0	},
	{	"actor",	kParamType_Actor,	1	},
};

static ParamInfo kParams_OneIntOneOptionalActor[2] =
{
	{	"flag",		kParamType_Integer,	0	},
	{	"actor",	kParamType_Actor,	1	},
};

class TESActorBase;
class TESNPC;
class TESLevCreature;
class TESLevCharacter;

TESActorBase* ExtractActorBase(COMMAND_ARGS);
TESActorBase* ExtractSetActorBase(COMMAND_ARGS, UInt32* bMod);
TESNPC* ExtractNPC(COMMAND_ARGS);
TESLevCreature* ExtractLevCreature(COMMAND_ARGS);
TESLevCharacter* ExtractLevCharacter(COMMAND_ARGS);

DEFINE_CMD_COND(GetAgeClass, Return NPC age class. From 0=Child -default- to 3=OldAged. -1 for non NPC. Based on race fullname., 0, kParams_OneOptionalActor);
DEFINE_CMD_COND(GetRespawn, Return if the respawn flag is set on a base form, 0, kParams_OneOptionalActor);
DEFINE_COMMAND(SetRespawn, sets the respawn flag on a base form, 0, 2, kParams_OneIntOneOptionalActor);

DEFINE_CMD_COND(GetPermanent, Return if the permanent flag is set on a base form, 0, kParams_OneOptionalObjectRef);
DEFINE_COMMAND(SetPermanent, sets the permanent flag on a base form, 0, 2, kParams_OneIntOneOptionalObjectRef);

DEFINE_CMD(GetRace, Return NPC race, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetRaceName, Return the name of the NPC race, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetClass, Return NPC class, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetNameOfClass, Return the name of the NPC class, 0, kParams_OneOptionalForm);

DEFINE_CMD_COND(GetPerkRank, Return actor Perk rank or -1 if the perk is not applied, 0, kParams_OnePerkOneOptionalActor);
DEFINE_CMD_COND(GetAltPerkRank, Return actor alternate Perk rank or -1 if the perk is not applied, 0, kParams_OnePerkOneOptionalActor);

DEFINE_CMD(GetRaceHairs, Return the array of hair for a race, 0, kParams_OneRace);
DEFINE_CMD(GetRaceEyes, Return the array of eyes for a race, 0, kParams_OneRace);

DEFINE_CMD(GetBaseSpellListSpells, Return the array of spells for a SpellList, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetBaseSpellListLevSpells, Return the array of leveled spells for a SpellList, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetBasePackages, returns an array of packages for an actor base form, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetBaseFactions, returns an array of factions to which an actor base form belongs, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetBaseRanks, returns an array of faction ranks to which an actor base form belongs, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetActiveFactions, returns the active factions on an actor, 0, kParams_OneOptionalActor);
DEFINE_CMD(GetActiveRanks, returns the active faction ranks on an actor, 0, kParams_OneOptionalActor);

DEFINE_CMD(GetFactionRankNames, returns an array of name of faction ranks, 0, kParams_OneFaction);
DEFINE_CMD(GetFactionRankFemaleNames, returns an array of the female name of faction ranks, 0, kParams_OneFaction);

DEFINE_CMD(GetHeadParts, returns an array of head parts for a character, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetLevCreatureRefs, returns an array of ref in a leveled creature leveled list, 0, kParams_OneOptionalForm);
DEFINE_CMD(GetLevCharacterRefs, returns an array of ref in a leveled character leveled list, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetListForms, returns an array of forms in a form list, 0, kParams_FormList);

DEFINE_CMD(GenericAddForm, adds a form to a list of form of an object at index, 0, kParams_GenericForm);
DEFINE_CMD(GenericReplaceForm, change a form in a list of form of an object at index, 0, kParams_GenericForm);
DEFINE_CMD(GenericDeleteForm, delete a form from a list of form of an object at index, 0, kParams_GenericDeleteForm);
DEFINE_CMD(GenericGetForm, returns a form from a list of form of an object at index, 0, kParams_GenericDeleteForm);
DEFINE_CMD(GenericCheckForm, checks if a form is in a list of form of an object, 0, kParams_GenericCheckForm);

DEFINE_CMD(GetNthDefaultForm, returns the Nth default form, 0, kParams_OneInt);
DEFINE_CMD(SetNthDefaultForm, sets the Nth default form, 0, kParams_OneIntOneForm);
DEFINE_CMD(GetDefaultForms, returns all default forms as an array, 0, NULL);

DEFINE_CMD_ALT(GetCurrentQuestObjectiveTeleportLinks, GCQOTL, returns an array of array of the path to the current objective targets, 0, 0, 0);

DEFINE_CMD(GetNthAnimation, returns the nth animation selected on an NPC or a creature, 0, kParams_OneIntOneOptionalForm);
DEFINE_CMD(AddAnimation, Add an animation to an NPC or a creature, 0, kParams_OneStringOneOptionalForm);
DEFINE_CMD(DelAnimation, Delete an animation from an NPC or a creature, 0, kParams_OneStringOneOptionalForm);
DEFINE_CMD(DelAnimations, Delete all animation from an NPC or a creature, 0, kParams_OneOptionalForm);

static ParamInfo kNVSEParams_GetDoorSound[] =
{
	{ "door form"          , kNVSEParamType_Form  , 0 },
	{ "0 = open, 1 = close", kNVSEParamType_Number,	0 },
};
DEFINE_COMMAND_EXP(GetDoorSound, Get the open / close sound of a door, 0, kNVSEParams_GetDoorSound)

static ParamInfo kNVSEParams_FireChallenge[] = {
	{ "Challenge type", kNVSEParamType_Number, 0},
	{ "Count", kNVSEParamType_Number, 0 },
	{ "Weapon", kNVSEParamType_FormOrNumber, 0 },
	{ "Value 1", kNVSEParamType_Number, 0 },
	{ "Value 2", kNVSEParamType_Number, 0 },
	{ "Value 3", kNVSEParamType_Number, 0 }
};

DEFINE_COMMAND_EXP(FireChallenge, Increment a challenge using specified params, 0, kNVSEParams_FireChallenge)