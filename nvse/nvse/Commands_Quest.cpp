#include "Commands_Quest.h"

#if _RUNTIME
#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"

bool Cmd_GetQuestObjectiveCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESQuest* pQuest = NULL;
	if (ExtractArgs(EXTRACT_ARGS, &pQuest)) {
		tList<TESQuest::LocalVariableOrObjectivePtr> list = pQuest->lVarOrObjectives;
		UInt32 count = list.Count();
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

		tList<TESQuest::LocalVariableOrObjectivePtr> list = pQuest->lVarOrObjectives;
		BGSQuestObjective* pObjective = (BGSQuestObjective*)list.GetNthItem(nIndex);
		if (pObjective) {
			*result = pObjective->objectiveId;
		}
	}
	return true;
}

bool Cmd_GetCurrentObjective_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	PlayerCharacter* pPC = PlayerCharacter::GetSingleton();
	tList<BGSQuestObjective> list = pPC->questObjectiveList;
	BGSQuestObjective* pCurObjective = (BGSQuestObjective*)list.GetFirstItem();
	if (pCurObjective) {
		*refResult = pCurObjective->objectiveId;
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