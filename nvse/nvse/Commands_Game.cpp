#include "Commands_Game.h"
#include "GameAPI.h"
#include "GameObjects.h"
#include "GameSettings.h"
#include "GameUI.h"
#include "GameData.h"
#include "Hooks_SaveLoad.h"

#include "containers.h"
#include "GameScript.h"
#include "SafeWrite.h"
#include "Hooks_Gameplay.h"

Cmd_Execute Cmd_AddSpell_Execute = (Cmd_Execute)0x005C1D40;

bool Cmd_GetNumericGameSetting_Execute(COMMAND_ARGS)
{
	char settingName[512];
	*result = -1;

	if (ExtractArgs(EXTRACT_ARGS, &settingName))
	{
		if (GetNumericGameSetting(settingName, result))
		{
			if (IsConsoleMode())
				Console_Print("GetNumericGameSetting >> %g", *result);
		}
		else if (IsConsoleMode())
			Console_Print("GetNumericGameSetting >> SETTING NOT FOUND");
	}

	return true;
}

bool Cmd_SetNumericGameSetting_Execute(COMMAND_ARGS)
{
	char settingName[512];
	float newVal = 0;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &settingName, &newVal))
	{
		Setting* setting;
		GameSettingCollection* gmsts = GameSettingCollection::GetSingleton();
		if (gmsts && gmsts->GetGameSetting(settingName, &setting))
		{
			if (setting->Set(newVal))
				*result = 1;
		}
		else if (IsConsoleMode())
			Console_Print("SetNumericGameSetting >> NOT FOUND");
	}

	return true;
}

bool Cmd_GetNumericIniSetting_Execute(COMMAND_ARGS)
{
	char settingName[512];
	*result = -1;

	if (ExtractArgs(EXTRACT_ARGS, &settingName))
	{
		if (GetNumericIniSetting(settingName, result))
		{
			if (IsConsoleMode())
				Console_Print("GetNumericIniSetting >> %g", *result);
		}
		else if (IsConsoleMode())
			Console_Print("GetNumericIniSetting >> SETTING NOT FOUND");
	}

	return true;
}

bool Cmd_SetNumericIniSetting_Execute(COMMAND_ARGS)
{
	char settingName[512];
	float newVal = 0;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &settingName, &newVal))
	{
		Setting* setting;
		if (GetIniSetting(settingName, &setting))
		{
			if (setting->Set(newVal))
				*result = 1;
		}
		else if (IsConsoleMode())
			Console_Print("SetNumericIniSetting >> NOT FOUND");
	}

	return true;
}

bool Cmd_GetCrosshairRef_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	InterfaceManager* intfc = InterfaceManager::GetSingleton();
	if (intfc && intfc->crosshairRef)
		*refResult = intfc->crosshairRef->refID;

	if (IsConsoleMode())
		Console_Print("GetCrosshairRef >> %08x", *refResult);

	return true;
}

UnorderedSet<UInt32> s_gameLoadedInformedScripts, s_gameRestartedInformedScripts;

bool Cmd_GetGameLoaded_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (scriptObj)
	{
		// have we returned 'true' to this script yet?
		if (s_gameLoadedInformedScripts.Insert(scriptObj->refID))
		{
			// no, return true and add to the set
			*result = 1;
		}
		if (IsConsoleMode())
			Console_Print("GetGameLoaded >> %.0f", *result);
	}
	
	return true;
}

bool Cmd_GetGameRestarted_Execute(COMMAND_ARGS)
{
	if (scriptObj && s_gameRestartedInformedScripts.Insert(scriptObj->refID))
		*result = 1;
	else *result = 0;

	return true;
}

bool Cmd_IsModLoaded_Execute(COMMAND_ARGS)
{
	char modName[512];
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &modName))
		return true;

	const ModInfo* pModInfo = DataHandler::Get()->LookupModByName(modName);
	if (pModInfo && pModInfo->IsLoaded()) {
		*result = 1;
	}
	if (IsConsoleMode())
	{
		if (*result)
			Console_Print("Mod Loaded");
		else
			Console_Print("Mod not loaded");
	}

	return true;
}

