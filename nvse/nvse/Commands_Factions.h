#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "GameForms.h"
#include "GameData.h"

#ifdef RUNTIME

extern bool Cmd_GetFactionRank_Eval(COMMAND_ARGS_EVAL);
extern bool Cmd_GetFactionRank_Execute(COMMAND_ARGS);
extern bool Cmd_ModFactionRank_Execute(COMMAND_ARGS);

#endif

DEFINE_CMD(GetBaseNumFactions, returns the number of factions to which an actor base form belongs, 0, kParams_OneOptionalActorBase);
DEFINE_CMD(GetBaseNthFaction, returns the nth faction to which an actor base form belongs, 0, kParams_OneIntOneOptionalActorBase);
DEFINE_CMD(GetBaseNthRank, returns the nth faction rank to which an actor base form belongs, 0, kParams_OneIntOneOptionalActorBase);

DEFINE_CMD(GetNumRanks, returns the number of rank of a faction, 0, kParams_OneFaction);
