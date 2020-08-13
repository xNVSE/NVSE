#include "nvse/GameUI.h"

UInt8	* g_MenuVisibilityArray = (UInt8 *)0x011F308F;
NiTArray <TileMenu *> * g_TileMenuArray = (NiTArray <TileMenu *> *)0x011F3508;

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_RaceSexMenu__UpdatePlayerHead	= 0x007B25A0;	// End of RaceSexMenu::Func003.case0, call containing QueuedHead::Init (3rd before jmp)
	static UInt8*	g_bUpdatePlayerModel		= (UInt8*)0x011C5CB4;	// this is set to true when player confirms change of race in RaceSexMenu -
																		// IF requires change of skeleton - and back to false when model updated
	const _TempMenuByType TempMenuByType = (_TempMenuByType)0x00707990;	// Called from called from call RaceSexMenu::Init
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	static const UInt32 s_RaceSexMenu__UpdatePlayerHead	= 0x007B2660;	// End of RaceSexMenu::Func003.case0, call containing QueuedHead::Init (3rd before jmp)
	static UInt8*	g_bUpdatePlayerModel		= (UInt8*)0x011C5CB4;	// this is set to true when player confirms change of race in RaceSexMenu -
																		// IF requires change of skeleton - and back to false when model updated
	const _TempMenuByType TempMenuByType = (_TempMenuByType)0x007078C0;
#elif EDITOR
#else
#error
#endif

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

const char kMenuNames[] =
"MessageMenu\0InventoryMenu\0StatsMenu\0HUDMainMenu\0LoadingMenu\0ContainerMenu\0DialogMenu\0SleepWaitMenu\0StartMenu\0\
LockpickMenu\0QuantityMenu\0MapMenu\0BookMenu\0LevelUpMenu\0RepairMenu\0RaceSexMenu\0CharGenMenu\0TextEditMenu\0BarterMenu\0\
SurgeryMenu\0HackingMenu\0VATSMenu\0ComputersMenu\0RepairServicesMenu\0TutorialMenu\0SpecialBookMenu\0ItemModMenu\0LoveTesterMenu\0\
CompanionWheelMenu\0TraitSelectMenu\0RecipeMenu\0SlotMachineMenu\0BlackjackMenu\0RouletteMenu\0CaravanMenu\0TraitMenu";
const UInt32 kMenuIDs[] =
{
	kMenuType_Message, kMenuType_Inventory, kMenuType_Stats, kMenuType_HUDMain, kMenuType_Loading, kMenuType_Container,
	kMenuType_Dialog, kMenuType_SleepWait, kMenuType_Start, kMenuType_LockPick, kMenuType_Quantity, kMenuType_Map, kMenuType_Book,
	kMenuType_LevelUp, kMenuType_Repair, kMenuType_RaceSex, kMenuType_CharGen, kMenuType_TextEdit, kMenuType_Barter, kMenuType_Surgery,
	kMenuType_Hacking, kMenuType_VATS, kMenuType_Computers, kMenuType_RepairServices, kMenuType_Tutorial, kMenuType_SpecialBook,
	kMenuType_ItemMod, kMenuType_LoveTester, kMenuType_CompanionWheel, kMenuType_TraitSelect, kMenuType_Recipe, kMenuType_SlotMachine,
	kMenuType_Blackjack, kMenuType_Roulette, kMenuType_Caravan, kMenuType_Trait
};
TileMenu **g_tileMenuArray = NULL;
UnorderedMap<const char*, UInt32> s_menuNameToID(0x40);

// Split component path into "top-level menu name" and "everything else".
// Path is of format "MenuType/tile/tile/..." following hierarchy defined in menu's xml.
// Returns pointer to top-level menu or NULL.
// pSlashPos is set to the slash character after the top-level menu name.
TileMenu* InterfaceManager::GetMenuByPath(const char * componentPath, const char ** pSlashPos)
{
	if (s_menuNameToID.Empty())
	{
		g_tileMenuArray = *(TileMenu***)0x11F350C;
		const char *strPos = kMenuNames;
		UInt32 count = 0;
		do
		{
			s_menuNameToID[strPos] = kMenuIDs[count];
			strPos = StrEnd(strPos) + 1;
		}
		while (++count < 36);
	}

	char *slashPos = SlashPos(componentPath);
	if (slashPos) *slashPos = 0;
	*pSlashPos = slashPos;
	UInt32 menuID = s_menuNameToID.Get(componentPath);
	return menuID ? g_tileMenuArray[menuID - kMenuType_Min] : NULL;
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