bool Cmd_GetModIndex_Execute(COMMAND_ARGS)
{
	char modName[512];
	if (!ExtractArgs(EXTRACT_ARGS, &modName))
		return true;

	UInt32 modIndex = DataHandler::Get()->GetModIndex(modName);
	*result = modIndex;
	if (IsConsoleMode())
		Console_Print("Mod Index: %02X", modIndex);

	return true;
}

bool Cmd_GetNumLoadedMods_Execute(COMMAND_ARGS)
{
	*result = DataHandler::Get()->GetActiveModCount();
	if (IsConsoleMode()) {
		Console_Print("Mods Loaded: %.0f", *result);
	}
	return true;
}

bool Cmd_GetSourceModIndex_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	*result = -1;

	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &form))
		return true;

	if (!form)
		form = thisObj;
	
	if (form)
	{
		*result = (UInt8)(form->refID >> 24);
	}

	return true;
}

bool Cmd_GetLocalRefIndex_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	*result = -1;

	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &form))
		return true;

	if (!form)
		form = thisObj;

	if (form)
	{
		*result = form->refID & 0x00FFFFFF;
	}

	return true;
}

bool Cmd_BuildRef_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	* refResult = (UInt32 *)result;

	UInt32	modIdx, refIdx;

	if(!ExtractArgsEx(EXTRACT_ARGS_EX, &modIdx, &refIdx))
		return true;

	*refResult = ((modIdx & 0xFF) << 24) | (refIdx & 0x00FFFFFF);

	return true;
}

bool Cmd_GetDebugSelection_Execute(COMMAND_ARGS)
{
	*result = 0;

	InterfaceManager	* interfaceManager = InterfaceManager::GetSingleton();
	if(interfaceManager && interfaceManager->debugSelection)
	{
		UInt32	* refResult = (UInt32 *)result;

		*refResult = interfaceManager->debugSelection->refID;
	}

	return true;
}

bool Cmd_MessageEx_Execute(COMMAND_ARGS)
{
	*result = 0;

	char buffer[kMaxMessageLength];

	if(ExtractFormatStringArgs(0, buffer, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, kCommandInfo_MessageEx.numParams))
	{
		*result = 1;
		QueueUIMessage(buffer, 0, NULL, NULL, 2.0f, 0);
	}

	return true;
}

bool Cmd_MessageBoxEx_Execute(COMMAND_ARGS)
{
	*result = 0;

	char buffer[kMaxMessageLength];

	if(!ExtractFormatStringArgs(0, buffer, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, kCommandInfo_MessageBoxEx.numParams))
		return true;

	//extract the buttons
	const char * b[10] = {nullptr};
	UInt32 btnIdx = 0;

	for(char* ch = buffer; *ch && btnIdx < 10; ch++)
	{
		if(*ch == GetSeparatorChar(scriptObj))
		{
			*ch = '\0';
			b[btnIdx++] = ch + 1;
		}
	}

	if(!btnIdx)				// supply default OK button
		b[0] = "Ok";

	if(thisObj && !(thisObj->flags & TESForm::kFormFlags_DontSaveForm))		// if not temporary object and not quest script
		*ShowMessageBox_pScriptRefID = thisObj->refID;
	else
		*ShowMessageBox_pScriptRefID = scriptObj->refID;

	*ShowMessageBox_button = 0xFF;	// overwrite any previously pressed button
	ShowMessageBox(buffer,
		0, 0, ShowMessageBox_Callback, 0, 0x17, 0, 0,
		b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], NULL);

	return true;
}

#if RUNTIME
static UInt32* kuGridsToLoadAddr = (UInt32*)0x011C63D0;
static UInt32 kuGridsSquareAddr = 0x005D45A5;
static Cmd_Execute Cmd_OutputLocalMapPictures_Execute = (Cmd_Execute)0x005D4580;
#else
#error
#endif

