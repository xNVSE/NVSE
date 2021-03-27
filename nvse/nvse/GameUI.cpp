#include "nvse/GameUI.h"

UInt8	* g_MenuVisibilityArray = (UInt8 *)0x011F308F;
NiTArray <TileMenu *> * g_TileMenuArray = (NiTArray <TileMenu *> *)0x011F3508;

static const UInt32 s_RaceSexMenu__UpdatePlayerHead	= 0x007B25A0;	// End of RaceSexMenu::Func003.case0, call containing QueuedHead::Init (3rd before jmp)
static UInt8*	g_bUpdatePlayerModel		= (UInt8*)0x011C5CB4;	// this is set to true when player confirms change of race in RaceSexMenu -
																	// IF requires change of skeleton - and back to false when model updated
const _TempMenuByType TempMenuByType = (_TempMenuByType)0x00707990;	// Called from called from call RaceSexMenu::Init

InterfaceManager * InterfaceManager::GetSingleton(void)
{
	return *(InterfaceManager **)0x011D8A80;
}

bool InterfaceManager::IsMenuVisible(UInt32 menuType)
{
	if((menuType >= kMenuType_Min) && (menuType <= kMenuType_Max))
		return g_MenuVisibilityArray[menuType] != 0;

	return false;
}

Menu * InterfaceManager::GetMenuByType(UInt32 menuType)
{
	if((menuType >= kMenuType_Min) && (menuType <= kMenuType_Max))
	{
		TileMenu * tileMenu = g_TileMenuArray->Get(menuType - kMenuType_Min);
		if (tileMenu)
			return tileMenu->menu;
	}

	return NULL;
}

Menu * InterfaceManager::TempMenuByType(UInt32 menuType)
{
	if((menuType >= kMenuType_Min) && (menuType <= kMenuType_Max))
	{
		return ::TempMenuByType(menuType);
	}
	return NULL;
}

TileMenu ***g_tileMenuArray = (TileMenu***)0x11F350C;
UnorderedMap<const char*, UInt32> s_menuNameToID({{"MessageMenu", kMenuType_Message}, {"InventoryMenu", kMenuType_Inventory}, {"StatsMenu", kMenuType_Stats},
	{"HUDMainMenu", kMenuType_HUDMain}, {"LoadingMenu", kMenuType_Loading}, {"ContainerMenu", kMenuType_Container}, {"DialogMenu", kMenuType_Dialog},
	{"SleepWaitMenu", kMenuType_SleepWait}, {"StartMenu", kMenuType_Start}, {"LockpickMenu", kMenuType_LockPick}, {"QuantityMenu", kMenuType_Quantity},
	{"MapMenu", kMenuType_Map}, {"BookMenu", kMenuType_Book}, {"LevelUpMenu", kMenuType_LevelUp}, {"RepairMenu", kMenuType_Repair},
	{"RaceSexMenu", kMenuType_RaceSex}, {"CharGenMenu", kMenuType_CharGen}, {"TextEditMenu", kMenuType_TextEdit}, {"BarterMenu", kMenuType_Barter},
	{"SurgeryMenu", kMenuType_Surgery}, {"HackingMenu", kMenuType_Hacking}, {"VATSMenu", kMenuType_VATS}, {"ComputersMenu", kMenuType_Computers},
	{"RepairServicesMenu", kMenuType_RepairServices}, {"TutorialMenu", kMenuType_Tutorial}, {"SpecialBookMenu", kMenuType_SpecialBook},
	{"ItemModMenu", kMenuType_ItemMod}, {"LoveTesterMenu", kMenuType_LoveTester}, {"CompanionWheelMenu", kMenuType_CompanionWheel},
	{"TraitSelectMenu", kMenuType_TraitSelect}, {"RecipeMenu", kMenuType_Recipe}, {"SlotMachineMenu", kMenuType_SlotMachine},
	{"BlackjackMenu", kMenuType_Blackjack}, {"RouletteMenu", kMenuType_Roulette}, {"CaravanMenu", kMenuType_Caravan}, {"TraitMenu", kMenuType_Trait}});

// Split component path into "top-level menu name" and "everything else".
// Path is of format "MenuType/tile/tile/..." following hierarchy defined in menu's xml.
// Returns pointer to top-level menu or NULL.
// pSlashPos is set to the slash character after the top-level menu name.
TileMenu* InterfaceManager::GetMenuByPath(const char * componentPath, const char ** pSlashPos)
{
	char *slashPos = SlashPos(componentPath);
	if (slashPos) *slashPos = 0;
	*pSlashPos = slashPos;
	UInt32 menuID = s_menuNameToID.Get(componentPath);
	return menuID ? (*g_tileMenuArray)[menuID - kMenuType_Min] : NULL;
}

Tile::Value* InterfaceManager::GetMenuComponentValue(const char * componentPath)
{
	// path is of format "MenuType/tile/tile/.../traitName" following hierarchy defined in menu's xml
	const char *slashPos;
	TileMenu *tileMenu = GetMenuByPath(componentPath, &slashPos);
	if (tileMenu && slashPos)
		return tileMenu->GetComponentValue(slashPos + 1);
	return NULL;
}

Tile::Value* InterfaceManager::GetMenuComponentValueAlt(const char * componentPath)
{
	// path is of format "MenuType/tile/tile/.../traitName" following hierarchy defined in menu's xml
	const char *slashPos;
	TileMenu *tileMenu = GetMenuByPath(componentPath, &slashPos);
	if (tileMenu && slashPos)
		return tileMenu->GetComponentValueAlt(slashPos + 1);
	return NULL;
}

Tile* InterfaceManager::GetMenuComponentTile(const char * componentPath)
{
	// path is of format "MenuType/tile/tile/.../tile" following hierarchy defined in menu's xml
	const char *slashPos;
	TileMenu *tileMenu = GetMenuByPath(componentPath, &slashPos);
	if (tileMenu && slashPos)
		return tileMenu->GetComponentTile(slashPos + 1);
	return tileMenu;
}

void Debug_DumpMenus(void)
{
	for(UInt32 i = 0; i < g_TileMenuArray->Length(); i++)
	{
		TileMenu	* tileMenu = g_TileMenuArray->Get(i);

		if(tileMenu)
		{
			_MESSAGE("menu %d at %x:", i, tileMenu);
			gLog.Indent();

			tileMenu->Dump();

			gLog.Outdent();
		}
	}
}

void RaceSexMenu::UpdatePlayerHead(void)
{
	ThisStdCall(s_RaceSexMenu__UpdatePlayerHead, this);
}
