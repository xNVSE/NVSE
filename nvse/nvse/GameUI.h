#pragma once

#include "GameTiles.h"
#include "GameTypes.h"

class Menu;
class SceneGraph;
class FOPipboyManager;
class NiObject;
class TESObjectREFR;
class NiNode;
class BSShaderAccumulator;
class ShadowSceneNode;
class NiSourceTexture;
class FORenderedMenu;
class TileExtra;
class NiRefObject;

typedef Menu* (*_TempMenuByType)(UInt32 menuType);
extern const _TempMenuByType TempMenuByType;

enum MenuSpecialKeyboardInputCode
{
	kEnter = -2,
	kUpArrow = 0x1,
	kDownArrow = 0x2,
	kRightArrow = 0x3,
	kLeftArrow = 0x4,
	kSpace = 0x9,
	kTab = 0xA,
	kShiftEnter = 0xB,
	kAltEnter = 0xC,
	kShiftLeft = 0xD,
	kShiftRight = 0xE,
	kPageUp = 0xF,
	kPageDown = 0x10,
};

struct InterfaceManagerStruct0178;

// 584
class InterfaceManager
{
public:
	InterfaceManager();
	~InterfaceManager();

	// lStewieAl's Tweaks
	static InterfaceManager* 	GetSingleton(void) { return *(InterfaceManager * *)(0x11D8A80); };
	static bool					IsMenuVisible(UInt32 menuType);
	static Menu *				GetMenuByType(UInt32 menuType);
	static Menu *				TempMenuByType(UInt32 menuType);
	//static TileMenu *			GetMenuByPath(const char * componentPath, const char ** slashPos);
	//static Tile::Value *		GetMenuComponentValue(const char * componentPath);
	//static Tile *				GetMenuComponentTile(const char * componentPath);

	// JIP LN
	UInt32 GetTopVisibleMenuID();
	TESObjectREFR *GetCursorPick();
	Tile *GetActiveTile();
	void ClearHighlights();
	void AddHighlightedRef(TESObjectREFR *refr);

	struct HighlightedRef
	{
		TESObjectREFR	*refr;
		BSFadeNode		*node;

		void Set(TESObjectREFR *_refr)
		{
			refr = _refr;
			ThisStdCall(0x66B0D0, &node, _refr->GetNiNode());
		}

		void Clear()
		{
			refr = NULL;
			if (node) ThisStdCall(0x66B0D0, &node, 0);
		}
	};

	struct Struct0178
	{
		UInt32 unk00;
		UInt32 NiTPrimitiveArray[9];
		UInt8 byte28;
		UInt8 byte29;
		UInt8 byte2A;
		UInt8 byte2B;
		UInt32 startTime;
		float durationX;
		float durationY;
		float intensityX;
		float intensityY;
		float frequencyX;
		float frequencyY;
		float unk48;
		float unk4C;
		UInt32 imageSpaceEffectParam;
		UInt8 isFlycamEnabled;
		UInt8 byte55;
		UInt8 byte56;
		UInt8 byte57;
		float fBlurRadiusHUD;
		float fScanlineFrequencyHUD;
		float fBlurIntensityHUD;
	};

	struct RefAndNiNode
	{
		TESObjectREFR* ref;
		NiNode* node;
	};

