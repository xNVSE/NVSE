#pragma once

enum FormType
{
	kFormType_None = 0,
	kFormType_TES4,
	kFormType_Group,
	kFormType_GMST,
	kFormType_BGSTextureSet,
	kFormType_BGSMenuIcon,
	kFormType_TESGlobal,
	kFormType_TESClass,
	kFormType_TESFaction,
	kFormType_BGSHeadPart,
	kFormType_TESHair,
	kFormType_TESEyes,
	kFormType_TESRace,
	kFormType_TESSound,
	kFormType_BGSAcousticSpace,
	kFormType_TESSkill,
	kFormType_EffectSetting,
	kFormType_Script,
	kFormType_TESLandTexture,
	kFormType_EnchantmentItem,
	kFormType_SpellItem,
	kFormType_TESObjectACTI,
	kFormType_BGSTalkingActivator,
	kFormType_BGSTerminal,
	kFormType_TESObjectARMO,					// Inventory object
	kFormType_TESObjectBOOK,					// Inventory object
	kFormType_TESObjectCLOT,					// Inventory object
	kFormType_TESObjectCONT,
	kFormType_TESObjectDOOR,
	kFormType_IngredientItem,					// Inventory object
	kFormType_TESObjectLIGH,					// Inventory object
	kFormType_TESObjectMISC,					// Inventory object
	kFormType_TESObjectSTAT,
	kFormType_BGSStaticCollection,
	kFormType_BGSMovableStatic,
	kFormType_BGSPlaceableWater,
	kFormType_TESGrass,
	kFormType_TESObjectTREE,
	kFormType_TESFlora,
	kFormType_TESFurniture,
	kFormType_TESObjectWEAP,					// Inventory object
	kFormType_TESAmmo,							// Inventory object
	kFormType_TESNPC,
	kFormType_TESCreature,
	kFormType_TESLevCreature,
	kFormType_TESLevCharacter,
	kFormType_TESKey,							// Inventory object
	kFormType_AlchemyItem,						// Inventory object
	kFormType_BGSIdleMarker,
	kFormType_BGSNote,							// Inventory object
	kFormType_BGSConstructibleObject,			// Inventory object
	kFormType_BGSProjectile,
	kFormType_TESLevItem,						// Inventory object
	kFormType_TESWeather,
	kFormType_TESClimate,
	kFormType_TESRegion,
	kFormType_NavMeshInfoMap,
	kFormType_TESObjectCELL,
	kFormType_TESObjectREFR,
	kFormType_Character,
	kFormType_Creature,
	kFormType_MissileProjectile,
	kFormType_GrenadeProjectile,
	kFormType_BeamProjectile,
	kFormType_FlameProjectile,
	kFormType_TESWorldSpace,
	kFormType_TESObjectLAND,
	kFormType_NavMesh,
	kFormType_TLOD,
	kFormType_TESTopic,
	kFormType_TESTopicInfo,
	kFormType_TESQuest,
	kFormType_TESIdleForm,
	kFormType_TESPackage,
	kFormType_TESCombatStyle,
	kFormType_TESLoadScreen,
	kFormType_TESLevSpell,
	kFormType_TESObjectANIO,
	kFormType_TESWaterForm,
	kFormType_TESEffectShader,
	kFormType_TOFT,
	kFormType_BGSExplosion,
	kFormType_BGSDebris,
	kFormType_TESImageSpace,
	kFormType_TESImageSpaceModifier,
	kFormType_BGSListForm,
	kFormType_BGSPerk,
	kFormType_BGSBodyPartData,
	kFormType_BGSAddonNode,
	kFormType_ActorValueInfo,
	kFormType_BGSRadiationStage,
	kFormType_BGSCameraShot,
	kFormType_BGSCameraPath,
	kFormType_BGSVoiceType,
	kFormType_BGSImpactData,
	kFormType_BGSImpactDataSet,
	kFormType_TESObjectARMA,
	kFormType_BGSEncounterZone,
	kFormType_BGSMessage,
	kFormType_BGSRagdoll,
	kFormType_DOBJ,
	kFormType_BGSLightingTemplate,
	kFormType_BGSMusicType,
	kFormType_TESObjectIMOD,					// Inventory object
	kFormType_TESReputation,
	kFormType_ContinuousBeamProjectile,
	kFormType_TESRecipe,
	kFormType_TESRecipeCategory,
	kFormType_TESCasinoChips,					// Inventory object
	kFormType_TESCasino,
	kFormType_TESLoadScreenType,
	kFormType_MediaSet,
	kFormType_MediaLocationController,
	kFormType_TESChallenge,
	kFormType_TESAmmoEffect,
	kFormType_TESCaravanCard,					// Inventory object
	kFormType_TESCaravanMoney,					// Inventory object
	kFormType_TESCaravanDeck,
	kFormType_BGSDehydrationStage,
	kFormType_BGSHungerStage,
	kFormType_BGSSleepDeprevationStage,
	kFormType_Max,
};

enum ObjectVtbl
{
	kVtbl_BGSTextureSet =							0x1033D1C,
	kVtbl_BGSMenuIcon =								0x1033654,
	kVtbl_TESGlobal =								0x1036524,
	kVtbl_TESClass =								0x1048BB4,
	kVtbl_TESFaction =								0x10498DC,
	kVtbl_BGSHeadPart =								0x10464B4,
	kVtbl_TESHair =									0x1049B9C,
	kVtbl_TESEyes =									0x104973C,
	kVtbl_TESRace =									0x104B4BC,
	kVtbl_TESSound =								0x1044FFC,
	kVtbl_BGSAcousticSpace =						0x10320FC,
	kVtbl_TESSkill =								0x104CC0C,
	kVtbl_EffectSetting =							0x1012834,
	kVtbl_Script =									0x1037094,
	kVtbl_TESLandTexture =							0x102E6C4,
	kVtbl_EnchantmentItem =							0x1012EA4,
	kVtbl_SpellItem =								0x1013F8C,
	kVtbl_TESObjectACTI =							0x1029D5C,
	kVtbl_BGSTalkingActivator =						0x1025594,
	kVtbl_BGSTerminal =								0x1025914,
	kVtbl_TESObjectARMO =							0x102A62C,
	kVtbl_TESObjectBOOK =							0x102A9C4,
	kVtbl_TESObjectCLOT =							0x102AC0C,
	kVtbl_TESObjectCONT =							0x102AEB4,
	kVtbl_TESObjectDOOR =							0x102B1FC,
	kVtbl_IngredientItem =							0x1013284,
	kVtbl_TESObjectLIGH =							0x1028EE4,
	kVtbl_TESObjectMISC =							0x102B844,
	kVtbl_TESObjectSTAT =							0x102BA2C,
	kVtbl_BGSStaticCollection =						0x102535C,
	kVtbl_BGSMovableStatic =						0x1024CEC,	//	Has an irregular structure; the enumed vtbl is the "effective" one used at runtime; actual vtbl is 0x1024E84
	kVtbl_BGSPlaceableWater =						0x1024F4C,
	kVtbl_TESGrass =								0x102814C,
	kVtbl_TESObjectTREE =							0x102BC94,
	kVtbl_TESFlora =								0x1026BD0,
	kVtbl_TESFurniture =							0x1026D0C,
	kVtbl_TESObjectWEAP =							0x102C51C,
	kVtbl_TESAmmo =									0x1026064,
	kVtbl_TESNPC =									0x104A2F4,
	kVtbl_TESCreature =								0x1048F5C,
	kVtbl_TESLevCreature =							0x102886C,
	kVtbl_TESLevCharacter =							0x102864C,
	kVtbl_TESKey =									0x1028444,
	kVtbl_AlchemyItem =								0x1011964,
	kVtbl_BGSIdleMarker =							0x104664C,
	kVtbl_BGSNote =									0x1046874,
	kVtbl_BGSConstructibleObject =					0x10245A4,
	kVtbl_BGSProjectile =							0x10251AC,
	kVtbl_TESLevItem =								0x1028A64,
	kVtbl_TESWeather =								0x103168C,
	kVtbl_TESClimate =								0x102D5C4,
	kVtbl_TESRegion =								0x102397C,
	kVtbl_NavMeshInfoMap =							0x106BB8C,
	kVtbl_TESObjectCELL =							0x102E9B4,
	kVtbl_TESObjectREFR =							0x102F55C,
	kVtbl_Character =								0x1086A6C,
	kVtbl_Creature =								0x10870AC,
	kVtbl_MissileProjectile =						0x108FA44,
	kVtbl_GrenadeProjectile =						0x108F674,
	kVtbl_BeamProjectile =							0x108C3C4,
	kVtbl_FlameProjectile =							0x108F2F4,
	kVtbl_Explosion =								0x108EE04,
	kVtbl_TESWorldSpace =							0x103195C,
	kVtbl_TESObjectLAND =							0x102DCD4,
	kVtbl_NavMesh =									0x106A0B4,
	kVtbl_TESTopic =								0x104D19C,
	kVtbl_TESTopicInfo =							0x104D5B4,
	kVtbl_TESQuest =								0x104AC44,
	kVtbl_TESIdleForm =								0x1049D0C,
	kVtbl_TESPackage =								0x106847C,
	kVtbl_TESCombatStyle =							0x10266E4,
	kVtbl_TESLoadScreen =							0x10366CC,
	kVtbl_TESLevSpell =								0x1028C5C,
	kVtbl_TESObjectANIO =							0x102A0A4,
	kVtbl_TESWaterForm =							0x103140C,
	kVtbl_TESEffectShader =							0x102685C,
	kVtbl_BGSExplosion =							0x1024A94,
	kVtbl_BGSDebris =								0x1024834,
	kVtbl_TESImageSpace =							0x102D7F4,
	kVtbl_TESImageSpaceModifier =					0x102D97C,
	kVtbl_BGSListForm =								0x10334B4,
	kVtbl_BGSPerk =									0x1046EC4,
	kVtbl_BGSBodyPartData =							0x1045504,
	kVtbl_BGSAddonNode =							0x1024214,
	kVtbl_ActorValueInfo =							0x1067A2C,
	kVtbl_BGSRadiationStage =						0x1033B34,
	kVtbl_BGSCameraShot =							0x10327F4,
	kVtbl_BGSCameraPath =							0x103245C,
	kVtbl_BGSVoiceType =							0x104733C,
	kVtbl_BGSImpactData =							0x1032F6C,
	kVtbl_BGSImpactDataSet =						0x103323C,
	kVtbl_TESObjectARMA =							0x102A31C,
	kVtbl_BGSEncounterZone =						0x102CBBC,
	kVtbl_BGSMessage =								0x10337C4,
	kVtbl_BGSRagdoll =								0x10470EC,
	kVtbl_BGSLightingTemplate =						0x102CD94,
	kVtbl_BGSMusicType =							0x103397C,
	kVtbl_TESObjectIMOD =							0x102B5AC,
	kVtbl_TESReputation =							0x104BA24,
	kVtbl_ContinuousBeamProjectile =				0x108EA64,
	kVtbl_TESRecipe =								0x1036B2C,
	kVtbl_TESRecipeCategory =						0x10369DC,
	kVtbl_TESCasinoChips =							0x10263DC,
	kVtbl_TESCasino =								0x1026574,
	kVtbl_TESLoadScreenType =						0x1036854,
	kVtbl_MediaSet =								0x10342EC,
	kVtbl_MediaLocationController =					0x10340C4,
	kVtbl_TESChallenge =							0x104891C,
	kVtbl_TESAmmoEffect =							0x103449C,
	kVtbl_TESCaravanCard =							0x103478C,
	kVtbl_TESCaravanMoney =							0x10349B4,
	kVtbl_TESCaravanDeck =							0x1034B4C,
	kVtbl_BGSDehydrationStage =						0x101144C,
	kVtbl_BGSHungerStage =							0x10115B4,
	kVtbl_BGSSleepDeprevationStage =				0x10116FC,
	kVtbl_PlayerCharacter =							0x108AA3C,

	kVtbl_BGSQuestObjective =						0x1047088,
	kVtbl_TESModelTextureSwap =						0x101D124,
	kVtbl_BGSPrimitiveBox =							0x101E8C4,
	kVtbl_BGSPrimitiveSphere =						0x101EA64,
	kVtbl_BGSPrimitivePlane =						0x101E75C,
	kVtbl_MagicShaderHitEffect =					0x107B70C,

	kVtbl_BGSQuestPerkEntry =						0x1046B84,
	kVtbl_BGSAbilityPerkEntry =						0x1046C44,
	kVtbl_BGSEntryPointPerkEntry =					0x1046D0C,
	kVtbl_BGSEntryPointFunctionDataOneValue =		0x10462C0,
	kVtbl_BGSEntryPointFunctionDataTwoValue =		0x1046300,
	kVtbl_BGSEntryPointFunctionDataLeveledList =	0x1046320,
	kVtbl_BGSEntryPointFunctionDataActivateChoice =	0x1046340,

	kVtbl_ExtraDataList =							0x10143E8,
	kVtbl_ExtraSeenData =							0x1014294,
	kVtbl_ExtraSpecialRenderFlags =					0x1014458,
	kVtbl_ExtraPrimitive =							0x10151B4,
	kVtbl_ExtraLinkedRef =							0x1015CC0,
	kVtbl_ExtraRadius =								0x1015208,
	kVtbl_ExtraCellWaterType =						0x1014270,
	kVtbl_ExtraCellImageSpace =						0x1014258,
	kVtbl_ExtraCellMusicType =						0x1014234,
	kVtbl_ExtraCellClimate =						0x101424C,
	kVtbl_ExtraTerminalState =						0x1015190,
	kVtbl_ExtraCellAcousticSpace =					0x1014240,
	kVtbl_ExtraOriginalReference =					0x1015BC4,
	kVtbl_ExtraContainerChanges =					0x1015BB8,
	kVtbl_ExtraWorn =								0x1015BDC,
	kVtbl_ExtraHealth =								0x10158E4,
	kVtbl_ExtraLock =								0x101589C,
	kVtbl_ExtraCount =								0x10158D8,
	kVtbl_ExtraTeleport =							0x10158A8,
	kVtbl_ExtraWeaponModFlags =						0x10159A4,
	kVtbl_ExtraHotkey =								0x101592C,
	kVtbl_ExtraCannotWear =							0x1015BF4,
	kVtbl_ExtraOwnership =							0x10158B4,
	kVtbl_ExtraRank =								0x10158CC,
	kVtbl_ExtraAction =								0x1015BAC,
	kVtbl_ExtraFactionChanges =						0x1015F30,
	kVtbl_ExtraScript =								0x1015914,
	kVtbl_ExtraObjectHealth =						0x1015184,
	kVtbl_ExtraStartingPosition =					0x1015B40,
	kVtbl_ExtraPoison =								0x101595C,
	kVtbl_ExtraCharge =								0x1015908,
	kVtbl_ExtraRadiation =							0x1015214,
	kVtbl_ExtraNorthRotation =						0x10142A0,
	kVtbl_ExtraTimeLeft =							0x10158FC,
	kVtbl_ExtraUses =								0x10158F0,

	kVtbl_SeenData =								0x1083FC4,
	kVtbl_IntSeenData =								0x1083FE4,

	kVtbl_TileMenu =								0x106ED44,

	kVtbl_MessageMenu =								0x107566C,
	kVtbl_InventoryMenu =							0x10739B4,
	kVtbl_StatsMenu =								0x106FFD4,
	kVtbl_HUDMainMenu =								0x1072DF4,
	kVtbl_LoadingMenu =								0x1073EBC,
	kVtbl_ContainerMenu =							0x10721AC,
	kVtbl_DialogMenu =								0x107257C,
	kVtbl_SleepWaitMenu =							0x10763AC,
	kVtbl_StartMenu =								0x1076D1C,
	kVtbl_LockpickMenu =							0x107439C,
	kVtbl_QuantityMenu =							0x10701C4,
	kVtbl_MapMenu =									0x1074D44,
	kVtbl_LevelUpMenu =								0x1073CDC,
	kVtbl_RepairMenu =								0x1075C5C,
	kVtbl_RaceSexMenu =								0x1075974,
	kVtbl_CharGenMenu =								0x1071BB4,
	kVtbl_TextEditMenu =							0x1070034,
	kVtbl_BarterMenu =								0x10706EC,
	kVtbl_SurgeryMenu =								0x1070084,
	kVtbl_HackingMenu =								0x10728F4,
	kVtbl_VATSMenu =								0x10700D4,
	kVtbl_ComputersMenu =							0x1072004,
	kVtbl_RepairServicesMenu =						0x1075DB4,
	kVtbl_TutorialMenu =							0x106FF84,
	kVtbl_SpecialBookMenu =							0x1070124,
	kVtbl_ItemModMenu =								0x1073B7C,
	kVtbl_LoveTesterMenu =							0x1070174,
	kVtbl_CompanionWheelMenu =						0x1071D0C,
	kVtbl_TraitSelectMenu =							0x1077ABC,
	kVtbl_RecipeMenu =								0x107048C,
	kVtbl_SlotMachineMenu =							0x10764DC,
	kVtbl_BlackjackMenu =							0x10708FC,
	kVtbl_RouletteMenu =							0x1075F7C,
	kVtbl_CaravanMenu =								0x107108C,
	kVtbl_TraitMenu =								0x10779BC,

	kVtbl_NiNode =									0x109B5AC,
	kVtbl_BSFadeNode =								0x10A8F90,
	kVtbl_NiTriShape =								0x109D454,
	kVtbl_NiTriStrips =								0x109CD44,
	kVtbl_NiControllerManager =						0x109619C,
	kVtbl_BSScissorTriShape =						0x10C2E7C,
	kVtbl_NiPointLight =							0x109DD0C,
	kVtbl_NiAlphaProperty =							0x10162DC,
	kVtbl_NiMaterialProperty =						0x109D6C4,
	kVtbl_NiStencilProperty =						0x101E07C,
	kVtbl_TileShaderProperty =						0x10B9D28,
	kVtbl_WaterShaderProperty =						0x10AE438,
	kVtbl_BSShaderNoLightingProperty =				0x10AE670,
	kVtbl_BSShaderPPLightingProperty =				0x10AE0D0,
	kVtbl_SkyShaderProperty =						0x10B8CE0,

	kVtbl_ImageSpaceModifierInstanceForm =			0x102D12C,

	kVtbl_hkpAabbPhantom =							0x10CC004,
	kVtbl_hkpSimpleShapePhantom =					0x10CE15C,
	kVtbl_hkpCachingShapePhantom =					0x10D087C,
	kVtbl_hkpRigidBody =							0x10C7888,
	kVtbl_hkpSphereMotion =							0x10C6D54,
	kVtbl_hkpBoxMotion =							0x10C6DC4,
	kVtbl_hkpThinBoxMotion =						0x10C6E34,
	kVtbl_ahkpCharacterProxy =						0x10C83E8,

	kVtbl_StartMenuOption =							0x1076EE0,
	kVtbl_StartMenuUserOption =						0x107704C,
};

#define IS_TYPE(form, type) (*(UInt32*)form == kVtbl_##type)
#define NOT_TYPE(form, type) (*(UInt32*)form != kVtbl_##type)

#define IS_ID(form, type) (form->typeID == kFormType_##type)
#define NOT_ID(form, type) (form->typeID != kFormType_##type)

/**** bases ****/

class BaseFormComponent
{
public:
	BaseFormComponent();
	~BaseFormComponent();

	virtual void	Init(void);
	virtual void	Free(void);
	virtual void	CopyFromBase(BaseFormComponent * component);
	virtual bool	CompareWithBase(BaseFormComponent * src);

//	void		** _vtbl;	// 000
};

struct PermanentClonedForm
{
	UInt32 orgRefID;
	UInt32 cloneRefID;
};

// 018
class TESForm : public BaseFormComponent
{
public:
	TESForm();
	~TESForm();

	virtual void *		Destroy(bool noDealloc);			// func_00C in GECK ?? I think ??
	virtual void		Unk_05(void);						// Might be set default value (called from constructor)
	virtual UInt32		Unk_06(void);
	virtual bool		Unk_07(void);
	virtual bool		LoadForm(ModInfo * modInfo);		// func_010 in GECK
	virtual bool		Unk_09(void * arg);					// points to LoadForm on TESForm
	virtual bool		AppendForm(ModInfo* modInfo);		// (ie SaveForm + append to modInfo)
	virtual void		SaveForm(void);						// saves in same format as in .esp	//	func_013 in GECK
															// data buffer and buffer size stored in globals when done, doesn't return anything
	virtual bool		LoadForm2(ModInfo * modInfo);		// just calls LoadForm
	virtual void		WriteFormInfo(ModInfo* modInfo);	// does some saving stuff, then calls Fn0A
	virtual bool		Unk_0E(void * arg);					// prapares a GRUP formInfo
	virtual bool		Sort(TESForm * form);				// returns if the argument is "greater or equal" to this form
	virtual TESForm *	CreateForm(void * arg0, void * mapToAddTo);	// makes a new form, 
	virtual void		Unk_11(void * arg);
	virtual void		MarkAsModified(UInt32 changedFlags);		// enable changed flag?
	virtual void		MarkAsUnmodified(UInt32 changedFlags);		// disable changed flag?
	virtual UInt32		GetSaveSize(UInt32 changedFlags);	// bytes taken by the delta flags for this form, UNRELIABLE, not (always) overriden
	virtual void		Unk_15(void * arg);					// collect referenced forms?
	virtual void		SaveGame(UInt32 changedFlags);		// Used as part of CopyFromBase with LoadGame.
	virtual void		LoadGame(void * arg);				// load from BGSLoadFormBuffer arg
	virtual void		LoadGame2(UInt32 changedFlags);		// load from TESSaveLoadGame
	virtual void		Unk_19(void * arg);
	virtual void		Unk_1A(void * arg0, void * arg1);
	virtual void		Unk_1B(void * arg0, void * arg1);
	virtual void		Revert(UInt32 changedFlags);		// reset changes in form
	virtual void		Unk_1D(void * arg);
	virtual void		Unk_1E(void * arg);
	virtual bool		Unk_1F(void * arg);
	virtual void		Unk_20(void * arg);
	virtual void		Unk_21(void * arg);
	virtual void		InitItem(void);
	virtual UInt32		GetTypeID(void);
	virtual void		GetDebugName(String * dst);
	virtual bool		IsQuestItem(void);
										// Unk_26 though Unk_36 get or set flag bits
	virtual bool		HasTalkedToPC(void);		// 00000040
	virtual bool		Unk_27(void);		// 00010000
	virtual bool		Unk_28(void);		// 00010000
	virtual bool		Unk_29(void);		// 00020000
	virtual bool		Unk_2A(void);		// 00020000
	virtual bool		Unk_2B(void);		// 00080000
	virtual bool		Unk_2C(void);		// 02000000
	virtual bool		Unk_2D(void);		// 40000000
	virtual bool		Unk_2E(void);		// 00000200
	virtual void		Unk_2F(bool set);	// 00000200
	virtual bool		Unk_30(void);		// returns false
	virtual void		Unk_31(bool set);	// 00000020 then calls Fn12 MarkAsModified
	virtual void		Unk_32(bool set);	// 00000002 with a lot of housekeeping
	virtual void		SetQuestItem(bool set);	// 00000400 then calls Fn12 MarkAsModified
	virtual void		Unk_34(bool set);	// 00000040 then calls Fn12 MarkAsModified
	virtual void		Unk_35(bool set);	// 00010000 then calls Fn12 MarkAsModified
	virtual void		Unk_36(bool set);	// 00020000
	virtual void		Unk_37(void);		// write esp format
	virtual void		readOBNDSubRecord(ModInfo * modInfo);	// read esp format
	virtual bool		Unk_39(void);
	virtual bool		IsBoundObject();
	virtual bool		Unk_3B(void);
	virtual bool		GetIsReference();
	virtual bool		IsArmorAddon();
	virtual bool		Unk_3E(void);
	virtual bool		Unk_3F(void);	// returnTrue for refr whose baseForm is a TESActorBase
	virtual bool		IsActor(void);
	virtual UInt32		Unk_41(void);
	virtual void		CopyFrom(const TESForm * form);
	virtual bool		Compare(TESForm * form);
	virtual bool		CheckFormGRUP(void * arg);	// Checks the group is valid for the form
	virtual void		InitFormGRUP(void * dst, void * arg1);	// Fills the groupInfo with info valid for the form
	virtual bool		Unk_46(void);
	virtual bool		Unk_47(void);
	virtual bool		Unk_48(UInt32 formType);	// returns if the same FormType is passed in
	virtual bool		Unk_49(void * arg0, void * arg1, void * arg2, void * arg3, void * arg4);	// looks to be func33 in Oblivion
	virtual void		SetRefID(UInt32 refID, bool generateID);
	virtual char *		GetName2(void);
	virtual char *		GetName(void);
	virtual bool		SetEditorID(const char * edid);		// simply returns true at run-time
	// 4E
	uint32_t GetId() const
	{
		return *(uint32_t *)((uintptr_t)this + 0xC);
	}

	uint8_t GetType() const
	{
		return *(uint8_t *)((uintptr_t)this + 0x4);
	}

	const char *hk_GetName();
	bool hk_SetEditorId(const char *Name);
	bool hk_SetEditorID_REFR(const char* Name);
	struct EditorData {
		String		editorID;			// 00
		UInt32		vcMasterFormID;		// 08 - Version control 1 (looks to be a refID inside the Version Control master)
		UInt32		vcRevision;			// 0C
	};
	// 10

	enum
	{
		kFormFlags_Initialized =	0x00000008,	// set by TESForm::InitItem()
		kFormFlags_CastShadows =	0x00000200,
		kFormFlags_QuestItem =		0x00000400,
		kFormFlags_IsPermanent =	0x00000800,
		kFormFlags_DontSaveForm =	0x00004000,	// TODO: investigate
		kFormFlags_Compressed =		0x00040000,
	};

