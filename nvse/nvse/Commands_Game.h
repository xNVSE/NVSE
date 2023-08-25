#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

// windows sdk sucks
#undef MessageBoxEx

DEFINE_COMMAND(GetNumericGameSetting, returns the value of a numeric game setting, 0, 1, kParams_OneString);
DEFINE_COMMAND(SetNumericGameSetting, sets a numeric game setting, 0, 2, kParams_OneString_OneFloat);
DEFINE_COMMAND(GetNumericIniSetting, returns the value of an ini setting, 0, 1, kParams_OneString);
DEFINE_COMMAND(SetNumericIniSetting, sets an ini setting, 0, 2, kParams_OneString_OneFloat);
DEFINE_COMMAND(GetCrosshairRef, returns the reference under the crosshair, 0, 0, NULL);
DEFINE_COMMAND(GetGameLoaded, returns 1 after a game is loaded, 0, 0, NULL);
DEFINE_COMMAND(GetGameRestarted, returns 1 when the game is restarted, 0, 0, NULL);
DEFINE_COMMAND(IsModLoaded, returns the whether the specified mod is loaded, 0, 1, kParams_OneString);
DEFINE_COMMAND(GetModIndex, returns the index of the specified mod, 0, 1, kParams_OneString);
DEFINE_COMMAND(GetNumLoadedMods, returns the number of loaded mods, 0, 0, NULL);
DEFINE_COMMAND(GetSourceModIndex, returns the index of the mod associated with the form, 0, 1, kParams_OneOptionalForm);
DEFINE_COMMAND(GetLocalRefIndex, returns the index of the ref, 0, 1, kParams_OneOptionalForm);
DEFINE_COMMAND(BuildRef, builds a reference from a mod and ref index, 0, 2, kParams_TwoInts);
DEFINE_COMMAND(GetDebugSelection, returns the current selected object in the console, 0, 0, NULL);
DEFINE_COMMAND(MessageEx, prints a formatted message, 0, 21, kParams_FormatString);
DEFINE_COMMAND(MessageBoxEx, displays a formatted message box, 0, 21, kParams_FormatString);

DEFINE_COMMAND(GetGridsToLoad, returns the effective value of the uGridsToLoad ini setting, 0, 0, NULL);
DEFINE_CMD_ALT(OutputLocalMapPicturesOverride, OLMPOR, "identical to the OutputLocalMapPictures console command, but overrides the uGridsToLoad ini setting", 0, 0, NULL);
DEFINE_CMD_ALT(SetOutputLocalMapPicturesGrids, SetOLMPGrids, sets the value with which to override uGridsToLoad when generating local maps with OLMPOR, 0, 1, kParams_OneInt);

DEFINE_COMMAND(AddSpellNS, identical to AddSpell but without the UI message, 0, 1, kParams_OneSpellItem);

DEFINE_CMD_ALIAS(DisablePlayerControlsAlt, DPCAlt, "Per-mod version with added args", false, kParams_SevenOptionalInts);
DEFINE_CMD_ALIAS(EnablePlayerControlsAlt, EPCAlt, "Per-mod version with added args", false, kParams_SevenOptionalInts);
DEFINE_CMD_ALIAS(GetPlayerControlsDisabledAlt, GPCDAlt, "", false, kParams_EightOptionalInts);

// Ex versions simply extracts a bitfield instead of multiple args
DEFINE_CMD_ALIAS(DisablePlayerControlsAltEx, DPCEx, "Per-mod version with added args", false, kParams_OneOptionalInt);
DEFINE_CMD_ALIAS(EnablePlayerControlsAltEx, EPCEx, "Per-mod version with added args", false, kParams_OneOptionalInt);
DEFINE_CMD_ALIAS(GetPlayerControlsDisabledAltEx, GPCDEx, "", false, kParams_TwoOptionalInts);
