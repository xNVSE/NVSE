#include "Commands_Misc.h"

#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameBSExtraData.h"
#include "GameExtraData.h"
#include "GameProcess.h"
#include "StringVar.h"
#include "ArrayVar.h"

bool Cmd_GetAgeClass_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	Actor* pActor = NULL;
	TESNPC * pNPC = NULL;
	TESRace * pRace = NULL;
	char * name;
	char upperName[256];

	//DEBUG_MESSAGE("\t\tGAC Arg1:%x\n", arg1);

	if (arg1)
		pActor = DYNAMIC_CAST(arg1, TESObjectREFR, Actor);
	//DEBUG_MESSAGE("\t\tGAC Actor:%x\n", pActor);
	if (!pActor)
		if (thisObj) {
			//DEBUG_MESSAGE("\t\tGAC thisObj:%x\n", thisObj);
			pActor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);
		}
	//DEBUG_MESSAGE("\t\tGAC Actor:%x\n", pActor);
	if (pActor)
		pNPC = DYNAMIC_CAST(pActor->baseForm, TESForm, TESNPC);
	//DEBUG_MESSAGE("\t\tGAC Actor:%x NPC:%x\n", pActor, pNPC);
	if (pNPC)
		pRace = pNPC->race.race;
	//DEBUG_MESSAGE("\t\tGAC Actor:%x Race:%x\n", pActor, pRace);
	if (pRace) {
		if (pRace->raceFlags & TESRace::kFlag_Child)
				*result = 0;
		else {
			name = pRace->fullName.name.m_data;
			//DEBUG_MESSAGE("\t\tGAC Actor:%x Race:%x Name:%x\n", pActor, pRace, name);
			if (name) {
				for (int i=0; i<=strlen(name); i++)
					upperName[i] = game_toupper(pRace->fullName.name.m_data[i]);
				//DEBUG_MESSAGE("\t\tGAC Actor:%x Race:%x Name:%s\n", pActor, pRace, upperName);
				if (strstr(upperName, "CHILD"))
					*result = 0;
				else if (strstr(upperName, "OLDAGED"))
					*result = 3;
				else if (strstr(upperName, "OLD"))
					*result = 2;
				else
					*result = 1;
			}
			else
				*result = 1;
			//DEBUG_MESSAGE("\t\tGAC Actor:%x Race:%x Name:%s Result:%d\n", pActor, pRace, upperName, *result);
		}
	}
	else
		*result = -1;
	if (IsConsoleMode())
		Console_Print("GetAgeClass: %.0f", *result);

	//DEBUG_MESSAGE("\t\tGAC Actor:%x Result:%f\n", pActor, *result);
	return true;
}

bool Cmd_GetAgeClass_Execute(COMMAND_ARGS)
{
	void * arg1 = NULL;
	ExtractArgs(EXTRACT_ARGS, &arg1);
	return Cmd_GetAgeClass_Eval(thisObj, arg1, 0, result);
}

bool Cmd_GetRespawn_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	Actor* pActor = NULL;
	TESForm * pForm = NULL;
	TESActorBaseData* pActorBaseData = NULL;

	if (arg1)
		pActor = DYNAMIC_CAST(arg1, TESForm, Actor);

	if (!pActor && thisObj)
		pActor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);

	if (pActor)
		pForm = pActor->baseForm;

	if (pForm)
		pActorBaseData =  DYNAMIC_CAST(pForm, TESForm, TESActorBaseData);

	if (pActorBaseData)
		*result = pActorBaseData->flags & TESActorBaseData::kFlags_Respawn ? 1.0 : 0.0;

	if (IsConsoleMode())
		Console_Print("GetRespawn: %.0f", *result);

	return true;
}

bool Cmd_GetRespawn_Execute(COMMAND_ARGS)
{
	void * arg1 = NULL;
	ExtractArgs(EXTRACT_ARGS, &arg1);
	return Cmd_GetRespawn_Eval(thisObj, arg1, 0, result);
}

bool Cmd_SetRespawn_Execute(COMMAND_ARGS)
{
	*result = 0;
	double flag = 0;
	Actor* pActor = NULL;
	TESForm * pForm = NULL;
	TESActorBaseData* pActorBaseData = NULL;

	if (!ExtractArgs(EXTRACT_ARGS, &flag, &pActor))
		return true;

	if (!pActor && thisObj) 
		pActor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);

	if (pActor)
		pForm = pActor->baseForm;

	if (pForm)
		pActorBaseData =  DYNAMIC_CAST(pForm, TESForm, TESActorBaseData);

	if (pActorBaseData) 
		flag ? pActorBaseData->flags |= TESActorBaseData::kFlags_Respawn : pActorBaseData->flags &= ~TESActorBaseData::kFlags_Respawn;

	return true;
}

