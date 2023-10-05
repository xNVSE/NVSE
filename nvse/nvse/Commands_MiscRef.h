#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "ScriptUtils.h"

DEFINE_CMD_ALT(GetBaseObject, gbo, returns the base object id of the reference, 1, 0, NULL);
DEFINE_CMD_ALT(GetBaseForm, gbf, returns the permanent base object id of the reference, 1, 0, NULL);
DEFINE_COMMAND(IsPersistent, returns true if the calling reference is persistent, 1, 0, NULL);
DEFINE_CMD_ALT(GetParentCell, gpc, returns the parent cell of the reference, 1, 0, NULL);
DEFINE_CMD_ALT(GetParentWorldspace, gpw, returns the parent worldspace of the reference, 1, 0, NULL);
DEFINE_CMD_ALT(GetTeleportCell, , returns the cell to which the calling door reference teleports, 1, 0, NULL);
DEFINE_CMD_ALT(GetLinkedDoor, , returns the door to which the calling reference is linked, 1, 0, NULL);
DEFINE_COMMAND(GetOwner, returns the owner of the calling reference, 1, 0, NULL);
DEFINE_COMMAND(GetParentCellOwner, returns the owner of the cell of the calling reference, 1, 0, NULL);
DEFINE_COMMAND(GetOwningFactionRequiredRank, returns the required rank for ownership of the calling reference, 1, 0, NULL);
DEFINE_COMMAND(GetParentCellOwningFactionRequiredRank, returns the required rank for ownership of the cell of the calling reference, 1, 0, NULL);

static ParamInfo kParams_GetFirstRef[3] =
{
	{	"form type",			kParamType_Integer,	1	},
	{	"cell depth",			kParamType_Integer,	1	},
	{	"include taken refs",	kParamType_Integer,	1	},
};

static ParamInfo kParams_GetRefs[6] =
{
	{	"form type",			kParamType_Integer,	1	},
	{	"cell depth",			kParamType_Integer,	1	},
	{	"include taken refs",	kParamType_Integer,	1	},
	{	"max distance",			kParamType_Float,	1	},
	{	"max angle",			kParamType_Float,	1	},
	{	"baseForm",				kParamType_AnyForm,	1	},
};

DEFINE_COMMAND(GetFirstRef, returns the first reference of the specified type in the current cell, 0, 3, kParams_GetFirstRef);
DEFINE_COMMAND(GetNextRef, returns the next reference of a given type in the current cell, 0, 0, NULL);
DEFINE_CMD(GetNumRefs, returns the number of references of a given type in the current cell, false, kParams_GetRefs);
DEFINE_CMD(GetRefs, returns an array of references of a given type in the current cell, false, kParams_GetRefs);

static ParamInfo kParams_GetInGrid[3] =
{
	{	"refr",					kParamType_ObjectRef,	1	},
	{	"cell depth",			kParamType_Integer,		1	},
	{	"include taken refs",	kParamType_Integer,		1	},
};

DEFINE_COMMAND(GetInGrid, returns if a specific reference is in the current cell, 0, 3, kParams_GetInGrid);

static ParamInfo kParams_GetFirstRefInCell[4] =
{
	{	"cell",					kParamType_Cell,	0	},
	{	"form type",			kParamType_Integer,	1	},
	{	"cell depth",			kParamType_Integer,	1	},
	{	"include taken refs",	kParamType_Integer,	1	},
};

DEFINE_COMMAND(GetFirstRefInCell, returns the first reference of the specified type in the specified cell, 0, 4, kParams_GetFirstRefInCell);

static ParamInfo kParams_GetRefsInCell[7] =
{
	{	"cell",					kParamType_Cell,	0	},
	{	"form type",			kParamType_Integer,	1	},
	{	"cell depth",			kParamType_Integer,	1	},
	{	"include taken refs",	kParamType_Integer,	1	},
	{	"max distance",			kParamType_Float,	1	},
	{	"max angle",			kParamType_Float,	1	},
	{	"baseForm",				kParamType_AnyForm,	1	},
};

DEFINE_CMD(GetNumRefsInCell, returns the number of references of a given type in the specified cell, false, kParams_GetRefsInCell);
DEFINE_CMD(GetRefsInCell, returns an array of references of a given type in the specified cell, false, kParams_GetRefsInCell);

static ParamInfo kParams_GetInGridInCell[4] =
{
	{	"cell",					kParamType_Cell,		0	},
	{	"refr",					kParamType_ObjectRef,	0	},
	{	"cell depth",			kParamType_Integer,		1	},
	{	"include taken refs",	kParamType_Integer,		1	},
};

DEFINE_COMMAND(GetInGridInCell, returns if a specific reference is in the grid centered on the specified cell, 0, 4, kParams_GetInGridInCell);