	enum
	{
		kModified_Flags =	0x00000001
		//	UInt32	flags;
	};

	UInt8		typeID;				// 004
	UInt8		jipFormFlags5;		// 005
	UInt16		jipFormFlags6;		// 006
	UInt32		flags;				// 008
	union
	{
		UInt32		refID;
		struct
		{
			UInt8	id[3];
			UInt8	modIndex;
		};
	};

#ifdef EDITOR
	EditorData	editorData;			// +10
#endif
	tList<ModInfo> mods;			// 010 ModReferenceList in Oblivion	
	// 018 / 028

	TESForm *TryGetREFRParent();
	UInt8 GetModIndex() const;
	TESFullName *GetFullName();
	const char *GetTheName();
	bool IsCloned() const;

	// adds a new form to the game (from CloneForm or LoadForm)
	void DoAddForm(TESForm* newForm, bool bPersist = true, bool record = true) const;
	// return a new base form which is the clone of this form
	TESForm *CloneForm(bool bPersist = true) const;
	bool IsInventoryObject() const;
	bool IsReference();

	bool HasScript();
	bool GetScriptAndEventList(Script **script, ScriptEventList **eventList);
	bool IsItemPlayable();
	UInt32 GetItemValue();
	float GetWeight();
	UInt8 GetOverridingModIdx();
	const char *GetDescriptionText();
	const char *RefToString();
	TESLeveledList *GetLvlList();
	void SetJIPFlag(UInt8 jipFlag, bool bSet);
	bool IsQuestItem2() { return this->flags & kFormFlags_QuestItem; };

	MEMBER_FN_PREFIX(TESForm);
	DEFINE_MEMBER_FN(MarkAsTemporary, void, 0x00484490);	// probably a member of TESForm
};
STATIC_ASSERT(sizeof(TESForm) == 0x18);

struct Condition
{
	UInt8			type;				// 00
	UInt8			pad01[3];			// 01
	union
	{
		float		value;
		UInt32		global;
	}				comparisonValue;	// 04
	UInt32			opcode;				// 08
	union
	{
		float		value;
		UInt32		number;
		TESForm		*form;
	}				parameter1;			// 0C
	union
	{
		float		value;
		UInt32		number;
		TESForm		*form;
	}				parameter2;			// 10
	UInt32			runOnType;			// 14	Subject, Target, Reference, CombatTarget, LinkedReference
	TESObjectREFR	*reference;			// 18

	bool Evaluate(TESObjectREFR *runOnRef, TESForm *arg2, bool *result);
};

struct ConditionList : tList<Condition>
{
	bool Evaluate(TESObjectREFR *runOnRef, TESForm *arg2, bool *result, bool arg4);
};

class TESObject : public TESForm
{
public:
	TESObject();
	~TESObject();

	virtual UInt32	Unk_4E(void);
	virtual bool	Unk_4F(void);
	virtual UInt32	Unk_50(void);
	virtual bool	Unk_51(void);
	virtual void	Unk_52(void * arg);
	virtual NiNode	*_CreateNiNode(TESObjectREFR *refr, bool arg1);
	virtual void	Unk_54(void * arg);
	virtual bool	IsInternal(void);
	virtual bool	IsInternalMarker(void);
	virtual void	Unk_57(void);
	virtual bool	Unk_58(void);	// BoundObject: Calls Unk_5F on the object model
	virtual bool	Unk_59(void * arg);
	virtual void	Unk_5A(void * arg0, void * arg1);
	virtual UInt32	Unk_5B(void);
	virtual UInt32	Unk_5C(void);
	virtual bool	Unk_5D(TESObjectREFR * refr);	// if false, no NiNode gets returned by Unk_53, true for NPC
};

// 30
class TESBoundObject : public TESObject
{
public:
	TESBoundObject();
	~TESBoundObject();

	virtual NiNode	*CreateNiNode(TESObjectREFR * refr);	// calls Fn53, for NPC calls ReadBones, for LevelledActor level them if necessary
	virtual bool	Unk_5F(void);

	BoundObjectListHead		*head;		// 018
	TESBoundObject			*prev;		// 01C
	TESBoundObject			*next;		// 020
	SInt16					bounds[6];	// 024
};

// C
class TESFullName : public BaseFormComponent
{
public:
	TESFullName();
	~TESFullName();

	String	name;		// 004
};

// 0C
class TESTexture : public BaseFormComponent
{
public:
	TESTexture();
	~TESTexture();

	virtual UInt32	Unk_04(void);
	virtual void	GetNormalMap(String *str);
	virtual char	*GetPathRoot(void);

	String		ddsPath;	// 04
};

// 0C
class TESTexture1024 : public TESTexture
{
public:
	TESTexture1024();
	~TESTexture1024();
};

// 00C
class TESIcon : public TESTexture
{
public:
	TESIcon();
	~TESIcon();

	void SetPath(const char* newPath)	{ ddsPath.Set(newPath); }
};

// C
class TESScriptableForm : public BaseFormComponent
{
public:
	TESScriptableForm();
	~TESScriptableForm();

	Script	* script;	// 004
	bool	resolved;	// 008	called during LoadForm, so scripts do not wait for TESForm_InitItem to be resolved
	UInt8	pad[3];		// 009
};

// 010
class BGSMessageIcon : public BaseFormComponent
{
public:
	BGSMessageIcon();
	~BGSMessageIcon();

	TESIcon	icon;		// 004
};

// 008
class TESValueForm : public BaseFormComponent
{
public:
	enum
	{
		kModified_GoldValue =	0x00000008,
		// UInt32	value
	};

	TESValueForm();
	~TESValueForm();

	virtual UInt32	GetSaveSize(UInt32 changedFlags);
	virtual void	Save(UInt32 changedFlags);
	virtual void	Load(UInt32 changedFlags);

//	DEFINE_MEMBER_FN_LONG(TESValueForm, SetValue, void, _TESValueForm_SetValue, UInt32 newVal);

	UInt32	value;
	// 008
};

// 10
class TESEnchantableForm : public BaseFormComponent
{
public:
	TESEnchantableForm();
	~TESEnchantableForm();

	virtual UInt32	Unk_04(void);	// returns unk2

	EnchantmentItem* enchantItem;	// 04
	UInt16	enchantment;			// 08
	UInt16	unk1;					// 0A
	UInt32	unk2;					// 0C
	// 010
};

// 08
class TESImageSpaceModifiableForm : public BaseFormComponent
{
public:
	TESImageSpaceModifiableForm();
	~TESImageSpaceModifiableForm();

	UInt32	unk04;	// 04
};

// 008
class TESWeightForm : public BaseFormComponent
{
public:
	TESWeightForm();
	~TESWeightForm();

	float	weight;		// 004
	// 008
};

// 008
class TESHealthForm : public BaseFormComponent
{
public:
	TESHealthForm();
	~TESHealthForm();

	virtual UInt32	GetHealth(void);	// 0004

	UInt32	health;		// 004
};

// 008
class TESAttackDamageForm : public BaseFormComponent
{
public:
	TESAttackDamageForm();
	~TESAttackDamageForm();

	virtual UInt16	GetDamage(void);

	UInt16	damage;	// 04
	UInt16	unk0;	// 06 - bitmask? perhaps 2 UInt8s?
	// 008
};

// 24
class EffectItem
{
public:
	EffectItem();
	~EffectItem();

	enum
	{
		kRange_Self = 0,
		kRange_Touch,
		kRange_Target,
	};

	struct ScriptEffectInfo
	{
		UInt32 scriptRefID;
		UInt32 school;
		String effectName;
		UInt32 visualEffectCode;
		UInt32 isHostile;

		void SetName(const char* name);
		void SetSchool(UInt32 school);
		void SetVisualEffectCode(UInt32 code);
		void SetIsHostile(bool bIsHostile);
		bool IsHostile() const;
		void SetScriptRefID(UInt32 refID);

		ScriptEffectInfo* Clone() const;
		void CopyFrom(const ScriptEffectInfo* from);
		static ScriptEffectInfo* Create();
	};

	// mising flags
	UInt32				magnitude;			// 00	used as a float
	UInt32				area;				// 04
	UInt32				duration;			// 08
	UInt32				range;				// 0C
	UInt32				actorValueOrOther;	// 10
	EffectSetting		*setting;			// 14
	float				cost;				// 18 on autocalc items this seems to be the cost
	ConditionList		conditions;			// 1C

	int GetSkillCode();
};

// 10
class EffectItemList : public BSSimpleList<EffectItem>
{
public:
	EffectItemList();
	~EffectItemList();

	UInt32		unk0C;			// 0C

	bool RemoveNthEffect(UInt32 index);
};

STATIC_ASSERT(sizeof(EffectItemList) == 0x10);

// 1C
class MagicItem : public TESFullName
{
public:
	MagicItem();
	~MagicItem();

	virtual void	Unk_04(void); // pure virtual
	virtual void	Unk_05(void); // pure virtual
	virtual UInt32	GetType();
	virtual bool	Unk_07(void);
	virtual bool	Unk_08(void);
	virtual void	Unk_09(void); // pure virtual
	virtual void	Unk_0A(void); // pure virtual
	virtual void	Unk_0B(void); // pure virtual
	virtual void	Unk_0C(void); // pure virtual
	virtual void	Unk_0D(void); // pure virtual
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void); // pure virtual

	EffectItemList	list;	// 00C
//	UInt32	unk018;			// 018
	// perhaps types are no longer correct!
	enum EType{
		kType_None = 0,
		kType_Spell = 1,
		kType_Enchantment = 2,
		kType_Alchemy = 3,
		kType_Ingredient = 4,
	};
	EType Type() const;
};

STATIC_ASSERT(sizeof(MagicItem) == 0x1C);

// 034
class MagicItemForm : public TESForm
{
public:
	MagicItemForm();
	~MagicItemForm();

	virtual void	ByteSwap(void); // pure virtual

	// base
	MagicItem	magicItem;	// 018
};

STATIC_ASSERT(sizeof(MagicItemForm) == 0x34);

// 18
class TESModel : public BaseFormComponent
{
public:
	TESModel();
	~TESModel();

	enum
	{
		kFacegenFlag_Head =			0x01,
		kFacegenFlag_Torso =		0x02,
		kFacegenFlag_RightHand =	0x04,
		kFacegenFlag_LeftHand =		0x08,
	};

	virtual void	*Destroy(bool noDealloc);	// 04
	virtual const char	*GetModelPath();
	virtual void	SetModelPath(const char *path);	// 06

	String	nifPath;		// 04
	UInt32	unk0C;			// 0C	referenced when saving Texture Hashes, init'd as a byte or is it a pointer to a structure starting with a byte followed by a pointer to some allocated data ?
	void	* unk10;		// 10
	UInt8	facegenFlags;	// 14
	UInt8	pad15[3];		// 15

	void SetPath(const char* newPath)	{ nifPath.Set(newPath); }

};

// 18
class BGSTextureModel : public TESModel
{
public:
	BGSTextureModel();
	~BGSTextureModel();
};

// 020
class TESModelTextureSwap : public TESModel
{
public:
	TESModelTextureSwap();
	~TESModelTextureSwap();

	struct Texture 
	{
		UInt32	textureID;			// 00
		UInt32	index3D;			// 04
		char	textureName[0x80];	// 08
	};	// there seem to be an array (length 6) post 0x88

	virtual void *	Destroy(bool noDealloc);
	virtual char *	GetPath(void);
	virtual void	SetPath(char * path);
	virtual void *	Unk_07(void);

	tList<Texture> textureList;	// 018
};

// 008
class BGSClipRoundsForm : public BaseFormComponent
{
public:
	BGSClipRoundsForm();
	~BGSClipRoundsForm();

	UInt8	clipRounds;
	UInt8	padding[3];
	// 008
};

// 18
struct DestructionStage
{
	enum
	{
		kFlags_CapDamage =		1,
		kFlags_DisableObject =	2,
		kFlags_DestroyObject =	4,
	};

	UInt8				dmgStage;		// 00
	UInt8				healthPrc;		// 01
	UInt16				flags;			// 02
	UInt32				selfDmgSec;		// 04
	BGSExplosion		*explosion;		// 08
	BGSDebris			*debris;		// 0C
	UInt32				debrisCount;	// 10
	TESModelTextureSwap	*replacement;	// 14
};

// 14
struct DestructibleData
{
	UInt32				health;		// 00
	UInt8				stageCount;	// 04
	bool				targetable;	// 05
	UInt8				unk06[2];	// 06
	DestructionStage	**stages;	// 08
	UInt32				unk0C;		// 0C
	UInt32				unk10;		// 10
};

// 08
class BGSDestructibleObjectForm : public BaseFormComponent
{
public:
	BGSDestructibleObjectForm();
	~BGSDestructibleObjectForm();

	DestructibleData	*data;			// 04
};

STATIC_ASSERT(sizeof(BGSDestructibleObjectForm) == 0x8);

// 00C
class BGSPickupPutdownSounds : public BaseFormComponent
{
public:
	BGSPickupPutdownSounds();
	~BGSPickupPutdownSounds();

	TESSound	* pickupSound;		// 004
	TESSound	* putdownSound;		// 008
};

// 008
class BGSAmmoForm : public BaseFormComponent
{
public:
	BGSAmmoForm();
	~BGSAmmoForm();

	TESForm* ammo; // 04	either TESAmmo or BGSListForm
};

// 008
class BGSRepairItemList : public BaseFormComponent
{
public:
	BGSRepairItemList();
	~BGSRepairItemList();

	BGSListForm	* listForm;	// 04
};

// 008
class BGSEquipType : public BaseFormComponent
{
public:
	BGSEquipType();
	~BGSEquipType();

	enum
	{
		kBigGuns = 0,
		kEnergyWeapons = 1,
		kSmallGuns = 2,
		kMeleeWeapons = 3,
		kUnarmedWeapons = 4,
		kThrowWeapons = 5,
		kMine = 6,
		kBodyWear = 7,
		kHeadWear = 8,
		kHandWear = 9,
		kChems = 10,
		kStimpak = 11,
		kFood = 12,
		kAlcohol = 13
	};

	UInt32	equipType;	// 08
};

// 004
class BGSPreloadable : public BaseFormComponent
{
public:
	BGSPreloadable();
	~BGSPreloadable();

	virtual void	Fn_04(void); // pure virtual
};

// 008
class BGSBipedModelList : public BaseFormComponent
{
public:
	BGSBipedModelList();
	~BGSBipedModelList();
	
	BGSListForm*    models;		// 004
	// 008
};

// 018
class TESModelRDT : public TESModel
{
public:
	TESModelRDT();
	~TESModelRDT();

	virtual UInt32	Fn_07(void);
};

// 0DC
class TESBipedModelForm : public BaseFormComponent
{
public:
	TESBipedModelForm();
	~TESBipedModelForm();

	// bit indices starting from lsb
	enum EPartBit {
		ePart_Head = 0,
		ePart_Hair,
		ePart_UpperBody,
		ePart_LeftHand,
		ePart_RightHand,
		ePart_Weapon,
		ePart_PipBoy,
		ePart_Backpack,
		ePart_Necklace,
		ePart_Headband,
		ePart_Hat,
		ePart_Eyeglasses,
		ePart_Nosering,
		ePart_Earrings,
		ePart_Mask,
		ePart_Choker,
		ePart_MouthObject,
		ePart_BodyAddon1,
		ePart_BodyAddon2,
		ePart_BodyAddon3
	};

	enum EPartBitMask {
		ePartBitMask_Full = 0x07FFFF,
	};

	enum ESlot {
		eSlot_Head =		0x1 << ePart_Head,
		eSlot_Hair =		0x1 << ePart_Hair,
		eSlot_UpperBody =	0x1 << ePart_UpperBody,
		eSlot_LeftHand =	0x1 << ePart_LeftHand,
		eSlot_RightHand =	0x1 << ePart_RightHand,
		eSlot_Weapon =		0x1 << ePart_Weapon,
		eSlot_PipBoy =		0x1 << ePart_PipBoy,
		eSlot_Backpack =	0x1 << ePart_Backpack,
		eSlot_Necklace =	0x1 << ePart_Necklace,
		eSlot_Headband =	0x1 << ePart_Headband,
		eSlot_Hat =			0x1 << ePart_Hat,
		eSlot_Eyeglasses =	0x1 << ePart_Eyeglasses,
		eSlot_Nosering =	0x1 << ePart_Nosering,
		eSlot_Earrings =	0x1 << ePart_Earrings,
		eSlot_Mask =		0x1 << ePart_Mask,
		eSlot_Choker =		0x1 << ePart_Choker,
		eSlot_MouthObject=	0x1 << ePart_MouthObject,
		eSlot_BodyAddon1 =	0x1 << ePart_BodyAddon1,
		eSlot_BodyAddon2 =	0x1 << ePart_BodyAddon2,
		eSlot_BodyAddon3 =	0x1 << ePart_BodyAddon3
	};

	enum EBipedFlags {
		eBipedFlag_HasBackPack	= 0x4,
		eBipedFlag_MediumArmor	= 0x8,
		eBipedFlag_PowerArmor	= 0x20,
		eBipedFlag_NonPlayable	= 0x40,
		eBipedFlag_HeavyArmor	= 0x80,
	};

	enum EBipedPath {
		ePath_Biped,
		ePath_Ground,
		ePath_Icon,
		ePath_Max
	};

	// missing part mask and flags
	UInt32					partMask;			// 004
	UInt32					bipedFlags;			// 008
	TESModelTextureSwap		bipedModel[2];		// 00C
	TESModelTextureSwap		groundModel[2];		// 04C
	TESIcon					icon[2];			// 08C
	BGSMessageIcon			messageIcon[2];		// 0A4
	TESModelRDT				modelRDT;			// 0C4
	// 0DC

	static UInt32 MaskForSlot(UInt32 mask);

	bool IsPowerArmor() const { return (bipedFlags & eBipedFlag_PowerArmor) == eBipedFlag_PowerArmor; }
	bool IsNonPlayable() const { return (bipedFlags & eBipedFlag_NonPlayable) == eBipedFlag_NonPlayable; }
	bool IsPlayable() const { return !IsNonPlayable(); }
	void SetPlayable(bool doset) { if (doset) bipedFlags &= ~eBipedFlag_NonPlayable; else bipedFlags |= eBipedFlag_NonPlayable; }
	void SetPowerArmor(bool bPA) {
		if (bPA) {
			bipedFlags |= eBipedFlag_PowerArmor;
		} else {
			bipedFlags &= ~eBipedFlag_PowerArmor;
		}
	}
	void SetNonPlayable(bool bNP) {
		if (bNP) {
			bipedFlags |= eBipedFlag_NonPlayable;
		} else {
			bipedFlags &= ~eBipedFlag_NonPlayable;
		}
	}
	bool IsHelmet() const { return partMask & (TESBipedModelForm::ePart_Hat | TESBipedModelForm::ePart_Head | TESBipedModelForm::ePart_Hair); }
	void  SetPath(const char* newPath, UInt32 whichPath, bool bfemalePath);
	const char* GetPath(UInt32 whichPath, bool bFemalePath);

	UInt32 GetSlotsMask() const;
	void SetSlotsMask(UInt32 mask);	// Limited by ePartBitMask_Full

	UInt32 GetBipedMask() const;
	void SetBipedMask(UInt32 mask);
};

STATIC_ASSERT(sizeof(TESBipedModelForm) == 0x0DC);

// 30
class TESBoundAnimObject : public TESBoundObject
{
public:
	TESBoundAnimObject();
	~TESBoundAnimObject();
};

// 0C
struct LvlListExtra
{
	union						// 00
	{
		TESFaction	*ownerFaction;
		TESNPC		*ownerNPC;
	};
	union						// 04
	{
		UInt32		requiredRank;
		TESGlobal	*globalVar;
	};
	float			health;		// 08
};

// 0C
class TESContainer : public BaseFormComponent
{
public:
	TESContainer();
	~TESContainer();

	struct FormCount
	{
		SInt32				count;			//	00
		TESForm				*form;			//	04
		LvlListExtra		*contExtraData;	//	08
	};
	typedef tList<FormCount> FormCountList;

	FormCountList	formCountList;	// 04

	SInt32 GetCountForForm(TESForm *form);
};

// 00C
class BGSTouchSpellForm : public BaseFormComponent
{
public:
	BGSTouchSpellForm();
	~BGSTouchSpellForm();

	TESForm	*unarmedEffect;	// 04
	UInt16	unarmedAnim;	// 08
	UInt16	pad0A;			// 0A
};

struct FactionListData
{
	TESFaction	*faction;
	SInt8		rank;
	UInt8		pad[3];
};

// 034
class TESActorBaseData : public BaseFormComponent
{
public:
	TESActorBaseData();
	~TESActorBaseData();

	virtual void			Fn_04(TESForm * selectedForm);	// Called during form initialization after LoadForm and InitForm
	// flags access
	virtual bool			Fn_05(void);	// 00100000
	virtual bool			Fn_06(void);	// 00200000
	virtual bool			Fn_07(void);	// 10000000
	virtual bool			Fn_08(void);	// 20000000
	virtual bool			Fn_09(void);	// 80000000
	virtual bool			Fn_0A(void);	// 00400000
	virtual bool			Fn_0B(void);	// 00400000
	virtual bool			Fn_0C(void);	// 00800000
	virtual bool			Fn_0D(void);
	virtual bool			Fn_0E(void);
	virtual bool			Fn_0F(void);
	virtual bool			Fn_10(void);
	virtual bool			Fn_11(void);
	virtual bool			Fn_12(void);
	virtual void			Fn_13(void * arg);
	virtual bool			Fn_14(void);
	virtual void			Fn_15(void * arg);
	virtual UInt32			Fn_16(void);
	virtual void			Fn_17(void * arg);
	virtual UInt32			Fn_18(void);	// return unk08
	virtual float			Fn_19(void);	// return unk14
	virtual BGSVoiceType *	GetVoiceType(void);

	enum
	{
		kFlags_Female =					1 << 0,
		kFlags_Essential =				1 << 1,
		kFlags_HasCharGenFace =			1 << 2,
		kFlags_Respawn =				1 << 3,
		kFlags_AutoCalcStats =			1 << 4,
		//								1 << 5,
		//								1 << 6,
		kFlags_PCLevelMult =			1 << 7,
		kFlags_UseTemplate =			1 << 8,
		kFlags_NoLowLevelProcessing =	1 << 9,
		//								1 << 10,
		kFlags_NoBloodSpray =			1 << 11,
		kFlags_NoBloodDecal =			1 << 12,
		//								1 << 13,
		//								1 << 14,
		//								1 << 15,
		//								1 << 16,
		//								1 << 17,
		//								1 << 18,
		//								1 << 19,
		kFlags_NoVATSMelee =			1 << 20,
		//								1 << 21,
		kFlags_CanBeAllRaces =			1 << 22,
		//								1 << 23,
		//								1 << 24,
		//								1 << 25,
		kFlags_NoKnockdowns =			1 << 26,
		kFlags_NotPushable =			1 << 27,
		//								1 << 28,
		//								1 << 29,
		kFlags_NoRotateToHeadTrack =	1 << 30,
		//								1 << 31,
	};

	enum TemplateFlags
	{
		kTemplFlag_UseTraits =		1 << 0,
		kTemplFlag_UseStats =		1 << 1,
		kTemplFlag_UseFactions =	1 << 2,
		kTemplFlag_UseEffectList =	1 << 3,
		kTemplFlag_UseAIData =		1 << 4,
		kTemplFlag_UseAIPackages =	1 << 5,
		kTemplFlag_UseModel =		1 << 6,
		kTemplFlag_UseBaseData =	1 << 7,
		kTemplFlag_UseInventory =	1 << 8,
		kTemplFlag_UseScript =		1 << 9,
	};

	UInt32			flags;				// 04	Comparing with LoadForm and FNVEdit
	UInt16			fatigue;			// 08	Fatique
	UInt16			barterGold;			// 0A	Barter Gold
	SInt16			level;				// 0C	Level/ Level Mult
	UInt16			calcMin;			// 0E	Calc min
	UInt16			calcMax;			// 10	Calc max
	UInt16			speedMultiplier;	// 12	Speed Multiplier (confirmed)
	float			karma;				// 14	Karma
	UInt16			dispositionBase;	// 18	Disposition Base
	UInt16			templateFlags;		// 1A	Template Flags
	TESForm			*deathItem;			// 1C	Death Item: object or FormList
	BGSVoiceType	*voiceType;			// 20
	TESActorBase	*templateActor;		// 24	Points toward Template
	UInt32			changedFlags;		// 28
	tList<FactionListData>	factionList;	// 2C

	char GetFactionRank(TESFaction* faction);
	void SetFactionRank(TESFaction* faction, char rank);

	bool IsFemale() { return flags & kFlags_Female ? true : false; }	// place holder until GECK 
};

// 14
class TESSpellList : public BaseFormComponent
{
public:
	enum
	{
		kModified_BaseSpellList =	0x00000020,
		// CHANGE_ACTOR_BASE_SPELLLIST
		//	UInt16	numSpells;
		//	UInt32	spells[numSpells];
	};

	TESSpellList();
	~TESSpellList();

	virtual UInt32	GetSaveSize(UInt32 changedFlags);
	virtual void	Save(UInt32 changedFlags);
	virtual void	Load(UInt32 changedFlags);

	tList<SpellItem>	spellList;			// 004
	tList<SpellItem>	leveledSpellList;	// 00C

	UInt32	GetSpellCount() const {
		return spellList.Count();
	}

	// return the nth spell
	SpellItem* GetNthSpell(SInt32 whichSpell) const
	{
		return spellList.GetNthItem(whichSpell);
	}

	// removes all spells and returns how many spells were removed
	//UInt32 RemoveAllSpells();
};

// 020
class TESAIForm : public BaseFormComponent
{
public:
	TESAIForm();
	~TESAIForm();

	typedef tList<TESPackage> PackageList;

