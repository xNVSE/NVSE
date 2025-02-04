#pragma once
#include <unordered_set>

#include "CommandTable.h"
#include "Utilities.h"
#include "GameObjects.h"


class Menu;
extern std::unordered_set<UInt8> g_myMods;

void Hook_Gameplay_Init(void);
void ToggleUIMessages(bool enableSpam);
void ToggleConsoleOutput(bool enable);
bool RunCommand_NS(COMMAND_ARGS, Cmd_Execute cmd);

extern DWORD g_mainThreadID;

// this returns a refID rather than a TESObjectREFR* as dropped items are non-persistent references
UInt32 GetPCLastDroppedItemRef();
TESForm* GetPCLastDroppedItem();		// returns the base object

void SetRetainExtraOwnership(bool bRetain);

namespace TogglePlayerControlsAlt
{
	using flags_t = UInt32;
	extern UnorderedMap<const char*, flags_t> g_disabledFlagsPerMod;

	extern flags_t g_disabledControls;
	enum CheckDisabledHow : UInt8;

	void __fastcall DisablePlayerControlsAlt(flags_t flagsToAddForMod, const char* modName);
	void __fastcall EnablePlayerControlsAlt(flags_t flagsToRemoveForMod, const char* modName);
	bool __cdecl GetPlayerControlsDisabledAlt(CheckDisabledHow disabledHow, flags_t flagsToCheck, const char* modName);
	flags_t __fastcall GetDisabledPlayerControls(CheckDisabledHow disabledHow, const char* modName);

	flags_t CondenseVanillaFlagArgs(UInt32 movementFlag, UInt32 pipboyFlag, UInt32 fightingFlag, UInt32 POVFlag,
		UInt32 lookingFlag, UInt32 rolloverTextFlag, UInt32 sneakingFlag);

	template <bool IsEnable>
	std::pair<bool, flags_t> ExtractArgsForEnableOrDisablePlayerControls(COMMAND_ARGS)
	{
		UInt32 movementFlag = 1, pipboyFlag = 1, fightingFlag = 1, POVFlag = 1,
			lookingFlag = IsEnable ? 1 : 0, rolloverTextFlag = IsEnable ? 1 : 0, sneakingFlag = IsEnable ? 1 : 0;

		if (!ExtractArgsEx(EXTRACT_ARGS_EX, &movementFlag, &pipboyFlag, &fightingFlag, &POVFlag,
			&lookingFlag, &rolloverTextFlag, &sneakingFlag))
		{
			return std::pair<bool, flags_t>(false, 0);
		}

		auto newFlagsForMod = CondenseVanillaFlagArgs(movementFlag, pipboyFlag, fightingFlag,
			POVFlag, lookingFlag, rolloverTextFlag, sneakingFlag);

		return std::pair<bool, flags_t>(true, newFlagsForMod);
	}

	void ApplyImmediateDisablingEffects(flags_t changedFlagsForMod);
	void ApplyImmediateEnablingEffects(flags_t changedFlagsForMod);

	void ResetOnLoad();
}