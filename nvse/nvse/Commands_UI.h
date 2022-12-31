#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

static ParamInfo kParams_SetUIStringEx[] =
{
	{	"component name",	kParamType_String,	0 },
	FORMAT_STRING_PARAMS,
};

DEFINE_COMMAND(GetUIFloat, returns the value of a float UI trait, 0, 1, kParams_OneString);
DEFINE_COMMAND(SetUIFloat, sets the value of a float UI trait, 0, 2, kParams_OneString_OneFloat);
DEFINE_COMMAND(SetUIString, sets the value of a string UI trait, 0, 2, kParams_TwoStrings);
DEFINE_COMMAND(SetUIStringEx, sets the value of a string UI trait to a formatted string, 0, 22, kParams_SetUIStringEx);
DEFINE_COMMAND(SortUIListBox, sorts the items in a UI list_box, 0, 2, kParams_TwoStrings);

DEFINE_COMMAND(GetUIFloatAlt, , 0, 1, kParams_OneString);
DEFINE_COMMAND(SetUIFloatAlt, , 0, 2, kParams_OneString_OneFloat);
DEFINE_COMMAND(SetUIStringAlt, , 0, 22, kParams_SetUIStringEx);
DEFINE_COMMAND(ModUIFloat, , 0, 2, kParams_OneString_OneFloat);

DEFINE_COMMAND(PrintActiveTile, prints name of highlighted UI component for debug purposes, 0, 0, NULL);

// VATS camera
DEFINE_COMMAND(EndVATScam, "Remove all targets and close VATS mode", false, 0, NULL)

// Show LevelUp menu
DEFINE_COMMAND(ShowLevelUpMenu, shows the LevelUp menu, 0, 0, NULL);
