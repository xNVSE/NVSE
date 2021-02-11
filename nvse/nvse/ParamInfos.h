#pragma once

#include "CommandTable.h"

static ParamInfo kParams_OneInt[1] =
{
	{	"int", kParamType_Integer, 0 }, 
};

static ParamInfo kParams_TwoInts[2] =
{
	{	"int", kParamType_Integer, 0 },
	{	"int", kParamType_Integer, 0 },
};

static ParamInfo kParams_OneOptionalInt[1] =
{
	{	"int", kParamType_Integer, 1 }, 
};

static ParamInfo kParams_OneInt_OneOptionalInt[2] =
{
	{	"int", kParamType_Integer, 0 },
	{	"int", kParamType_Integer, 1 },
};

static ParamInfo kParams_OneFloat[1] =
{
	{	"float", kParamType_Float,	0 },
};

static ParamInfo kParams_OneString[1] =
{
	{	"string",	kParamType_String,	0 },
};

static ParamInfo kParams_OneString_OneFloat[] =
{
	{	"string",	kParamType_String,	0 },
	{	"float",	kParamType_Float,	0 },
};

static ParamInfo kParams_TwoFloats[2] =
{
	{	"float",	kParamType_Float,	0 },
	{	"float",	kParamType_Float,	0 },
};

static ParamInfo kParams_OneObjectID[1] =
{
	{	"item", kParamType_ObjectID, 0},
};

static ParamInfo kParams_OneOptionalObjectID[1] =
{
	{	"item", kParamType_ObjectID, 1},
};

static ParamInfo kParams_OneInt_OneOptionalObjectID[2] =
{
	{	"path type",	kParamType_Integer,			0	},
	{	"item",			kParamType_ObjectID,	1	},
};

static ParamInfo kParams_OneInt_OneOptionalForm[2] =
{
	{	"path type",	kParamType_Integer,			0	},
	{	"form",			kParamType_AnyForm,	1	},
};

static ParamInfo kParams_OneObjectID_OneInt[2] =
{
	{	"item",		kParamType_ObjectID,	0	},
	{	"integer",	kParamType_Integer,			0	},
};

static ParamInfo kParams_OneFloat_OneOptionalObjectID[2] =
{
	{	"float",		kParamType_Float,			0	},
	{	"item",			kParamType_ObjectID,	1	},
};

 static ParamInfo kParams_OneMagicItem_OneOptionalObjectID[2] =
 {
 	{	"magic item",	kParamType_MagicItem,		0	},
 	{	"item",			kParamType_ObjectID,	1	},
 };
 
 static ParamInfo kParams_OneInventoryItem_OneOptionalObjectID[2] =
 {
 	{	"inv item",		kParamType_AnyForm,		0	},
 	{	"target item",	kParamType_ObjectID,	1	},
 };
 
 static ParamInfo kParams_OneFormList_OneOptionalObjectID[2] =
 {
 	{	"form list",	kParamType_FormList,		0	},
 	{	"target item",	kParamType_ObjectID,	1	},
 };


static ParamInfo kParams_OneActorValue[1] =
{
	{	"actor value", kParamType_ActorValue, 0},
};

#define FORMAT_STRING_PARAMS 	\
	{"format string",	kParamType_String, 0}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1}, \
	{"variable",		kParamType_Float, 1} 

static ParamInfo kParams_FormatString[21] =
{
	FORMAT_STRING_PARAMS
};

#define SIZEOF_FMT_STRING_PARAMS 21
#define NUM_PARAMS(paramInfoName) SIZEOF_ARRAY(paramInfoName, ParamInfo)

static ParamInfo kParams_OneActorRef[1] =
{
	{	"actor reference",	kParamType_Actor,	0	},
};

static ParamInfo kParams_OneOptionalActorRef[1] =
{
	{	"actor reference",	kParamType_Actor,	1	},
};

static ParamInfo kParams_Axis[1] = 
{
	{	"axis",	kParamType_Axis,	0	},
};

static ParamInfo kParams_FormList[1] =
{
	{	"form list", kParamType_FormList,	0		},
};

static ParamInfo kParams_OneString_OneOptionalObjectID[2] =
{
	{	"string",		kParamType_String,			0	},
	{	"item",			kParamType_ObjectID,	1	},
};

static ParamInfo kParams_TwoStrings[2] =
{
	{	"string",	kParamType_String,	0	},
	{	"string",	kParamType_String,	0	},
};

static ParamInfo kParams_OneObject[1] =
{
	{	"target item",	kParamType_TESObject,	0	},
};

 static ParamInfo kParams_OneOptionalObject[1] =
 {
 	{	"target item",	kParamType_TESObject,	1	},
 };
 
static ParamInfo kParams_OneString_OneOptionalObject[2] =
{
	{	"string",		kParamType_String,			0	},
	{	"item",			kParamType_AnyForm,	1	},
};

static ParamInfo kParams_OneObject_OneOptionalObject[2] =
{
	{	"target item",	kParamType_AnyForm,		0	},
	{	"object",		kParamType_AnyForm,		1	},
};

