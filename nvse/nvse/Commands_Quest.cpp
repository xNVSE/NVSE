#include "Commands_Quest.h"

#if RUNTIME
#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"

bool Cmd_GetQuestObjectiveCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESQuest* pQuest = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pQuest)) {
		UInt32 count = pQuest->lVarOrObjectives.Count();
		*result = count;
		if (IsConsoleMode())
			Console_Print("%s has %d objectives", GetFullName(pQuest), count);
	}

	return true;
}

bool Cmd_GetNthQuestObjective_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESQuest* pQuest = NULL;
	UInt32 nIndex = 0;
	if (ExtractArgs(EXTRACT_ARGS, &pQuest, &nIndex)) {
		if (!pQuest) return true;
		BGSQuestObjective* obj = reinterpret_cast<BGSQuestObjective*>(pQuest->lVarOrObjectives.GetNthItem(nIndex));
		if (obj) {
			*result = obj->objectiveId;
		}
	}
	return true;
}

bool Cmd_GetCurrentObjective_Execute(COMMAND_ARGS)
{

	PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
	tList<BGSQuestObjective> list = pPC->questObjectiveList;
	BGSQuestObjective* pCurObjective = (BGSQuestObjective*)list.GetFirstItem();
	if (pCurObjective) {
		*result = pCurObjective->objectiveId;
	}

	return true;
}

bool Cmd_SetCurrentQuest_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESQuest* pQuest = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pQuest)) {
		PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
		pPC->quest = pQuest;
	}
	return true;
}
#endif