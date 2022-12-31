#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

static ParamInfo kParams_QuestInteger[2] =
{
	{	"quest", kParamType_Quest,		0	},
	{	"index", kParamType_Integer,	0	}
};


DEFINE_CMD_ALT(GetQuestObjectiveCount, GetObjectiveCount, returns the number of objectives for the specified quest, 0, 1, kParams_OneQuest);
DEFINE_CMD_ALT(GetNthQuestObjective, GetNthObjective, returns the objectiveID of the Nth objective of the quest, 0, 2, kParams_QuestInteger);
DEFINE_CMD_ALT(GetCurrentObjective, GetCurObjID, returns the objectiveID of the current objective, 0, 0, NULL);
DEFINE_CMD_ALT(SetCurrentQuest, , sets the current quest to the specified quest, 0, 1, kParams_OneQuest);