bool Cmd_GetPermanent_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESObjectREFR* pObj = NULL;
	TESForm * pForm = NULL;

	if (arg1)
		pObj = DYNAMIC_CAST(arg1, TESForm, TESObjectREFR);

	if (!pObj && thisObj)
		pObj = thisObj;

	if (pObj)
		pForm = pObj->baseForm;

	if (pForm)
		*result = pForm->flags & TESForm::kFormFlags_QuestItem ? 1.0 : 0.0;

	if (IsConsoleMode())
		Console_Print("GetPermanent: %.0f", *result);

	return true;
}

bool Cmd_GetPermanent_Execute(COMMAND_ARGS)
{
	void * arg1 = NULL;
	ExtractArgs(EXTRACT_ARGS, &arg1);
	return Cmd_GetPermanent_Eval(thisObj, arg1, 0, result);
}

bool Cmd_SetPermanent_Execute(COMMAND_ARGS)
{
	*result = 0;
	double flag = 0;
	TESObjectREFR* pObj = NULL;

	if (!ExtractArgs(EXTRACT_ARGS, &flag, &pObj))
		return true;

	if (!pObj && thisObj) 
		pObj = thisObj;

	if (pObj)
	{
		flag ? pObj->flags |= TESForm::kFormFlags_QuestItem : pObj->flags &= ~TESForm::kFormFlags_QuestItem;
		pObj->MarkAsModified(TESForm::kModified_Flags);	// Makes the change permanent
	}
	return true;
}

bool Cmd_GetRace_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	TESNPC* npc = NULL;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm))
		return true;

	if (pForm)
		pForm = pForm->TryGetREFRParent();
	if (!pForm && thisObj)
		pForm = thisObj->baseForm;
	if (pForm)
		npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);

	if (npc && npc->race.race)
		*refResult = npc->race.race->refID;

	return true;
}

bool Cmd_GetRaceName_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	TESNPC* npc = NULL;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm))
		return true;

	if (pForm)
		pForm = pForm->TryGetREFRParent();
	if (!pForm && thisObj)
		pForm = thisObj->baseForm;
	if (pForm)
		npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);

	if (npc && npc->race.race)
		AssignToStringVar(PASS_COMMAND_ARGS, npc->race.race->fullName.name.CStr());
	return true;
}

bool Cmd_GetClass_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	TESNPC* npc = NULL;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm))
		return true;

	if (pForm)
		pForm = pForm->TryGetREFRParent();
	if (!pForm && thisObj)
		pForm = thisObj->baseForm;
	if (pForm)
		npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);

	if (npc && npc->classID)
		*refResult = npc->classID->refID;

	return true;
}

bool Cmd_GetNameOfClass_Execute(COMMAND_ARGS)
{
	TESForm* pForm = NULL;
	TESNPC* npc = NULL;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &pForm))
		return true;

	if (pForm)
		pForm = pForm->TryGetREFRParent();
	if (!pForm && thisObj)
		pForm = thisObj->baseForm;
	if (pForm)
		npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);

	if (npc && npc->classID)
		AssignToStringVar(PASS_COMMAND_ARGS, npc->classID->fullName.name.CStr());
	return true;
}

bool Cmd_GetPerkRank(COMMAND_ARGS_EVAL, bool alt)
{
	*result = -1;
	Actor* pActor = NULL;
	BGSPerk * pPerk = NULL;

	if (arg1)
		pPerk = DYNAMIC_CAST(arg1, TESForm, BGSPerk);
	if (!pPerk)
		return true;

	if (arg2)
		pActor = DYNAMIC_CAST(arg2, TESForm, Actor);

	if (!pActor && thisObj)
		pActor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);

	if (pActor)
		*result = pActor->GetPerkRank(pPerk, alt);

	if (IsConsoleMode())
		if (alt)
			Console_Print("GetAltPerkRank: %.0f", *result);
		else
			Console_Print("GetPerkRank: %.0f", *result);

	return true;
}

bool Cmd_GetPerkRank_Eval(COMMAND_ARGS_EVAL)
{
	return Cmd_GetPerkRank(PASS_CMD_ARGS_EVAL, false);
}

bool Cmd_GetPerkRank_Execute(COMMAND_ARGS)
{
	BGSPerk	* pPerk = NULL;
	Actor	* pActor = NULL;

	if (ExtractArgs(EXTRACT_ARGS, &pPerk, &pActor))
		return Cmd_GetPerkRank_Eval(thisObj, pPerk, pActor, result);
	return true;
}

bool Cmd_GetAltPerkRank_Eval(COMMAND_ARGS_EVAL)
{
	return Cmd_GetPerkRank(PASS_CMD_ARGS_EVAL, true);
}

bool Cmd_GetAltPerkRank_Execute(COMMAND_ARGS)
{
	BGSPerk	* pPerk = NULL;
	Actor	* pActor = NULL;

	if (ExtractArgs(EXTRACT_ARGS, &pPerk, &pActor))
		return Cmd_GetAltPerkRank_Eval(thisObj, pPerk, pActor, result);
	return true;
}