	virtual UInt32	GetSaveSize(UInt32 changedFlags);
	virtual void	Save(UInt32 changedFlags);
	virtual void	Load(UInt32 changedFlags);

	UInt8	agression;				// 04
	UInt8	confidence;				// 05 
	UInt8	energyLevel;			// 06
	UInt8	responsibility;			// 07
	UInt8	mood;					// 08
	UInt8	pad09[3];				// 09

	UInt32	buySellsAndServices;	// 0C
	UInt8	teaches;				// 10
	UInt8	maximumTrainingLevel;	// 11
	UInt8	assistance;				// 12
	UInt8	aggroRadiusBehavior;	// 13
	SInt32	aggroRadius;			// 14

	PackageList	packageList;	// 18

	UInt32	GetPackageCount() const {
		return packageList.Count();
	}

	// return the nth package
	TESPackage* GetNthPackage(SInt32 anIndex) const
	{
		return packageList.GetNthItem(anIndex);
	}

	// replace the nth package
	TESPackage* SetNthPackage(TESPackage* pPackage, SInt32 anIndex)
	{
		return packageList.ReplaceNth(anIndex == -1 ? eListEnd : anIndex, pPackage);
	}

	// return the nth package
	SInt32 AddPackageAt(TESPackage* pPackage, SInt32 anIndex)
	{
		return packageList.AddAt(pPackage, anIndex == -1 ? eListEnd : anIndex);
	}

	TESPackage* RemovePackageAt(SInt32 anIndex)
	{
		return packageList.RemoveNth(anIndex == -1 ? eListEnd : anIndex);
	}

	// removes all packages and returns how many were removed
	UInt32 RemoveAllPackages() const
	{
		UInt32 cCount = GetPackageCount();
		packageList.RemoveAll();
		return cCount - GetPackageCount();
	}
};

// 00C
class TESAttributes : public BaseFormComponent
{
public:
	TESAttributes();
	~TESAttributes();

	enum
	{
		kStrength = 0,
		kPerception,
		kEndurance,
		kCharisma,
		kIntelligence,
		kAgility,
		kLuck,
	};

	UInt8	attributes[7];	// 4
	UInt8	padB;			// B
};

// 00C
class TESAnimation : public BaseFormComponent
{
public:
	TESAnimation();
	~TESAnimation();

	//UInt32	unk004;	// constructor and Fn_01 sugest this is a tList of char string.
	//UInt32	unk008;
	tList<char>	animNames;
	// 00C
};

class ActorValueOwner
{
public:
	ActorValueOwner();
	~ActorValueOwner();

	virtual UInt32	GetBaseActorValue(UInt32 avCode);		// GetBaseActorValue (used from Eval) result in EAX
	virtual float	GetBaseAVFloat(UInt32 avCode);			// GetBaseActorValue internal, result in st
	virtual int		Fn_02(UInt32 avCode);					// GetActorValue internal, result in EAX
	virtual float	GetActorValue(UInt32 avCode);			// GetActorValue (used from Eval) result in EAX
	virtual float	Fn_04(UInt32 avCode);					// GetBaseActorValue04 (internal) result in st
	virtual float	GetActorValueDamage(UInt32 avCode);
	virtual float	Fn_06(UInt32 avCode);					// GetDamageActorValue or GetModifiedActorValue		called from Fn_08, result in st, added to Fn_01
	virtual UInt32	Fn_07(UInt32 avCode);					// Manipulate GetPermanentActorValue, maybe convert to integer.
	virtual float	GetPermanentActorValue(UInt32 avCode);	// GetPermanentActorValue (used from Eval) result in EAX
	virtual Actor*	Fn_09(void);							// GetActorBase (= this - 0x100) or GetActorBase (= this - 0x0A4)
	virtual UInt16	GetLevel();								// GetLevel (from ActorBase)

	// SkillsCurrentValue[14] at index 20
};

STATIC_ASSERT(sizeof(ActorValueOwner) == 0x004);

class CachedValuesOwner
{
public:
	CachedValuesOwner();
	~CachedValuesOwner();

	virtual float	GetRadius(void); // computed from the BSBounds
	virtual float	GetWidthX(void);
	virtual float	GetWidthY(void);
	virtual float	GetHeight(void);
	virtual float	GetDPS(void);
	virtual float	GetMedicineSkillMult(void);
	virtual float	GetSurvivalSkillMult(void);
	virtual float	GetParalysis(void);
	virtual float	GetHealRate(void);
	virtual float	GetFatigueReturnRate(void); // calculated the same as HealRate
	virtual float	GetPerceptionCondition(void);
	virtual UInt32	GetEyeHeight(void);
	virtual UInt32	GetUnkShouldAttack(void);
	virtual float	GetAssistance(void);
	virtual float	GetWalkSpeedMult(void);
	virtual float	GetRunSpeedMult(void);
	virtual bool	GetHasNoCrippledLimbs(void);
};
STATIC_ASSERT(sizeof(CachedValuesOwner) == 4);

// 10C
class TESActorBase : public TESBoundAnimObject
{
public:
	TESActorBase();
	~TESActorBase();

	virtual BGSBodyPartData *	GetBodyPartData(void);
	virtual void				Fn_61(void * arg);
	virtual TESCombatStyle *	GetCombatStyle(void);	// Result saved as ZNAM GetCombatStyle
	virtual void				SetCombatStyle(TESCombatStyle * combatStyle);
	virtual void				SetAttr(UInt32 idx, float value);	// calls Fn65
	virtual void				SetAttr(UInt32 idx, UInt32 value);
	virtual void				ModActorValue(UInt32 actorValueCode, float value);
	virtual void				Fn_67(UInt32 arg0, UInt32 arg1);	// mod actor value?

	TESActorBaseData			baseData;		// 030
	TESContainer				container;		// 064
	BGSTouchSpellForm			touchSpell;		// 070
	TESSpellList				spellList;		// 07C
	TESAIForm					ai;				// 090
	TESHealthForm				health;			// 0B0
	TESAttributes				attributes;		// 0B8
	TESAnimation				animation;		// 0C4
	TESFullName					fullName;		// 0D0
	TESModel					model;			// 0DC
	TESScriptableForm			scriptable;		// 0F4
	ActorValueOwner				avOwner;		// 100
	BGSDestructibleObjectForm	destructible;	// 104
};

STATIC_ASSERT(sizeof(TESActorBase) == 0x10C);

// 14
class TESModelList : public BaseFormComponent
{
public:
	TESModelList();
	~TESModelList();

	tList<char>		modelList;	// 04
	UInt32			count;		// 0C
	UInt32			unk10;		// 10

	bool ModelListAction(char *path, char action);
	void CopyFrom(TESModelList *source);
};

// 008
class TESDescription : public BaseFormComponent
{
public:
	TESDescription();
	~TESDescription();

	virtual const char *Get(TESForm * overrideForm, UInt32 chunkID);

	UInt32	formDiskOffset;	// 4 - how does this work for descriptions in mods?
	// maybe extracts the mod ID then uses that to find the src file?
};

// 10
class TESReactionForm : public BaseFormComponent
{
public:
	TESReactionForm();
	~TESReactionForm();

	struct Reaction
	{
		enum
		{
			kNeutral = 0,
			kEnemy,
			kAlly,
			kFriend
		};

		TESFaction	* faction;
		SInt32		modifier;
		UInt32		reaction;
	};

	tList <Reaction>	reactions;	// 4
	UInt8	unkC;		// C
	UInt8	padD[3];	// D
};

// 08
class TESRaceForm : public BaseFormComponent
{
public:
	TESRaceForm();
	~TESRaceForm();

	TESRace	* race;	// 04
};

// 8
// ### derives from NiObject
class BSTextureSet : public NiObject
{
public:
	BSTextureSet();
	~BSTextureSet();
};

// 0C
class TESSoundFile : public BaseFormComponent
{
public:
	TESSoundFile();
	~TESSoundFile();

	virtual void	Set(const char * str);

	String			path;	// 04
};

// 24
class BGSQuestObjective
{
public:
	BGSQuestObjective();
	~BGSQuestObjective();

	virtual void *Destroy(bool noDealloc);

	enum
	{
		eQObjStatus_displayed = 1,
		eQObjStatus_completed = 2,
	};

	struct TargetData 
	{
		TESObjectREFR*	target;
		UInt8			flags;
		UInt8			filler[3];
	};

	struct ParentSpaceNode {};

	struct TeleportLink
	{
		TESObjectREFR*	door;
		UInt32			unk04[3];
	};

	struct Target
	{
		struct Data
		{
			BSSimpleArray<ParentSpaceNode>	parentSpaceNodes;	// 00
			BSSimpleArray<TeleportLink>		teleportLinks;		// 10
			UInt32							unk20[6];			// 20
		};

		UInt8							byte00;			// 00
		UInt8							pad01[3];		// 01
		ConditionList					conditions;		// 04
		TESObjectREFR					*target;		// 0C
		Data							data;			// 10
	};

	UInt32			objectiveId;	// 004 Objective Index in the GECK
	String			displayText;	// 008
	TESQuest		*quest;			// 010
	tList<Target>	targets;		// 014
	UInt32			unk01C;			// 01C
	UInt32			status;			// 020	bit0 = displayed, bit 1 = completed. 1 and 3 significant. If setting it to 3, quest flags bit1 will be set also.

	SInt32 GetTargetIndex(TESObjectREFR *refr);
};

class BGSOpenCloseForm
{
public:
	virtual void	Unk_00(UInt32 arg0, UInt32 arg1);
	virtual void	Unk_01(UInt32 arg0, UInt32 arg1);
	virtual bool	Unk_02(void);

	BGSOpenCloseForm();
	~BGSOpenCloseForm();
};

/**** forms ****/

class TESModelAnim : public TESModel
{
public:
	TESModelAnim();		// Identical to TESModel with a different vTable
	~TESModelAnim();

};	// 018

// 54
class TESIdleForm : public TESForm
{
public:
	TESIdleForm();
	~TESIdleForm();

	enum {
		eIFgf_groupIdle = 0,
		eIFgf_groupMovement = 1,
		eIFgf_groupLeftArm = 2,
		eIFgf_groupLeftHand = 3,
		eIFgf_groupLeftWeapon = 4,
		eIFgf_groupLeftWeaponUp = 5,
		eIFgf_groupLeftWeaponDown = 6,
		eIFgf_groupSpecialIdle = 7,
		eIFgf_groupWholeBody = 20,
		eIFgf_groupUpperBody = 20,

		eIFgf_flagOptionallyReturnsAFile = 128,
		eIFgf_flagUnknown = 64,
	};

	struct Data
	{
		UInt8			groupFlags;		// 000	animation group and other flags
		UInt8			loopMin;		// 001
		UInt8			loopMax;		// 002
		UInt8			fil03B;			// 003
		UInt16			replayDelay;	// 004
		UInt8			flags;			// 006	bit0 is No attacking
		UInt8			fil03F;			// 007
	};

	TESModelAnim				anim;			// 018
	ConditionList				conditions;		// 030
	Data						data;			// 038
	BSSimpleArray<TESIdleForm*>	*children;		// 040	NiFormArray, contains all idle anims in path if eIFgf_flagUnknown is set
	TESIdleForm					*parent;		// 044
	TESIdleForm					*previous;		// 048
	String						editorID;		// 04C

	TESIdleForm *FindIdle(Actor *animActor);
};

struct TESTopicInfoResponse
{
	struct Data
	{
		UInt32	emotionType;	//	00
		UInt32	emotionValue;	//	04	Init'd to 0x32
		UInt32	unused;			//	08
		UInt8	responseNumber;	//	0C
		UInt8	pad00D[3];		
		UInt32	sound;			//	10
		UInt8	flags;			//	14	Init'd to 1
		UInt8	pad015[3];
	};

	Data			data;					//	000
	String			responseText;			//	018
	TESIdleForm	*	spkeakerAnimation;		//	020
	TESIdleForm	*	listenerAnimation;		//	024
	TESTopicInfoResponse	* next;			//	028
};

// 50
class TESTopicInfo : public TESForm
{
public:
	TESTopicInfo();
	~TESTopicInfo();

	struct RelatedTopics
	{
		tList<TESTopic>		linkFrom;
		tList<TESTopic>		choices;
		tList<TESTopic>		unknown;
	};

	ConditionList		conditions;			// 18
	UInt16				unk20;				// 20
	UInt8				saidOnce;			// 22
	UInt8				type;				// 23
	UInt8				nextSpeaker;		// 24
	UInt8				flags1;				// 25
	UInt8				flags2;				// 26
	UInt8				pad27;				// 27
	String				prompt;				// 28
	tList<TESTopic>		addTopics;			// 30
	RelatedTopics		*relatedTopics;		// 38
	UInt32				speaker;			// 3C
	UInt32				actorValueOrPerk;	// 40
	UInt32				speechChallenge;	// 44
	TESQuest			*quest;				// 48
	UInt32				modInfoFileOffset;	// 4C	during LoadForm

	void RunResultScript(bool onEnd, Actor *actor);
};

typedef NiTLargeArray<TESTopicInfo*> TopicInfoArray;
typedef void* INFO_LINK_ELEMENT;

// 48
class TESTopic : public TESForm
{
public:
	TESTopic();
	~TESTopic();

	struct Info	//	34
	{
		TESQuest		*quest;		//	00
		TopicInfoArray	infoArray;	//	04
		BSSimpleArray<INFO_LINK_ELEMENT>		unk01C;
		TESQuest		*quest2;	//	2C
		UInt8			unk030;
		UInt8			pad031[3];
	};

	TESFullName		fullName;		// 18

	UInt8			unk24;			// 24
	UInt8			unk25;			// 25	used as bool or flag, connected to INFOGENERAL
	UInt8			pad26[2];		// 26
	float			unk28;			// 28
	tList<Info>		infos;			// 2C
	UInt32			unk34;			// 34	string TDUM
	UInt32			unk38;			// 38
	UInt16			unk3C;			// 3C
	UInt16			unk3E;			// 3E
	String			editorIDstr;	// 40
};

// A0
class BGSTextureSet : public TESBoundObject
{
public:
	BGSTextureSet();
	~BGSTextureSet();

	enum	// texture types
	{
		kDiffuse = 0,
		kNormal,
		kEnvMask,
		kGlow,
		kParallax,
		kEnv
	};

	enum
	{
		kTexFlag_NoSpecMap = 0x0001,
	};

	// 24
	struct DecalInfo
	{
		enum
		{
			kFlag_Parallax =	0x01,
			kFlag_AlphaBlend =	0x02,
			kFlag_AlphaTest =	0x04,
		};

		float	minWidth;		// 00
		float	maxWidth;		// 04
		float	minHeight;		// 08
		float	maxHeight;		// 0C
		float	depth;			// 10
		float	shininess;		// 14
		float	parallaxScale;	// 18
		UInt8	parallaxPasses;	// 1C
		UInt8	flags;			// 1D
		UInt8	pad1E[2];		// 1E
		UInt32	color;			// 20
	};

	BSTextureSet	bsTexSet;		// 30

	TESTexture		textures[6];	// 38
	DecalInfo		* decalInfo;	// 80
	UInt16			texFlags;		// 84
	UInt8			pad86[2];		// 86
	UInt32			unk88;			// 88
	UInt32			unk8C;			// 8C
	UInt32			unk90;			// 90
	UInt32			unk94;			// 94
	UInt32			unk98;			// 98
	UInt32			unk9C;			// 9C
};

STATIC_ASSERT(sizeof(BGSTextureSet) == 0xA0);

// 24
class BGSMenuIcon : public TESForm
{
public:
	BGSMenuIcon();
	~BGSMenuIcon();

	TESIcon	icon;	// 18
};

STATIC_ASSERT(sizeof(BGSMenuIcon) == 0x24);

// 28
class TESGlobal : public TESForm
{
public:
	TESGlobal();
	~TESGlobal();

	enum
	{
		kType_Float =		'f',
		kType_Long =		'l',
		kType_Short =		's'
	};

	String			name;		// 18	(EDID)
	UInt8			type;		// 20
	UInt8			pad21[3];	// 21
	union
	{
		float		data;
		UInt32		uRefID;
	};

	UInt32 ResolveRefValue();
};

STATIC_ASSERT(sizeof(TESGlobal) == 0x28);

// 60
class TESClass : public TESForm
{
public:
	TESClass();
	~TESClass();

	enum
	{
		kFlag_Playable =	0x00000001,
		kFlag_Guard =		0x00000002,
	};

	enum
	{
		kService_Weapons =		0x00000001,
		kService_Armor =		0x00000002,
		kService_Clothing =		0x00000004,
		kService_Books =		0x00000008,
		kService_Food =			0x00000010,
		kService_Chems =		0x00000020,
		kService_Stimpacks =	0x00000040,
		kService_Lights =		0x00000080,	// ??
		kService_Misc =			0x00000400,
		kService_Potions =		0x00002000,	// probably deprecated
		kService_Training =		0x00004000,
		kService_Recharge =		0x00010000,
		kService_Repair =		0x00020000,
	};

	TESFullName		fullName;		// 18
	TESDescription	description;	// 24
	TESTexture		texture;		// 2C
	TESAttributes	attributes;		// 38

	// corresponds to DATA chunk
	UInt32			tagSkills[4];	// 44
	UInt32			classFlags;		// 54
	UInt32			services;		// 58
	UInt8			teaches;		// 5C
	UInt8			trainingLevel;	// 5D
	UInt8			pad5E[2];		// 5E
};

STATIC_ASSERT(sizeof(TESClass) == 0x60);

class TESReputation : public TESForm
{
public:
	TESReputation();
	~TESReputation();
};

// 4C
class TESFaction : public TESForm
{
public:
	TESFaction();
	~TESFaction();
	enum
	{
		// TESForm flags

		// TESReactionForm flags

		kModified_FactionFlags =	0x00000004
		// CHANGE_FACTION_FLAGS
		// UInt8	flags;
	};

	enum
	{
		kFlag_HiddenFromPC =	0x00000001,
		kFlag_Evil =			0x00000002,
		kFlag_SpecialCombat =	0x00000004,

		kFlag_TrackCrime =		0x00000100,
		kFlag_AllowSell =		0x00000200,
	};

	// 1C
	struct Rank
	{
		String		name;		// 00
		String		femaleName;	// 08
		TESTexture	insignia;	// 10 - effectively unused, can be set but there is no faction UI
	};

	TESFullName		fullName;	// 18
	TESReactionForm	reaction;	// 24

	UInt32			factionFlags;	// 34
	TESReputation	*reputation;	// 38
	tList<Rank>		ranks;			// 3C
	UInt32			crimeCount44;	// 44
	UInt32			crimeCount48;	// 48

	bool IsFlagSet(UInt32 flag)
	{
		return (factionFlags & flag) != 0;
	}
	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) factionFlags |= pFlag;
		else factionFlags &= ~pFlag;
		MarkAsModified(kModified_FactionFlags);
	}
	bool IsHidden()
	{	return IsFlagSet(kFlag_HiddenFromPC);	}
	bool IsEvil()
	{	return IsFlagSet(kFlag_Evil);	}
	bool HasSpecialCombat()
	{	return IsFlagSet(kFlag_SpecialCombat);	}
	void SetHidden(bool bHidden)
	{	SetFlag(kFlag_HiddenFromPC, bHidden);	}
	void SetEvil(bool bEvil)
	{	SetFlag(kFlag_Evil, bEvil);	}
	void SetSpecialCombat(bool bSpec)
	{	SetFlag(kFlag_SpecialCombat, bSpec);	}
	const char* GetNthRankName(UInt32 whichRank, bool bFemale = false);
	void SetNthRankName(const char* newName, UInt32 whichRank, bool bFemale);
};

STATIC_ASSERT(sizeof(TESFaction) == 0x4C);

// 50
class BGSHeadPart : public TESForm
{
public:
	BGSHeadPart();
	~BGSHeadPart();

	enum
	{
		kFlag_Playable =	0x01,
	};

	TESFullName			fullName;	// 18
	TESModelTextureSwap	texSwap;	// 24

	UInt8				headFlags;	// 44
	UInt8				pad45[3];	// 45
	UInt32				unk48;		// 48
	UInt32				unk4C;		// 4C
};

STATIC_ASSERT(sizeof(BGSHeadPart) == 0x50);

// 4C
class TESHair : public TESForm
{
public:
	TESHair();
	~TESHair();

	enum
	{
		kFlag_Playable =	0x01,
		kFlag_NotMale =		0x02,
		kFlag_NotFemale =	0x04,
		kFlag_Fixed =		0x08,
	};

	TESFullName		fullName;	// 18
	TESModel		model;		// 24
	TESTexture		texture;	// 3C

	UInt8			hairFlags;	// 48	Playable, not Male, not Female, Fixed
	UInt8			pad49[3];	// 49

	bool IsPlayable() {return (hairFlags & kFlag_Playable) == kFlag_Playable;}
	void SetPlayable(bool doset) {if (doset) hairFlags |= kFlag_Playable; else hairFlags &= ~kFlag_Playable;}
};

STATIC_ASSERT(sizeof(TESHair) == 0x4C);

// 34
class TESEyes : public TESForm
{
public:
	TESEyes();
	~TESEyes();

	enum
	{
		kFlag_Playable =	0x01,
		kFlag_NotMale =		0x02,
		kFlag_NotFemale =	0x04,
	};

	TESFullName		fullName;	// 18
	TESTexture		texture;	// 24

	UInt8			eyeFlags;	// 30
	UInt8			pad31[3];	// 31

	bool IsPlayable() {return (eyeFlags & kFlag_Playable) == kFlag_Playable;}
	void SetPlayable(bool doset) {if (doset) eyeFlags |= kFlag_Playable; else eyeFlags &= ~kFlag_Playable;}
};

STATIC_ASSERT(sizeof(TESEyes) == 0x34);

// 524
class TESRace : public TESForm
{
public:
	// 18
	struct FaceGenData
	{
		UInt32	unk00;
		UInt32	unk04;
		UInt32	unk08;
		UInt32	unk0C;
		UInt32	unk10;
		UInt32	unk14;
	};

	// 2
	struct SkillMod
	{
		UInt8	actorValue;
		char	mod;
	};

	enum
	{
		kFlag_Playable =	0x00000001,
		kFlag_Child =		0x00000004,
	};

	TESRace();
	~TESRace();

	TESFullName		fullName;				// 018
	TESDescription	desc;					// 024
	TESSpellList	spells;					// 02C
	TESReactionForm	reaction;				// 040

	SkillMod		skillMods[7];			// 050
	UInt8			pad05E[2];				// 05E
	float			height[2];				// 060 male/female
	float			weight[2];				// 068 male/female
	UInt32			raceFlags;				// 070

	TESAttributes	baseAttributes[2];		// 074 male/female
	tList<TESHair>	hairs;					// 08C
	TESHair *		defaultHair[2];			// 094 male/female
	UInt8			defaultHairColor[2];	// 09C male/female
	UInt8			fill09E[2];				// 09E

	UInt32			unk0A0[(0xA8 - 0xA0) >> 2];	// 0A0

	tList<TESEyes>	eyes;					// 0A8

	TESModel		faceModels[2][8];			// 0B0	male/female Head, Ears, Mouth, TeethLower, TeethUpper, Tongue, LeftEye, RightEye
	TESTexture		faceTextures[2][8];			// 230	male/female Head, Ears, Mouth, TeethLower, TeethUpper, Tongue, LeftEye, RightEye
	TESTexture		bodyPartsTextures[2][3];	// 2F0	male/female	UpperBody, LeftHand, RightHand
	TESModel		bodyModels[2][3];			// 338	male/female	UpperBody, LeftHand, RightHand
	BGSTextureModel	bodyTextures[2];			// 3C8	male/female	EGT file, not DDS.
	FaceGenData		unk3F8[2][4];				// 3F8  male/female

	UInt32			unk4B8[(0x4CC - 0x4B8) >> 2]; // 4B8

	String				name;				// 4CC
	NiTArray <void *>	faceGenUndo;		// 4D4 - NiTPrimitiveArray<FaceGenUndo *>
	UInt32				unk4E4[6];			// 4E4
	BGSVoiceType*		voiceTypes[2];		// 4FC // VTCK male/female
	TESRace*			ageRace[2];			// 504 // ONAM/YNAM
	String				editorID;			// 50C
	UInt32				unk514[4];			// 514

	bool IsPlayable() const {return (raceFlags & kFlag_Playable) == kFlag_Playable;}
	void SetPlayable(bool doset) {if (doset) raceFlags |= kFlag_Playable; else raceFlags &= ~kFlag_Playable;}
};
STATIC_ASSERT(sizeof(TESRace) == 0x524);

// 68
class TESSound : public TESBoundAnimObject
{
public:
	TESSound();
	~TESSound();

	enum
	{
		kFlag_RandomFrequencyShift =	1,
		kFlag_PlayAtRandom =			2,
		kFlag_EnvironmentIgnored =		4,
		kFlag_RandomLocation =			8,
		kFlag_Loop =					16,
		kFlag_MenuSound =				32,
		kFlag_2D =						64,
		kFlag_360LFE =					128,
		kFlag_DialogueSound =			256,
		kFlag_EnvelopeFast =			512,
		kFlag_EnvelopeSlow =			1024,
		kFlag_2DRadius =				2048,
		kFlag_MuteWhenSubmerged =		4096,
		kFlag_StartAtRandomPosition =	8192,
	};

	TESSoundFile	soundFile;				// 30

	String			editorID;				// 3C
	UInt8			minAttenuationDist;		// 44
	UInt8			maxAttenuationDist;		// 45
	SInt16			frequencyAdj;			// 46
	UInt32			soundFlags;				// 48
	UInt16			staticAttenuation;		// 4C
	UInt8			endsAt;					// 4E
	UInt8			startsAt;				// 4F
	UInt16			attenuationCurve[5];	// 50
	UInt16			reverbAttenuation;		// 5A
	UInt32			priority;				// 5C
	UInt32			unk60;					// 60
	UInt32			unk64;					// 64

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) soundFlags |= pFlag;
		else soundFlags &= ~pFlag;
	}
};
STATIC_ASSERT(sizeof(TESSound) == 0x68);