	struct VATSHighlightData
	{
		UInt32 mode;						// 000
		RefAndNiNode target;				// 004
		UInt32 numHighlightedRefs;			// 00C
		UInt32 flashingRefIndex;			// 010
		RefAndNiNode highlightedRefs[32];	// 014
		UInt32 unk114;						// 114
		UInt8 isOcclusionEnabled;			// 118
		UInt8 byte119[16];					// 119
		UInt8 byte12B;						// 12B
		UInt32 unk12C[16];					// 12C
		UInt32 unk16C[16];					// 16C
		UInt8 byte1AC[16];					// 1AC
		UInt32 unk1BC[16];					// 1BC
		UInt32 unk1FC[16];					// 1FC
		UInt32 selectedLimbID;				// 23C
		UInt32 count240;					// 240 indexes into unk244 when it is appended to
		UInt32 unk244[16];					// 244
		UInt8 byte284;						// 284
		UInt8 pad285[3];					// 285
		UInt32 unk288;						// 288
		float unk28C;						// 28C
		UInt32 unk290;						// 290
		float vatsDistortTime;				// 294
		UInt8 byte298;						// 298
		UInt8 pad299[3];					// 299
		float time29C;						// 29C
		float unk2A0;						// 2A0
		float unk2A4;						// 2A4
		float unk2A8;						// 2A8
		float unk2AC;						// 2AC
		float unk2B0;						// 2B0
		UInt8 byte2B4;						// 2B4
		UInt8 byte2B5;						// 2B5
		UInt8 pad2B6[2];					// 2B6
		float unk2B8;						// 2B8
		float unk2BC;						// 2BC
		float fVATSTargetPulseRate;			// 2C0
		NiRefObject* unk2C4;				// 2C4
		UInt32 unk2C8;						// 2C8
		NiRefObject* unk2CC;				// 2CC

		void AddRef(TESObjectREFR* ref);
		void AddRefAndSetFlashing(TESObjectREFR* ref);
		void ResetRefs();
	};

	struct Tutorials
	{
		SInt32 tutorialFlags[41];
		UInt32 currentShownHelpID;
		UInt32 timeA8;
	};

	enum PipboyTabs
	{
		kStats = 1,
		kItems,
		kData
	};

	enum KeyModifier
	{
		kAltHeld = 0x1,
		kControlHeld = 0x2,
		kShiftHeld = 0x4,
	};

	enum Mode
	{
		kGameMode = 1,
		kMenuMode
	};
	

	struct Struct0178
	{
		UInt32 unk00;
		UInt32 NiTPrimitiveArray[9];
		UInt8 byte28;
		UInt8 byte29;
		UInt8 byte2A;
		UInt8 byte2B;
		UInt32 startTime;
		float durationX;
		float durationY;
		float intensityX;
		float intensityY;
		float frequencyX;
		float frequencyY;
		float unk48;
		float unk4C;
		UInt32 imageSpaceEffectParam;
		UInt8 isFlycamEnabled;
		UInt8 byte55;
		UInt8 byte56;
		UInt8 byte57;
		float fBlurRadiusHUD;
		float fScanlineFrequencyHUD;
		float fBlurIntensityHUD;
	};

	struct RefAndNiNode
	{
		TESObjectREFR* ref;
		NiNode* node;
	};

	struct VATSHighlightData
	{
		UInt32 mode;						// 000
		RefAndNiNode target;				// 004
		UInt32 numHighlightedRefs;			// 00C
		UInt32 flashingRefIndex;			// 010
		RefAndNiNode highlightedRefs[32];	// 014
		UInt32 unk114;						// 114
		UInt8 isOcclusionEnabled;			// 118
		UInt8 byte119[16];					// 119
		UInt8 byte12B;						// 12B
		UInt32 unk12C[16];					// 12C
		UInt32 unk16C[16];					// 16C
		UInt8 byte1AC[16];					// 1AC
		UInt32 unk1BC[16];					// 1BC
		UInt32 unk1FC[16];					// 1FC
		UInt32 selectedLimbID;				// 23C
		UInt32 count240;					// 240 indexes into unk244 when it is appended to
		UInt32 unk244[16];					// 244
		UInt8 byte284;						// 284
		UInt8 pad285[3];					// 285
		UInt32 unk288;						// 288
		float unk28C;						// 28C
		UInt32 unk290;						// 290
		float vatsDistortTime;				// 294
		UInt8 byte298;						// 298
		UInt8 pad299[3];					// 299
		float time29C;						// 29C
		float unk2A0;						// 2A0
		float unk2A4;						// 2A4
		float unk2A8;						// 2A8
		float unk2AC;						// 2AC
		float unk2B0;						// 2B0
		UInt8 byte2B4;						// 2B4
		UInt8 byte2B5;						// 2B5
		UInt8 pad2B6[2];					// 2B6
		float unk2B8;						// 2B8
		float unk2BC;						// 2BC
		float fVATSTargetPulseRate;			// 2C0
		NiRefObject* unk2C4;				// 2C4
		UInt32 unk2C8;						// 2C8
		NiRefObject* unk2CC;				// 2CC