static ParamInfo kParams_OneInt_OneOptionalObject[2] =
{
	{	"int",	kParamType_Integer,			0	},
	{	"item",			kParamType_AnyForm,	1	},
};

static ParamInfo kParams_SetEquippedFloat[2] =
{
	{	"val", kParamType_Float, 0 },
	{	"slot", kParamType_Integer, 0 },
};

static ParamInfo kParams_FormListInteger[2] =
{
	{	"form list", kParamType_FormList,	0		},
	{	"index",	 kParamType_Integer,	0		}
};

static ParamInfo kParams_OneQuest[1] =
{
	{	"quest", kParamType_Quest, 0 }, 
};

static ParamInfo kParams_OneNPC[1] =
{
	{	"NPC",	kParamType_NPC,	1	},
};

static ParamInfo kParams_OneOptionalObjectRef[1] =
{
	{	"ref", kParamType_ObjectRef, 1},
};

static ParamInfo kParams_OneIntOneOptionalObjectRef[2] =
{
	{	"flag",		kParamType_Integer,	0	},
	{	"ref",		kParamType_ObjectRef,	1	},
};

static ParamInfo kParams_OneIndexOneOptionalObjectRef[2] =
{
	{	"index",		kParamType_Integer,	0	},
	{	"ref",		kParamType_ObjectRef,	1	},
};

static ParamInfo kParams_OnePackageOneIndexOneOptionalObjectRef[3] =
{
	{	"package",		kParamType_AnyForm,		0	},
	{	"index",		kParamType_Integer,		0	},
	{	"ref",			kParamType_ObjectRef,	1	},
};

static ParamInfo kParams_OneForm_OneOptionalObjectRef[2] =
{
	{	"form",	kParamType_AnyForm,		0	},
	{	"ref",	kParamType_ObjectRef,	1	},
};

static ParamInfo kParams_OneForm_OneInt[2] =
{
	{	"form",	kParamType_AnyForm,	0	},
	{	"int",	kParamType_Integer, 0	}, 
};

static ParamInfo kParams_OneForm[1] =
{
	{	"form",	kParamType_AnyForm,	0	},
};

static ParamInfo kParams_OneForm_OneFloat[2] =
{
	{	"form",		kParamType_AnyForm,	0	},
	{	"float",	kParamType_Float,	0	}, 
};

static ParamInfo kParams_OneOptionalForm[1] =
{
	{	"form",	kParamType_AnyForm,	1	},
};

static ParamInfo kParams_OneForm_OneOptionalForm[2] =
{
	{	"form",	kParamType_AnyForm,	0	},
	{	"form",	kParamType_AnyForm,	1	},
};

static ParamInfo kParams_EquipItem[3] =
{
	{	"item",			kParamType_ObjectID,	0	},
	{	"silent",		kParamType_Integer,		1	},
	{	"lockEquip",	kParamType_Integer,		1	},
};

static ParamInfo kParams_OneFaction[1] =
{
	{	"faction",	kParamType_Faction,	0	},
};

static ParamInfo kParams_OneActorBase[1] =
{
	{	"base actor",	kParamType_ActorBase,	0	},
};

static ParamInfo kParams_OneOptionalActorBase[1] =
{
	{	"base actor",	kParamType_ActorBase,	1	},
};

static ParamInfo kParams_OneIntOneOptionalActorBase[2] =
{
	{	"bool",			kParamType_Integer,		0	},
	{	"base actor",	kParamType_ActorBase,	1	},
};

static ParamInfo kParams_OneRace[1] =
{
	{	"race",	kParamType_Race,	0	},
};

static ParamInfo kParams_GenericForm[4] =
{
	{	"which",			kParamType_Integer,	0	},
	{	"containingForm",	kParamType_AnyForm,	0	},
	{	"form",				kParamType_AnyForm,	0	},
	{	"index",			kParamType_Integer,	0	},
};

static ParamInfo kParams_GenericDeleteForm[3] =
{
	{	"which",			kParamType_Integer,	0	},
	{	"containingForm",	kParamType_AnyForm,	0	},
	{	"index",			kParamType_Integer,	0	},
};

static ParamInfo kParams_GenericCheckForm[3] =
{
	{	"which",			kParamType_Integer,	0	},
	{	"containingForm",	kParamType_AnyForm,	0	},
	{	"form",				kParamType_AnyForm,	0	},
};

static ParamInfo kParams_OneIntOneForm[2] =
{
	{	"index",	kParamType_Integer, 0	}, 
	{	"form",		kParamType_AnyForm,	0	},
};

static ParamInfo kParams_OneIntOneOptionalForm[2] =
{
	{	"index",	kParamType_Integer, 0	}, 
	{	"form",		kParamType_AnyForm,	1	},
};

static ParamInfo kParams_OneStringOneOptionalForm[2] =
{
	{	"index",	kParamType_String,	0	}, 
	{	"form",		kParamType_AnyForm,	1	},
};

static ParamInfo kParams_OneSpellItem[1] =
{
	{	"spell", kParamType_SpellItem, 0 }, 
};