// 54
class TESRegion;
class BGSAcousticSpace : public TESBoundObject
{
public:
	BGSAcousticSpace();
	~BGSAcousticSpace();

	enum EnvironmentTypes
	{
		kNone,
		kDefault,
		kGeneric,
		kPaddedCell,
		kRoom,
		kBathroom,
		kLivingroom,
		kStoneRoom,
		kAuditorium,
		kConcerthall,
		kCave,
		kArena,
		kHangar,
		kCarpetedHallway,
		kHallway,
		kStoneCorridor,
		kAlley,
		kForest,
		kCity,
		kMountains,
		kQuarry,
		kPlain,
		kParkinglot,
		kSewerpipe,
		kUnderwater,
		kSmallRoom,
		kMediumRoom,
		kLargeRoom,
		kMediumHall,
		kLargeHall,
		kPlate
	};

	UInt8		isInterior;			// 30
	UInt8		pad31[3];			// 31
	TESSound	*dawnSound;			// 34
	TESSound	*noonSound;			// 38
	TESSound	*duskSound;			// 3C
	TESSound	*nightSound;		// 40
	TESSound	*wallaSound;		// 44
	TESRegion	*region;			// 48
	UInt32		environmentType;	// 4C
	UInt32		wallaTriggerCount;	// 50
};
STATIC_ASSERT(sizeof(BGSAcousticSpace) == 0x54);

// 60
class TESSkill : public TESForm
{
public:
	TESSkill();
	~TESSkill();

	TESDescription	description;	// 18
	TESTexture		texture;		// 20

	UInt32			unk2C;			// 2C
	UInt32			unk30;			// 30
	UInt32			unk34;			// 34
	float			unk38;			// 38
	float			unk3C;			// 3C
	TESDescription	desc2[3];		// 40
	UInt32			unk58[(0x60 - 0x58) >> 2];	// 58
};

STATIC_ASSERT(sizeof(TESSkill) == 0x60);

// B0
class EffectSetting : public TESForm
{
public:
	EffectSetting();
	~EffectSetting();

	enum
	{
		kArchType_ValueModifier = 0,
		kArchType_Script,
		kArchType_Dispel,
		kArchType_CureDisease,
		kArchType_Absorb,
		kArchType_Shield,
		kArchType_Calm,
		kArchType_Demoralize,
		kArchType_Frenzy,
		kArchType_CommandCreature,
		kArchType_CommandHumanoid,
		kArchType_Invisibility,
		kArchType_Chameleon,
		kArchType_Light,
		kArchType_Darkness,
		kArchType_NightEye,
		kArchType_Lock,
		kArchType_Open,
		kArchType_BoundItem,
		kArchType_SummonCreature,
		kArchType_DetectLife,
		kArchType_Telekinesis,
		kArchType_DisintigrateArmor,
		kArchType_DisinitgrateWeapon,
		kArchType_Paralysis,
		kArchType_Reanimate,
		kArchType_SoulTrap,
		kArchType_TurnUndead,
		kArchType_SunDamage,
		kArchType_Vampirism,
		kArchType_CureParalysis,
		kArchType_CureAddiction,
		kArchType_CurePoison,
		kArchType_Concussion,
		kArchType_ValueAndParts,
		kArchType_LimbCondition,
		kArchType_Turbo,
	};

	TESModel		model;			// 18
	TESDescription	description;	// 30
	TESFullName		fullName;		// 38
	TESIcon			icon;			// 44
	UInt32			unk50;			// 50
	UInt32			unk54;			// 54
	UInt32			effectFlags;	// 58
	float			unk5C;			// 5C
	TESForm			*associatedItem;// 60	// Script* for ScriptEffects
	UInt32			unk64;			// 64
	UInt32			resistVal;		// 68 - actor value for resistance
	UInt16			unk6C;			// 6C
	UInt8			pad6E[2];		// 6E
	TESObjectLIGH	*light;			// 70
	float			projectileSpeed;// 74
	TESEffectShader	*effectShader;	// 78 - effect shader
	UInt32			unk7C;			// 7C
	UInt32			unk80;			// 80
	UInt32			unk84;			// 84
	UInt32			hitSound;		// 88
	UInt32			unk8C;			// 8C
	float			unk90;			// 90 - fMagicDefaultCEEnchantFactor
	float			unk94;			// 94 - fMagicDefaultCEBarterFactor
	UInt8			archtype;		// 98
	UInt8			pad99[3];		// 99
	UInt8			actorVal;		// 9C - actor value
	UInt8			pad9D[3];		// 9D
	UInt32			unkA0;			// A0
	UInt32			unkA4;			// A4
	UInt32			unkA8;			// A8
	UInt32			unkAC;			// AC
};

STATIC_ASSERT(sizeof(EffectSetting) == 0xB0);

// 68
class TESGrass : public TESBoundObject
{
public:
	TESGrass();
	~TESGrass();

	TESModel		model;					// 30

	UInt8			density;				// 48
	UInt8			minSlope;				// 49
	UInt8			maxSlope;				// 4A
	UInt8			pad4B;					// 4B
	UInt16			unitFromWaterAmount;	// 4C
	UInt8			pad4E[2];				// 4E
	UInt8			unitFromWaterType;		// 50
	UInt8			pad51[3];				// 51
	float			positionRange;			// 54
	float			heightRange;			// 58
	float			colorRange;				// 5C
	float			wavePeriod;				// 60
	UInt8			flags;					// 64
	UInt8			pad65[3];				// 65
};

STATIC_ASSERT(sizeof(TESGrass) == 0x68);

// 28
class TESLandTexture : public TESForm
{
public:
	TESLandTexture();
	~TESLandTexture();

	BGSTextureSet	*textureSet;		// 18
	UInt8			materialType;		// 1C
	UInt8			friction;			// 1D
	UInt8			restitution;		// 1E
	UInt8			specularExponent;	// 1F
	tList<TESGrass>	grasses;			// 20
};

STATIC_ASSERT(sizeof(TESLandTexture) == 0x28);

// 44
class EnchantmentItem : public MagicItemForm
{
public:
	EnchantmentItem();
	~EnchantmentItem();

	virtual void	ByteSwap(void);

	enum
	{
		kType_Weapon = 2,
		kType_Apparel,
	};

	UInt32		type;		// 34
	UInt32		unk38;		// 38
	UInt32		unk3C;		// 3C
	UInt8		enchFlags;	// 40
	UInt8		pad41[3];	// 41
};

STATIC_ASSERT(sizeof(EnchantmentItem) == 0x44);

// 44
class SpellItem : public MagicItemForm
{
public:
	SpellItem();
	~SpellItem();

	virtual void	ByteSwap(void);

	enum
	{
		kType_ActorEffect	= 0,
		kType_Disease,
		kType_Power,
		kType_LesserPower,
		kType_Ability,
		kType_Poison,
		kType_Addiction		= 10,
	};

	UInt32		type;		// 34
	UInt32		unk38;		// 38
	UInt32		unk3C;		// 3C
	UInt8		spellFlags;	// 40
	UInt8		pad41[3];	// 41
};
STATIC_ASSERT(sizeof(SpellItem) == 0x44);

// 90
class TESObjectACTI : public TESBoundAnimObject
{
public:
	TESObjectACTI();
	~TESObjectACTI();

	TESFullName					fullName;			// 30
	TESModelTextureSwap			modelTextureSwap;	// 3C
	TESScriptableForm			scriptable;			// 5C
	BGSDestructibleObjectForm	destructible;		// 68
	BGSOpenCloseForm			openClose;			// 70

	TESSound					*loopingSound;		// 74
	TESSound					*activationSound;	// 78
	TESSound					*radioTemplate;		// 7C
	TESWaterForm				*waterType;			// 80
	BGSTalkingActivator			*radioStation;		// 84
	String						activationPrompt;	// 88
};

STATIC_ASSERT(sizeof(TESObjectACTI) == 0x90);

// 98
class BGSTalkingActivator : public TESObjectACTI
{
public:
	BGSTalkingActivator();
	~BGSTalkingActivator();

	Actor				*talkingActor;	// 90
	BGSVoiceType		*voiceType;		// 94
};
STATIC_ASSERT(sizeof(BGSTalkingActivator) == 0x98);

// BGSTerminal (9C)
class BGSTerminal : public TESObjectACTI
{
public:
	BGSTerminal();
	~BGSTerminal();

	enum
	{
		kTerminalFlagLeveled = 1 << 0,
		kTerminalFlagUnlocked = 1 << 1,
		kTerminalFlagAltColors = 1 << 2,
		kTerminalFlagHideWelcome = 1 << 3,
	};

	enum
	{
		kEntryFlagAddNote = 1 << 0,
		kEntryFlagForceRedraw = 1 << 1,
	};

	struct TermData
	{
		UInt8 difficulty;       // 0: very easy, 1: easy, 2: average, 3: hard, 4: very hard, 5: requires key
		UInt8 terminalFlags;
		UInt8 type;             // 0-9, corresponds to GECK types 1-10
	};

	struct MenuEntry
	{
		String				entryText;
		String				resultText;
		UInt8				entryFlags;
		UInt8				pad[3];
		BGSNote				*displayNote;
		BGSTerminal			*subMenu;
		ScriptEventList		*scriptEventList;
		tList<Condition*>	conditions;
	};

	String				desc;			// 090	DESC
	tList<MenuEntry>	menuEntries;	// 098
	BGSNote				*password;		// 0A0	PNAM
	TermData			data;			// 0A4	DNAM
};

// 98
class TESFurniture : public TESObjectACTI
{
public:
	TESFurniture();
	~TESFurniture();

	UInt32			unk90[2];	// 90
};
STATIC_ASSERT(sizeof(TESFurniture) == 0x98);

// 190
class TESObjectARMO : public TESBoundObject
{
public:
	TESObjectARMO();
	~TESObjectARMO();

	struct MovementSound
	{
		TESSound		*sound;
		UInt8			unk04[3];
		UInt8			chance;
		UInt32			type;
		//				0x11	Walk
		//				0x12	Sneak
		//				0x13	Run
		//				0x14	Sneak (Armor)
		//				0x15	Run (Armor)
		//				0x16	Walk (Armor)
	};

	TESFullName					fullName;				// 030
	TESScriptableForm			scriptable;				// 03C
	TESEnchantableForm			enchantable;			// 048
	TESValueForm				value;					// 058
	TESWeightForm				weight;					// 060
	TESHealthForm				health;					// 068
	TESBipedModelForm			bipedModel;				// 070
	BGSDestructibleObjectForm	destuctible;			// 14C
	BGSEquipType				equipType;				// 154
	BGSRepairItemList			repairItemList;			// 15C
	BGSBipedModelList			bipedModelList;			// 164
	BGSPickupPutdownSounds		pickupPutdownSounds;	// 16C

	UInt16						armorRating;			// 178
	UInt16						modifiesVoice;			// 17A
	float						damageThreshold;		// 17C
	UInt32						armorFlags;				// 180
	UInt32						unk184;					// 184
	union												// 188
	{
		TESObjectARMO			*audioTemplate;
		tList<MovementSound>	*movementSounds;
	};
	UInt8						overrideSounds;			// 18C
	UInt8						pad18D[3];				// 18D
	void SetFacegenFlag(UInt32 pFlag, UInt32 bFemale, bool bEnable)
	{
		if (bEnable) bipedModel.bipedModel[bFemale].facegenFlags |= pFlag;
		else bipedModel.bipedModel[bFemale].facegenFlags &= ~pFlag;
	}
};
STATIC_ASSERT(sizeof(TESObjectARMO) == 0x190);

// C4
class TESObjectBOOK : public TESBoundObject
{
public:
	TESObjectBOOK();
	~TESObjectBOOK();

	TESFullName					fullName;		// 30
	TESModelTextureSwap			model;			// 3C
	TESIcon						icon;			// 5C
	TESScriptableForm			scriptable;		// 68
	TESEnchantableForm			enchantable;	// 74
	TESValueForm				value;			// 84
	TESWeightForm				weight;			// 8C
	TESDescription				description;	// 94
	BGSDestructibleObjectForm	destuctible;	// 9C
	BGSMessageIcon				messageIcon;	// A4
	BGSPickupPutdownSounds		sounds;			// B4

	UInt8						bookFlags;		// C0
	UInt8						skillCode;		// C1
	UInt8						byteC2;			// C2
	UInt8						byteC3;			// C3

};
STATIC_ASSERT(sizeof(TESObjectBOOK) == 0xC4);

// 154
class TESObjectCLOT : public TESBoundObject
{
public:
	TESObjectCLOT();
	~TESObjectCLOT();

	// bases
	TESFullName					fullName;		// 030
	TESScriptableForm			scriptable;		// 03C
	TESEnchantableForm			enchantable;	// 048
	TESValueForm				value;			// 058
	TESWeightForm				weight;			// 060
	TESBipedModelForm			bipedModel;		// 068
	BGSDestructibleObjectForm	destuctible;	// 144
	BGSEquipType				equipType;		// 14C
	// unk data
};

// 9C
class TESObjectCONT : public TESBoundAnimObject
{
public:
	TESObjectCONT();
	~TESObjectCONT();

	TESContainer                container;				// 30
	TESFullName					name;					// 3C
	TESModelTextureSwap			model;					// 48
	TESScriptableForm			scriptForm;				// 68
	TESWeightForm				weightForm;				// 74
	BGSDestructibleObjectForm	destructForm;			// 7C
	BGSOpenCloseForm			openCloseForm;			// 84

	UInt32						unk88;					// 88
	TESSound					*openSound;				// 8C
	TESSound					*closeSound;			// 90
	TESSound					*randomLoopingSound;	// 94
	UInt8						flags;					// 98
	UInt8						pad99[3];				// 99
};

// 90
class TESObjectDOOR : public TESBoundAnimObject
{
public:
	TESObjectDOOR();
	~TESObjectDOOR();

	TESFullName					name;					// 30
	TESModelTextureSwap			model;					// 3C
	TESScriptableForm			scriptForm;				// 5C
	BGSDestructibleObjectForm	destructForm;			// 68
	BGSOpenCloseForm			openCloseForm;			// 70

	UInt32						unk74;					// 74
	TESSound					*openSound;				// 78
	TESSound					*closeSound;			// 7C
	TESSound					*randomLoopingSound;	// 80
	UInt32						unk84;					// 84
	tList<void>					list88;					// 88
};

// IngredientItem (A4)
class IngredientItem;

// TESObjectLIGH (C8)
class TESObjectLIGH : public TESBoundAnimObject
{
public:
	TESObjectLIGH();
	~TESObjectLIGH();

	enum
	{
		kFlag_Dynamic =			1,
		kFlag_CanBeCarried =	2,
		kFlag_Negative =		4,
		kFlag_Flicker =			8,
		kFlag_Unused =			16,
		kFlag_OffByDefault =	32,
		kFlag_FlickerSlow =		64,
		kFlag_Pulse =			128,
		kFlag_PulseSlow =		256,
		kFlag_SpotLight =		512,
		kFlag_SpotShadow =		1024,
	};

	TESFullName					fullName;		// 030
	TESModelTextureSwap			modelSwap;		// 03C
	TESIcon						icon;			// 05C
	BGSMessageIcon				messageIcon;	// 068
	TESScriptableForm			scriptable;		// 078
	TESWeightForm				weight;			// 084
	TESValueForm				value;			// 08C
	BGSDestructibleObjectForm	destructible;	// 094

	SInt32						time;			// 09C
	UInt32						radius;			// 0A0
	UInt8						red;			// 0A4
	UInt8						green;			// 0A5
	UInt8						blue;			// 0A6
	UInt8						padA7;			// 0A7
	UInt32						lightFlags;		// 0A8
	float						falloffExp;		// 0AC
	float						FOV;			// 0B0
	float						fadeValue;		// 0B4
	TESSound					*sound;			// 0B8
	UInt32						padBC[3];		// 0BC

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) lightFlags |= pFlag;
		else lightFlags &= ~pFlag;
	}

	NiPointLight *CreatePointLight(TESObjectREFR *targetRef, NiNode *targetNode, bool arg3);
};
STATIC_ASSERT(sizeof(TESObjectLIGH) == 0x0C8);

// AC
class TESObjectMISC : public TESBoundObject
{
public:
	TESObjectMISC();
	~TESObjectMISC();

	TESFullName					fullName;		// 30
	TESModelTextureSwap			modelSwap;		// 3C
	TESIcon						icon;			// 5C
	TESScriptableForm			scriptable;		// 68
	TESValueForm				value;			// 74
	TESWeightForm				weight;			// 7C
	BGSDestructibleObjectForm	destructible;	// 84
	BGSMessageIcon				messageIcon;	// 8C
	BGSPickupPutdownSounds		pickupPutdown;	// 9C

	UInt32						unkA8;			// A8
};
STATIC_ASSERT(sizeof(TESObjectMISC) == 0xAC);

// 9C
class TESCasinoChips : public TESBoundObject
{
public:
	TESCasinoChips();
	~TESCasinoChips();

	TESFullName					fullName;		// 30
	TESModelTextureSwap			modelSwap;		// 3C
	TESIcon						icon;			// 5C
	BGSMessageIcon				messageIcon;	// 68
	TESValueForm				value;			// 78
	BGSDestructibleObjectForm	destructible;	// 80
	BGSPickupPutdownSounds		pickupPutdown;	// 88

	UInt32						unk94[2];		// 94
};
STATIC_ASSERT(sizeof(TESCasinoChips) == 0x9C);

// CC
class TESCaravanMoney : public TESBoundObject
{
public:
	TESCaravanMoney();
	~TESCaravanMoney();

	TESFullName					fullName;		// 30
	TESModelTextureSwap			modelSwap;		// 3C
	TESIcon						icon;			// 5C
	BGSMessageIcon				messageIcon;	// 68
	TESValueForm				value;			// 78
	BGSPickupPutdownSounds		pickupPutdown;	// 80

	UInt32						unk8C[16];		// 8C
};
STATIC_ASSERT(sizeof(TESCaravanMoney) == 0xCC);

// 58
class TESObjectSTAT : public TESBoundObject
{
public:
	TESObjectSTAT();
	~TESObjectSTAT();

	TESModelTextureSwap		model;		// 30
	UInt32					unk50[2];	// 50
};

/*// 74
class BGSMovableStatic : public TESFullName
{
public:
	BGSMovableStatic();
	~BGSMovableStatic();

	BGSDestructibleObjectForm	destructible;	// 0C
	TESObjectSTAT				objectSTAT;		// 14
	UInt32						unk6C[2];		// 6C
};
STATIC_ASSERT(sizeof(BGSMovableStatic) == 0x74);*/

// Note: The above (commented-out) is the "actual" definition; The one below is the "effective" definition, used at runtime.
// Access to TESFullName and BGSDestructibleObjectForm via DYNAMIC_CAST.
class BGSMovableStatic : public TESObjectSTAT
{
	BGSMovableStatic();
	~BGSMovableStatic();

	UInt32			unk58[2];	// 58
};
STATIC_ASSERT(sizeof(BGSMovableStatic) == 0x60);

// 50
class BGSStaticCollection : public TESBoundObject
{
public:
	BGSStaticCollection();
	~BGSStaticCollection();

	TESModelTextureSwap		model;		// 30
};
STATIC_ASSERT(sizeof(BGSStaticCollection) == 0x50);

// 50
class BGSPlaceableWater : public TESBoundObject
{
public:
	BGSPlaceableWater();
	~BGSPlaceableWater();

	TESModel			model;	// 30
	UInt32				flags;	// 48
	TESWaterForm		*water;	// 4C
};

// TESObjectTREE (94)
class TESObjectTREE;

// TESFlora (90)
class TESFlora;

class TESObjectIMOD : public TESBoundObject
{
public:
	TESObjectIMOD();
	~TESObjectIMOD();

	// bases
	TESFullName					name;				// 030
	TESModelTextureSwap			model;				// 03C
	TESIcon						icon;				// 05C
	TESScriptableForm			scriptForm;			// 068
	TESDescription				description;		// 074
	TESValueForm				value;				// 07C
	TESWeightForm				weight;				// 084
	BGSDestructibleObjectForm	destructible;		// 08C
	BGSMessageIcon				messageIcon;		// 094
	BGSPickupPutdownSounds		pickupPutdownSounds;// 0A4
};

// 388
class TESObjectWEAP : public TESBoundObject
{
public:
	TESObjectWEAP();
	~TESObjectWEAP();

	enum EWeaponType {
		kWeapType_HandToHandMelee = 0,
		kWeapType_OneHandMelee,
		kWeapType_TwoHandMelee,
		kWeapType_OneHandPistol,
		kWeapType_OneHandPistolEnergy,
		kWeapType_TwoHandRifle,
		kWeapType_TwoHandAutomatic,
		kWeapType_TwoHandRifleEnergy,
		kWeapType_TwoHandHandle,
		kWeapType_TwoHandLauncher,
		kWeapType_OneHandGrenade,
		kWeapType_OneHandMine,
		kWeapType_OneHandLunchboxMine,
		kWeapType_OneHandThrown,
		kWeapType_Last	// During animation analysis, player weapon can be classified as 0x0C = last
	};

	enum EWeaponSounds {
		kWeapSound_Shoot3D = 0,
		kWeapSound_Shoot2D,
		kWeapSound_Shoot3DLooping,
		kWeapSound_NoAmmo,
		kWeapSound_Swing = kWeapSound_NoAmmo,
		kWeapSound_Block,
		kWeapSound_Idle,
		kWeapSound_Equip,
		kWeapSound_Unequip
	};

	enum EHandGrip {
		eHandGrip_Default	= 0xFF,
		eHandGrip_1			= 0xE6,
		eHandGrip_2			= 0xE7,
		eHandGrip_3			= 0xE8,
		eHandGrip_4			= 0xE9,
		eHandGrip_5			= 0xEA,
		eHandGrip_6			= 0xEB,
		eHandGrip_Count		= 7,
	};

	enum EAttackAnimations {
		eAttackAnim_Default =		255,
		eAttackAnim_Attack3 =		38,
		eAttackAnim_Attack4 =		44,
		eAttackAnim_Attack5 =		50,
		eAttackAnim_Attack6 =		56,
		eAttackAnim_Attack7 =		62,
		eAttackAnim_Attack8 =		68,
		eAttackAnim_Attack9 =		144,
		eAttackAnim_AttackLeft =	26,
		eAttackAnim_AttackLoop =	74,
		eAttackAnim_AttackRight =	32,
		eAttackAnim_AttackSpin =	80,
		eAttackAnim_AttackSpin2 =	86,
		eAttackAnim_AttackThrow =	114,
		eAttackAnim_AttackThrow2 =	120,
		eAttackAnim_AttackThrow3 =	126,
		eAttackAnim_AttackThrow4 =	132,
		eAttackAnim_AttackThrow5 =	138,
		eAttackAnim_AttackThrow6 =	150,
		eAttackAnim_AttackThrow7 =	156,
		eAttackAnim_AttackThrow8 =	162,
		eAttackAnim_PlaceMine =		102,
		eAttackAnim_PlaceMine2 =	108,
		eAttackAnim_Count =			23,
	};

	enum ReloadAnim {
		eReload_A = 0,
		eReload_B,
		eReload_C,
		eReload_D,
		eReload_E,
		eReload_F,
		eReload_G,
		eReload_H,
		eReload_I,
		eReload_J,
		eReload_K,
		eReload_L,
		eReload_M,
		eReload_N,
		eReload_O,
		eReload_P,
		eReload_Q,
		eReload_R,
		eReload_S,
		eReload_W,
		eReload_X,
		eReload_Y,
		eReload_Z,
		eReload_Count,
	};
	STATIC_ASSERT(eReload_Count == 23);

	enum EWeaponFlags1 {
		eFlag_IgnoresNormalWeapResist	= 0x1,
		eFlag_IsAutomatic				= 0x2,
		eFlag_HasScope					= 0x4,
		eFlag_CantDrop					= 0x8,
		eFlag_HideBackpack				= 0x10,
		eFlag_EmbeddedWeapon			= 0x20,
		eFlag_No1stPersonISAnims		= 0x40,
		Eflag_NonPlayable				= 0x80
	};

	enum EWeaponFlags2 {
		eFlag_PlayerOnly				= 0x1,
		eFlag_NPCsUseAmmo				= 0x2,
		eFlag_NoJamAfterReload			= 0x4,
		eFlag_ActionPointOverride		= 0x8,
		eFlag_MinorCrime				= 0x10,
		eFlag_FixedRange				= 0x20,
		eFlag_NotUsedNormalCombat		= 0x40,
		eFlag_DamageToWeaponOverride	= 0x80,
		eFlag_No3rdPersonISAnims		= 0x100,
		eFlag_BurstShot					= 0x200,
		eFlag_RumbleAlternate			= 0x400,
		eFlag_LongBurst					= 0x800,
		eFlag_NightVision				= 0x1000,
	};

	enum EEmbedWeapAV {
		eEmbedAV_Perception				= 0,
		eEmbedAV_Endurance,
		eEmbedAV_LeftAttack,
		eEmbedAV_RightAttack,
		eEmbedAV_LeftMobility,
		eEmbedAV_RightMobility,
		eEmbedAV_Brain,
	};

	enum EOnHit {
		eOnHit_Normal					= 0,
		eOnHit_DismemberOnly,
		eOnHit_ExplodeOnly,
		eOnHit_NoDismemberOrExplode,
	};

	enum ERumblePattern {
		eRumblePattern_Constant			= 0,
		eRumblePattern_Square,
		eRumblePattern_Triangle,
		eRumblePattern_Sawtooth
	};

	enum ECritDamageFlags {
		eCritDamage_OnDeath				= 0x1
	};