static SInt8 sGridsToLoad_Override = -1;

bool Cmd_SetOutputLocalMapPicturesGrids_Execute(COMMAND_ARGS)
{
	SInt32 grids = 0;
	*result = 0;

	// uGrids override must be <= original value and odd
	if (ExtractArgs(EXTRACT_ARGS, &grids) && grids > 0 && grids <= *kuGridsToLoadAddr && (grids & 1))
	{
		sGridsToLoad_Override = grids*grids;
		*result = 1;
	}

	return true;
}

bool Cmd_OutputLocalMapPicturesOverride_Execute(COMMAND_ARGS)
{
	UInt8 oldGrids = *(SInt8*)kuGridsSquareAddr;

	bool bDoHook = (sGridsToLoad_Override != -1);
	if (bDoHook)
	{
		// temporarily modify uGridToLoad
		//*kuGridsSquareAddr = sGridsToLoad_Override;
		SafeWrite8(kuGridsSquareAddr, sGridsToLoad_Override);

		// install hook
		// not needed WriteRelJump(kOLMPPatchAddr, (UInt32)&GridCellArray_GetGridEntry_Hook);
	}

	Cmd_OutputLocalMapPictures_Execute(PASS_COMMAND_ARGS);

	if (bDoHook)
	{
		// restore original uGridsToLoad
		//*kuGridsToLoadAddr = oldGrids;
		SafeWrite8(kuGridsSquareAddr, oldGrids);

		// uninstall hook
		// not needed WriteRelCall(kOLMPPatchAddr, kGridCellArray_GetGridEntry);
	}

	return true;
}

bool Cmd_GetGridsToLoad_Execute(COMMAND_ARGS)
{
	*result = (double)(*kuGridsToLoadAddr);
	return true;
}

bool Cmd_AddSpellNS_Execute(COMMAND_ARGS)
{
	return RunCommand_NS(PASS_COMMAND_ARGS, Cmd_AddSpell_Execute);
}

namespace DisablePlayerControlsAlt
{
	void Cmd_DisablePlayerControlsAlt_Call(flags_t flagsToAddForMod, ModID modId)
	{
		if (auto iter = g_disabledFlagsPerMod.find(modId);
			iter != g_disabledFlagsPerMod.end())
		{
			auto flagsModHad = iter->second;
			flags_t realFlagChanges = flagsToAddForMod & ~flagsModHad;
			ApplyImmediateDisablingEffects(realFlagChanges);

			// update disabled flags (might just be re-applying the same disabled flags)
			iter->second |= flagsToAddForMod;
		}
		else
		{
			ApplyImmediateDisablingEffects(flagsToAddForMod);
			g_disabledFlagsPerMod.emplace(modId, flagsToAddForMod);
		}
		g_disabledControls |= flagsToAddForMod;
	}
}

bool Cmd_DisablePlayerControlsAlt_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	auto [success, flagsToAddForMod] = ExtractArgsForEnableOrDisablePlayerControls<false>(PASS_COMMAND_ARGS);
	if (!success || !flagsToAddForMod)
		return true;

	Cmd_DisablePlayerControlsAlt_Call(flagsToAddForMod, scriptObj->GetOverridingModIdx());
	return true;
}

// Ex version simply extracts a bitfield instead of multiple args
bool Cmd_DisablePlayerControlsAltEx_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	UInt32 flagsToAddForMod = DisablePlayerControlsAlt::kVanillaDefaultDisableFlags;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &flagsToAddForMod))
		return true;

	Cmd_DisablePlayerControlsAlt_Call(flagsToAddForMod, scriptObj->GetOverridingModIdx());
	return true;
}