		void AddRef(TESObjectREFR* ref);
		void AddRefAndSetFlashing(TESObjectREFR* ref);
		void ResetRefs();
	};

	struct Tutorials
	{
		SInt32 tutorialFlags[41];
		UInt32 currentShownHelpID;
		UInt32 timeA8;
	};

	enum PipboyTabs
	{
		kStats = 1,
		kItems,
		kData
	};

	enum KeyModifier
	{
		kAltHeld = 0x1,
		kControlHeld = 0x2,
		kShiftHeld = 0x4,
	};

	enum Mode
	{
		kGameMode = 1,
		kMenuMode
	};

	UInt32					flags;				// 000
	SceneGraph				*sceneGraph004;		// 004
	SceneGraph				*sceneGraph008;		// 008
	UInt32					currentMode;		// 00C	1 = GameMode; 2 = MenuMode
	// checked for 1 at 0x70BA97
	// set to 2 at 0x70BA8D
	// set to 3 at 0x714E96, 0x714E20 - checked at 0x70B94E
	// set to 4 at 0x714D5D, 0x715177 (CloseAllMenus)
	// set to 5 at 0x70B972 - checked at 0x70BA84, 0x70CA14, 0x70CC7A
	UInt32					unk010;				// 010
	UInt32					unk014;				// 014
	UInt32					pickLength;			// 018
	UInt32					unk01C;				// 01C
	UInt8					byte020;			// 020
	UInt8					byte021;			// 021
	UInt8					byte022;			// 022
	UInt8					byte023;			// 023
	UInt32					unk024;				// 024
	TileImage				*cursor;			// 028
	float					flt02C;				// 02C
	float					flt030;				// 030
	float					flt034;				// 034
	float					cursorX;			// 038
	float					flt03C;				// 03C
	float					cursorY;			// 040
	float					mouseWheel;			// 044	-120.0 = down; 120.0 = up
	float					flt048;				// 048
	Tile					*draggedTile;		// 04C
	int						unk050;				// 050
	float					flt054;				// 054
	float					flt058;				// 058
	int						unk05C;				// 05C
	int						unk060;				// 060
	int						unk064;				// 064
	UInt32					unk068[2];			// 068
	tList<TESObjectREFR>	selectableRefs;		// 070
	UInt32					selectedRefIndex;	// 078
	bool					debugText;			// 07C
	UInt8					byte07D;			// 07D
	UInt8					byte07E;			// 07E
	UInt8					byte07F;			// 07F
	NiNode					*niNode080;			// 080
	NiNode					*niNode084;			// 084
	UInt32					unk088;				// 088
	BSShaderAccumulator		*shaderAccum08C;	// 08C
	BSShaderAccumulator		*shaderAccum090;	// 090
	ShadowSceneNode			*shadowScene094;	// 094
	ShadowSceneNode			*shadowScene098;	// 098
	Tile					*menuRoot;			// 09C
	Tile					*globalsTile;		// 0A0	globals.xml
	NiNode					*niNode0A4;			// 0A4
	UInt32					unk0A8;				// 0A8
	NiObject				*unk0AC;			// 0AC seen NiAlphaProperty
	UInt32					unk0B0[3];			// 0B0
	Tile					*activeTileAlt;		// 0BC
	UInt32					unk0C0;				// 0C0
	UInt32					unk0C4;				// 0C4
	UInt8					byte0C8;			// 0C8
	UInt8					byte0C9;			// 0C9
	UInt8					byte0CA;			// 0CA
	UInt8					byte0CB;			// 0CB
	Tile					*activeTile;		// 0CC
	Menu					*activeMenu;		// 0D0
	Tile					*tile0D4;			// 0D4
	Menu					*menu0D8;			// 0D8
	UInt32					unk0DC[2];			// 0DC
	UInt8					msgBoxButton;		// 0E4 -1 if no button pressed
	UInt8					byte0E5;			// 0E5
	UInt8					byte0E6;			// 0E6
	UInt8					byte0E7;			// 0E7
	UInt32					unk0E8;				// 0E8
	UInt8					byte0EC;			// 0EC
	UInt8					hasMouseMoved;		// 0ED
	UInt8					byte0EE;			// 0EE
	UInt8					byte0EF;			// 0EF
	TESObjectREFR			*debugSelection;	// 0F0	compared to activated object during Activate
	UInt32					unk0F4;				// 0F4
	UInt32					unk0F8;				// 0F8
	TESObjectREFR			*crosshairRef;		// 0FC
	UInt32					unk100[4];			// 100
	UInt8					byte110;			// 110
	UInt8					pad111[3];			// 111
	UInt32					menuStack[10];		// 114
	void					*ptr13C;			// 13C	Points to a struct, possibly. First member is *bhkSimpleShapePhantom
	UInt32					unk140;				// 140
	UInt32					unk144;				// 144
	UInt8					byte148;			// 148
	UInt8					isShift;			// 149
	UInt8					byte14A;			// 14A
	UInt8					byte14B;			// 14B
	KeyModifier				keyModifiers;		// 14C
	UInt32					currentKey;			// 150
	UInt32					keyRepeatStartTime;	// 154
	UInt32					lastKeyRepeatTime;	// 158
	UInt32					unk15C[4];			// 15C
	FORenderedMenu*			renderedMenu;		// 16C
	UInt8					byte170;			// 170
	UInt8					byte171;			// 171
	UInt8					byte172;			// 172
	UInt8					byte173;			// 173
	FOPipboyManager			*pipboyManager;		// 174
	Struct0178				unk178;				// 178
	VATSHighlightData		vatsHighlightData;	// 1DC
	float					scale4AC;			// 4AC
	float					unk4B0;				// 4B0
	UInt8					isRenderedMenuOrPipboyManager;		// 4B4
	UInt8					byte4B5;			// 4B5
	UInt8					byte4B6;			// 4B6
	UInt8					byte4B7;			// 4B7
	UInt32					queuedPipboyTabToSwitchTo;	// 4B8
	UInt32					pipBoyMode;			// 4BC
	void (*onPipboyOpenCallback)(void);			// 4C0
	UInt32					unk4C4[2];			// 4C4
	UInt8					byte4CC;			// 4CC
	UInt8					byte4CD;			// 4CD
	UInt8					pad4CE;				// 4CE
	UInt8					pad4CF;				// 4CF
	UInt32					unk4D0;				// 4D0
	Tutorials				help;				// 4D4
};
STATIC_ASSERT(sizeof(InterfaceManager) == 0x580);
STATIC_ASSERT(sizeof(InterfaceManager::Struct0178) == 0x64);

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