	enum
	{
		kWeaponModEffect_None = 0,
		kWeaponModEffect_IncreaseDamage,
		kWeaponModEffect_IncreaseClipCapacity,
		kWeaponModEffect_DecreaseSpread,
		kWeaponModEffect_DecreaseWeight,
		kWeaponModEffect_RegenerateAmmo_Shots,
		kWeaponModEffect_RegenerateAmmo_Seconds,
		kWeaponModEffect_DecreaseEquipTime,
		kWeaponModEffect_IncreaseRateOfFire,		// 8
		kWeaponModEffect_IncreaseProjectileSpeed,
		kWeaponModEffect_IncreaseMaxCondition,
		kWeaponModEffect_Silence,
		kWeaponModEffect_SplitBeam,
		kWeaponModEffect_VATSBonus,
		kWeaponModEffect_IncreaseZoom,				// 14
	};

	enum EResistType
	{
		kWeapResistType_Electric = 58,
		kWeapResistType_Energy = 60,
		kWeapResistType_Emp = 61,
	};

	// bases
	TESFullName					fullName;			// 030
	TESModelTextureSwap			textureSwap;		// 03C
	TESIcon						icon;				// 05C
	TESScriptableForm			scritpable;			// 068
	TESEnchantableForm			enchantable;		// 074
	TESValueForm				value;				// 084
	TESWeightForm				weight;				// 08C
	TESHealthForm				health;				// 094
	TESAttackDamageForm			attackDmg;			// 09C
	BGSAmmoForm					ammo;				// 0A4
	BGSClipRoundsForm			clipRounds;			// 0AC
	BGSDestructibleObjectForm	destructible;		// 0B4
	BGSRepairItemList			repairItemList;		// 0BC
	BGSEquipType				equipType;			// 0C4
	BGSPreloadable				preloadable;		// 0CC
	BGSMessageIcon				messageIcon;		// 0D0
	BGSBipedModelList			bipedModelList;		// 0E0
	BGSPickupPutdownSounds		pickupPutdownSounds;// 0E8

	UInt8				eWeaponType;		// 0F4
	UInt8				pad[3];
	float				animMult;			// 0F8
	float				reach;				// 0FC
	Bitfield8			weaponFlags1;		// 100
	UInt8				handGrip;			// 101
	UInt8				ammoUse;			// 102
	UInt8				reloadAnim;			// 103
	float				minSpread;			// 104
	float				spread;				// 108
	UInt32				unk10C;				// 10C
	float				sightFOV;			// 110
	UInt32				unk114;				// 114
	BGSProjectile		*projectile;		// 118
	UInt8				baseVATSChance;		// 11C
	UInt8				attackAnim;			// 11D
	UInt8				numProjectiles;		// 11E
	UInt8				embedWeaponAV;		// 11F
	float				minRange;			// 120
	float				maxRange;			// 124
	UInt32				onHit;				// 128
	Bitfield32			weaponFlags2;		// 12C
	float				animAttackMult;		// 130
	float				fireRate;			// 134
	float				AP;					// 138
	float				rumbleLeftMotor;	// 13C
	float				rumbleRightMotor;	// 140
	float				rumbleDuration;		// 144
	float				damageToWeaponMult;	// 148
	float				animShotsPerSec;	// 14C
	float				animReloadTime;		// 150
	float				animJamTime;		// 154		
	float				aimArc;				// 158
	UInt32				weaponSkill;		// 15C - actor value
	UInt32				rumblePattern;		// 160 - reload anim?
	float				rumbleWavelength;	// 164
	float				limbDamageMult;		// 168
	SInt32				resistType;			// 16c - actor value
	float				sightUsage;			// 170
	float				semiAutoFireDelay[2];	// 174
	UInt32				unk17C;				// 17C - 0-0x10: 0x8:str req 0x10: - skill req  - 0xb:kill impulse B158 - mod 1 val B15C - Mod 2 val Effects: 0x1: e(zoom) 0x2: a 0x3:0 0x4-6: Values c-e Mod Effects Val2:1-3 
	UInt32				effectMods[3];		// 180
	float				value1Mod[3];		// 18C
	UInt32				powerAttackAnimOverride;	// 198
	UInt32				strRequired;		// 19C
	UInt8				pad1A0;				// 1A0
	UInt8				modReloadAnim;		// 1A1
	UInt8				pad1A2[2];			// 1A2
	float				regenRate;			// 1A4
	float				killImpulse;		// 1A8
	float				value2Mod[3];		// 1AC
	float				impulseDist;		// 1B8
	UInt32				skillRequirement;	// 1BC
	UInt16				criticalDamage;		// 1C0
	UInt8				unk1C2[2];			// 1C2
	float				criticalPercent;	// 1C4
	UInt8				critDamageFlags;	// 1C8
	UInt8				pad1C9[3];			// 1C9
	SpellItem			*criticalEffect;	// 1CC
	TESModel			shellCasingModel;	// 1DO
	TESModel			targetNIF;			// 1E8 - target NIF
	TESModel			model200;			// 200 - could be a texture swap
	UInt32				unk218;				// 218
	TESSound			*openSound;			// 21C
	TESSound			*openSound2;		// 220
	TESSound			*openSound3;		// 224
	TESSound			*soundShoot3DLooping; //228
	TESSound			*noAmmoSound;		// 22C
	TESSound			*blockSound;		// 230
	TESSound			*idleSound;			// 234
	TESSound			*equipSound;		// 238
	TESSound			*unequipSound;		// 23C
	TESSound			*unequipSound2;		// 240
	TESSound			*unequipSound3;		// 244
	TESSound			*unequipSound4;		// 248
	BGSImpactDataSet	*impactDataSet;		// 24C
	TESObjectSTAT		*worldStatic;		// 250
	TESObjectSTAT		*modStatics[7];		// 254
	TESModelTextureSwap	modModels[7];		// 270
	TESObjectIMOD		*itemMod[3];		// 350
	String				embeddedNodeName;	// 35C
	UInt32				soundLevel;			// 364
	UInt32				unk368;				// 368
	UInt32				unk36C;				// 36C
	SpellItem			*VATSEffect;		// 370
	UInt32				unk374;				// 374
	UInt32				unk378;				// 378
	UInt32				unk37C;				// 37C
	UInt32				recharge;			// 380 maybe recharge
	UInt32				unk384;				// 384

	bool IsAutomatic() const { return weaponFlags1.IsSet(eFlag_IsAutomatic); }
	void SetIsAutomatic(bool bAuto) { weaponFlags1.Write(eFlag_IsAutomatic, bAuto); }
	bool HasScope() const { return weaponFlags1.IsSet(eFlag_HasScope); }
	bool IsNonPlayable() { return weaponFlags1.IsSet( 0x80 ); }
	bool IsPlayable() { return !IsNonPlayable(); }
	void SetPlayable(bool doset) { weaponFlags1.Write(Eflag_NonPlayable, !doset); }
	bool IsMeleeWeapon();
	UInt8 HandGrip() const;
	void SetHandGrip(UInt8 handGrip);
	UInt8 AttackAnimation() const;
	void SetAttackAnimation(UInt32 attackAnim);
	TESObjectIMOD* GetItemMod(UInt8 which);
	TESAmmo *GetAmmo();
	float GetModBonuses(UInt8 modFlags, UInt32 effectID);
	UInt32 GetItemModEffect(UInt8 which)	{ which -= 1; return (which < 3) ? effectMods[which] : 0; }
	float GetItemModValue1(UInt8 which)		{ which -= 1; return (which < 3) ? value1Mod[which] : 0; }
	float GetItemModValue2(UInt8 which)		{ which -= 1; return (which < 3) ? value2Mod[which] : 0; }
};
STATIC_ASSERT(sizeof(TESObjectWEAP) == 0x388);

enum AmmoEffectID
{
	kAmmoEffect_DamageMod =		0,
	kAmmoEffect_DRMod =			1,
	kAmmoEffect_DTMod =			2,
	kAmmoEffect_SpreadMod =		3,
	kAmmoEffect_ConditionMod =	4,
	kAmmoEffect_FatigueMod =	5,
};

// 30
class TESAmmoEffect : public TESForm
{
public:
	TESAmmoEffect();
	~TESAmmoEffect();

	enum
	{
		kOperation_Add =		0,
		kOperation_Multiply =	1,
		kOperation_Subtract =	2,
	};

	TESFullName		fullName;		// 18
	UInt32			type;			// 24
	UInt32			operation;		// 28
	float			value;			// 2C
};

STATIC_ASSERT(sizeof(TESAmmoEffect) == 0x30);

// DC
class TESAmmo : public TESBoundObject
{
public:
	TESAmmo();
	~TESAmmo();

	enum eAmmoFlags
	{
		kFlags_IgnoreWeapResistance =	1,
		kFlags_NonPlayable =			2,
	};

	// bases
	TESFullName					fullName;				// 030
	TESModelTextureSwap			model;					// 03C
	TESIcon						icon;					// 05C
	BGSMessageIcon				messageIcon;			// 068	
	TESValueForm				value;					// 078
	BGSClipRoundsForm			clipRounds;				// 080
	BGSDestructibleObjectForm	destructible;			// 088
	BGSPickupPutdownSounds		pickupPutdownsounds;	// 090
	TESScriptableForm			scriptable;				// 09C

	float						speed;					// 0A8
	UInt32						flags;					// 0AC
	UInt32						projPerShot;			// 0B0
	BGSProjectile				*projectile;			// 0B4
	float						weight;					// 0B8
	TESObjectMISC				*casing;				// 0BC
	float						ammoPercentConsumed;	// 0C0
	String						shortName;				// 0C4
	String						abbreviation;			// 0CC
	tList<TESAmmoEffect>		effectList;				// 0D4

	bool IsNonPlayable() {return (flags & kFlags_NonPlayable) == kFlags_NonPlayable;}
	bool IsPlayable() {return !IsNonPlayable();}
	void SetPlayable(bool doset) {if (doset) flags &= ~kFlags_NonPlayable; else flags |= kFlags_NonPlayable;}
};
STATIC_ASSERT(sizeof(TESAmmo) == 0xDC);

class TESCaravanCard : public TESBoundObject
{
public:
	TESCaravanCard();
	~TESCaravanCard();
};

// 2B0
struct ValidBip01Names
{
	enum eOptionalBoneType
	{
		kOptionalBone_Bip01Head			= 0,
		kOptionalBone_Weapon			= 1,
		kOptionalBone_Bip01LForeTwist	= 2,
		kOptionalBone_Bip01Spine2		= 3,
		kOptionalBone_Bip01Neck1		= 4,
	};

	// 08
	struct OptionalBone	
	{
		bool		exists;
		NiNode		*bone;
	};

	// 10
	struct Data
	{
		union									// 00 can be a modelled form (Armor or Weapon) or a Race if not equipped
		{
			TESForm				*item;
			TESObjectARMO		*armor;
			TESObjectWEAP		*weapon;
			TESRace				*race;
		};
		TESModelTextureSwap		*modelTexture;	// 04 texture or model for said form
		NiNode					*boneNode;		// 08 NiNode for the modelled form
		UInt32					unk00C;			// 0C Number, bool or flag
	};

	NiNode				*bip01;			// 000 receive Bip01 node, then optionally Bip01Head, Weapon, Bip01LForeTwist, Bip01Spine2, Bip01Neck1
	OptionalBone		bones[5];		// 004
	Data				slotData[20];	// 02C indexed by the EquipSlot
	Data				unk016C[20];	// 16C indexed by the EquipSlot
	Character			*character;		// 2AC
};
STATIC_ASSERT(sizeof(ValidBip01Names) == 0x2B0);

// 20
struct FaceGenData
{
	UInt32		unk00;		// 00
	void		*unk04;		// 04
	UInt32		unk08;		// 08
	float		**values;	// 0C
	UInt32		useOffset;	// 10
	UInt32		maxOffset;	// 14
	UInt32		count;		// 18
	UInt32		size;		// 1C
};

// 1EC
class TESNPC : public TESActorBase
{
public:
	TESNPC();
	~TESNPC();

	TESRaceForm				race;				// 10C
	UInt8					skillValues[14];	// 114
	UInt8					skillOffsets[14];	// 122
	TESClass				*classID;			// 130
	FaceGenData				faceGenData[3];		// 134
	UInt32					unk194[8];			// 194
	FaceGenData				*faceGenDataPtr;	// 1B4
	TESHair					*hair;				// 1B8
	float					hairLength;			// 1BC
	TESEyes					*eyes;				// 1C0
	BSFaceGenNiNode			*unk1C4;			// 1C4
	BSFaceGenNiNode			*unk1C8;			// 1C8
	UInt32					unk1CC;				// 1CC
	UInt16					unk1D0;				// 1D0
	UInt16					unk1D2;				// 1D2
	TESCombatStyle			*combatStyle;		// 1D4
	UInt32					hairColor;			// 1D8
	tList<BGSHeadPart>		headPart;			// 1DC
	UInt32					impactMaterialType;	// 1E4
	UInt32					unk01E8;			// 1E8
	TESRace					*race1EC;			// 1EC
	TESNPC					*copyFrom;			// 1F0	Not set once PlayerRef exists and the target is the Player
	float					height;				// 1F4
	float					weight;				// 1F8	Aparently, getWeight purposly returns height except for the player.
	NiTArray<FaceGenUndo*>	faceGenUndo;		// 1FC

	void SetSex(UInt32 flags);
	void SetRace(TESRace *pRace);
	void CopyAppearance(TESNPC *srcNPC);
};

STATIC_ASSERT(sizeof(TESNPC) == 0x20C);

// 160
class TESCreature : public TESActorBase
{
public:
	TESCreature();
	~TESCreature();

	enum Types
	{
		kAnimal = 0,
		kMutatedAnimal,
		kMutatedInsect,
		kAbomination,
		kSupermutant,
		kFeralGhoul,
		kRobot,
		kGiant
	};

	TESAttackDamageForm			attackDmg;			// 10C
	TESModelList				modelList;			// 114

	TESCreature					*audioTemplate;		// 128
	UInt8						type;				// 12C
	UInt8						combatSkill;		// 12D
	UInt8						magicSkill;			// 12E
	UInt8						stealthSkill;		// 12F
	UInt8						attackReach;		// 130
	UInt8						pad0131[3];			// 131
	float						turningSpeed;		// 134
	float						footWeight;			// 138
	float						baseScale;			// 13C
	TESCombatStyle				*combatStyle;		// 140
	BGSBodyPartData				*bodyPartData;		// 144
	UInt32						materialType;		// 148
	BGSImpactDataSet			*impactDataSet;		// 14C
	UInt32						unk0150;			// 150
	UInt32						soundLevel;			// 154
	BGSListForm					*weaponList;		// 158
	UInt8						byt015C;			// 15C
	UInt8						pad015D[3];			// 15D
};

// 34
class TESLeveledList : public BaseFormComponent
{
public:
	struct LoadBaseData	// as used by LoadForm
	{
		SInt16			level;		// 00
		UInt16			fill002;	// 02
		TESObjectREFR	*refr;		// 04
		SInt16			count;		// 08
		UInt16			fill00A;	// 0A
	};	// 0C

	struct ListData
	{
		TESForm			*form;		// 00
		SInt16			count;		// 04
		SInt16			level;		// 06
		LvlListExtra	*extra;		// 08
	}; // 0C

	enum
	{
		kFlags_CalcAllLevels =		1 << 0,
		kFlags_CalcEachInCount =	1 << 1,
		kFlags_UseAll =				1 << 2,
	};

	tList<ListData>		list;			// 04
	UInt8				chanceNone;		// 0C
	UInt8				flags;			// 0D
	UInt16				pad00E;			// 0E
	TESGlobal			*global;		// 10 use global value for chance none?
	ExtraDataList		extraDatas;		// 14 ??? BaseExtraList::DebugDump() shows no data

	void AddItem(TESForm *form, UInt16 level, UInt16 count, float health);
	UInt32 RemoveItem(TESForm *form);
	void Dump();
	bool RemoveNthItem(UInt32 itemIndex);
	SInt32 GetItemIndexByForm(TESForm *form);
	SInt32 GetItemIndexByLevel(UInt32 level);
};

// TESLevCreature (68)
class TESLevCreature : public TESBoundObject
{
public:
	TESLevCreature();
	~TESLevCreature();

	TESLeveledList		list;		// 030
	TESModelTextureSwap	texture;	// 04C
};

// TESLevCharacter (68)
class TESLevCharacter : public TESBoundObject
{
public:
	TESLevCharacter();
	~TESLevCharacter();

	TESLeveledList		list;		// 030
	TESModelTextureSwap	texture;	// 04C
};

// TESKey (A8)
class TESKey : public TESObjectMISC
{
public:
	TESKey();
	~TESKey();
};

// D8
class AlchemyItem : public TESBoundObject
{
public:
	AlchemyItem();
	~AlchemyItem();

	MagicItem					magicItem;				// 30
	TESModelTextureSwap			model;					// 4C
	TESIcon						icon;					// 6C
	BGSMessageIcon				messageIcon;			// 78
	TESScriptableForm			scriptable;				// 88
	TESWeightForm				weight;					// 94
	BGSEquipType				equipType;				// 9C
	BGSDestructibleObjectForm	destructible;			// A4
	BGSPickupPutdownSounds		pickupPutdownsounds;	// AC
	UInt32						value;					// B8
	UInt8						alchFlags;				// BC
	UInt8						padBD[3];				// BD
	SpellItem					*withdrawalEffect;		// C0
	float						addictionChance;		// C4
	TESSound					*consumeSound;			// C8
	TESIcon						iconCC;					// CC

	bool IsPoison();
};

STATIC_ASSERT(sizeof(AlchemyItem) == 0xD8);

// BGSIdleMarker (40)
class BGSIdleMarker;

// BGSNote (80)
class BGSNote : public TESBoundObject
{
public:
	BGSNote();
	~BGSNote();

	enum Type
	{
		kSound = 0,
		kText = 1,
		kImage = 2,
		kVoice = 3,
	};

	// bases
	TESModel					model;					// 30
	TESFullName					fullName;				// 48
	TESIcon						icon;					// 54
	BGSPickupPutdownSounds		pickupPutdownSounds;	// 60
	union												// 6C
	{
		TESDescription			*noteText;
		TESTexture				*picture;
		TESTopic				*voice;
	};
	UInt32						unk70;					// 70
	UInt32						unk74;					// 74
	UInt32						unk78;					// 78
	UInt8                       noteType;				// 7C
	UInt8                       read;					// 7D
	UInt8                       byte7E;					// 7E
	UInt8                       byte7F;					// 7F

	void MarkAsRead()
	{
		this->read = 1;
		this->MarkAsModified(0x80000000);
	}
};
STATIC_ASSERT(sizeof(BGSNote) == 0x80);

// BGSConstructibleObject (B0)
class BGSConstructibleObject;

// C0
class BGSProjectile : public TESBoundObject
{
public:
	BGSProjectile();
	~BGSProjectile();

	enum
	{
		kFlags_Hitscan =				0x1,
		kFlags_Explosion =				0x2,
		kFlags_AltTrigger =				0x4,
		kFlags_MuzzleFlash =			0x8,
		//								0x10,
		kFlags_CanBeDisabled =			0x20,
		kFlags_CanBePicked =			0x40,
		kFlags_Supersonic =				0x80,
		kFlags_PinsLimbs =				0x100,
		kFlags_PassSmallTransparent =	0x200,
		kFlags_Detonates =				0x400,
		kFlags_Rotation =				0x800,
	};

	TESFullName						fullName;			// 30
	TESModel						model;				// 3C
	BGSPreloadable					preloadable;		// 54
	BGSDestructibleObjectForm		destructible;		// 58

	UInt16							projFlags;			// 60
	UInt16							type;				// 62
	float							gravity;			// 64
	float							speed;				// 68
	float							range;				// 6C
	TESObjectLIGH					*lightProjectile;	// 70
	TESObjectLIGH					*lightMuzzleFlash;	// 74
	float							tracerChance;		// 78
	float							altProximity;		// 7C
	float							altTimer;			// 80
	BGSExplosion					*explosion;			// 84
	TESSound						*soundProjectile;	// 88
	float							flashDuration;		// 8C
	float							fadeDuration;		// 90
	float							impactForce;		// 94
	TESSound						*soundCountDown;	// 98
	TESSound						*soundDisable;		// 9C
	TESObjectWEAP					*defaultWeapSrc;	// A0
	float							rotationX;			// A4
	float							rotationY;			// A8
	float							rotationZ;			// AC
	float							bouncyMult;			// B0
	TESModel						muzzleFlash;		// B4
	UInt8							soundLevel;			// CC

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) projFlags |= pFlag;
		else projFlags &= ~pFlag;
	}
};

// TESLevItem (44)
class TESLevItem : public TESBoundObject
{
public:
	TESLevItem();
	~TESLevItem();

	TESLeveledList		list;
};

// 36C
class TESWeather : public TESForm
{
public:
	TESWeather();
	~TESWeather();

	struct WeatherSound
	{
		UInt32		soundID;	// refID of TESSound
		UInt32		type;		// 0 - Default; 1 - Precip; 2 - Wind; 3 - Thunder
	};

	UInt32					unk018;						// 018	TESImageSpaceModifiableCountForm<6>
	TESImageSpaceModifier	*imageSpaceMods[6];			// 01C
	TESTexture1024			layerTextures[4];			// 034
	UInt8					cloudSpeed[4];				// 064
	UInt32					cloudColor[4][6];			// 068
	TESModel				model;						// 0C8
	UInt8					windSpeed;					// 0E0
	UInt8					cloudSpeedLower;			// 0E1
	UInt8					cloudSpeedUpper;			// 0E2
	UInt8					transDelta;					// 0E3
	UInt8					sunGlare;					// 0E4
	UInt8					sunDamage;					// 0E5
	UInt8					precipitationBeginFadeIn;	// 0E6
	UInt8					precipitationEndFadeOut;	// 0E7
	UInt8					lightningBeginFadeIn;		// 0E8
	UInt8					lightningEndFadeOut;		// 0E9
	UInt8					lightningFrequency;			// 0EA
	UInt8					weatherClassification;		// 0EB
	UInt32					lightningColor;				// 0EC
	float					fogDistance[6];				// 0F0
	UInt32					colors[10][6];				// 108
	tList<WeatherSound>		sounds;						// 1F8
	UInt32					unk200[91];					// 200
};
STATIC_ASSERT(sizeof(TESWeather) == 0x36C);

struct WeatherEntry
{
	TESWeather		*weather;
	UInt32			chance;
	TESGlobal		*global;
};
typedef tList<WeatherEntry> WeatherTypes;

// 58
class TESClimate : public TESForm
{
public:
	TESClimate();
	~TESClimate();

	TESModel			nightSkyModel;		// 18
	WeatherTypes		weatherTypes;		// 30
	TESTexture			sunTexture;			// 38
	TESTexture			sunGlareTexture;	// 44
	UInt8				sunriseBegin;		// 50
	UInt8				sunriseEnd;			// 51
	UInt8				sunsetBegin;		// 52
	UInt8				sunsetEnd;			// 53
	UInt8				volatility;			// 54
	UInt8				phaseLength;		// 55
	UInt8				pad56[2];			// 56

	WeatherEntry *GetWeatherEntry(TESWeather *weather, bool remove);
};

STATIC_ASSERT(sizeof(TESClimate) == 0x58);

// 08
class TESRegionData
{
public:
	TESRegionData();
	~TESRegionData();

	enum
	{
		kRegionData_Weather = 3,
		kRegionData_Map,
		kRegionData_Landscape,
		kRegionData_Grass,
		kRegionData_Sound,
		kRegionData_Imposter
	};

	virtual TESRegionData	*Destroy(bool doFree);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual UInt32	GetType();
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);

	bool				bOverride;	// 04
	UInt8				byte05;		// 05
	UInt8				priority;	// 06
	UInt8				byte07;		// 07
};
typedef tList<TESRegionData> RegionDataEntryList;

class TESRegionDataGrass : public TESRegionData
{
public:
	TESRegionDataGrass();
	~TESRegionDataGrass();

	virtual void	Unk_0A(void);
};

// 10
class TESRegionDataImposter : public TESRegionData
{
public:
	TESRegionDataImposter();
	~TESRegionDataImposter();

	tList<TESObjectREFR>	imposters;	// 08
};

class TESRegionDataLandscape : public TESRegionData
{
public:
	TESRegionDataLandscape();
	~TESRegionDataLandscape();

	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
};

// 10
class TESRegionDataMap : public TESRegionData
{
public:
	TESRegionDataMap();
	~TESRegionDataMap();

	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);

	String		mapName;	// 08
};

struct SoundType
{
	TESSound		*sound;
	UInt32			flags;
	UInt32			chance;
};
typedef tList<SoundType> SoundTypeList;

class TESRegionDataSound : public TESRegionData
{
public:
	TESRegionDataSound();
	~TESRegionDataSound();

	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);

	UInt32			unk08;
	SoundTypeList	soundTypes;
	UInt32			incidentalMediaSet;
	tList<UInt32>	mediaSetEntries;
};

// 10
class TESRegionDataWeather : public TESRegionData
{
public:
	TESRegionDataWeather();
	~TESRegionDataWeather();

	WeatherTypes	weatherTypes;	// 08
};

struct AreaPointEntry
{
	float	x;
	float	y;
};
typedef tList<AreaPointEntry> AreaPointEntryList;

struct RegionAreaEntry
{
	AreaPointEntryList	points;
	UInt32				unk08[2];
	float				unk10[4];
	UInt32				edgeFallOff;
	UInt32				pointCount;
};
typedef tList<RegionAreaEntry> RegionAreaEntryList;

// 38
class TESRegion : public TESForm
{
public:
	TESRegion();
	~TESRegion();