namespace DisablePlayerControlsAlt
{
	void Cmd_EnablePlayerControlsAlt_Call(flags_t flagsToRemoveForMod, ModID modId)
	{
		if (auto foundIter = g_disabledFlagsPerMod.find(modId);
			foundIter != g_disabledFlagsPerMod.end())
		{
			auto flagsModHad = foundIter->second;
			ApplyImmediateEnablingEffects(flagsModHad);

			// update disabled flags
			foundIter->second &= ~flagsToRemoveForMod;
			if (!foundIter->second)
				g_disabledFlagsPerMod.erase(foundIter);

			g_disabledControls = 0;

			// Re-form g_disabledControls based on all the chached per-mod disabled flags 
			for (auto const& loopIter : g_disabledFlagsPerMod)
			{
				g_disabledControls |= loopIter.second;
			}
		}
	}
}

bool Cmd_EnablePlayerControlsAlt_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	auto [success, flagsToRemoveForMod] = ExtractArgsForEnableOrDisablePlayerControls<true>(PASS_COMMAND_ARGS);
	if (!success || !flagsToRemoveForMod)
		return true;

	Cmd_EnablePlayerControlsAlt_Call(flagsToRemoveForMod, scriptObj->GetOverridingModIdx());
	return true;
}

// Ex version simply extracts a bitfield instead of multiple args
bool Cmd_EnablePlayerControlsAltEx_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	UInt32 flagsToRemoveForMod = DisablePlayerControlsAlt::kAllFlags;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &flagsToRemoveForMod))
		return true;

	Cmd_EnablePlayerControlsAlt_Call(flagsToRemoveForMod, scriptObj->GetOverridingModIdx());
	return true;
}

namespace DisablePlayerControlsAlt
{
	void Cmd_GetPlayerControlsDisabledAlt_Call(UInt32 disabledHow, flags_t flagsToCheck, ModID modId, double* result)
	{
		// Check if any of the specified player controls are currently disabled by calling mod
		if (disabledHow == 0)
		{
			if (auto foundIter = g_disabledFlagsPerMod.find(modId);
				foundIter != g_disabledFlagsPerMod.end())
			{
				auto flagsModHas = foundIter->second;
				*result = (flagsModHas & flagsToCheck) != 0;
			}
		}
		else
		{
			*result = (g_disabledControls & flagsToCheck) != 0;
			if (disabledHow == 2)
				*result = *result || (PlayerCharacter::GetSingleton()->disabledControlFlags & flagsToCheck) != 0;
		}
	}
}

bool Cmd_GetPlayerControlsDisabledAlt_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	*result = 0;

	// If 0, check if it's disabled by calling mod
	// If 1, ... by any mod (DisablePlayerControls*ALT* only)
	// If 2, ... by any mod OR disabled by vanilla DisablePlayerControls
	UInt32 disabledHow = 0;

	UInt32 movementFlag = 1, pipboyFlag = 1, fightingFlag = 1, POVFlag = 1,
		lookingFlag = 1, rolloverTextFlag = 1, sneakingFlag = 1;

	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &disabledHow, &movementFlag, &pipboyFlag, &fightingFlag, &POVFlag,
		&lookingFlag, &rolloverTextFlag, &sneakingFlag))
	{
		return true;
	}

	auto flagsToCheck = CondenseVanillaFlagArgs(movementFlag, pipboyFlag, fightingFlag, POVFlag,
		lookingFlag, rolloverTextFlag, sneakingFlag);

	ModID modId = scriptObj->GetOverridingModIdx();
	Cmd_GetPlayerControlsDisabledAlt_Call(disabledHow, flagsToCheck, modId, result);
	return true;
}

// Ex version simply extracts a bitfield instead of multiple args
bool Cmd_GetPlayerControlsDisabledAltEx_Execute(COMMAND_ARGS)
{
	using namespace DisablePlayerControlsAlt;

	UInt32 disabledHow = 0;
	UInt32 flagsToCheck = DisablePlayerControlsAlt::kAllFlags;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &disabledHow, &flagsToCheck))
		return true;

	ModID modId = scriptObj->GetOverridingModIdx();
	Cmd_GetPlayerControlsDisabledAlt_Call(disabledHow, flagsToCheck, modId, result);
	return true;
}