DEFINE_COMMAND(GetRefCount, returns the count in a stacked reference, 1, 0, NULL);
DEFINE_COMMAND(SetRefCount, sets the count on a stacked reference, 1, 1, kParams_OneInt);

DEFINE_CMD_ALT(GetOpenKey, GetKey, returns the key associated with a lockable object, 1, 0, NULL);
DEFINE_CMD_ALT(SetOpenKey, SetKey, sets the key used to unlock the calling object, 1, 1, kParams_OneObjectID);
DEFINE_CMD_ALT(ClearOpenKey, ClearKey, clears the key used to unlock the calling object, 1, 0, NULL);

static ParamInfo kParamType_OneOptionalActorBase[1] =
{
	{ "actor base",	kParamType_ActorBase,	1 }
};

static ParamInfo kParamType_OneInt_OneOptionalActorBase[2] =
{
	{ "flags",		kParamType_Integer,		0 },
	{ "actor base",	kParamType_ActorBase,	1 }
};

DEFINE_COMMAND(GetActorBaseFlagsLow, returns a bitfield containing actor base flags, 0, 1, kParamType_OneOptionalActorBase);
DEFINE_COMMAND(SetActorBaseFlagsLow, sets actor base flags, 0, 2, kParamType_OneInt_OneOptionalActorBase);
DEFINE_COMMAND(GetActorBaseFlagsHigh, returns a bitfield containing actor base flags, 0, 1, kParamType_OneOptionalActorBase);
DEFINE_COMMAND(SetActorBaseFlagsHigh, sets actor base flags, 0, 2, kParamType_OneInt_OneOptionalActorBase);

static ParamInfo kParamType_OneOptionalAnyForm[1] =
{
	{ "actor base",	kParamType_AnyForm,	1 }
};

static ParamInfo kParamType_OneInt_OneOptionalAnyForm[2] =
{
	{ "flags",		kParamType_Integer,	0 },
	{ "actor base",	kParamType_AnyForm,	1 }
};

DEFINE_COMMAND(GetFlagsLow, returns a bitfield containing object flags, 0, 1, kParamType_OneOptionalAnyForm);
DEFINE_COMMAND(SetFlagsLow, sets actor flags, 0, 2, kParamType_OneInt_OneOptionalAnyForm);
DEFINE_COMMAND(GetFlagsHigh, returns a bitfield containing actor flags, 0, 1, kParamType_OneOptionalAnyForm);
DEFINE_COMMAND(SetFlagsHigh, sets actor base flags, 0, 2, kParamType_OneInt_OneOptionalAnyForm);

DEFINE_CMD_COND(HasOwnership, check if an NPC has direct or indirect ownership, 1, kParams_OneOptionalObjectRef);
DEFINE_CMD_COND(IsOwned, check if an object is directly or indirectly owned by an NPC, 1, kParams_OneOptionalActorRef);

DEFINE_COMMAND(SetOwningFactionRequiredRank, sets the required rank for ownership of the calling reference, 1, 1, kParams_OneInt);

// Using ObjectID was blocked by IsInventoryObject

static ParamInfo kParams_SetNPCBodyData[2] =
{
	{	"body data",	kParamType_AnyForm,	0	},
	{	"base NPC",		kParamType_NPC,		1	},
};

static ParamInfo kParams_SetHairLength[2] =
{
	{	"hair length",	kParamType_Float,	0	},
	{	"base NPC",		kParamType_NPC,		1	},
};

static ParamInfo kParams_SetHairColor[2] =
{
	{	"hair color",	kParamType_Integer,	0	},
	{	"base NPC",		kParamType_NPC,		1	},
};

static ParamInfo kParams_GetHairColor[2] =
{
	{	"hair color code",	kParamType_Integer,	0	},
	{	"base NPC",			kParamType_NPC,		1	},
};

static ParamInfo kParams_SetNPCWeight[2] =
{
	{	"weight",	kParamType_Float,	0	},
	{	"base NPC",	kParamType_NPC,		1	},
};

static ParamInfo kParams_SetNPCHeight[2] =
{
	{	"height",	kParamType_Float,	0	},
	{	"base NPC",	kParamType_NPC,		1	},
};

static ParamInfo kParams_OneEyes_OneOptionalMask[2] =
{
	{	"eyes", kParamType_AnyForm,		0 }, 
	{	"mask", kParamType_Integer,	1 }, 
};

static ParamInfo kParams_OneHair_OneOptionalMask[2] =
{
	{	"hair", kParamType_AnyForm,	0 }, 
	{	"mask", kParamType_Integer,	1 }, 
};