	RegionDataEntryList	*dataEntries;	// 18
	RegionAreaEntryList	*areaEntries;	// 1C
	TESWorldSpace		*worldSpace;	// 20
	TESWeather			*weather;		// 24
	UInt32				unk28;			// 28
	float				flt2C;			// 2C
	float				flt30;			// 30
	float				flt34;			// 34
};
STATIC_ASSERT(sizeof(TESRegion) == 0x38);

// 10
class TESRegionList : public BSSimpleList<TESRegion>
{
public:
	TESRegionList();
	~TESRegionList();

	UInt8			byte0C;		// 0C
	UInt8			pad0D[3];	// 0D
};

// NavMeshInfoMap (40)
class NavMeshInfoMap;

class NavMesh;

// E0
class TESObjectCELL : public TESForm
{
public:
	TESObjectCELL();
	~TESObjectCELL();

	typedef tList<TESObjectREFR> RefList;
	
	struct ExteriorCoords
	{
		SInt32		x;			// 00
		SInt32		y;			// 04
		UInt8		byte08;		// 08
		UInt8		pad09[3];	// 09
	};
	struct Color
	{
		UInt8 r;
		UInt8 g;
		UInt8 b;
		UInt8 alpha;
	};
	struct LightingData
	{
		Color		ambientRGB;		// 00
		Color		directionalRGB;		// 04
		Color		fogRGB;		// 08
		float		fogNear;		// 0C
		float		fogFar;		// 10
		int			directionalRotXY;		// 14
		int			directionalRotZ;		// 18
		float		directionalFade;		// 1C
		float		fogClipDist;		// 20
		float		fogPower;		// 24
		UInt32*		getValuesFrom;		// 28
	};

	union CellCoordinates
	{
		ExteriorCoords	*exterior;
		LightingData	*interior;
	};

	// 64
	struct CellRenderData
	{
		NiNode									*masterNode;	// 00
		tList<TESObjectREFR>					list04;			// 04
		NiTMapBase<TESObjectREFR*, NiNode*>		map0C;			// 0C
		NiTMapBase<TESForm*, TESObjectREFR*>	map1C;			// 1C
		NiTMapBase<TESObjectREFR*, NiNode*>		map2C;			// 2C
		NiTMapBase<TESObjectREFR*, NiNode*>		map3C;			// 3C
		tList<TESObjectREFR>					list4C;			// 4C
		tList<void>								list54;			// 54
		tList<TESObjectREFR>					list5C;			// 5C
	};

	enum
	{
		kCellFlag_IsInterior =					1 << 0,
		kCellFlag_HasWater =					1 << 1,
		kCellFlag_InvertFastTravelBehavior =	1 << 2,
		kCellFlag_ForceHideLand =				1 << 3,
		kCellFlag_PublicPlace =					1 << 5,
		kCellFlag_HandChanged =					1 << 6,
		kCellFlag_BehaveLikeExterior =			1 << 7,
	};

	TESFullName				fullName;				// 18
	UInt8					cellFlags;				// 24
	UInt8					byte25;					// 25
	UInt8					byte26;					// 26	5 or 6 would mean cell is loaded
	UInt8					byte27;					// 27
	ExtraDataList			extraDataList;			// 28
	CellCoordinates			coords;					// 48
	TESObjectLAND			*land;					// 4C
	float					waterHeight;			// 50
	UInt32					unk54;					// 54
	TESTexture				noiseTexture;			// 58
	BSSimpleArray<NavMesh>	*navMeshArray;			// 64
	UInt32					unk68[6];				// 68
	void					*refLockSemaphore;		// 80
	UInt32					unk84[8];				// 84
	UInt32					actorCount;				// A4
	UInt16					countVisibleDistant;	// A8
	UInt16					unkAA;					// AA
	RefList					objectList;				// AC
	NiNode					*niNodeB4;				// B4
	NiNode					*niNodeB8;				// B8
	UInt32					unkBC;					// BC
	TESWorldSpace			*worldSpace;			// C0
	CellRenderData			*renderData;			// C4
	float					fltC8;					// C8
	UInt8					byteCC;					// CC
	UInt8					byteCD;					// CD
	UInt8					byteCE;					// CE
	UInt8					byteCF;					// CF
	UInt8					byteD0;					// D0
	UInt8					byteD1;					// D1
	UInt8					byteD2;					// D2
	UInt8					byteD3;					// D3
	BSPortalGraph			*portalGraph;			// D4
	BGSLightingTemplate		*lightingTemplate;		// D8
	UInt32					inheritFlags;			// DC

	bool IsInterior() { return worldSpace == NULL; }
	NiNode *Get3DNode(UInt32 index);
	void ToggleNodes(UInt32 nodeBits, UInt8 doHide);
};
STATIC_ASSERT(sizeof(TESObjectCELL) == 0xE0);

// 3C	Init proc: 0x6FC490
struct LODdata
{
	// 60
	struct LODNode
	{
		LODdata			*parent;		// 00
		UInt32			lodLevel;		// 04
		Coordinate		cellXY;			// 08
		UInt8			byte0C;			// 0C
		UInt8			byte0D;			// 0D
		UInt8			byte0E;			// 0E
		UInt8			byte0F;			// 0F
		UInt32			ukn10;			// 10
		void			*object;		// 14
		UInt32			ukn18;			// 18
		UInt32			ukn1C;			// 1C
		LODNode			*linked[4];		// 20
		UInt32			unk30;			// 30
		float			flt34;			// 34
		float			flt38;			// 38
		float			flt3C;			// 3C
		float			flt40;			// 40
		float			flt44;			// 44
		float			flt48;			// 48
		float			flt4C;			// 4C
		UInt32			unk50;			// 50
		UInt32			ukn54;			// 54
		UInt32			ukn58;			// 58
		UInt8			byte5C;			// 5C
		UInt8			byte5D;			// 5D
		UInt8			byte5E;			// 5E
		UInt8			byte5F;			// 5F

		LODNode *GetNodeByCoord(UInt32 coord);
	};
	STATIC_ASSERT(sizeof(LODNode) == 0x60);

	TESWorldSpace					*world;		// 00
	LODNode							*lodNode;	// 04
	NiNode							*node08;	// 08
	NiNode							*node0C;	// 0C
	Coordinate						coordNW;	// 10
	Coordinate						coordSE;	// 14
	UInt32							ukn18;		// 18
	UInt32							ukn1C;		// 1C
	UInt32							ukn20;		// 20
	UInt32							lodLevel;	// 24
	UInt8							byte28;		// 28
	UInt8							byte29;		// 29
	UInt8							byte2A;		// 2A
	UInt8							byte2B;		// 2B
	BSSimpleArray<TESObjectREFR>	array2C;	// 2C
};
STATIC_ASSERT(sizeof(LODdata) == 0x3C);

typedef NiTPointerMap<TESObjectCELL> CellPointerMap;

// EC
class TESWorldSpace : public TESForm
{
public:
	TESWorldSpace();
	~TESWorldSpace();

	virtual bool	Unk_4E(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
	virtual void	Unk_4F(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4, UInt32 arg5, UInt32 arg6);

	struct DCoordXY
	{
		SInt32	X;
		SInt32	Y;
	};

	struct WCoordXY
	{
		SInt16	X;
		SInt16	Y;
	};

	struct Offset_Data
	{
		UInt32		**unk00;	// 00 array of UInt32 stored in OFST record.
		CoordXY		min;		// 04 NAM0
		CoordXY		max;		// 0C NAM9
		UInt32		fileOffset;	// 14 TESWorldspace file offset in modInfo
	};	// 014

	struct MapData
	{
		DCoordXY	usableDimensions;	// 00
		WCoordXY	cellNWCoordinates;	// 08
		WCoordXY	cellSECoordinates;	// 0C
	};	// 010

	struct ImpactData
	{
		typedef NiTMapBase<BGSImpactData*, BGSImpactData*> ImpactImpactMap;
		enum MaterialType
		{
			eMT_Stone,
			eMT_Dirt,
			eMT_Grass,
			eMT_Glass,
			eMT_Metal,
			eMT_Wood,
			eMT_Organic,
			eMT_Cloth,
			eMT_Water,
			eMT_HollowMetal,
			eMT_OrganicBug,
			eMT_OrganicGlow,
			eMT_Max
		};

		ImpactImpactMap		*impactImpactMap[eMT_Max];	// 000
		char				footstepMaterials[0x12C];	// 030
	};

	typedef NiTPointerMap<BSSimpleList<TESObjectREFR> > RefListPointerMap;
	typedef NiTMapBase<ModInfo*, TESWorldSpace::Offset_Data*> OffsetDataMap;

	enum
	{
		kWorldFlag_SmallWorld =			1 << 0,
		kWorldFlag_NoFastTravel =		1 << 1,
		kWorldFlag_NoLODWater =			1 << 4,
		kWorldFlag_NoLODNoise =			1 << 5,
		kWorldFlag_NoFallDamage =		1 << 6,
		kWorldFlag_WaterAdjustment =	1 << 7,

		kParentFlag_UseLandData =		1 << 0,
		kParentFlag_UseLODData =		1 << 1,
		kParentFlag_UseMapData =		1 << 2,
		kParentFlag_UseWaterData =		1 << 3,
		kParentFlag_UseClimateData =	1 << 4,
		kParentFlag_UseISData =			1 << 5,
	};

	TESFullName			fullName;			// 18
	TESTexture			texture;			// 24
	CellPointerMap		*cellMap;			// 30
	TESObjectCELL		*cell;				// 34
	UInt32				unk38;				// 38
	LODdata				*lodData;			// 3C
	TESClimate			*climate;			// 40
	TESImageSpace		*imageSpace;		// 44
	ImpactData			*impacts;			// 48
	UInt8				flags;				// 4C
	UInt8				unk4D;				// 4D
	UInt16				parentFlags;		// 4E
	RefListPointerMap	pointerMap;			// 50
	tList<void>			lst60;				// 60
	tList<void>			lst68;				// 68
	TESWorldSpace		*parent;			// 70
	TESWaterForm		*waterFormFirst;	// 74
	TESWaterForm		*waterFormLast;		// 78
	float				waterLODHeight;		// 7C
	MapData				mapData;			// 80
	float				worldMapScale;		// 90
	float				worldMapCellX;		// 94
	float				worldMapCellY;		// 98
	BGSMusicType		*musicType;			// 9C
	CoordXY				min;				// A0
	CoordXY				max;				// A8
	OffsetDataMap		offsetMap;			// B0
	String				editorID;			// C0
	float				defaultLandHeight;	// C8
	float				defaultWaterHeight;	// CC
	BGSEncounterZone	*encounterZone;		// D0
	TESTexture			canopyShadow;		// D4
	TESTexture			waterNoiseTexture;	// E0

	TESWorldSpace *GetRootMapWorld();
};
STATIC_ASSERT(sizeof(TESWorldSpace) == 0xEC);

// 04
class TESChildCell
{
public:
	TESChildCell();
	~TESChildCell();

	virtual TESObjectCELL	*GetChildCell();
};

// 2C
class TESObjectLAND : public TESForm
{
public:
	TESObjectLAND();
	~TESObjectLAND();

	// A4
	struct LandData
	{
		//	Note: All arrays in the structs are of 289 elements.
		struct Geometry
		{
			NiVector3		*quad0Vertices;
			NiVector3		*quad1Vertices;
			NiVector3		*quad2Vertices;
			NiVector3		*quad3Vertices;
		};

		struct Struct08
		{
			NiVector3		*quad0Unk;
			NiVector3		*quad1Unk;
			NiVector3		*quad2Unk;
			NiVector3		*quad3Unk;
		};

		struct Struct0C
		{
			NiVector4		*quad0Unk;
			NiVector4		*quad1Unk;
			NiVector4		*quad2Unk;
			NiVector4		*quad3Unk;
		};

		struct Struct10
		{
			UInt8			*quad0Unk;
			UInt8			*quad1Unk;
			UInt8			*quad2Unk;
			UInt8			*quad3Unk;
		};

		struct Struct30
		{
			UInt32			unk00;
			UInt32			unk04;
			UInt32			unk08;
			UInt32			unk0C;
			UInt32			unk10;
			UInt32			unk14;
		};

		struct GrassAreaParam;
		typedef NiTPointerMap<GrassAreaParam*> GrassAreaParamMap;

		void					*ptr00;			// 00
		Geometry				*geometry;		// 04
		Struct08				*ptr08;			// 08
		Struct0C				*ptr0C;			// 0C
		void					*ptr10;			// 10
		NiObject				*object14;		// 14
		float					minHeight;		// 18
		float					maxHeight;		// 1C
		TESLandTexture			*textures20[4];	// 20
		Struct30				*ptrs30[4];		// 30
		void					**ptrs40[4];	// 40
		UInt32					unk50;			// 50
		GrassAreaParamMap		grassParams54;	// 54
		GrassAreaParamMap		grassParams64;	// 64
		GrassAreaParamMap		grassParams74;	// 74
		GrassAreaParamMap		grassParams84;	// 84
		NiObject				*object94;		// 94
		SInt32					cellCoordX;		// 98
		SInt32					cellCoordY;		// 9C
		float					meanHeight;		// A0
	};

	TESChildCell		childCell;		// 18
	UInt32				landFlags;		// 1C
	TESObjectCELL		*cell;			// 20
	QueuedFile			*queuedFile;	// 24
	LandData			*landData;		// 28
};
STATIC_ASSERT(sizeof(TESObjectLAND) == 0x2C);

struct VariableInfo
{
	UInt32			idx;		// 00
	UInt32			pad04;		// 04
	double			data;		// 08
	UInt8			type;		// 10
	UInt8			pad11[3];	// 11
	UInt32			unk14;		// 14
	String			name;		// 18
};

// TESQuest (6C)
class TESQuest : public TESForm
{
public:
	TESQuest();
	~TESQuest();

	virtual char *GetEditorName() const;

	TESScriptableForm		scriptable;			// 18
	TESIcon					icon;				// 24
	TESFullName				fullName;			// 30

	struct StageInfo
	{
		UInt8			stage;		// 00 stageID
		UInt8			isDone;		// 01 status ?
		UInt8			pad[2];		// 02
		tList<void>		unk004;		// 04 log entries
	};

	enum
	{
		kCompleted = 0x2,
		kStarted = 0x20
	};

	UInt8					flags;				// 3C	bit0 is startGameEnabled/isRunning
	UInt8					priority;			// 3D
	UInt8					pad3E[2];			// 3E
	float					questDelayTime;		// 40
	tList<StageInfo>		stages;				// 44
	tList<void>				lVarOrObjectives;	// 4C
		// So: this list would contain both Objectives and LocalVariables !
		// That seems very strange but still, looking at Get/SetObjective... and ShowQuestVars there's no doubt.
	ConditionList			conditions;			// 54
	ScriptEventList			*scriptEventList;	// 5C
	UInt8					currentStage;		// 60
	UInt8					pad61[3];			// 61
	String					editorName;			// 64

	bool SetStage(UInt8 stageID);
	BGSQuestObjective *GetObjective(UInt32 objectiveID);
};
STATIC_ASSERT(sizeof(TESQuest) == 0x6C);

class TESPackageData
{
public:
	TESPackageData();
	~TESPackageData();
	virtual void	Destroy(bool free);
	virtual void	CopyFrom(TESPackageData * packageData);
	virtual void	Unk_02(void);
	virtual void	Save(ModInfo* modInfo);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
};

class TESPatrolPackageData : public TESPackageData
{
public:
	TESPatrolPackageData();
	~TESPatrolPackageData();

	UInt8	patrolFlags;
};

enum
{
	kPackageFlag_OffersServices =			1 << 0,
	kPackageFlag_MustReachLocation =		1 << 1,
	kPackageFlag_MustComplete =				1 << 2,
	kPackageFlag_LockDoorsAtStart =			1 << 3,
	kPackageFlag_LockDoorsAtEnd =			1 << 4,
	kPackageFlag_LockDoorsAtLocation =		1 << 5,
	kPackageFlag_UnlockDoorsAtStart =		1 << 6,
	kPackageFlag_UnlockDoorsAtEnd =			1 << 7,
	kPackageFlag_UnlockDoorsAtLocation =	1 << 8,
	kPackageFlag_ContinueIfPCNear =			1 << 9,
	kPackageFlag_OncePerDay =				1 << 10,
	kPackageFlag_Unk11 =					1 << 11,
	kPackageFlag_SkipFalloutBehavior =		1 << 12,
	kPackageFlag_AlwaysRun =				1 << 13,
	kPackageFlag_Unk14 =					1 << 14,
	kPackageFlag_NeverRun =					1 << 15,
	kPackageFlag_Unk16 =					1 << 16,
	kPackageFlag_AlwaysSneak =				1 << 17,
	kPackageFlag_AllowSwimming =			1 << 18,
	kPackageFlag_AllowFalls =				1 << 19,
	kPackageFlag_ArmorUnequipped =			1 << 20,
	kPackageFlag_WeaponsUnequipped =		1 << 21,
	kPackageFlag_DefensiveCombat =			1 << 22,
	kPackageFlag_WeaponsDrawn =				1 << 23,
	kPackageFlag_NoIdleAnims =				1 << 24,
	kPackageFlag_PretendInCombat =			1 << 25,
	kPackageFlag_ContinueDuringCombat =		1 << 26,
	kPackageFlag_NoCombatAlert =			1 << 27,
	kPackageFlag_NoWarnAttackBehavior =		1 << 28,
	kPackageFlag_AlwaysWalk =				1 << 29,	//	JIP LN
	kPackageFlag_Unk30 =					1 << 30,
	kPackageFlag_Unk31 =					1 << 31
};

// 04
class ActorPackageData
{
public:
	ActorPackageData();
	~ActorPackageData();

	virtual void	Destroy(bool doFree);
	virtual void	Unk_01(TESObjectREFR *refr);
	virtual UInt32	GetPackageType();
	virtual void	SaveGame(BGSSaveLoadGame *saveLoadGame);
	virtual void	LoadGame(BGSSaveLoadGame *saveLoadGame);
	virtual void	Unk_05(UInt32 arg);
};

// 64
class SandBoxActorPackageData : public ActorPackageData
{
public:
	SandBoxActorPackageData();
	~SandBoxActorPackageData();

	struct SandboxChoice
	{
		TESObjectREFR	*markerRef;		// 00
		UInt32			unk04;			// 04
		UInt32			unk08;			// 08
		UInt32			unk0C;			// 0C
	};

	UInt32							unk04;		// 04
	UInt32							unk08;		// 08
	float							flt0C;		// 0C
	UInt32							unk10;		// 10
	float							flt14;		// 14
	float							flt18;		// 18
	UInt32							unk1C;		// 1C
	BSSimpleArray<SandboxChoice>	choices;	// 20
	UInt32							unk30;		// 30
	UInt8							byte34;		// 34
	UInt8							byte35;		// 35
	UInt8							byte36;		// 36
	UInt8							byte37;		// 37
	UInt32							unk38;		// 38
	UInt32							unk3C;		// 3C
	UInt8							byte40;		// 40
	UInt8							pad41[3];	// 41
	UInt32							unk44;		// 44
	UInt32							unk48;		// 48
	UInt32							unk4C;		// 4C
	float							flt50;		// 50
	float							flt54;		// 54
	UInt32							unk58;		// 58
	UInt8							byte5C[6];	// 5C
	UInt8							pad62[2];	// 62
};
STATIC_ASSERT(sizeof(SandBoxActorPackageData) == 0x64);

// 18
class EscortActorPackageData : public ActorPackageData
{
public:
	EscortActorPackageData();
	~EscortActorPackageData();

	UInt32				unk04;		// 04
	UInt32				unk08;		// 08
	UInt32				unk0C;		// 0C
	tList<void>			list10;		// 10
};

// 14
class GuardActorPackageData : public ActorPackageData
{
public:
	GuardActorPackageData();
	~GuardActorPackageData();

	UInt32				unk04;		// 04
	UInt32				unk08;		// 08
	UInt32				unk0C;		// 0C
	float				flt10;		// 10
};

// 38
class PatrolActorPackageData : public ActorPackageData
{
public:
	PatrolActorPackageData();
	~PatrolActorPackageData();

	UInt32							unk04;		// 04
	UInt32							unk08;		// 08
	UInt32							unk0C;		// 0C
	UInt32							unk10;		// 10
	UInt32							unk14;		// 14
	UInt32							unk18;		// 18
	BSSimpleArray<TESObjectREFR>	pointRefs;	// 1C
	UInt32							unk2C[3];	// 2C
};
STATIC_ASSERT(sizeof(PatrolActorPackageData) == 0x38);

// 08
class UseWeaponActorPackageData : public ActorPackageData
{
public:
	UseWeaponActorPackageData();
	~UseWeaponActorPackageData();

	UInt32				unk04;		// 04
};

// TESPackage (80)
class TESPackage : public TESForm
{
public:
	TESPackage();
	~TESPackage();

	virtual void	Unk_4E();
	virtual void	Unk_4F();
	virtual void	Unk_50();
	virtual void	Unk_51();
	virtual void	Unk_52();
	virtual void	Unk_53();
	virtual void	Unk_54();
	virtual void	Unk_55();
	virtual void	Unk_56();
	virtual void	Unk_57();

	enum
	{
		kPackageType_Explore,
		kPackageType_Follow,
		kPackageType_Escort,
		kPackageType_Eat,
		kPackageType_Sleep,
		kPackageType_Wander,
		kPackageType_Travel,
		kPackageType_Accompany,
		kPackageType_UseItemAt,
		kPackageType_Ambush,
		kPackageType_FleeNotCombat,
		kPackageType_CastMagic,
		kPackageType_Sandbox,
		kPackageType_Patrol,
		kPackageType_Guard,
		kPackageType_Dialogue,
		kPackageType_UseWeapon,
		kPackageType_Find,
		kPackageType_Combat,
		kPackageType_CombatLow,
		kPackageType_Activate,
		kPackageType_Alarm,
		kPackageType_Flee,
		kPackageType_Trespass,
		kPackageType_Spectator,
		kPackageType_ReactToDead,
		kPackageType_GetUp,
		kPackageType_DoNothing,
		kPackageType_InGameDialogue,
		kPackageType_Surface,
		kPackageType_SearchForAttacker,
		kPackageType_AvoidRadiation,
		kPackageType_ReactToDestroyedObject,
		kPackageType_ReactToGrenadeOrMine,
		kPackageType_StealWarning,
		kPackageType_PickpocketWarning,
		kPackageType_MovementBlocked,
		kPackageType_Sandman,
		kPackageType_Cannibal,
		kPackType_MAX
	};

	// 8
	struct PackageTime
	{
		enum
		{
			kDay_Any = 0,
			kTime_Any = 0xFF,
		};

		enum
		{
			kMonth_January = 0,
			kMonth_February,
			kMonth_March,
			kMonth_April,
			kMonth_May,
			kMonth_June,
			kMonth_July,
			kMonth_August,
			kMonth_September,
			kMonth_October,
			kMonth_November,
			kMonth_December,
			kMonth_Spring,	// march, april, may
			kMonth_Summer,	// june, july, august
			kMonth_Autumn,	// september, august, november (in Geck)
			kMonth_Winter,	// december, january, february

			kMonth_Any = 0xFF,
		};

		enum
		{
			kWeekday_Sundays = 0,
			kWeekday_Morndays,
			kWeekday_Tuesdays,
			kWeekday_Wednesdays,
			kWeekday_Thursdays,
			kWeekday_Frydays,
			kWeekday_Saturdays,
			kWeekday_Weekdays,
			kWeekday_Weekends,
			kWeekday_MWF,
			kWeekday_TT,

			kWeekday_Any = 0xFF
		};

		UInt8	month;
		UInt8	weekDay;
		UInt8	date;
		UInt8	time;
		UInt32	duration;

		static const char* MonthForCode(UInt8 monthCode);
		static const char* DayForCode(UInt8 dayCode);
		static UInt8 CodeForMonth(const char* monthStr);
		static UInt8 CodeForDay(const char* dayStr);
		static bool IsValidMonth(UInt8 m) { return (m+1) <= kMonth_Winter; }
		static bool IsValidTime(UInt8 t) { return (t+1) <= 24; }
		static bool IsValidDay(UInt8 d) { return (d+1) <= kWeekday_TT; }
		static bool IsValidDate(UInt8 d) { return d <= 31; }
	};

	union ObjectType
	{
		TESForm			* form;
		TESObjectREFR	* refr;
		UInt32			objectCode;
	};

	// order only somewhat related to kFormType_XXX (values off by 17, 20, or 21)
	enum	// From OBSE and FNVEdit
	{	
		kObjectType_None	=	0,
		kObjectType_Activators,
		kObjectType_Armor,
		kObjectType_Books,
		kObjectType_Clothing,
		kObjectType_Containers,
		kObjectType_Doors,
		kObjectType_Ingredients,
		kObjectType_Lights,
		kObjectType_Misc,
		kObjectType_Flora,
		kObjectType_Furniture,
		kObjectType_WeaponsAny,
		kObjectType_Ammo,
		kObjectType_NPCs,
		kObjectType_Creatures,
		kObjectType_Keys,				//	10
		kObjectType_Alchemy,
		kObjectType_Food,
		kObjectType_AllCombatWearable,
		kObjectType_AllWearable,
		kObjectType_WeaponsRanged,
		kObjectType_WeaponsMelee,
		kObjectType_WeaponsNone,
		kObjectType_ActorEffectAny,
		kObjectType_ActorEffectRangeTarget,
		kObjectType_ActorEffectRangeTouch,
		kObjectType_ActorEffectRangeSelf,
		kObjectType_ActorsAny,

		kObjectType_Max,						//	1E
	};

	struct LocationData
	{
		enum {
			kPackLocation_NearReference		= 0,
			kPackLocation_InCell			= 1,
			kPackLocation_CurrentLocation	= 2,
			kPackLocation_EditorLocation	= 3,
			kPackLocation_ObjectID			= 4,
			kPackLocation_ObjectType		= 5,
			kPackLocation_LinkedReference	= 6,

			kPackLocation_Max,
		};

