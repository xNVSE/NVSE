#pragma once
#include "CommandTable.h"

class Menu;
class TESRace;
class TESForm;
class TESObjectREFR;
class AlchemyItem;
class TESDescription;

void Hook_Gameplay_Init(void);
void ToggleUIMessages(bool enableSpam);
void ToggleConsoleOutput(bool enable);
bool ToggleMenuShortcutKeys(bool bEnable, Menu* menu = NULL);
bool RunCommand_NS(COMMAND_ARGS, Cmd_Execute cmd);

void SetRaceAlias(TESRace* race, TESRace* alias, bool bEnableAlias);

void ModPlayerSpellEffectiveness(double modBy);
double GetPlayerSpellEffectivenessModifier();

extern TESForm* g_LastEnchantedItem;
extern TESForm* g_LastCreatedSpell;
extern TESForm* g_LastCreatedPotion;
extern TESForm* g_LastUniqueCreatedPotion;

extern DWORD g_mainThreadID;

void QueueRefForDeletion(TESObjectREFR* refr);

// returns a potion that matches the effects of toMatch if one exists
AlchemyItem* MatchPotion(AlchemyItem* toMatch);

void ModPlayerMovementSpeed(double modBy);
double GetPlayerMovementSpeedModifier();

// this returns a refID rather than a TESObjectREFR* as dropped items are non-persistent references
UInt32 GetPCLastDroppedItemRef();
TESForm* GetPCLastDroppedItem();		// returns the base object

bool SetDescriptionTextForForm(TESForm* form, const char* newText, UInt8 skillIndex = -1);
bool SetDescriptionText(TESDescription* desc, const char* newText);
bool IsDescriptionModified(TESDescription* desc);

bool GetLastTransactionInfo(TESForm** form, UInt32* quantity);

struct TransactionInfo {
	TESObjectREFR	* buyer;
	TESObjectREFR	* seller;
	TESForm			* item;
	UInt32			quantity;
	UInt32			price;
};

enum eTransactionType {
	kPC_Buy,
	kPC_Sell
};

const TransactionInfo* GetLastTransactionInfo(eTransactionType type, UInt32 callingScriptRefID = 0);

bool GetLastSigilStoneInfo(TESForm** outStone, TESForm** outOldItem, TESForm** outCreatedItem);

bool GetCellChanged();

void SetRetainExtraOwnership(bool bRetain);

bool ToggleSkillPerk(UInt32 actorVal, UInt32 mastery, bool bEnable);