DEFINE_COMMAND(SetEyes, set an NPCs eyes, false, 2, kParams_SetNPCBodyData);
DEFINE_COMMAND(GetEyes, get an NPCs eyes, false, 1, kParams_OneNPC);
DEFINE_COMMAND(SetHair, set an NPCs hair, false, 2, kParams_SetNPCBodyData);
DEFINE_COMMAND(GetHair, set an NPCs hair, false, 1, kParams_OneNPC);
DEFINE_COMMAND(SetHairLength, set an NPCs hair length, false, 2, kParams_SetHairLength);
DEFINE_COMMAND(GetHairLength, get an NPCs hair length, false, 1, kParams_OneNPC);
DEFINE_COMMAND(SetHairColor, set an NPCs hair color (RGB bytes), false, 2, kParams_SetHairColor);
DEFINE_COMMAND(GetHairColor, get an NPCs hair color (code: 1=R, 2=G, 3=B), false, 2, kParams_GetHairColor);
DEFINE_COMMAND(SetNPCWeight, set an NPCs weight, false, 2, kParams_SetNPCWeight);
DEFINE_COMMAND(GetNPCWeight, get an NPCs weight, false, 1, kParams_OneNPC);
DEFINE_COMMAND(SetNPCHeight, set an NPCs height, false, 2, kParams_SetNPCWeight);
DEFINE_COMMAND(GetNPCHeight, get an NPCs height, false, 1, kParams_OneNPC);

DEFINE_COMMAND(Update3D, updates the visual representation of the calling reference, true, 0, NULL);	// Not finished for player

DEFINE_CMD_COND(IsPlayerSwimming, check if the player is swimming, 0, NULL);
DEFINE_CMD_COND(GetTFC, returns TFC status, 0, NULL);

static ParamInfo kParams_PlaceAtMe[4] =
{
	{	"ObjectID",		kParamType_TESObject,	0	},
	{	"count",		kParamType_Integer,		0	},
	{	"distance",		kParamType_Float,		1	},
	{	"direction",	kParamType_Integer,		1	},
};

DEFINE_CMD(PlaceAtMeAndKeep, calls PlaceAtMe and marks the resulting form persistent, true, kParams_PlaceAtMe);

DEFINE_COMMAND(IsLoadDoor, "returns 1 if the calling reference is a load door", 1, 0, NULL);
DEFINE_COMMAND(GetDoorTeleportX, returns x-coord to which the door teleports, 1, 0, NULL);
DEFINE_COMMAND(GetDoorTeleportY, returns y-coord to which the door teleports, 1, 0, NULL);
DEFINE_COMMAND(GetDoorTeleportZ, returns z-coord to which the door teleports, 1, 0, NULL);
DEFINE_COMMAND(GetDoorTeleportRot, returns z rotation to which the door teleports, 1, 0, NULL);

static ParamInfo kParams_SetDoorTeleport[5] =
{
	{	"linkedDoor",	kParamType_ObjectRef,	0	},
	{	"x",			kParamType_Float,		1	},
	{	"y",			kParamType_Float,		1	},
	{	"z",			kParamType_Float,		1	},
	{	"rot",			kParamType_Float,		1	},
};

DEFINE_COMMAND(SetDoorTeleport, sets the linked door and coordinates to which the door teleports, 0, 5, kParams_SetDoorTeleport);

DEFINE_COMMAND(GetEyesFlags, get the flags of eyes, false, 2, kParams_OneEyes_OneOptionalMask);
DEFINE_COMMAND(SetEyesFlags, set the flags of eyes, false, 2, kParams_OneEyes_OneOptionalMask);
DEFINE_COMMAND(GetHairFlags, get the flags of an hair, false, 2, kParams_OneHair_OneOptionalMask);
DEFINE_COMMAND(SetHairFlags, set the flags of an hair, false, 2, kParams_OneHair_OneOptionalMask);

DEFINE_CMD_ALT_COND(GetActorFIKstatus, GetFIK, get an actor Foot IK status, true, NULL);
DEFINE_CMD_ALT(SetActorFIKstatus, SetFIK, set an actor Foot IK status, true, 1, kParams_OneInt);

static ParamInfo kParams_OneEffectShader[1] =
{
	{ "effectShader",	kParamType_EffectShader,	0	},
};

DEFINE_COMMAND(HasEffectShader, returns 1 if the reference is playing the effect shader, 1, 1, kParams_OneEffectShader);

DEFINE_CMD(SetEditorID, "sets editor id of form", 0, kParams_OneForm_OneString);

static ParamInfo kNVSEParams_OneOptionalString_OneOptionalArray[2] =
{
	{	"string",	kNVSEParamType_String,	1	},
	{	"array",	kNVSEParamType_Array,	1	}
};

DEFINE_COMMAND_EXP(CreateFormList, "creates a formList, optionally set with an editorID and filled by an array.", 
	0, kNVSEParams_OneOptionalString_OneOptionalArray);

DEFINE_CMD(GetHeadingAngleX, "Gets the up/down angle the calling reference is relative to a target.", 
	1, kParams_OneObjectRef);