		UInt8		locationType;	
		UInt8		pad[3];
		UInt32		radius;
		ObjectType  object;

		static LocationData* Create();
		static const char* StringForLocationCode(UInt8 locCode);
		const char* StringForLocationCodeAndData(void);
		static UInt8 LocationCodeForString(const char* locStr);
		static bool IsValidLocationType(UInt8 locCode) { return locCode < kPackLocation_Max; }
	};

	enum
	{
		kTargetType_Refr		= 0,
		kTargetType_BaseObject	= 1,
		kTargetType_TypeCode	= 2,
		
		kTargetType_Max	= 3,
	};

	struct TargetData 
	{
		UInt8		targetType;	// 00
		UInt8		pad[3];		// 01
		ObjectType	target;		// 04
		UInt32		count;		// 08 can be distance too
		float		unk0C;		// 0C

		static TargetData* Create();
		static const char* StringForTargetCode(UInt8 targetCode);
		const char* StringForTargetCodeAndData(void);
		static UInt8 TargetCodeForString(const char* targetStr);
		static bool IsValidTargetCode(UInt8 c) { return c < TESPackage::kTargetType_Max; }
	};


	enum eProcedure {			// UInt32	// Checked the Geck Wiki. Not consistent with s_procNames (which has a diffferent order and 0x37 procedures)
		kProcedure_TRAVEL = 0,
		kProcedure_ACTIVATE,
		kProcedure_ACQUIRE,
		kProcedure_WAIT,
		kProcedure_DIALOGUE,
		kProcedure_GREET,
		kProcedure_GREET_DEAD,
		kProcedure_WANDER,
		kProcedure_SLEEP,
		kProcedure_OBSERVE_COMBAT,
		kProcedure_EAT,
		kProcedure_FOLLOW,
		kProcedure_ESCORT,
		kProcedure_COMBAT,
		kProcedure_ALARM,
		kProcedure_PURSUE,
		kProcedure_FLEE,					// 0x10
		kProcedure_DONE,
		kProcedure_YELD,
		kProcedure_TRAVEL_TARGET,
		kProcedure_CREATE_FOLLOW,
		kProcedure_GET_UP,
		kProcedure_MOUNT_HORSE,
		kProcedure_DISMOUNT_HORSE,
		kProcedure_DO_NOTHING,
		kProcedure_UNK019,
		kProcedure_UNK01A,
		kProcedure_ACCOMPANY,
		kProcedure_USE_ITEM_AT,
		kProcedure_SANDMAN,
		kProcedure_WAIT_AMBUSH,
		kProcedure_SURFACE,					// 0x20
		kProcedure_WAIT_FOR_SPELL,
		kProcedure_CHOOSE_CAST,
		kProcedure_FLEE_NON_COMBAT,
		kProcedure_REMOVE_WORN_ITEMS,
		kProcedure_SEARCH,
		kProcedure_CLEAR_MOUNT_POSITION,
		kProcedure_SUMMON_CREATURE_DEFEND,
		kProcedure_AVOID_AREA,
		kProcedure_UNEQUIP_ARMOR,
		kProcedure_PATROL,
		kProcedure_USE_WEAPON,
		kProcedure_DIALOGUE_ACTIVATE,
		kProcedure_GUARD,
		kProcedure_SANDBOX,
		kProcedure_USE_IDLE_MARKER,
		kProcedure_TAKE_BACK_ITEM,
		kProcedure_SITTING,					// 0x30
		kProcedure_MOVEMENT_BLOCKED,
		kProcedure_CANIBAL_FEED,			// 0x32

		kProcedure_MAX						// 0x33
	};

	// In DialoguePackage, there are 0x58 virtual functions (including 0x4E from TESForm)

	UInt32			procedureArrayIndex;	// 18 index into array of array of eProcedure terminated by 0x2C. -1 if no procedure array exists for package type.
	UInt32			packageFlags;			// 1C
	char			type;					// 20
	UInt8			byte21;					// 21
	UInt16			behaviorFlags;			// 22
	UInt32			specificFlags;			// 24
	TESPackageData	*packageData;			// 28
	LocationData	*location;				// 2C
	TargetData		*target;				// 30	target ?
	UInt32			unk34;					// 34	idles
	PackageTime		time;					// 38
	UInt32			unk40[16];				// 40	040 is a tList of Condition, 7C is an Interlocked counter
		//	048 is a DWord CombatStyle, 
		//	04C, 05C and 06C are the same 4 DWord struct onBegin onEnd onChange, { TESIdle* idle; EmbeddedScript* script; Topic* topic; UInt32 unk0C; }
		//	07C is a DWord

	void SetTarget(TESObjectREFR* refr);
	void SetTarget(TESForm* baseForm, UInt32 count);
	void SetTarget(UInt8 typeCode, UInt32 count);
	void SetCount(UInt32 aCount);
	void SetDistance(UInt32 aDistance) { SetCount(aDistance); }
	TargetData* GetTargetData();
	LocationData* GetLocationData();

	bool IsFlagSet(UInt32 flag);
	void SetFlag(UInt32 flag, bool bSet);

	bool GetFleeCombat();

	static const char* StringForPackageType(UInt32 pkgType);
	static const char* StringForObjectCode(UInt8 objCode);
	static UInt8 ObjectCodeForString(const char* objString);
	static bool IsValidObjectCode(UInt8 o) { return o < kObjectType_Max; }
	static const char* StringForProcedureCode(eProcedure proc);
};
STATIC_ASSERT(sizeof(TESPackage) == 0x80);

// DialoguePackage : Only package tested and verified effectivly
class DialoguePackage : public TESPackage
{
public:
	DialoguePackage();
	~DialoguePackage();

	UInt32 unk080[(0x8C - 0x80) >> 2];	// 080
	TESTopic	* topic;		// 08C
	UInt32		unk090;			// 090
	Character	* speaker;		// 094
	UInt8		unk098;			// 098
	UInt8		unk099;			// 098
	UInt8		unk09A;			// 098
	UInt8		unk09B;			// 098
	TESForm		* unk09C;		// 09C
	UInt32		unk0A0;			// 0A0
	void *		unk0A4;			// 0A4	list of Dialogue Item and Dialogue Response, plus current item and current response
	UInt32		unk0A8;			// 0A8
	UInt32		unk0AC;			// 0AC
	Character	* subject;		// 0B0
	Character	* target;		// 0B4
	TESForm		* unk0B8;		// 0B8
	UInt8		unk0BC;			// 0BC
	UInt8		unk0BD;			// 0BD
	UInt8		unk0BE;			// 0BE
	UInt8		unk0BF;			// 0BF
	UInt8		unk0C0;			// 0C0
	UInt8		unk0C1;			// 0C1
	UInt8		unk0C2;			// 0C2
	UInt8		unk0C3;			// 0C3
	UInt32		unk0C4;			// 0C4
	UInt32		unk0C8;			// 0C8
	UInt8		unk0CC;			// OCC
	UInt8		unk0CD[3];		// OCD
};	// 0D0

typedef struct {
	float vector[3];
} ThreeFloatArray;

class FleePackage : public TESPackage
{
public:
	FleePackage();
	~FleePackage();

	UInt8			unk080;		// 080
	UInt8			unk081;		// 081
	UInt8			pad082[2];	// 082
	ThreeFloatArray	unk084;		// 084	is array of 3 floats, should be Pos
	float			unk090;		// 090
	UInt8			unk094;		// 094
	UInt8			pad095[3];	// 095
	tList<TESForm*>	list098;	// 098
	TESForm			* unk0A0;	// 0A0
	TESForm			* unk0A4;	// 0A4
	UInt8			unk0A8;		// 0A8
	UInt8			unk0A9;		// 0A9
	UInt8			pad0AA[2];	// 0AA
};	// 0AC

class TressPassPackage : public TESPackage
{
public:
	TressPassPackage();
	~TressPassPackage();

	float		unk080;		// 080
	UInt32		unk084;		// 084
	TESForm		* unk088;	// 088
	TESForm		* unk08C;	// 08C
	UInt32		unk090;		// 090
	UInt32		unk094;		// 094
	UInt32		unk098;		// 098
};	// 09C

struct SpectatorThreatInfo
{
	TESForm			* unk000;	// 000
	TESForm			* unk004;	// 004
	UInt32			unk008;		// 008
	UInt32			unk00C;		// 00C	elapsed tick count
	UInt32			unk010;		// 010
	ThreeFloatArray	unk014;		// 014	is array of 3 floats, should be Pos
	ThreeFloatArray	unk020;		// 020	is array of 3 floats, should be Rot
	UInt8			unk02C;		// 02C
	UInt8			unk02D;		// 02D
	UInt8			pad[2];		// 02E
};	// 030

class SpectatorPackage : public TESPackage
{
public:
	SpectatorPackage();
	~SpectatorPackage();

	UInt32			unk080;		// 080
	UInt32			unk084;		// 084
	UInt32			unk088;		// 088
	UInt32			unk08C;		// 08C
	UInt8			unk090;		// 090
	UInt8			pad091[3];	// 091
	ThreeFloatArray	unk094;		// 094	is array of 3 floats, should be Pos
	BSSimpleArray<SpectatorThreatInfo>	arr0A0;	// 0A0
	// There is an object containing a semaphore at B0/B4
};	// 0B4

class TESFollowPackageData : public TESPackageData
{
public:
	TESFollowPackageData();
	~TESFollowPackageData();
	TESPackage::LocationData* endLocation;
	float	flt008;
};

// 108
class TESCombatStyle : public TESForm
{
public:
	TESCombatStyle();
	~TESCombatStyle();

	enum
	{
		kFlag_ChooseAttackUsingChance		= 1,
		kFlag_MeleeAlertOK					= 2,
		kFlag_FleeBasedOnPersonalSurvival	= 4,
		kFlag_IgnoreThreats					= 16,
		kFlag_IgnoreDamagingSelf			= 32,
		kFlag_IgnoreDamagingGroup			= 64,
		kFlag_IgnoreDamagingSpectators		= 128,
		kFlag_CannotUseStealthboy			= 256,
	};

	float	coverSearchRadius;				// 018
	float	takeCoverChance;				// 01C
	float	waitTimeMin;					// 020
	float	waitTimeMax;					// 024
	float	waitToFireTimerMin;				// 028
	float	waitToFireTimerMax;				// 02C
	float	fireTimerMin;					// 030
	float	fireTimerMax;					// 034
	float	rangedWeapRangeMultMin;			// 038
	UInt8	pad3C[4];						// 03C
	UInt8	weaponRestrictions;				// 040
	UInt8	pad41[3];						// 041
	float	rangedWeapRangeMultMax;			// 044
	float	maxTargetingFOV;				// 048
	float	combatRadius;					// 04C
	float	semiAutoFiringDelayMultMin;		// 050
	float	semiAutoFiringDelayMultMax;		// 054
	UInt8	dodgeChance;					// 058
	UInt8	LRChance;						// 059
	UInt8	pad5A[2];						// 05A
	float	dodgeLRTimerMin;				// 05C
	float	dodgeLRTimerMax;				// 060
	float	dodgeFWTimerMin;				// 064
	float	dodgeFWTimerMax;				// 068
	float	dodgeBKTimerMin;				// 06C
	float	dodgeBKTimerMax;				// 070
	float	idleTimerMin;					// 074
	float	idleTimerMax;					// 078
	UInt8	blockChance;					// 07C
	UInt8	attackChance;					// 07D
	UInt8	pad7E[2];						// 07E
	float	staggerBonusToAttack;			// 080
	float	KOBonusToAttack;				// 084
	float	H2HBonusToAttack;				// 088
	UInt8	powerAttackChance;				// 08C
	UInt8	pad8D[3];						// 08D
	float	staggerBonusToPower;			// 090
	float	KOBonusToPower;					// 094
	UInt8	powerAttackN;					// 098
	UInt8	powerAttackF;					// 099
	UInt8	powerAttackB;					// 09A
	UInt8	powerAttackL;					// 09B
	UInt8	powerAttackR;					// 09C
	UInt8	pad9D[3];						// 09D
	float	holdTimerMin;					// 0A0
	float	holdTimerMax;					// 0A4
	UInt16	csFlags;						// 0A8
	UInt8	pad0AA[2];						// 0AA
	UInt8	acrobaticDodgeChance;			// 0AC
	UInt8	rushAttackChance;				// 0AD
	UInt8	pad0AE[2];						// 0AE
	float	rushAttackDistMult;				// 0B0
	float	dodgeFatigueModMult;			// 0B4
	float	dodgeFatigueModBase;			// 0B8
	float	encumSpeedModBase;				// 0BC
	float	encumSpeedModMult;				// 0C0
	float	dodgeUnderAttackMult;			// 0C4
	float	dodgeNotUnderAttackMult;		// 0C8
	float	dodgeBackUnderAttackMult;		// 0CC
	float	dodgeBackNotUnderAttackMult;	// 0D0
	float	dodgeFWAttackingMult;			// 0D4
	float	dodgeFWNotAttackingMult;		// 0D8
	float	blockSkillModMult;				// 0DC
	float	blockSkillModBase;				// 0E0
	float	blockUnderAttackMult;			// 0E4
	float	blockNotUnderAttackMult;		// 0E8
	float	attackSkillModMult;				// 0EC
	float	attackSkillModBase;				// 0F0
	float	attackUnderAttackMult;			// 0F4
	float	attackNotUnderAttackMult;		// 0F8
	float	attackDuringBlockMult;			// 0FC
	float	powerAttackFatigueModBase;		// 100
	float	powerAttackFatigueModMult;		// 104

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) csFlags |= pFlag;
		else csFlags &= ~pFlag;
	}
};

STATIC_ASSERT(sizeof(TESCombatStyle) == 0x108);

// 2C
class TESRecipeCategory : public TESForm
{
public:
	TESRecipeCategory();
	~TESRecipeCategory();

	TESFullName			fullName;	// 18

	UInt32				flags;		// 24
};

STATIC_ASSERT(sizeof(TESRecipeCategory) == 0x28);

struct RecipeComponent
{
	UInt32		quantity;
	TESForm		*item;
};

// 5C
class TESRecipe : public TESForm
{
public:
	TESRecipe();
	~TESRecipe();

	struct ComponentList : tList<RecipeComponent>
	{
		void *GetComponents(Script *scriptObj);
		void AddComponent(TESForm *form, UInt32 quantity);
		UInt32 RemoveComponent(TESForm *form);
		void ReplaceComponent(TESForm *form, TESForm *replace);
		UInt32 GetQuantity(TESForm *form);
		void SetQuantity(TESForm *form, UInt32 quantity);
	};

	TESFullName				fullName;		// 18

	UInt32					reqSkill;		// 24
	UInt32					reqSkillLevel;	// 28
	UInt32					categoryID;		// 2C
	UInt32					subCategoryID;	// 30
	ConditionList			conditions;		// 34
	ComponentList			inputs;			// 3C
	ComponentList			outputs;		// 44
	UInt32					unk4C;			// 4C
	UInt32					unk50;			// 50
	TESRecipeCategory		*category;		// 54
	TESRecipeCategory		*subCategory;	// 58
};

STATIC_ASSERT(sizeof(TESRecipe) == 0x5C);

class TESLoadScreenType : public TESForm
{
public:
	TESLoadScreenType();
	~TESLoadScreenType();

	enum ScreenType
	{
		kScrType_None,
		kScrType_XP_Progress,
		kScrType_Objective,
		kScrType_Tip,
		kScrType_Stats
	};

	struct floatRGB
	{
		float R, G, B;
	};

	UInt32			type;						// 018
	// Data 1
	UInt32			x;							// 01C
	UInt32			y;							// 020
	UInt32			width;						// 024
	UInt32			height;						// 028
	UInt32			orientation;				// 02C
	UInt32			font1;						// 030
	floatRGB		fontcolor1;					// 034
	UInt32			justification;				// 040
	UInt32			unk044[(0x58 - 0x44) >> 2];	// 044
	// Data 2
	UInt32			font2;						// 058
	floatRGB		fontcolor2;					// 05C
	UInt32			unk068;						// 068
	UInt32			stats;						// 06C
};

// 40
class TESLoadScreen : public TESForm
{
public:
	TESLoadScreen();
	~TESLoadScreen();

	struct Location
	{
		UInt32		interiorRefID;	// 00	Used if cell is an interior.
		UInt32		worldRefID;		// 04	Used if cell is an exterior, with grid coords
		Coordinate	gridCoords;		// 08
	};

	TESTexture			texture;		// 18
	TESDescription		description;	// 24	Appears to be unused
	tList<Location>		locations;		// 2C
	TESLoadScreenType	*type;			// 34
	String				screenText;		// 38
};
STATIC_ASSERT(sizeof(TESLoadScreen) == 0x40);

// TESLevSpell (44)
class TESLevSpell;

// TESObjectANIO (3C)
class TESObjectANIO : public TESForm
{
public:
	TESObjectANIO();
	~TESObjectANIO();

	TESModelTextureSwap	modelSwap;		// 18
	TESIdleForm			*idleForm;		// 38
};

// 194
class TESWaterForm : public TESForm
{
public:
	TESWaterForm();
	~TESWaterForm();

	TESFullName				fullName;		// 018
	TESAttackDamageForm		attackDamage;	// 024
	UInt32					unk02C[14];		// 02C
	TESTexture				noiseMap;		// 064
	UInt8					opacity;		// 070 ANAM
	UInt8					flags;			// 071 FNAM (0x01: causes damage, 0x02: reflective)
	UInt8					unk072[2];		// 072
	UInt32					unk074[2];		// 074
	TESSound				*sound;			// 07C
	TESWaterForm			*waterForm;		// 080
	float					visData[49];	// 084
	UInt32					unk148[12];		// 148
	SpellItem				*drinkEffect;	// 178
	UInt32					unk17C[3];		// 17C
	UInt8					radiation;		// 188
	UInt8					pad189[3];		// 189
	UInt32					unk18C[2];		// 18C
};

// TESEffectShader (170)
class TESEffectShader : public TESForm
{
public:
	TESEffectShader();
	~TESEffectShader();

	UInt32 unk018[(0x170 - 0x18) >> 2];
};

// A8
class BGSExplosion : public TESBoundObject
{
public:
	BGSExplosion();
	~BGSExplosion();

	enum
	{
		kFlags_Unknown =					1,
		kFlags_AlwaysUseWorldOrientation =	2,
		kFlags_KnockDownAlways =			4,
		kFlags_KnockDownByFormula =			8,
		kFlags_IgnoreLOSCheck =				16,
		kFlags_PushSourceRefOnly =			32,
		kFlags_IgnoreImageSpaceSwap =		64,
	};

	TESFullName					fullName;			// 30
	TESModel					model;				// 3C
	TESEnchantableForm			enchantable;		// 54
	BGSPreloadable				preloadable;		// 64
	TESImageSpaceModifiableForm	imageSpaceModForm;	// 68

	TESForm						*placedObj;			// 70
	float						force;				// 74
	float						damage;				// 78
	float						radius;				// 7C
	TESObjectLIGH				*light;				// 80
	TESSound					*sound1;			// 84
	UInt32						explFlags;			// 88
	float						ISradius;			// 8C
	BGSImpactDataSet			*impactDataSet;		// 90
	TESSound					*sound2;			// 94
	float						RADlevel;			// 98
	float						dissipationTime;	// 9C
	float						RADradius;			// A0
	UInt8						soundLevel;			// A4	0 - Loud, 1 - Normal, 2 - Silent
	UInt8						padA5[3];			// A5

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) explFlags |= pFlag;
		else explFlags &= ~pFlag;
	}
};

STATIC_ASSERT(sizeof(BGSExplosion) == 0xA8);

// BGSDebris (24)
class BGSDebris : public TESForm
{
	BGSDebris();
	~BGSDebris();

	BGSPreloadable				preloadable;	// 018
	UInt32	unk01C;
	UInt32	unk020;
};

// B0
class TESImageSpace : public TESForm
{
public:
	TESImageSpace();
	~TESImageSpace();

	float		traitValues[33];	// 18
	// 00:	HDR: Eye Adapt Speed
	// 01:	HDR: Blur Radius
	// 02:	HDR: Blur Passes
	// 03:	HDR: Emissive Mult
	// 04:	HDR: Target LUM
	// 05:	HDR: Upper LUM Clamp
	// 06:	HDR: Bright Scale
	// 07:	HDR: Bright Clamp
	// 08:	HDR: LUM Ramp No Tex
	// 09:	HDR: LUM Ramp Min
	// 10:	HDR: LUM Ramp Max
	// 11:	HDR: Sunlight Dimmer
	// 12:	HDR: Grass Dimmer
	// 13:	HDR: Tree Dimmer
	// 14:	HDR: Skin Dimmer
	// 15:	Bloom: Blur Radius
	// 16:	Bloom: Alpha Mult Interior
	// 17:	Bloom: Alpha Mult Exterior
	// 18:	Get Hit: Blur Radius
	// 19:	Get Hit: Blur Damping Contrast
	// 20:	Get Hit: Damping Contrast
	// 21:	Night Eye: Tint: Red
	// 22:	Night Eye: Tint: Green
	// 23:	Night Eye: Tint: Blue
	// 24:	Night Eye: Brightness
	// 25:	Cinematic: Saturation: Value
	// 26:	Cinematic: Contrast: Avg Lum Value
	// 27:	Cinematic: Contrast: Value
	// 28:	Cinematic: Brightness: Value
	// 29:	Cinematic: Tint: Red
	// 30:	Cinematic: Tint: Green
	// 31:	Cinematic: Tint: Blue
	// 32:	Cinematic: Tint: Value
	UInt32		unk9C[5];			// 9C
};
STATIC_ASSERT(sizeof(TESImageSpace) == 0xB0);

// 730
class TESImageSpaceModifier : public TESForm
{
public:
	TESImageSpaceModifier();
	~TESImageSpaceModifier();

	TESSound				*outroSound;		// 018
	TESSound				*introSound;		// 01C
	UInt8					animable;			// 020
	UInt8					pad021[3];			// 021
	float					duration;			// 024
	UInt32					unk028[49];			// 028
	float					radialBlurCentreX;	// 0EC
	float					radialBlurCentreY;	// 0F0
	UInt32					unk0F4[3];			// 0F4
	UInt8					useTarget;			// 100
	UInt8					pad101[3];			// 101
	UInt32					unk104[4];			// 104
	NiFloatInterpolator		fltIntrpl1[44];		// 114
	NiColorInterpolator		clrIntrpl[2];		// 534
	NiFloatInterpolator		fltIntrpl2[9];		// 57C
	FloatData				*data654[44];		// 654
	// 00:	HDR: Eye Adapt Speed (Multiply)
	// 01:	HDR: Eye Adapt Speed (Add)
	// 02:	HDR: Blur Radius (Multiply)
	// 03:	HDR: Blur Radius (Add)
	// 04:	HDR: Skin Dimmer (Multiply)
	// 05:	HDR: Skin Dimmer (Add)
	// 06:	HDR: Emissive Mult (Multiply)
	// 07:	HDR: Emissive Mult (Add)
	// 08:	HDR: Target LUM (Multiply)
	// 09:	HDR: Target LUM (Add)
	// 10:	HDR: Upper LUM Clamp (Multiply)
	// 11:	HDR: Upper LUM Clamp (Add)
	// 12:	HDR: Bright Scale (Multiply)
	// 13:	HDR: Bright Scale (Add)
	// 14:	HDR: Bright Clamp (Multiply)
	// 15:	HDR: Bright Clamp (Add)
	// 16:	HDR: LUM Ramp No Tex (Multiply)
	// 17:	HDR: LUM Ramp No Tex (Add)
	// 18:	HDR: LUM Ramp Min (Multiply)
	// 19:	HDR: LUM Ramp Min (Add)
	// 20:	HDR: LUM Ramp Max (Multiply)
	// 21:	HDR: LUM Ramp Max (Add)
	// 22:	HDR: Sunlight Dimmer (Multiply)
	// 23:	HDR: Sunlight Dimmer (Add)
	// 24:	HDR: Grass Dimmer (Multiply)
	// 25:	HDR: Grass Dimmer (Add)
	// 26:	HDR: Tree Dimmer (Multiply)
	// 27:	HDR: Tree Dimmer (Add)
	// 28:	Bloom: Blur Radius (Multiply)
	// 29:	Bloom: Blur Radius (Add)
	// 30:	Bloom: Alpha Mult Interior (Multiply)
	// 31:	Bloom: Alpha Mult Interior (Add)
	// 32:	Bloom: Alpha Mult Exterior (Multiply)
	// 33:	Bloom: Alpha Mult Exterior (Add)
	// 34:	Cinematic: Saturation (Multiply)
	// 35:	Cinematic: Saturation (Add)
	// 36:	Cinematic: Contrast (Multiply)
	// 37:	Cinematic: Contrast (Add)
	// 38:	Cinematic: Contrast Avg Lum (Multiply)
	// 39:	Cinematic: Contrast Avg Lum (Add)
	// 40:	Cinematic: Brightness (Multiply)
	// 41:	Cinematic: Brightness (Add)
	// 42:	Blur: Blur Radius
	// 43:	Double Vision: Strength
	ColorData				*data704[2];		// 704
	FloatData				*data70C[9];		// 70C
	// 00:	Radial Blur: Strength
	// 01:	Radial Blur: Rampup
	// 02:	Radial Blur: Up Start
	// 03:	Radial Blur: Rampdown
	// 04:	Radial Blur: Down Start
	// 05:	Depth of Field: Strength
	// 06:	Depth of Field: Distance
	// 07:	Depth of Field: Range
	// 08:	Full-Screen Motion Blur: Strength
};
STATIC_ASSERT(sizeof(TESImageSpaceModifier) == 0x730);

// 24
class BGSListForm : public TESForm
{
public:
	BGSListForm();
	~BGSListForm();

	tList<TESForm> list;			// 018

	UInt32	numAddedObjects;	// number of objects added via script - assumed to be at the start of the list