enum MenuSpecialKeyboardInputCode
{
	kEnter =		-2,
	kUpArrow =		1,
	kDownArrow,
	kRightArrow,
	kLeftArrow,
	kSpace =		9,
	kTab,
	kShiftEnter,
	kAltEnter,
	kShiftLeft,
	kShiftRight,
	kPageUp,
	kPageDown
};

class Menu
{
public:
	Menu();
	~Menu();

	virtual void	Destructor(bool doFree);
	virtual void	SetTile(UInt32 idx, Tile *value); // can be used to determine how many Tile*s are in a Menu class
	virtual void	HandleLeftClickPress(UInt32 tileID, Tile* activeTile); // called when the mouse has moved and left click is pressed
	virtual void	HandleClick(SInt32 tileID, Tile *clickedTile);
	virtual void	HandleMouseover(UInt32 arg0, Tile *activeTile);	//	Called on mouseover, activeTile is moused-over Tile
	virtual void	HandleMouseoverTileAlt(UInt32 arg0, Tile* tile);
	virtual void	Unk_06(UInt32 arg0, UInt32 arg1, UInt32 arg2);  // unused?
	virtual void	Unk_07(UInt32 arg0, UInt32 arg1, UInt32 arg2); // unused?
	virtual void	HandleActiveMenuClick(UInt32 tileID, Tile* activeTile); // StartMenu, RaceSexMenu, VATSMenu, BookMenu
	virtual void	Unk_09(UInt32 arg0, UInt32 arg1); // unused?
	virtual void	HandleMousewheel(UInt32 tileID, Tile* tile);
	virtual void	Update(void);	// Called every frame while the menu is active
	virtual bool	HandleKeyboardInput(UInt32 inputChar);	// Return false for handling keyboard shortcuts
	virtual UInt32	GetID(void);
	virtual bool	HandleSpecialKeyInput(MenuSpecialKeyboardInputCode code, UInt32 arg1);
	virtual bool	HandleControllerInput(int a2, Tile* tile);
	virtual void	Unk_10(void); // unused?
	virtual void	HandleControllerConnectOrDisconnect(bool isControllerConnected);
	
