#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

DEFINE_CMD_ALT(ListGetCount, , "returns the count of items in the form list", 0, 1, kParams_FormList);

DEFINE_CMD_ALT(ListGetNthForm, , returns the nth form in the form list, 0, 2, kParams_FormListInteger);

static ParamInfo kParams_FormListForm[2] = 
{
	{	"form list", kParamType_FormList,	0		},
	{	"form",		kParamType_AnyForm,		0		},
};

DEFINE_CMD_ALT_COND(ListGetFormIndex, , returns the index for the specified form, 0, kParams_FormListForm);
DEFINE_CMD_ALT_COND(IsRefInList, , returns the index for the specified form or base Form or permanent base Form, 0, kParams_FormListForm);

static ParamInfo kParams_AddToFormList[4] = 
{
	{	"form list", kParamType_FormList,	0		},
	{	"form",		kParamType_AnyForm,		0		},
	{	"index",	 kParamType_Integer,	1		},
	{	"bCheckForDuplicates",	 kParamType_Integer,	1		}
};

DEFINE_CMD_ALT(ListAddForm, , adds the form to the list at the given index (or at the end if not provided), false, 4, kParams_AddToFormList);

static ParamInfo kParams_FormList_OptionalInt[2] = 
{
	{	"form list", kParamType_FormList,	0		},
	{	"index",	 kParamType_Integer,	1		}
};
static ParamInfo kParams_FormList_TwoOptionalInts[3] =
{
	{	"form list", kParamType_FormList,	0		},
	{	"index",	 kParamType_Integer,	1		},
	{	"bCheckForDuplicates",	 kParamType_Integer,	1		}
};
DEFINE_CMD_ALT(ListAddReference, ListAddRef, adds the calling reference at the given index (or at the end if not provided), true, 3, kParams_FormList_TwoOptionalInts);

DEFINE_CMD_ALT(ListRemoveNthForm, ListRemoveNth, removes the nth form from the list, 0, 2, kParams_FormList_OptionalInt); 
DEFINE_CMD_ALT(ListRemoveForm, , removes the specified from from the list., 0, 2, kParams_FormListForm);

static ParamInfo kParams_ReplaceNthForm[3] = 
{
	{	"form list",	kParamType_FormList,	0		},
	{	"replaceWith",	kParamType_AnyForm,		0		},
	{	"formIndex",	kParamType_Integer,		1		}
};

static ParamInfo kParams_ReplaceForm[3] = 
{
	{	"form list",	kParamType_FormList,	0		},
	{	"replaceWith",	kParamType_AnyForm,		0		},
	{	"form",			kParamType_AnyForm,		0		},
};

DEFINE_CMD_ALT(ListReplaceNthForm, ListReplaceNth, replaces the nth form of the list with the specified form, 0, 3, kParams_ReplaceNthForm); 
DEFINE_CMD_ALT(ListReplaceForm, , replaces the specified from with another., 0, 3, kParams_ReplaceForm);

DEFINE_CMD_ALT(ListClear, , removes all entries from the list, 0, 1, kParams_FormList);

static ParamInfo kParams_OneFormList_OneFunction[2] =
{
	{	"form list",	kParamType_FormList,	0	},
	{	"condition user defined function",	kParamType_AnyForm,	0	},
};

DEFINE_COMMAND(ForEachInList, "invokes a UDF that is called on each form in a formlist.", false, 2, kParams_OneFormList_OneFunction);