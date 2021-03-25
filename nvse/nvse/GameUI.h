#pragma once

#include "GameTiles.h"
#include "GameTypes.h"


class Menu;
class SceneGraph;
class FOPipboyManager;
class NiObject;
class TESObjectREFR;
class NiNode;

typedef Menu * (* _TempMenuByType)(UInt32 menuType);
extern const _TempMenuByType TempMenuByType;

// 584
class InterfaceManager
{
public:
	InterfaceManager();
	~InterfaceManager();

	static InterfaceManager *	GetSingleton(void);
	static bool					IsMenuVisible(UInt32 menuType);
	static Menu *				GetMenuByType(UInt32 menuType);
	static Menu *				TempMenuByType(UInt32 menuType);
	static TileMenu *			GetMenuByPath(const char * componentPath, const char ** slashPos);
	static Tile::Value *		GetMenuComponentValue(const char * componentPath);
	static Tile *				GetMenuComponentTile(const char * componentPath);
	static Tile::Value *		GetMenuComponentValueAlt(const char * componentPath);



	UInt32				flags;							// 000	bit 1 would be "not menu mode"
	SceneGraph			* sceneGraph004;				// 004
	SceneGraph			* sceneGraph008;				// 008
	UInt8				byt00C;							// 00C
	UInt8				pad00D[3];						// 00D
	UInt32				unk010[(0x028 - 0x010) >> 2];	// 010
	Tile				* cursor;						// 028
	UInt32				unk02C[(0x070 - 0x02C) >> 2];	// 02C
	UInt32				unk070;							// 070
	UInt32				unk074;							// 074
	UInt32				unk078;							// 078
	UInt32				unk07C;							// 07C
	NiNode				* niNode080;					// 080
	NiNode				* niNode084;					// 084
	UInt32				unk088[(0x09C - 0x088) >> 2];	// 088
	Tile				* menuRoot;						// 09C
	Tile				* unk0A0;						// 0A0 seen "Strings"
	void				* unk0A4;						// 0A4 saw Tile? seen NiNode
	UInt32				unk0A8;							// 0A8
	NiObject			* unk0AC;						// 0AC seen NiAlphaProperty
	UInt32				unk0B0[(0x0BC - 0x0B0) >> 2];	// 0B0
	Tile				*activeTileAlt;					// 0BC
	UInt32				unk0C0[3];						// 0B0
	Tile				* activeTile;					// 0CC
	Menu				* activeMenu;					// 0D0
	UInt32				unk0D4[(0x0E4 - 0x0D4) >> 2];	// 0D4
	UInt8				msgBoxButton;					// 0E4 -1 if no button pressed
	UInt8				pad0E5[3];
	UInt32				unk0E8;							// 0E8
	UInt32				unk0EC;							// 0EC
	TESObjectREFR		*debugSelection;				// 0F0	compared to activated object during Activate
	UInt32				unk0F4;							// 0F4
	UInt32				unk0F8;							// 0F8
	TESObjectREFR		* crosshairRef;					// 0FC
	UInt32				unk100[(0x114 - 0x100) >> 2];	// 100
	UInt32				menuStack[10];					// 114
	UInt32				unk13C[14];						// 13C
	FOPipboyManager		* pipboyManager;				// 174
	UInt32				* unk178;						// 178 could be camera pointer, 0054 is bFlyCam
	NiTArray<UInt32>	array17C;						// 17C NiTPrimitiveArray@PackedMenu@BGSMenuPacker
	UInt32				unk18C[(0x394 - 0x18C) >> 2];	// 18C
	NiObject			* unk394;						// 394 seen NiSourceTexture
	UInt32				unk398[(0x454 - 0x398) >> 2];	// 398
	NiTArray<UInt32>	array454;						// 454 NiTPrimitiveArray@TextureType@BSTextureManager
	NiTArray<UInt32>	array464;						// 464 NiTPrimitiveArray@FilterMode@NiTexturingProperty
	UInt32				unk468[(0x584 - 0x474) >> 2];	// 474
};
STATIC_ASSERT(sizeof(InterfaceManager) == 0x584);
STATIC_ASSERT(offsetof(InterfaceManager, debugSelection) == 0x0F0);
STATIC_ASSERT(offsetof(InterfaceManager, crosshairRef) == 0x0FC);
STATIC_ASSERT(offsetof(InterfaceManager, pipboyManager) == 0x174);