	// 14
	struct TemplateInstance
	{
		UInt32		unk00;		// 00
		float		flt04;		// 04
		String		tileName;	// 08
		Tile		*tile;		// 10
	};

	// 14
	struct TemplateData
	{
		const char				*templateName;	// 00
		TileExtra				*tileExtra;		// 04
		DList<TemplateInstance>	instances;		// 08
	};

	TileMenu			*tile;			// 04
	tList<TemplateData>	menuTemplates;	// 08
	UInt32				unk10;			// 10
	UInt32				unk14;			// 14	Same as id (20)
	UInt32				unk18;			// 18
	UInt8				byte1C;			// 1C
	UInt8				byte1D;			// 1D
	UInt8				byte1E;			// 1E
	UInt8				byte1F;			// 1F
	UInt32				id;				// 20
	UInt32				unk24;			// 24

	// 14
	struct TemplateData
	{
		const char* templateName;	// 00
		TileExtra* tileExtra;		// 04
		DList<TemplateInstance>	instances;		// 08
	};

	TileMenu* tile;			// 04
	tList<TemplateData>	menuTemplates;	// 08
	UInt32				unk10;			// 10
	UInt32				unk14;			// 14	Same as id (20)
	UInt32				unk18;			// 18
	UInt8				byte1C;			// 1C
	UInt8				byte1D;			// 1D
	UInt8				byte1E;			// 1E
	UInt8				byte1F;			// 1F
	UInt32				id;				// 20
	UInt32				visibilityState;// 24 :: Initialised to 4
	// check 1 at 0xA0B174, 0x70D529, 0x70D592 :: set at 0x712224
	// check 2 at 0x711FF1 :: set 2 at 0xA1D987 (when closes menu), 0xA1DA41
    // check 4 at 0xA1D9EC (when closing menu) :: set at 0x7036A4, 0x71204D
	// check 8 at 0x712194 :: set 8 at 0xA1DB8F (when opening menu), 0x720B39
	
	Menu *HandleMenuInput(int tileID, Tile *clickedTile);
	Tile *AddTileFromTemplate(Tile *destTile, const char *templateName, UInt32 arg3);
};
/*
// 170
class RaceSexMenu : public Menu		// 1036
{
public:
	RaceSexMenu();
	~RaceSexMenu();

	UInt32				unk028[44];		// 028
	TESNPC				*npc;			// 0D8
	UInt32				unk0DC[37];		// 0DC

	void UpdatePlayerHead(void);
};
*/