	UInt32 Count() const {
		return list.Count();
	}

	TESForm* GetNthForm(SInt32 n) const {
		return list.GetNthItem(n);
	}

	UInt32 AddAt(TESForm* pForm, SInt32 n) {
		SInt32	result = list.AddAt(pForm, n);

		if(result >= 0 && IsAddedObject(n))
			numAddedObjects++;

		return result;
	}

	SInt32 GetIndexOf(TESForm* pForm);

	TESForm *RemoveNthForm(SInt32 n)
	{
		TESForm	*form = list.RemoveNth(n);

		if (form && IsAddedObject(n) && numAddedObjects)
			numAddedObjects--;

		return form;
	}

	TESForm* ReplaceNthForm(SInt32 n, TESForm* pReplaceWith) {
		return list.ReplaceNth(n, pReplaceWith);
	}

	SInt32 RemoveForm(TESForm* pForm);
	SInt32 ReplaceForm(TESForm* pForm, TESForm* pReplaceWith);

	bool IsAddedObject(SInt32 idx)
	{
		return (idx >= 0) && (idx < numAddedObjects);
	}

	void RemoveAll()
	{
		list.RemoveAll();
		numAddedObjects = 0;
	}
};

STATIC_ASSERT(sizeof(BGSListForm) == 0x024);

// 08
class BGSPerkEntry
{
public:
	BGSPerkEntry();
	~BGSPerkEntry();

	virtual void	Fn_00(void);
	virtual void	Fn_01(void);
	virtual void	Fn_02(void);
	virtual void	Fn_03(void);
	virtual UInt32	GetType();		//	0 - Quest; 1 - Ability; 2 - Entry Point
	virtual void	Fn_05(void);
	virtual void	Fn_06(void);
	virtual void	Fn_07(void);
	virtual void	Fn_08(void);
	virtual void	Fn_09(void);
	virtual void	Fn_0A(void);
	virtual void	Fn_0B(void);
	virtual void	Fn_0C(void);
	virtual void	Fn_0D(void);

	UInt8				rank;				// 04 +1 for value shown in GECK
	UInt8				priority;			// 05
	UInt16				type;				// 06 (Quest: 0xC24, Ability: 0xB27, Entry Point: 0xD16)
};

// 10
class BGSQuestPerkEntry : public BGSPerkEntry
{
public:
	BGSQuestPerkEntry();
	~BGSQuestPerkEntry();

	virtual void	Fn_0E(void);

	TESQuest			*quest;				// 08
	UInt8				stage;				// 0C
	UInt8				pad[3];				// 0D
};

// 0C
class BGSAbilityPerkEntry : public BGSPerkEntry
{
public:
	BGSAbilityPerkEntry();
	~BGSAbilityPerkEntry();

	virtual void	Fn_0E(void);

	SpellItem			*ability;			// 08
};

class BGSEntryPointFunctionData
{
public:
	BGSEntryPointFunctionData();
	~BGSEntryPointFunctionData();

	virtual void	Fn_00(void);
	virtual void	Fn_01(void);
	virtual void	Fn_02(void);
	virtual void	Fn_03(void);
	virtual void	Fn_04(void);
	virtual void	Fn_05(void);
	virtual void	Fn_06(void);
};

// 08
class BGSEntryPointFunctionDataOneValue : public BGSEntryPointFunctionData
{
public:
	BGSEntryPointFunctionDataOneValue();
	~BGSEntryPointFunctionDataOneValue();

	float				value;				// 04
};

// 0C
class BGSEntryPointFunctionDataTwoValue : public BGSEntryPointFunctionData
{
public:
	BGSEntryPointFunctionDataTwoValue();
	~BGSEntryPointFunctionDataTwoValue();

	float				value[2];			// 04
};

class BGSEntryPointFunctionDataLeveledList : public BGSEntryPointFunctionData
{
public:
	BGSEntryPointFunctionDataLeveledList();
	~BGSEntryPointFunctionDataLeveledList();

	TESLevItem			*leveledList;		// 04
};

class BGSEntryPointFunctionDataActivateChoice : public BGSEntryPointFunctionData
{
public:
	BGSEntryPointFunctionDataActivateChoice();
	~BGSEntryPointFunctionDataActivateChoice();

	virtual void		Fn_07(void);

	String				label;				// 04
	Script				*script;			// 0C
	UInt32				flags;				// 10
};

struct EntryPointConditions
{
	ConditionList		tab1;
	ConditionList		tab2;
	ConditionList		tab3;
};

// 14
class BGSEntryPointPerkEntry : public BGSPerkEntry
{
public:
	BGSEntryPointPerkEntry();
	~BGSEntryPointPerkEntry();

	virtual void	Fn_0E(void);

	UInt8						entryPoint;		// 08
	UInt8						function;		// 09
	UInt8						conditionTabs;	// 0A
	UInt8						pad0B;			// 0B
	BGSEntryPointFunctionData	*data;			// 0C
	EntryPointConditions		*conditions;	// 10
};

// 50
class BGSPerk : public TESForm
{
public:
	BGSPerk();
	~BGSPerk();

	struct PerkData
	{
		bool				isTrait;	// 00
		UInt8				minLevel;	// 01
		UInt8				numRanks;	// 02
		bool				isPlayable;	// 03
		bool				isHidden;	// 04
		UInt8				unk05;		// 05 todo: collapse to pad[3] after verifying isPlayable and isHidden
		UInt8				unk06;		// 06
		UInt8				unk07;		// 07
	};

	TESFullName				fullName;			// 18
	TESDescription			description;		// 24
	TESIcon					icon;				// 2C
	PerkData				data;				// 38
	ConditionList			conditions;			// 40
	tList<BGSPerkEntry>		entries;			// 48
};

class TESCasino : public TESForm
{
public:
	TESCasino();
	~TESCasino();

	TESFullName				fullName;
	TESModelTextureSwap		chip1;
	TESModelTextureSwap		chip5;
	TESModelTextureSwap		chip10;
	TESModelTextureSwap		chip25;
	TESModelTextureSwap		chip100;
	TESModelTextureSwap		chip500;
	TESModelTextureSwap		chipRoulette;
	TESModelTextureSwap		slotMachine;
	TESModelTextureSwap		blackjackTable;
	TESModelTextureSwap		rouletteTable;
	TESIcon					slotReel[7];
	TESTexture				blackjackDeck[4];
	float					shufflePercent;
	float					blackjackPayout;
	UInt32					reelStops[7];			// the values here must total 14
	UInt32					numDecks;
	UInt32					maxWinnings;
	UInt32					currencyRefID;			// ID, not form pointer
	UInt32					winningsQuestRefID;		// ID, not form pointer
	UInt32					flags;					// 1: dealer stand on soft 17 (no other flags)
	UInt32					unk220[2];
};

// 7C
class TESChallenge : public TESForm
{
public:
	TESChallenge();
	~TESChallenge();

	enum
	{
		kFlag_StartDisabled = 1 << 0,
		kFlag_Recurring = 1 << 1,
		kFlag_ShowZeroProgress = 1 << 2
	};

	enum ChallengeTypes
	{
		kChallengeType_KillInFormList,
		kChallengeType_KillFormID,
		kChallengeType_KillInCategory,
		kChallengeType_HitEnemy,
		kChallengeType_LocationDiscovered,
		kChallengeType_UseAnIngestible,
		kChallengeType_AcquireAnItem,
		kChallengeType_UseASkill,
		kChallengeType_DoDamage,
		kChallengeType_UseAnIngestibleInFormList,
		kChallengeType_AcquireItemInFormList,
		kChallengeType_MiscStat,
		kChallengeType_Crafting,
		kChallengeType_Scripted,
	};

	struct ChallengeData	// 018
	{
		UInt32		type;			// needs enumeration
		UInt32		threshold;
		UInt32		flags;
		UInt32		interval;
		UInt16		value1;			// these fields change based on challenge type
		UInt16		value2;			// might need unions...
		UInt32		value3;
	};

	TESFullName				fullName;		// 18
	TESDescription			description;	// 24
	TESScriptableForm		scriptable;		// 2C
	TESIcon					icon;			// 38
	BGSMessageIcon			msgIcon;		// 44
	
	ChallengeData			data;			// 54
	UInt32					amount;			// 6C
	UInt32					challengeflags;	// 70
	TESForm					*SNAM;			// 74
	TESForm					*XNAM;			// 78
};
STATIC_ASSERT(sizeof(TESChallenge) == 0x7C);

// B0
class BGSBodyPart : public BaseFormComponent
{
public:
	BGSBodyPart();
	~BGSBodyPart();

	enum
	{
		kFlags_Severable =		1,
		kFlags_IKData =			2,
		kFlags_BipedData =		4,
		kFlags_Explodable =		8,
		kFlags_IsHead =			16,
		kFlags_Headtracking =	32,
		kFlags_Absolute =		64,
	};

	String				partNode;				// 04
	String				VATSTarget;				// 0C
	String				startNode;				// 14
	String				partName;				// 1C
	String				targetBone;				// 24
	TESModel			limbReplacement;		// 2C
	UInt32				unk44[6];				// 44
	float				damageMult;				// 5C
	UInt8				flags;					// 60
	UInt8				pad61;					// 61
	UInt8				healthPercent;			// 62
	UInt8				actorValue;				// 63
	UInt8				toHitChance;			// 64
	UInt8				explChance;				// 65
	UInt8				explDebrisCount;		// 66
	UInt8				pad67;					// 67
	BGSDebris			*explDebris;			// 68
	BGSExplosion		*explExplosion;			// 6C
	float				trackingMaxAngle;		// 70
	float				explDebrisScale;		// 74
	UInt8				sevrDebrisCount;		// 78
	UInt8				pad79[3];				// 79
	BGSDebris			*sevrDebris;			// 7C
	BGSExplosion		*sevrExplosion;			// 80
	float				sevrDebrisScale;		// 84
	float				goreEffTranslate[3];	// 88
	float				goreEffRotation[3];		// 94
	BGSImpactDataSet	*sevrImpactDS;			// A0
	BGSImpactDataSet	*explImpactDS;			// A4
	UInt8				sevrDecalCount;			// A8
	UInt8				explDecalCount;			// A9
	UInt8				padAA[2];				// AA
	float				limbRepScale;			// AC

	void SetFlag(UInt32 pFlag, bool bEnable)
	{
		if (bEnable) flags |= pFlag;
		else flags &= ~pFlag;
	}
};

STATIC_ASSERT(sizeof(BGSBodyPart) == 0xB0);

// 74
class BGSBodyPartData : public TESForm
{
public:
	BGSBodyPartData();
	~BGSBodyPartData();

	enum
	{
		eBodyPart_Torso = 0,
		eBodyPart_Head1,
		eBodyPart_Head2,
		eBodyPart_LeftArm1,
		eBodyPart_LeftArm2,
		eBodyPart_RightArm1,
		eBodyPart_RightArm2,
		eBodyPart_LeftLeg1,
		eBodyPart_LeftLeg2,
		eBodyPart_LeftLeg3,
		eBodyPart_RightLeg1,
		eBodyPart_RightLeg2,
		eBodyPart_RightLeg3,
		eBodyPart_Brain,
		eBodyPart_Weapon,
	};

	TESModel		model;				// 018
	BGSPreloadable	preloadable;		// 030
	BGSBodyPart		*bodyParts[15];		// 034
	BGSRagdoll		*ragDoll;			// 070
};
STATIC_ASSERT(sizeof(BGSBodyPartData) == 0x74);

// C4
class MediaSet : public TESForm
{
public:
	~MediaSet();
	MediaSet();

	// 10
	struct NameAndDB
	{
		String	name;
		float	decibels;
		float	percent;
	};

	TESFullName		fullName;		// 18
	UInt32			unk24;			// 24
	UInt32			unk28;			// 28
	UInt32			unk2C;			// 2C
	UInt8			byte30;			// 30
	UInt8			byte31;			// 31
	UInt8			byte32;			// 32
	UInt8			byte33;			// 33
	UInt8			byte34;			// 34
	UInt8			pad35[3];		// 35
	TESFullName		name38;			// 38
	UInt32			type;			// 44	'Battle Set', 'Location Set', 'Dungeon Set', 'Incidental Set'
	NameAndDB		dayOuter;		// 48
	NameAndDB		dayMiddle;		// 58
	NameAndDB		dayInner;		// 68
	NameAndDB		nightOuter;		// 78
	NameAndDB		nightMiddle;	// 88
	NameAndDB		nightInner;		// 98
	UInt32			enableFlags;	// A8
	float			waitTime;		// AC
	float			loopFadeOut;	// B0
	float			recoveryTime;	// B4
	float			nightTimeMax;	// B8
	TESSound		*sound1;		// BC
	TESSound		*sound2;		// C0
};
STATIC_ASSERT(sizeof(MediaSet) == 0xC4);

// B8
class MediaLocationController : public TESForm
{
public:
	MediaLocationController();
	~MediaLocationController();

	TESFullName			fullName;		// 18
	UInt32				unk24[4];		// 24
	UInt8				byte34;			// 34
	UInt8				byte35;			// 35
	UInt8				byte36;			// 36
	UInt8				byte37;			// 37
	UInt8				byte38;			// 38
	UInt8				pad39[3];		// 39
	float				flt3C;			// 3C
	UInt32				unk40;			// 40
	UInt32				unk44;			// 44
	UInt32				unk48;			// 48
	UInt8				byte4C;			// 4C
	UInt8				pad4D[3];		// 4D
	MediaSet			*mediaSet;		// 50
	UInt32				unk54[3];		// 54
	TESFaction			*reputation;	// 60
	UInt32				unk64;			// 64
	UInt32				unk68;			// 68
	UInt32				mlcFlags;		// 6C
	float				flt70;			// 70
	float				flt74;			// 74
	float				retriggerDelay;	// 78
	float				locationDelay;	// 7C
	UInt32				dayStart;		// 80
	UInt32				nightStart;		// 84
	tList<MediaSet>		neutralSets;	// 88
	tList<MediaSet>		allySets;		// 90
	tList<MediaSet>		friendSets;		// 98
	tList<MediaSet>		enemySets;		// A0
	tList<MediaSet>		locationSets;	// A8
	tList<MediaSet>		battleSets;		// B0
};
STATIC_ASSERT(sizeof(MediaLocationController) == 0xB8);

// BGSAddonNode (60)
class BGSAddonNode : public TESBoundObject
{
public:
	BGSAddonNode();
	~BGSAddonNode();

	TESModel	model;				// 030
	UInt32 unk48[(0x60-0x48) >> 2]; // 048
};

STATIC_ASSERT(sizeof(BGSAddonNode) == 0x60);


enum ActorValueCode;
// C4
class ActorValueInfo : public TESForm
{
public:
	ActorValueInfo();
	~ActorValueInfo();

	enum ActorValueFlags
	{
		//flag 0x01	used in list of modified ActorValue for Player and others. Either can damage or "special damage", see 0x00937280, 0x93801B
		kAV_MaximumIs10 = 0x8,
		kAV_MinMaxIs0_100 = 0x10,
		kAV_MinimumIs1 = 0x8000,
		kAV_IsUnplayable = 0x2000,
		kAV_CannotBeModifiedInScriptsOrConsole = 0x4000,
	};

	TESFullName		fullName;
	TESDescription	description;
	TESIcon			icon;

	char			*infoName;		// 38
	String			avName;			// 3C
	UInt32			avFlags;		// 44
	UInt32			unk48;			// 48
	UInt32			initDefaultCallback;		// 4C
	UInt32			unk50;			// 50
	void(__cdecl*onChangeCallback)(ActorValueOwner* avOwner, int avCode, float previousVal, float newVal, ActorValueOwner* avOwner2);
	UInt32			numLinkedActorValues;
	ActorValueCode	linkedActorValues[15];
	UInt32			numUnk9Cs;
	UInt32			unk9C[10];
};

STATIC_ASSERT(sizeof(ActorValueInfo) == 0xC4);

extern const ActorValueInfo** ActorValueInfoPointerArray;

typedef ActorValueInfo* (* _GetActorValueInfo)(UInt32 actorValueCode);
extern const _GetActorValueInfo GetActorValueInfo;

// 20
class BGSRadiationStage : public TESForm
{
public:
	BGSRadiationStage();
	~BGSRadiationStage();

	UInt32		threshold;	// 18
	SpellItem	*effect;	// 1C
};

// 20
class BGSDehydrationStage : public TESForm
{
public:
	BGSDehydrationStage();
	~BGSDehydrationStage();

	UInt32		threshold;	// 18
	SpellItem	*effect;	// 1C
};

// 20
class BGSHungerStage : public TESForm
{
public:
	BGSHungerStage();
	~BGSHungerStage();

	UInt32		threshold;	// 18
	SpellItem	*effect;	// 1C
};

// 20
class BGSSleepDeprevationStage : public TESForm
{
public:
	BGSSleepDeprevationStage();
	~BGSSleepDeprevationStage();

	UInt32		threshold;	// 18
	SpellItem	*effect;	// 1C
};

// BGSCameraShot (78)
class BGSCameraShot : public TESForm
{
	BGSCameraShot();
	~BGSCameraShot();

	struct s30
	{
		UInt32 unk00;
		TESImageSpaceModifier* imod;
	};

	enum Action
	{
		kShoot,
		kFly,
		kHit,
		kZoom
	};

	enum Location
	{
		kAttacker,
		kProjectile,
		kTarget
	};
	
	enum Target
	{
		kTargetAttacker,
		kTargetProjectile,
		kTargetTarget
	};

	enum Flags
	{
		kPositionFollowsLocation = 1,
		kPositionFollowsTarget = 2,
		kDontFollowBone = 4,
		kFirstPersonCamera = 8,
		kNoTracer = 0x10,
		kStartAtTimeZero = 0x20
	};

	TESModel	model;								// 018
	s30 unk030;
	Action action;
	Location location;
	Target target;
	Flags flags;
	float unk048;
	float unk04C;
	float globalTimeMult;
	float unk054;
	float unk058;
	float unk05C;
	NiRefObject* unk060;
	NiRefObject* unk064;
	UInt32 unk068;
	NiRefObject* unk06C;
	NiRefObject* unk070;
	UInt8 byte074;
	UInt8 byte075;
	UInt8 byte076;
	UInt8 gap077;
};
STATIC_ASSERT(sizeof(BGSCameraShot) == 0x78);

// BGSCameraPath (38)
class BGSCameraPath;

// BGSVoiceType (24)
class BGSVoiceType : public TESForm
{
public:
	BGSVoiceType();
	~BGSVoiceType();

	UInt32		unk18;		// 18
	String		editorID;	// 1C
};

struct ColorRGB
{
	UInt8	red;	// 000
	UInt8	green;	// 001
	UInt8	blue;	// 002
	UInt8	alpha;	// 003 or unused if no alpha
};	// 004 looks to be endian swapped !

struct DecalData
{
	float		minWidth;		// 000
	float		maxWidth;		// 004
	float		minHeight;		// 008
	float		maxHeight;		// 00C
	float		depth;			// 010
	float		shininess;		// 014
	float		parallaxScale;	// 018
	UInt8		parallaxPasses;	// 01C
	UInt8		flags;			// 01D	Parallax, Alpha - Blending, Alpha - Testing
	UInt8		unk01E[2];		// 01E
	ColorRGB	color;			// 020
};	// 024

STATIC_ASSERT(sizeof(DecalData) == 0x024);

// 78
class BGSImpactData : public TESForm
{
public:
	BGSImpactData();
	~BGSImpactData();

	TESModel		model;				// 18

	float			effectDuration;		// 30
	UInt8			effectOrientation;	// 34	0 - Surface Normal, 1 - Projectile Vector, 2 - Projectile Reflection
	UInt8			pad35[3];			// 35
	float			angleThreshold;		// 38
	float			placementRadius;	// 3C
	UInt8			soundLevel;			// 40
	UInt8			pad41[3];			// 41
	UInt8			noDecalData;		// 44
	UInt8			pad45[3];			// 45

	BGSTextureSet	*textureSet;		// 48
	TESSound		*sound1;			// 4C
	TESSound		*sound2;			// 50

	float			decalMinWidth;		// 54
	float			decalMaxWidth;		// 58
	float			decalMinHeight;		// 5C
	float			decalMaxHeight;		// 60
	float			decalDepth;			// 64
	float			decalShininess;		// 68
	float			parallaxScale;		// 6C
	UInt8			parallaxPasses;		// 70
	UInt8			decalFlags;			// 71	1 - Parallax, 2 - Alpha-Blending, 4 - Alpha-Testing
	UInt8			unk72[2];			// 72
	UInt32			decalColor;			// 74
};

STATIC_ASSERT(sizeof(BGSImpactData) == 0x78);

// 4C
class BGSImpactDataSet : public TESForm
{
public:
	BGSImpactDataSet();
	~BGSImpactDataSet();

	BGSPreloadable	preloadable;		// 18
	BGSImpactData	*impactDatas[12];	// 1C
};

STATIC_ASSERT(sizeof(BGSImpactDataSet) == 0x4C);

// 190
class TESObjectARMA : public TESObjectARMO
{
public:
	TESObjectARMA();
	~TESObjectARMA();		
};

STATIC_ASSERT(sizeof(TESObjectARMA) == 0x190);

// BGSEncounterZone (30)
class BGSEncounterZone : public TESForm
{
public:
	BGSEncounterZone();
	~BGSEncounterZone();

	TESForm		*owner;			// 18
	UInt8		rank;			// 1C
	UInt8		minLevel;		// 1D
	UInt8		zoneFlags;		// 1E
	UInt8		byte1F;			// 1F
	UInt32		detachTime;		// 20
	UInt32		attachTime;		// 24
	UInt32		resetTime;		// 28
	UInt16		zoneLevel;		// 2C
	UInt8		pad2E[2];		// 2E
};

// 40
class BGSMessage : public TESForm
{
public:
	BGSMessage();
	~BGSMessage();

	struct Button
	{
		String			label;
		ConditionList	conditions;
	};

	TESFullName		fullName;		// 18
	TESDescription	description;	// 24

	BGSMenuIcon		*menuIcon;		// 2C
	tList<Button>	buttons;		// 30
	UInt8			msgFlags;		// 38	1 - Message Box, 2 - Auto-display
	UInt8			pad39[3];		// 39
	UInt32			displayTime;	// 3C
};

STATIC_ASSERT(sizeof(BGSMessage) == 0x40);

// BGSRagdoll (148)
class BGSRagdoll : public TESForm
{
public:
	BGSRagdoll();
	~BGSRagdoll();

	TESModel	model;					// 018
	UInt32	unk030[(0x148-0x30) >> 2];	// 030
};

STATIC_ASSERT(sizeof(BGSRagdoll) == 0x148);

// 44
class BGSLightingTemplate : public TESForm
{
public:
	BGSLightingTemplate();
	~BGSLightingTemplate();

	UInt32			ambientRGB;			// 18
	UInt32			directionalRGB;		// 1C
	UInt32			fogRGB;				// 20
	float			fogNear;			// 24
	float			fogFar;				// 28
	UInt32			directionalXY;		// 2C
	UInt32			directionalZ;		// 30
	float			directionalFade;	// 34
	float			fogClipDist;		// 38
	float			fogPower;			// 3C
	TESObjectCELL	*getValuesFrom;		// 40
};

STATIC_ASSERT(sizeof(BGSLightingTemplate) == 0x44);

// BGSMusicType (30)
class BGSMusicType : public TESForm
{
public:
	BGSMusicType();
	~BGSMusicType();

	TESSoundFile	soundFile;	// 18
	float			dB;		// 24
	tList<char*>	*filesInFolder;		// 28
	UInt32			randomFile;		// 2C
};

// BGSDefaultObjectManager, with help from "Luthien Anarion"

STATIC_ASSERT(sizeof(BGSMusicType) == 0x30);

const char kDefaultObjectNames[34][28] = {	// 0x0118C360 is an array of struct: { char * Name, UInt8 kFormType , UInt8 pad[3] }
      "Stimpack",
      "SuperStimpack",
      "RadX",
      "RadAway",
      "Morphine",
      "Perk Paralysis",
      "Player Faction",
      "Mysterious Stranger NPC",
      "Mysterious Stranger Faction",
      "Default Music",
      "Battle Music",
      "Death Music",
      "Success Music",
      "Level Up Music",
      "Player Voice (Male)",
      "Player Voice (Male Child)",
      "Player Voice (Female)",
      "Player Voice (Female Child)",
      "Eat Package Default Food",
      "Every Actor Ability",
      "Drug Wears Off Image Space",
      "Doctor's Bag",
      "Miss Fortune NPC",
      "Miss Fortune Faction",
      "Meltdown Explosion",
      "Unarmed Forward PA",
      "Unarmed Backward PA",
      "Unarmed Left PA",
      "Unarmed Right PA",
      "Unarmed Crouch PA",
      "Unarmed Counter PA",
      "Spotter Effect",
      "Item Detected Effect",
      "Cateye Mobile Effect (NYI)"
};

enum EActionListForm
{
	eActionListForm_AddAt	= 00,
	eActionListForm_DelAt,
	eActionListForm_ChgAt,
	eActionListForm_GetAt,
	eActionListForm_Max,
};

enum EWhichListForm
{
	eWhichListForm_RaceHair					= 00,
	eWhichListForm_RaceEyes,
	eWhichListForm_RaceHeadPart,			// ? //
	eWhichListForm_BaseFaction,
	eWhichListForm_BaseRank,
	eWhichListForm_BasePackage,
	eWhichListForm_BaseSpellListSpell,
	eWhichListForm_BaseSpellListLevSpell,
	eWhichListForm_FactionRankName,
	eWhichListForm_FactionRankFemaleName,
	eWhichListForm_HeadParts,
	eWhichListForm_LevCreatureRef,
	eWhichListForm_LevCharacterRef,
	eWhichListForm_FormList,
	eWhichListForm_Max,
};