TESNPC* ConvertNPC(TESObjectREFR* thisObj, TESForm* form)
{
	TESNPC* npc = NULL;

	if (!form)
		if (thisObj)
			form = thisObj->baseForm;

	if (form)
		form = form->TryGetREFRParent();

	if (form)
		npc = DYNAMIC_CAST(form, TESForm, TESNPC);

	return npc;
}

TESNPC* ExtractNPC(COMMAND_ARGS)
{
	TESNPC* npc = NULL;
	TESForm* actorForm = NULL;

	ExtractArgs(EXTRACT_ARGS, &actorForm);
	return ConvertNPC(thisObj, actorForm);
}

Actor* ConvertActor(TESObjectREFR* thisObj, TESForm* actorForm)
{
	Actor* actor = NULL;

	if (!actorForm)
		if (thisObj)
			actorForm = thisObj;

	if (actorForm)
		actor = DYNAMIC_CAST(actorForm, TESForm, Actor);

	return actor;
}

Actor* ExtractActor(COMMAND_ARGS)
{
	TESForm* actorForm = NULL;

	ExtractArgs(EXTRACT_ARGS, &actorForm);
	return ConvertActor(thisObj, actorForm);
}

TESActorBase* ConvertActorBase(TESObjectREFR* thisObj, TESForm* actorForm)
{
	TESActorBase* actorBase = NULL;

	if (!actorForm)
		if (thisObj)
			actorForm = thisObj->baseForm;

	if (actorForm)
		actorForm = actorForm->TryGetREFRParent();

	if (actorForm)
		actorBase = DYNAMIC_CAST(actorForm, TESForm, TESActorBase);

	return actorBase;
}

TESActorBase* ExtractActorBase(COMMAND_ARGS)
{
	TESForm* actorForm = NULL;

	ExtractArgs(EXTRACT_ARGS, &actorForm);
	return ConvertActorBase(thisObj, actorForm);
}

TESActorBase* ExtractSetActorBase(COMMAND_ARGS, UInt32* bMod)
{
	TESForm* actorForm = NULL;
	*bMod = 0;

	ExtractArgs(EXTRACT_ARGS, bMod, &actorForm);
	return ConvertActorBase(thisObj, actorForm);
}

TESLevCharacter* ConvertLevCharacter(TESObjectREFR* thisObj, TESForm* form)
{
	TESLevCharacter* lev = NULL;

	if (!form)
		if (thisObj)
			form = thisObj->baseForm;

	if (form)
		form = form->TryGetREFRParent();

	if (form)
		lev = DYNAMIC_CAST(form, TESForm, TESLevCharacter);

	return lev;
}

TESLevCharacter* ExtractLevCharacter(COMMAND_ARGS)
{
	TESLevCharacter* lev = NULL;
	TESForm* form = NULL;

	ExtractArgs(EXTRACT_ARGS, &form);
	return ConvertLevCharacter(thisObj, form);
}

TESLevCreature* ConvertLevCreature(TESObjectREFR* thisObj, TESForm* form)
{
	TESLevCreature* lev = NULL;

	if (!form)
		if (thisObj)
			form = thisObj->baseForm;

	if (form)
		form = form->TryGetREFRParent();

	if (form)
		lev = DYNAMIC_CAST(form, TESForm, TESLevCreature);

	return lev;
}

TESLevCreature* ExtractLevCreature(COMMAND_ARGS)
{
	TESLevCreature* lev = NULL;
	TESForm* form = NULL;

	ExtractArgs(EXTRACT_ARGS, &form);
	return ConvertLevCreature(thisObj, form);
}

// all list of forms to array goes here:

bool Cmd_GetRaceHairs_Execute(COMMAND_ARGS)
{
	TESRace* race = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (ExtractArgs(EXTRACT_ARGS, &race) && race)
	{
		double idx = 0;
		for (tList<TESHair>::Iterator iter = race->hairs.Begin(); !iter.End(); ++iter)
		{
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refID);
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetRaceEyes_Execute(COMMAND_ARGS)
{
	TESRace* race = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (ExtractArgs(EXTRACT_ARGS, &race) && race)
	{
		double idx = 0;
		for (tList<TESEyes>::Iterator iter = race->eyes.Begin(); !iter.End(); ++iter) {
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refID);
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetBaseSpellListSpells_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor base form
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		for (tList<SpellItem>::Iterator iter = actorBase->spellList.spellList.Begin(); !iter.End(); ++iter)
		{
			if (iter.Get())
			{
				arr->SetElementFormID(arrIndex, iter.Get()->refID);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetBaseSpellListLevSpells_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor base form
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		for (tList<SpellItem>::Iterator iter = actorBase->spellList.leveledSpellList.Begin(); !iter.End(); ++iter)
		{
			if (iter.Get())
			{
				arr->SetElementFormID(arrIndex, iter.Get()->refID);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetBaseFactions_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor base form
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		for (tList<TESActorBaseData::FactionListData>::Iterator iter = actorBase->baseData.factionList.Begin(); !iter.End(); ++iter)
		{
			TESActorBaseData::FactionListData * data = iter.Get();
			if (data && data->faction)
			{
				arr->SetElementFormID(arrIndex, data->faction->refID);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetBaseRanks_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor base form
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		for (tList<TESActorBaseData::FactionListData>::Iterator iter = actorBase->baseData.factionList.Begin(); !iter.End(); ++iter)
		{
			TESActorBaseData::FactionListData * data = iter.Get();
			if (data && data->faction)
			{
				arr->SetElementNumber(arrIndex, data->rank);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetBasePackages_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor base form
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	TESActorBase* actorBase = ExtractActorBase(PASS_COMMAND_ARGS);
	if (actorBase)
	{
		for (TESAIForm::PackageList::Iterator iter = actorBase->ai.packageList.Begin(); !iter.End(); ++iter)
		{
			TESPackage * data = iter.Get();
			if (data)
			{
				arr->SetElementFormID(arrIndex, data->refID);
				arrIndex += 1;
			}
		}
	}

	return true;
}

// Skipping Topic/Info for now

bool Cmd_GetFactionRankNames_Execute(COMMAND_ARGS)
{
	TESFaction* form = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (ExtractArgs(EXTRACT_ARGS, &form) && form)
	{
		double idx = 0;
		for (tList<TESFaction::Rank>::Iterator iter = form->ranks.Begin(); !iter.End(); ++iter)
		{
			if (iter.Get())
			{
				arr->SetElementString(idx, iter.Get()->name.CStr());
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetFactionRankFemaleNames_Execute(COMMAND_ARGS)
{
	TESFaction* form = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (ExtractArgs(EXTRACT_ARGS, &form) && form)
	{
		double idx = 0;
		for (tList<TESFaction::Rank>::Iterator iter = form->ranks.Begin(); !iter.End(); ++iter)
		{
			if (iter.Get())
			{
				arr->SetElementString(idx, iter.Get()->femaleName.CStr());
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetHeadParts_Execute(COMMAND_ARGS)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	TESNPC* form = ExtractNPC(PASS_COMMAND_ARGS);
	if (form) {
		double idx = 0;
		for (tList<BGSHeadPart>::Iterator iter = form->headPart.Begin(); !iter.End(); ++iter) {
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refID);
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetLevCreatureRefs_Execute(COMMAND_ARGS)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	TESLevCreature* form = ExtractLevCreature(PASS_COMMAND_ARGS);
	if (form) {
		double idx = 0;
		for (tList<TESLeveledList::BaseData>::Iterator iter = form->list.datas.Begin(); !iter.End(); ++iter) {
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refr->refID);
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetLevCharacterRefs_Execute(COMMAND_ARGS)
{
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	TESLevCharacter* form = ExtractLevCharacter(PASS_COMMAND_ARGS);
	if (form) {
		double idx = 0;
		for (tList<TESLeveledList::BaseData>::Iterator iter = form->list.datas.Begin(); !iter.End(); ++iter) {
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refr->refID);
				idx += 1;
			}
		}
	}

	return true;
}

// Skipping TESCell::objectList

bool Cmd_GetListForms_Execute(COMMAND_ARGS)
{
	BGSListForm* form = NULL;
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	if (ExtractArgs(EXTRACT_ARGS, &form) && form) {
		double idx = 0;
		for (tList<TESForm>::Iterator iter = form->list.Begin(); !iter.End(); ++iter) {
			if (iter.Get()) {
				arr->SetElementFormID(idx, iter.Get()->refID);
				idx += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetActiveFactions_Execute(COMMAND_ARGS)
{
	// returns an array of factions for the specified actor
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	Actor* actor = ExtractActor(PASS_COMMAND_ARGS);
	TESActorBase* actorBase = ConvertActorBase(thisObj, actor);
	if (actorBase && actor)
	{
		ExtraFactionChanges::FactionListEntry* entry = GetExtraFactionList(actor->extraDataList);
		for (tList<TESActorBaseData::FactionListData>::Iterator iter = actorBase->baseData.factionList.Begin(); !iter.End(); ++iter)
		{
			TESActorBaseData::FactionListData * data = iter.Get();
			if (data && data->faction)
			{
				bool changed = false;
				for (ExtraFactionChanges::FactionListEntry::Iterator iter = entry->Begin(); !iter.End(); ++iter)
				{
					ExtraFactionChanges::FactionListData* xdata = iter.Get();
					if (xdata && xdata->faction && (data->faction->refID == xdata->faction->refID))
						changed = true;
				}

				if (!changed)
				{
					arr->SetElementFormID(arrIndex, data->faction->refID);
					arrIndex += 1;
				}
			}
		}

		for (ExtraFactionChanges::FactionListEntry::Iterator iter = entry->Begin(); !iter.End(); ++iter)
		{
			ExtraFactionChanges::FactionListData* data = iter.Get();
			if (data && data->faction && (0 <= data->rank))	// negative rank means removed from faction
			{
				arr->SetElementFormID(arrIndex, data->faction->refID);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool Cmd_GetActiveRanks_Execute(COMMAND_ARGS)
{
	// returns an array of factions ranks for the specified actor
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	Actor* actor = ExtractActor(PASS_COMMAND_ARGS);
	TESActorBase* actorBase = ConvertActorBase(thisObj, actor);
	if (actorBase && actor)
	{
		ExtraFactionChanges::FactionListEntry* entry = GetExtraFactionList(actor->extraDataList);
		for (tList<TESActorBaseData::FactionListData>::Iterator iter = actorBase->baseData.factionList.Begin(); !iter.End(); ++iter)
		{
			TESActorBaseData::FactionListData * data = iter.Get();
			if (data && data->faction)
			{
				bool changed = false;
				for (ExtraFactionChanges::FactionListEntry::Iterator iter = entry->Begin(); !iter.End(); ++iter)
				{
					ExtraFactionChanges::FactionListData* xdata = iter.Get();
					if (xdata && xdata->faction && (data->faction->refID == xdata->faction->refID))
						changed = true;
				}

				if (!changed)
				{
					arr->SetElementNumber(arrIndex, data->rank);
					arrIndex += 1;
				}
			}
		}

		for (ExtraFactionChanges::FactionListEntry::Iterator iter = entry->Begin(); !iter.End(); ++iter)
		{
			ExtraFactionChanges::FactionListData* data = iter.Get();
			if (data && data->faction && (0 <= data->rank))	// negative rank means removed from faction
			{
				arr->SetElementNumber(arrIndex, data->rank);
				arrIndex += 1;
			}
		}
	}

	return true;
}

bool GenericForm_Execute(COMMAND_ARGS, UInt32 action)
{
	*result = eListInvalid;
	UInt32 whichType = 0;
	TESForm* pGenericListOwner = NULL;
	TESForm* pForm = NULL;
	UInt32 n = eListEnd;
	tList<TESForm>* pListForm = NULL;
	bool bExtract;
	bool noForm = (action == eActionListForm_DelAt) || (action == eActionListForm_GetAt);
	bool noindex = (action == 	eActionListForm_Check);
	UInt32 index = eListInvalid;
	UInt32 lCount;

	TESRace			* race	= NULL;
	TESHair			* hair	= NULL;
	TESEyes			* eyes	= NULL;
	TESActorBase	* base	= NULL;
	TESPackage		* pack	= NULL;
	SpellItem		* spell	= NULL;
	TESLevCreature	* lCrea	= NULL;
	TESLevCharacter	* lChar	= NULL;
	TESObjectREFR	* refr	= NULL;
	BGSListForm		* list	= NULL;
	TESNPC			* npc_	= NULL;
	TESCreature		* crea	= NULL;
	BGSHeadPart		* part	= NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (noForm)
		bExtract = ExtractArgsEx(EXTRACT_ARGS_EX, &whichType, &pGenericListOwner, &n);
	else if (noindex)
		bExtract = ExtractArgsEx(EXTRACT_ARGS_EX, &whichType, &pGenericListOwner, &pForm);
	else
		bExtract = ExtractArgsEx(EXTRACT_ARGS_EX, &whichType, &pGenericListOwner, &pForm, &n);

	if (!bExtract)
		return false;

	switch (whichType)
	{
	case eWhichListForm_RaceHair:
		race = DYNAMIC_CAST(pGenericListOwner, TESForm, TESRace);
		if (!noForm)
			hair = DYNAMIC_CAST(pForm, TESForm, TESHair);
		if (race && (hair || noForm))
			pListForm = (tList<TESForm>*)&(race->hairs);
		break;
	case eWhichListForm_RaceEyes:
		race = DYNAMIC_CAST(pGenericListOwner, TESForm, TESRace);
		if (!noForm)
			eyes = DYNAMIC_CAST(pForm, TESForm, TESEyes);
		if (race && (eyes || noForm))
			pListForm = (tList<TESForm>*)&(race->eyes);
		break;
	case eWhichListForm_BasePackage:
		base = ConvertActorBase(thisObj, pGenericListOwner);
		if (!noForm)
			pack = DYNAMIC_CAST(pForm, TESForm, TESPackage);
		if (base && (pack || noForm))
			pListForm = (tList<TESForm>*)&(base->ai.packageList);
		break;
	case eWhichListForm_BaseSpellListSpell:
		base = ConvertActorBase(thisObj, pGenericListOwner);
		if (!noForm)
			spell = DYNAMIC_CAST(pForm, TESForm, SpellItem);
		if (base && (spell || noForm))
			pListForm = (tList<TESForm>*)&(base->spellList.spellList);
		break;
	case eWhichListForm_BaseSpellListLevSpell:
		base = ConvertActorBase(thisObj, pGenericListOwner);
		if (!noForm)
			spell = DYNAMIC_CAST(pForm, TESForm, SpellItem);
		if (base && (spell || noForm))
			pListForm = (tList<TESForm>*)&(base->spellList.leveledSpellList);
		break;
	case eWhichListForm_HeadParts:
		npc_ = ConvertNPC(thisObj, pGenericListOwner);
		if (!noForm)
			part = DYNAMIC_CAST(pForm, TESForm, BGSHeadPart);
		if (npc_ && (part || noForm))
			pListForm = (tList<TESForm>*)&(npc_->headPart);
		break;
	//case eWhichListForm_LevCreatureRef:
	//	lCrea = ConvertLevCreature(thisObj, pGenericListOwner);
	//	if (!noForm)
	//		crea = DYNAMIC_CAST(pForm, TESForm, TESCreature);
	//	if (lCrea && (crea || noForm))
	//		pListForm = (tList<TESForm>*)&(lCrea->list.datas);
	//	break;
	//case eWhichListForm_LevCharacterRef:
	//	lChar = ConvertLevCharacter(thisObj, pGenericListOwner);
	//	if (!noForm)
	//		npc_ = DYNAMIC_CAST(pForm, TESForm, TESNPC);
	//	if (lChar && (npc_ || noForm))
	//		pListForm = (tList<TESForm>*)&(lChar->list.datas);
	//	break;
	case eWhichListForm_FormList:
		list = DYNAMIC_CAST(pGenericListOwner, TESForm, BGSListForm);
		if (list && (pForm || noForm))
			pListForm = (tList<TESForm>*)&(list->list);
		break;
	default:	// we are limited to list of descendant from TESForm
		return false;
		break;
	};

	if (pListForm && (pForm || noForm))
	{
		switch (action)
		{
		case eActionListForm_AddAt:
			index = pListForm->AddAt(pForm, n);
			if (index != eListInvalid) {
				*result = index;
			}
			if (IsConsoleMode()) {
				Console_Print("GenericAddForm: Which: %d, Index: %d", whichType, index);
			}
			break;
		case eActionListForm_ChgAt:
			//switch (whichType)
			//{
			//	case eWhichListForm_LevCreatureRef:
			//	case eWhichListForm_LevCharacterRef:
			//		TESLeveledList::BaseData* pBaseData = (TESLeveledList::BaseData*)(pListForm->GetNthItem(n));
			//		pBaseData->refr->baseForm = pForm;
			//		break;
			//}
			if (pForm = pListForm->ReplaceNth(n, pForm))
			{
				*((UInt32 *)result) = pForm->refID;
				if (IsConsoleMode()) {
					Console_Print("GenericChangeForm: Which: %d, Replaced: %08X.", whichType, pForm->refID);
				}
			}
			break;
		case eActionListForm_DelAt:
			if (pForm = pListForm->RemoveNth(n))
			{
				*((UInt32 *)result) = pForm->refID;
				if (IsConsoleMode()) {
					Console_Print("GenericDeleteForm: Which: %d, Deleted: %08X.", whichType, pForm->refID);
				}
			}
			break;
		case eActionListForm_GetAt:
			if (pForm = pListForm->GetNthItem(n))
			{
				*((UInt32 *)result) = pForm->refID;
				if (IsConsoleMode()) {
					Console_Print("GenericGetForm: Which: %d, Returned: %08X.", whichType, pForm->refID);
				}
			}
			break;
		case eActionListForm_Check:
			lCount = pListForm->Count();
			for (n = 0; n < lCount; n++ )
			{
				TESForm* aForm = pListForm->GetNthItem(n);
				if (aForm->refID == pForm->refID)
				{
					*result = n;
					if (IsConsoleMode()) {
						Console_Print("GenericCheckForm: Which: %d, Returned: %d.", whichType, n);
					}
					return true;
				}
			}
			*result = eListEnd;
			if (IsConsoleMode()) {
				Console_Print("GenericCheckForm: Which: %d, Not found!", whichType);
			}
			break;
		default:
			return false;
			break;
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_GenericAddForm_Execute(COMMAND_ARGS)
{
	return GenericForm_Execute(PASS_COMMAND_ARGS, eActionListForm_AddAt);
}

bool Cmd_GenericReplaceForm_Execute(COMMAND_ARGS)
{
	return GenericForm_Execute(PASS_COMMAND_ARGS, eActionListForm_ChgAt);
}

bool Cmd_GenericDeleteForm_Execute(COMMAND_ARGS)
{
	return GenericForm_Execute(PASS_COMMAND_ARGS, eActionListForm_DelAt);
}

bool Cmd_GenericGetForm_Execute(COMMAND_ARGS)
{
	return GenericForm_Execute(PASS_COMMAND_ARGS, eActionListForm_GetAt);
}

bool Cmd_GenericCheckForm_Execute(COMMAND_ARGS)
{
	return GenericForm_Execute(PASS_COMMAND_ARGS, eActionListForm_Check);
}

bool Cmd_GetNthDefaultForm_Execute(COMMAND_ARGS)
{
	TESForm*	pForm = NULL;
	UInt32		formIndex = BGSDefaultObjectManager::kDefaultObject_Max;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (ExtractArgs(EXTRACT_ARGS, &formIndex) && (formIndex < BGSDefaultObjectManager::kDefaultObject_Max)) {
		BGSDefaultObjectManager* g = BGSDefaultObjectManager::GetSingleton();
		pForm = g->defaultObjects.asArray[formIndex];
		if (pForm)
			(*refResult) = pForm->refID;
		return true;
	}
	return true;
}

bool Cmd_SetNthDefaultForm_Execute(COMMAND_ARGS)
{
	TESForm*	pForm = NULL;
	UInt32		formIndex = BGSDefaultObjectManager::kDefaultObject_Max;

	if (ExtractArgs(EXTRACT_ARGS, &formIndex, &pForm) && (formIndex < BGSDefaultObjectManager::kDefaultObject_Max)) {
		BGSDefaultObjectManager* g = BGSDefaultObjectManager::GetSingleton();
		g->defaultObjects.asArray[formIndex] = pForm;
		return true;
	}
	return true;
}

bool Cmd_GetDefaultForms_Execute(COMMAND_ARGS)
{
	// returns an array of all the default forms
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();

	BGSDefaultObjectManager* g = BGSDefaultObjectManager::GetSingleton();
	TESForm *pForm;
	double arrIndex = 0;
	for (UInt32 formIndex = 0; formIndex < BGSDefaultObjectManager::kDefaultObject_Max; formIndex++)
	{
		pForm = g->defaultObjects.asArray[formIndex];
		arr->SetElementFormID(arrIndex, pForm ? pForm->refID : 0);
		arrIndex += 1;
	}

	return true;
}

bool Cmd_GetCurrentQuestObjectiveTeleportLinks_Execute(COMMAND_ARGS)
{
	// returns an array of teleport links for the active objective of the active quest.
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	if (player)
	{
		QuestObjectiveTargets* targets = player->GetCurrentQuestObjectiveTargets();
		if (targets)
		{
			ArrayVar *subArr;
			double subArrIndex;
			for (tList<BGSQuestObjective::Target>::Iterator iter = targets->Begin() ; !iter.End(); ++iter)
			{
				subArr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
				subArrIndex = 0;
				arr->SetElementArray(arrIndex, subArr->ID());
				arrIndex += 1;
				BGSQuestObjective::Target * data = iter.Get();
				if (data && data->teleportLinks.size)
				{
					for (UInt32 i = 0; i < data->teleportLinks.size; i++)
					{
						BGSQuestObjective::TeleportLink teleportLink = data->teleportLinks.data[i];
						if (teleportLink.door)
						{
							subArr->SetElementFormID(subArrIndex, teleportLink.door->refID);
							subArrIndex += 1;
						}
					}
				}
				if (data && data->target)
					subArr->SetElementFormID(subArrIndex, data->target->refID);
			}

		}
	}

	return true;
}

bool Cmd_GetNthAnimation_Execute(COMMAND_ARGS)
{
	TESForm*		pForm = NULL;
	TESNPC*			npc = NULL;
	TESCreature*	crea = NULL;
	TESAnimation*	pAnim = NULL;
	SInt32			animIndex = -1;
	char*			pName = NULL;

	if (ExtractArgs(EXTRACT_ARGS, &animIndex, &pForm) && (animIndex >= 0)) {
		if (!pForm && thisObj)
			pForm = thisObj;

		if (pForm && pForm->GetIsReference())
		{
			TESObjectREFR* pRef = DYNAMIC_CAST(thisObj, TESForm, TESObjectREFR);
			if (pRef)
				pForm = pRef->baseForm;
			else
				pForm = NULL;
		}
		if (pForm)
		{
			npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);
			crea = DYNAMIC_CAST(pForm, TESForm, TESCreature);
		}

		if (npc)
			pAnim = &(npc->animation);
		else if (crea)
			pAnim = &(crea->animation);

		SInt32 index = -1;
		if (pAnim)
			for (TESAnimation::AnimNames::Iterator iter = pAnim->animNames.Begin(); !iter.End(); ++iter)
			{
				++index;
				if (index == animIndex)
				{
					pName = iter.Get();
					break;
				}
			}
	}
	AssignToStringVar(PASS_COMMAND_ARGS, pName);
	return true;
}

bool Cmd_AddAnimation_Execute(COMMAND_ARGS)
{
	TESForm*		pForm = NULL;
	TESNPC*			npc = NULL;
	TESCreature*	crea = NULL;
	TESAnimation*	pAnim = NULL;
	char			name[256];
	char*			pName = NULL;

	if (ExtractArgs(EXTRACT_ARGS, &name, &pForm) && (strlen(name) > 0)) {
		if (!pForm && thisObj)
			pForm = thisObj;

		if (pForm && pForm->GetIsReference())
		{
			TESObjectREFR* pRef = DYNAMIC_CAST(thisObj, TESForm, TESObjectREFR);
			if (pRef)
				pForm = pRef->baseForm;
			else
				pForm = NULL;
		}
		if (pForm)
		{
			npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);
			crea = DYNAMIC_CAST(pForm, TESForm, TESCreature);
		}

		if (npc)
			pAnim = &(npc->animation);
		else if (crea)
			pAnim = &(crea->animation);

		if (pAnim)
		{
			pName = (char*)FormHeap_Allocate(strlen(name)+1);
			strcpy_s(pName, strlen(name)+1, name);
			pAnim->animNames.AddAt(pName, eListEnd);
		}
	}
	return true;
}

bool Cmd_DelAnimation_Execute(COMMAND_ARGS)
{
	TESForm*		pForm = NULL;
	TESNPC*			npc = NULL;
	TESCreature*	crea = NULL;
	TESAnimation*	pAnim = NULL;
	char			name[256];
	char*			pName = (char*)name;

	if (ExtractArgs(EXTRACT_ARGS, &name, &pForm) && (strlen(name) > 0)) {
		if (!pForm && thisObj)
			pForm = thisObj;

		if (pForm && pForm->GetIsReference())
		{
			TESObjectREFR* pRef = DYNAMIC_CAST(thisObj, TESForm, TESObjectREFR);
			if (pRef)
				pForm = pRef->baseForm;
			else
				pForm = NULL;
		}
		if (pForm)
		{
			npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);
			crea = DYNAMIC_CAST(pForm, TESForm, TESCreature);
		}

		if (npc)
			pAnim = &(npc->animation);
		else if (crea)
			pAnim = &(crea->animation);

		SInt32 index = -1;
		if (pAnim)
			for (TESAnimation::AnimNames::Iterator iter = pAnim->animNames.Begin(); !iter.End(); ++iter)
			{
				++index;
				char* aName = iter.Get();
				if (aName && !StrCompare(aName, pName))
				{
					FormHeap_Free(pAnim->animNames.RemoveNth(index));
					break;
				}
			}
	}
	return true;
}

bool Cmd_DelAnimations_Execute(COMMAND_ARGS)
{
	TESForm*		pForm = NULL;
	TESNPC*			npc = NULL;
	TESCreature*	crea = NULL;
	TESAnimation*	pAnim = NULL;

	if (ExtractArgs(EXTRACT_ARGS, &pForm)) {
		if (!pForm && thisObj)
			pForm = thisObj;

		if (pForm && pForm->GetIsReference())
		{
			TESObjectREFR* pRef = DYNAMIC_CAST(thisObj, TESForm, TESObjectREFR);
			if (pRef)
				pForm = pRef->baseForm;
			else
				pForm = NULL;
		}
		if (pForm)
		{
			npc = DYNAMIC_CAST(pForm, TESForm, TESNPC);
			crea = DYNAMIC_CAST(pForm, TESForm, TESCreature);
		}

		if (npc)
			pAnim = &(npc->animation);
		else if (npc)
			pAnim = &(crea->animation);

		if (pAnim)
			pAnim->animNames.RemoveAll();
	}
	return true;
}

bool Cmd_GetDoorSound_Execute(COMMAND_ARGS) {
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs()) {
		TESForm* form = eval.Arg(0)->GetTESForm();
		int mode = static_cast<int>(eval.Arg(1)->GetNumber());

		TESObjectDOOR* doorForm = DYNAMIC_CAST(form, TESForm, TESObjectDOOR);
		if (!doorForm) {
			if (IsConsoleMode()) {
				Console_Print("Form %X is not a TESObjectDOOR", form->refID);
			}

			return true;
		}

		*refResult = mode == 0 ? doorForm->openSound->refID : doorForm->closeSound->refID;
	}

	return true;
}


bool Cmd_FireChallenge_Execute(COMMAND_ARGS) {
	*result = 0;

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (eval.ExtractArgs() && eval.NumArgs() == 6) {
		const auto challengeType = static_cast<int>(eval.Arg(0)->GetNumber());
		const auto count         = static_cast<int>(eval.Arg(1)->GetNumber());
		const auto weapon        = eval.Arg(2)->GetTESForm();
		const auto val1          = static_cast<int>(eval.Arg(3)->GetNumber());
		const auto val2          = static_cast<int>(eval.Arg(4)->GetNumber());
		const auto val3          = static_cast<int>(eval.Arg(5)->GetNumber());

		*result = CdeclCall<uint32_t>(0x5F5950, challengeType, count, nullptr, weapon, val1, val2, val3);
	}

	return true;
}
