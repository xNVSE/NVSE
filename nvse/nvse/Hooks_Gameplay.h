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

namespace DisablePlayerControlsAlt
{
	using ModID = UInt8;
	using flags_t = UInt16;
	extern std::map<ModID, flags_t> g_disabledFlagsPerMod;

	enum DisabledControlsFlags : flags_t
	{
		//== Vanilla flags
		// Order is different than the order which DisablePlayerControls receives its args
		kFlag_Movement = 1 << 0,
		kFlag_Looking = 1 << 1,
		kFlag_Pipboy = 1 << 2,
		kFlag_Fighting = 1 << 3,
		kFlag_POV = 1 << 4,
		kFlag_RolloverText = 1 << 5,
		kFlag_Sneaking = 1 << 6,
		kVanillaFlags = kFlag_Movement | kFlag_Looking | kFlag_Pipboy | kFlag_Fighting 
			| kFlag_POV | kFlag_RolloverText | kFlag_Sneaking,
		// Flags enabled by default calling DisablePlayerControls w/ no args
		kVanillaDefaultDisableFlags = kFlag_Movement | kFlag_Pipboy | kFlag_POV | kFlag_Fighting,
		
		//== New custom flags
		kFlag_Attacking = 1 << 7,
		kFlag_EnterVATS = 1 << 8,
		kFlag_Jumping = 1 << 9,
		kFlag_AimingOrBlocking = 1 << 10, // also disables zooming in.
		kFlag_Running = 1 << 11,
		// Added in v6.3.5
		kFlag_Sleep = 1 << 12,
		kFlag_Wait = 1 << 13,
		kFlag_FastTravel = 1 << 14,
		// Added in v6.3.6
		kFlag_Reload = 1 << 15,

		kNewFlags = kFlag_Attacking | kFlag_EnterVATS | kFlag_Jumping | kFlag_AimingOrBlocking
			| kFlag_Running | kFlag_Sleep | kFlag_Wait | kFlag_FastTravel | kFlag_Reload,

		kAllFlags = kVanillaFlags | kNewFlags
	};
	extern flags_t g_disabledControls;

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