void Debug_DumpMenus(void);

enum
{
	kMenuType_Min =				0x3E9,
	kMenuType_Message =			kMenuType_Min,
	kMenuType_Inventory,
	kMenuType_Stats,
	kMenuType_HUDMain,
	kMenuType_Loading =			0x3EF,
	kMenuType_Container,
	kMenuType_Dialog,
	kMenuType_SleepWait =		0x3F4,
	kMenuType_Start,
	kMenuType_LockPick,
	kMenuType_Quantity =		0x3F8,
	kMenuType_Map =				0x3FF,
	kMenuType_Book =			0x402,
	kMenuType_LevelUp,
	kMenuType_Repair =			0x40B,
	kMenuType_RaceSex,
	kMenuType_Credits =			0x417,
	kMenuType_CharGen,
	kMenuType_TextEdit =		0x41B,
	kMenuType_Barter =			0x41D,
	kMenuType_Surgery,
	kMenuType_Hacking,
	kMenuType_VATS,
	kMenuType_Computers,
	kMenuType_RepairServices,
	kMenuType_Tutorial,
	kMenuType_SpecialBook,
	kMenuType_ItemMod,
	kMenuType_LoveTester =		0x432,
	kMenuType_CompanionWheel,
	kMenuType_TraitSelect,
	kMenuType_Recipe,
	kMenuType_SlotMachine =		0x438,
	kMenuType_Blackjack,
	kMenuType_Roulette,
	kMenuType_Caravan,
	kMenuType_Trait =			0x43C,
	kMenuType_Max =				kMenuType_Trait,
};

class Menu
{
public:
	Menu();
	~Menu();

	virtual void	Destructor(bool arg0);		// pass false to free memory
	virtual void	SetField(UInt32 idx, Tile* value);
	virtual void	Unk_02(UInt32 arg0, UInt32 arg1);	// show menu?
	virtual void	HandleClick(UInt32 buttonID, Tile* clickedButton); // buttonID = <id> trait defined in XML
	virtual void	HandleMouseover(UInt32 arg0, Tile * activeTile);	//called on mouseover, activeTile is moused-over Tile
	virtual void	Unk_05(UInt32 arg0, UInt32 arg1);
	virtual void	Unk_06(UInt32 arg0, UInt32 arg1, UInt32 arg2);
	virtual void	Unk_07(UInt32 arg0, UInt32 arg1, UInt32 arg2);
	virtual void	Unk_08(UInt32 arg0, UInt32 arg1);
	virtual void	Unk_09(UInt32 arg0, UInt32 arg1);
	virtual void	Unk_0A(UInt32 arg0, UInt32 arg1);
	virtual void	Unk_0B(void);
	virtual bool	HandleKeyboardInput(char inputChar);	//for keyboard shortcuts, return true if handled
	virtual UInt32	GetID(void);
	virtual bool	Unk_0E(UInt32 arg0, UInt32 arg1);

	TileMenu		*tile;		// 04
	UInt32			unk08;		// 08
	UInt32			unk0C;		// 0C
	UInt32			unk10;		// 10
	UInt32			unk14;		// 14
	UInt32			unk18;		// 18
	UInt32			unk1C;		// 1C
	UInt32			id;			// 20
	UInt32			unk24;		// 24
};

class RaceSexMenu : public Menu
{
public:
	void UpdatePlayerHead(void);
};

