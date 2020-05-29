#include "Commands_Factions.h"
#include "Commands_Misc.h"

#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameBSExtraData.h"
#include "GameExtraData.h"

bool Cmd_GetFactionRank_Eval(COMMAND_ARGS_EVAL)
{
	SInt8 foundRank = -1;
	TESFaction* faction = (TESFaction*)arg1;
	if (faction)
	{
		bool bFoundRank = false;
		foundRank = GetExtraFactionRank(thisObj->extraDataList, faction);
		bFoundRank = ( -1 != foundRank );
		if (!bFoundRank) {
			TESActorBaseData* actorBase = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBaseData);
			if (actorBase)
			{
				foundRank = actorBase->GetFactionRank(faction);
				bFoundRank = true;
			}
		}
		if (bFoundRank && IsConsoleMode())
			Console_Print("GetFactionRank >> %.2f", *result);
	}

	*result = foundRank;
	return true;
}

bool Cmd_GetFactionRank_Execute(COMMAND_ARGS)
{
	*result = -1;
	TESFaction* faction;
	if (ExtractArgs(EXTRACT_ARGS, &faction))
	{
		return Cmd_GetFactionRank_Eval(thisObj, faction, 0, result);
	}
	return true;
}

bool Cmd_ModFactionRank_Execute(COMMAND_ARGS)
{
	TESFaction* faction;
	SInt32 rank;

	*result = -1;
	if (ExtractArgs(EXTRACT_ARGS, &faction, &rank)) {
		SInt8 foundRank = -1;
		if (faction)
		{
			bool bFoundRank = false;
			foundRank = GetExtraFactionRank(thisObj->extraDataList, faction);
			bFoundRank = ( -1 != foundRank );
			if (!bFoundRank) {
				TESActorBaseData* actorBase = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBaseData);
				if (actorBase)
				{
					foundRank = actorBase->GetFactionRank(faction);
					bFoundRank = true;
				}
			}
			if (bFoundRank) {
				rank += foundRank;
				if (rank<0)
					rank = 0;
				else if (rank>127)
					rank = 127;
				SetExtraFactionRank(thisObj->extraDataList, faction, rank);
			} else
				rank = foundRank;
			if (bFoundRank && IsConsoleMode())
				Console_Print("ModFactionRank >> %.2f", rank);
		}

		*result = rank;
	}
	return true;
}

bool Cmd_GetBaseNumFactions_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		*result = actorBase->baseData.factionList.Count();
	}

	return true;
}

bool Cmd_GetBaseNthFaction_Execute(COMMAND_ARGS)
{
	UInt32 factionIdx = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	TESActorBase* actorBase = ExtractSetActorBase(PASS_COMMAND_ARGS, &factionIdx);
	if (actorBase)
	{
		TESActorBaseData::FactionListData * data = actorBase->baseData.factionList.GetNthItem(factionIdx);
		if (data && data->faction)
			*refResult = data->faction->refID;
	}

	return true;
}

bool Cmd_GetBaseNthRank_Execute(COMMAND_ARGS)
{
	UInt32 factionIdx = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	TESActorBase* actorBase = ExtractSetActorBase(PASS_COMMAND_ARGS, &factionIdx);
	if (actorBase)
	{
		TESActorBaseData::FactionListData * data = actorBase->baseData.factionList.GetNthItem(factionIdx);
		if (data && data->faction)
			*result = data->rank;
	}

	return true;
}

bool Cmd_GetNumRanks_Execute(COMMAND_ARGS)
{
	TESFaction* fact = NULL;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &fact))
		*result = fact->ranks.Count();

	return true;
}

