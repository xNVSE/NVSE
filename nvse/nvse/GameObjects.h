#pragma once

#include "GameForms.h"
#include "GameBSExtraData.h"
#include "GameExtraData.h"
#include "GameProcess.h"

class TESObjectCELL;
struct ScriptEventList;
class ActiveEffect;
class NiNode;
class Animation;

// 008
class TESChildCell
{
public:
	TESChildCell();
	~TESChildCell();

	// no virtual destructor
	virtual TESObjectCELL *	GetPersistentCell(void);		// 000

//	void	** vtbl;	// 00
};

// 68
class TESObjectREFR : public TESForm
{
public:
	MEMBER_FN_PREFIX(TESObjectREFR);

	TESObjectREFR();
	~TESObjectREFR();

	virtual void		Unk_4E(void);	// GetStartingPosition(Position, Rotation, WorldOrCell)
	virtual void		Unk_4F(void);
	virtual void		Unk_50(void);
	virtual void		Unk_51(void);
	virtual bool		CastShadows(void);
	virtual void		Unk_53(void);
	virtual void		Unk_54(void);
	virtual void		Unk_55(void);
	virtual void		Unk_56(void);
	virtual void		Unk_57(void);
	virtual void		Unk_58(void);
	virtual void		Unk_59(void);
	virtual void		Unk_5A(void);
	virtual void		Unk_5B(void);
	virtual void		Unk_5C(void);
	virtual void		Unk_5D(void);
	virtual void		Unk_5E(void);
	virtual TESObjectREFR*		RemoveItem(TESForm* toRemove, BaseExtraList* extraList, UInt32 count, UInt32 unk3, UInt32 unk4, TESObjectREFR* destRef, 
		UInt32 unk6, UInt32 unk7, UInt32 unk8, UInt8 unk9);	// 40 unk2 quantity? Returns the reference assigned to the removed item.
	virtual void		Unk_60(void);
	virtual void		Unk_61(void);	// Linked to AddItem, (item, count, ExtraDataList), func0042 in OBSE
	virtual void		Unk_62(void);	// Linked to Unequip (and or equip maybe)
	virtual void		Unk_63(void);
	virtual void		AddItem(TESForm* item, ExtraDataList* xDataList, UInt32 Quantity);	// Needs confirmation
	virtual void		Unk_65(void);
	virtual void		Unk_66(void);
	virtual void		Unk_67(void);					// Actor: GetMagicEffectList
	virtual bool		GetIsChildSize(bool checkHeight);		// 068 Actor: GetIsChildSize
	virtual UInt32		GetActorUnk0148(void);			// result can be interchanged with baseForm, so TESForm* ?
	virtual void		SetActorUnk0148(UInt32 arg0);
	virtual void		Unk_6B(void);
	virtual void		Unk_6C(void);	// REFR: GetBSFaceGenNiNodeSkinned
	virtual void		Unk_6D(void);	// REFR: calls 006C
	virtual void		Unk_6E(void);	// MobileActor: calls 006D then NiNode::Func0040
	virtual void		Unk_6F(void);
	virtual void		Unk_70(void);
	virtual void		AnimateNiNode(void);					// same in FOSE ! identical to Func0052 in OBSE which says (inits animation-related data, and more)
	virtual void		GenerateNiNode(bool arg0);				// same in FOSE !
	virtual void		Set3D(NiNode* niNode, bool unloadArt);	// same in FOSE !
	virtual NiNode *	GetNiNode(void);						// same in FOSE !
	virtual void		Unk_75(void);
	virtual void		Unk_76(void);
	virtual void		Unk_77(void);
	virtual void		Unk_78(void);
	virtual AnimData*	GetAnimData(void);			// 0079
	virtual ValidBip01Names * GetValidBip01Names(void);	// 007A	Character only
	virtual ValidBip01Names * CallGetValidBip01Names(void);
	virtual void		SetValidBip01Names(ValidBip01Names validBip01Names);
	virtual void		GetPos(void);				// GetPos or GetDistance
	virtual void		Unk_7E(UInt32 arg0);
	virtual void		Unk_7F(void);
	virtual void		Unk_80(UInt32 arg0);
	virtual void		Unk_81(UInt32 arg0);
	virtual void		Unk_82(void);
	virtual UInt32		Unk_83(void);
	virtual void		Unk_84(UInt32 arg0);
	virtual UInt32		Unk_85(void);			// 0 or GetActor::Unk01AC
	virtual bool		Unk_86(void);			// return false for Projectile, Actor and Creature, true for character and PlayerCharacter
	virtual bool		Unk_87(void);			// seems to always return 0
	virtual bool		Unk_88(void);			// seems to always return 0
	virtual void		Unk_89(void);
	virtual void		Unk_8A(void);			// SetParentCell (Interior only ?)
	virtual void		Unk_8B(void);			// IsDead = HasNoHealth (baseForm health <= 0 or Flags bit23 set)
	virtual bool		Unk_8C(void);
	virtual bool		Unk_8D(void);
	virtual void		Unk_8E(void);
	virtual void		Unk_8F(void);

	enum {
		kFlags_Unk00000002	= 0x00000002,
		kFlags_Deleted		= 0x00000020,		// refr removed from .esp or savegame
		kFlags_Persistent	= 0x00000400,		//shared bit with kFormFlags_QuestItem
		kFlags_Temporary	= 0x00004000,
		kFlags_Taken		= kFlags_Deleted | kFlags_Unk00000002,

		kChanged_Inventory	= 0x08000000,
	};

	struct RenderState
	{
		UInt32	unk00;
		UInt32	unk04;
		float	unk08;		// waterLevel
		float	unk0C;
		UInt32	unk10;		// flags most likely
		NiNode	* niNode;	// same in FOSE
		// possibly more, need to find alloc
	};

	struct EditorData {
		UInt32	unk00;	// 00
	};
	// 0C

#ifdef EDITOR
	EditorData	editorData;			// +04
#endif

	TESChildCell	childCell;				// 018

	UInt32			unk1C;					// 01C

	TESForm			* baseForm;				// 020
	
	float			rotX, rotY, rotZ;		// 024 - either public or accessed via simple inline accessor common to all child classes
	float			posX, posY, posZ;		// 030 - seems to be private
	float			scale;					// 03C 

	TESObjectCELL	* parentCell;			// 040
	ExtraDataList	extraDataList;			// 044
	RenderState		* renderState;			// 064	- (05C in FOSE)

	ScriptEventList *	GetEventList() const;

	bool IsTaken() const { return (flags & kFlags_Taken) != 0; }
	bool IsPersistent() const { return (flags & kFlags_Persistent) != 0; }
	bool IsTemporary() { return (flags & kFlags_Temporary) ? true : false; }
	bool IsDeleted() const { return (flags & kFlags_Deleted) ? true : false; }

	bool Update3D();
	bool Update3D_v1c();	// Less worse version as used by some modders
	TESContainer* GetContainer();
	bool IsMapMarker();
	bool GetInventoryItems(InventoryItemsMap &invItems);
	ExtraDroppedItemList* GetDroppedItems();

	static TESObjectREFR* Create(bool bTemp = false);

	MEMBER_FN_PREFIX(TESObjectREFR);
#if RUNTIME
	DEFINE_MEMBER_FN(Activate, bool, 0x00573170, TESObjectREFR*, UInt32, UInt32, UInt32);	// Usage Activate(actionRef, 0, 0, 1); found inside Cmd_Activate_Execute as the last call (190 bytes)
	DEFINE_MEMBER_FN(Set3D, void, 0x0094EB40, NiNode*, bool);	// Usage Set3D(niNode, true); virtual func 0073
#endif
};

TESForm* GetPermanentBaseForm(TESObjectREFR* thisObj);	// For LevelledForm, find real baseForm, not temporary one.

STATIC_ASSERT(offsetof(TESObjectREFR, baseForm) == 0x020);
STATIC_ASSERT(offsetof(TESObjectREFR, extraDataList) == 0x044);
STATIC_ASSERT(sizeof(TESObjectREFR) == 0x068);

class BaseProcess;

// 088
class MobileObject : public TESObjectREFR
{
public:
	MobileObject();
	~MobileObject();

	virtual void		StartHighProcess(void);		// 090
	virtual void		Unk_91(void);
	virtual void		Unk_92(void);
	virtual void		Unk_93(void);
	virtual void		Unk_94(void);
	virtual void		Unk_95(void);
	virtual void		Unk_96(void);
	virtual void		Unk_97(void);
	virtual void		Unk_98(void);
	virtual void		Unk_99(void);
	virtual void		Unk_9A(void);
	virtual void		Unk_9B(void);
	virtual void		Unk_9C(void);
	virtual void		Unk_9D(void);
	virtual void		Unk_9E(void);
	virtual void		Unk_9F(void);
	virtual void		Unk_A0(void);	// StartConversation(targetActor, subjectLocationData, targetLocationData, headTrack, allowMovement, arg, topicID, arg, arg
	virtual void		Unk_A1(void);
	virtual void		Unk_A2(void);
	virtual void		Unk_A3(void);
	virtual void		Unk_A4(void);
	virtual void		Unk_A5(void);
	virtual void		Unk_A6(void);
	virtual void		Unk_A7(void);
	virtual void		Unk_A8(void);
	virtual void		Unk_A9(void);
	virtual void		Unk_AA(void);
	virtual void		Unk_AB(void);
	virtual void		Unk_AC(void);
	virtual void		Unk_AD(void);
	virtual void		Unk_AE(void);
	virtual void		Unk_AF(void);
	virtual void		Unk_B0(void);
	virtual void		Unk_B1(void);
	virtual void		Unk_B2(void);
	virtual void		Unk_B3(void);
	virtual void		Unk_B4(void);
	virtual void		Unk_B5(void);
	virtual void		Unk_B6(void);
	virtual void		Unk_B7(void);
	virtual void		Unk_B8(void);
	virtual void		Unk_B9(void);
	virtual void		Unk_BA(void);
	virtual void		Unk_BB(void);
	virtual void		Unk_BC(void);
	virtual void		Unk_BD(void);
	virtual void		Unk_BE(void);
	virtual void		Unk_BF(void);
	virtual void		Unk_C0(void);
	
	BaseProcess	* baseProcess;	// 068
	UInt32		unk06C;			// 06C - loaded	set to the talking actor ref when initialising ExtraTalkingActor
	UInt32		unk070;			// 070 - loaded
	UInt32		unk074;			// 074 - loaded
	UInt32		unk078;			// 078 - loaded
	UInt8		unk07C;			// 07C - loaded
	UInt8		unk07D;			// 07D - loaded
	UInt8		unk07E;			// 07E - loaded
	UInt8		unk07F;			// 07F - loaded
	UInt8		unk080;			// 080 - loaded
	UInt8		unk081;			// 081 - loaded
	UInt8		unk082;			// 082 - cleared when starting combat on player
	UInt8		unk083;			// 083 - loaded
	UInt8		unk084;			// 084 - loaded
	UInt8		unk085;			// 085 - loaded
	UInt8		unk086;			// 086 - loaded
	UInt8		unk087;			// 087	Init'd to the inverse of NoLowLevelProcessing
};
STATIC_ASSERT(offsetof(MobileObject, baseProcess) == 0x068);
STATIC_ASSERT(sizeof(MobileObject) == 0x088);

// 00C
class MagicCaster
{
public:
	MagicCaster();
	~MagicCaster();

	UInt32 vtabl;
	UInt32 unk004;	// 004
	UInt32 unk008;	// 008
};

STATIC_ASSERT(sizeof(MagicCaster) == 0x00C);

typedef tList<ActiveEffect> EffectNode;
// 010
class MagicTarget
{
public:
	MagicTarget();
	~MagicTarget();

	virtual void	Destructor(void);
	virtual TESObjectREFR *	GetParent(void);
	virtual EffectNode *	GetEffectList(void);

	UInt8			byt004;		// 004 
	UInt8			byt005;		// 005 
	UInt8			byt006[2];	// 006-7
	tList<void*>	lst008;		// 008

	void RemoveEffect(EffectItem *effItem);

	void StopEffect(void *arg0, bool arg1)
	{
		ThisStdCall(0x8248E0, this, arg0, arg1);
	}
};

STATIC_ASSERT(sizeof(MagicTarget) == 0x010);

class hkaRaycastInterface
{
public:
	hkaRaycastInterface();
	~hkaRaycastInterface();
	virtual hkaRaycastInterface*	Destroy(bool doFree);
	virtual void					Unk_01(void* arg0);
	virtual void					Unk_02(void);

	// Assumed to be 0x010 bytes due to context where the vftable is used
	UInt32	unk000[(0x010 - 0x004) >> 2];	// 0000
};
STATIC_ASSERT(sizeof(hkaRaycastInterface) == 0x010);

class bhkRagdollController : public hkaRaycastInterface
{
public:
	bhkRagdollController();
	~bhkRagdollController();

	UInt32	unk000[(0x021C - 0x010) >> 2];	// 0010
	UInt8	fill021C[3];					// 021C
	bool	bool021F;						// 021F	when null assume FIK status is false
	bool	fikStatus;						// 0220
	UInt8	fill0221[3];					// 0221
};
STATIC_ASSERT(sizeof(bhkRagdollController) == 0x0224);

class bhkRagdollPenetrationUtil;
class ActorMover;
class PlayerMover;
class ImageSpaceModifierInstanceDOF;
class ImageSpaceModifierInstanceDRB;

class ActorMover	// I need to call Func008
{
public:
	virtual void		Unk_00(void);
	virtual void		Unk_01(void);
	virtual void		Unk_02(void);
	virtual void		Unk_03(void);
	virtual void		Unk_04(void);
	virtual void		Unk_05(void);
	virtual void		Unk_06(void);
	virtual void		Unk_07(void);
	virtual UInt32		Unk_08(void);	// for PlayerMover, it is GetMovementFlags
		// bit 11 = swimming 
		// bit 9 = sneaking
		// bit 8 = run
		// bit 7 = walk
		// bit 0 = keep moving (Q)
};

typedef std::vector<TESForm*> EquippedItemsList;

class Actor : public MobileObject
{
public:
	Actor();
	~Actor();

	virtual void		Unk_C1(void);
	virtual void		Unk_C2(void);
	virtual void		Unk_C3(void);
	virtual void		Unk_C4(void);
	virtual void		Unk_C5(void);
	virtual void		Unk_C6(void);
	virtual void		Unk_C7(void);
	virtual void		Unk_C8(void);
	virtual void		Unk_C9(void);
	virtual void		Unk_CA(void);
	virtual void		Unk_CB(void);
	virtual void		Unk_CC(void);
	virtual void		Unk_CD(void);
	virtual void		Unk_CE(void);
	virtual void		Unk_CF(void);
	virtual void		Unk_D0(void);
	virtual void		Unk_D1(void);
	virtual void		Unk_D2(void);
	virtual void		Unk_D3(void);
	virtual void		Unk_D4(void);
	virtual void		Unk_D5(void);
	virtual void		Unk_D6(void);
	virtual void		Unk_D7(void);
	virtual void		Unk_D8(void);	// IsPlayerRef
	virtual void		Unk_D9(void);
	virtual void		Unk_DA(void);
	virtual void		Unk_DB(void);
	virtual void		Unk_DC(void);
	virtual void		Unk_DD(void);
	virtual void		Unk_DE(void);
	virtual void		Unk_DF(void);
	virtual void		Unk_E0(void);
	virtual void		Unk_E1(void);
	virtual void		Unk_E2(void);
	virtual void		Unk_E3(void);
	virtual void		Unk_E4(void);	// Creature = 0, Character = 1, PlayerCharacter = 2
	virtual void		Unk_E5(void);
	virtual void		Unk_E6(void);
	virtual void		Unk_E7(void);
	virtual void		Unk_E8(void);
	virtual void		Unk_E9(void);
	virtual void		Unk_EA(void);
	virtual void		Unk_EB(void);
	virtual void		Unk_EC(void);
	virtual void		Unk_ED(void);
	virtual void		Unk_EE(void);
	virtual void		Unk_EF(void);
	virtual void		Unk_F0(void);
	virtual void		Unk_F1(void);
	virtual void		Unk_F2(void);
	virtual void		Unk_F3(void);
	virtual void		Unk_F4(void);
	virtual void		Unk_F5(void);
	virtual void		Unk_F6(void);
	virtual void		Unk_F7(void);
	virtual void		Unk_F8(void);	// 0F8 bool AddSpell(spell)
	virtual void		Unk_F9(void);
	virtual void		Unk_FA(void);
	virtual void		Unk_FB(void);
	virtual void		Unk_FC(void);
	virtual void		Unk_FD(void);
	virtual void		Unk_FE(void);
	virtual void		Unk_FF(void);
	virtual void		Unk_100(void);
	virtual void		Unk_101(void);
	virtual void		Unk_102(void);
	virtual void		Unk_103(void);
	virtual void		Unk_104(void);
	virtual void		Unk_105(void);
	virtual void		Unk_106(void);
	virtual void		Unk_107(void);
	virtual void		Unk_108(void);
	virtual void		Unk_109(void);
	virtual bool		IsInCombat(void);
	virtual Actor		*GetCombatTarget(void);
	virtual void		Unk_10C(void);
	virtual void		Unk_10D(void);
	virtual void		Unk_10E(void);
	virtual void		Unk_10F(void);
	virtual void		Unk_110(void);
	virtual void		Unk_111(void);
	virtual void		Unk_112(void);
	virtual void		Unk_113(void);
	virtual void		Unk_114(void);
	virtual void		Unk_115(void);
	virtual void		Unk_116(void);
	virtual void		Unk_117(void);
	virtual void		Unk_118(void);
	virtual void		Unk_119(void);
	virtual void		Unk_11A(void);
	virtual void		Unk_11B(void);
	virtual void		Unk_11C(void);
	virtual void		Unk_11D(void);
	virtual void		Unk_11E(void);
	virtual void		Unk_11F(void);
	virtual void		Unk_120(void);
	virtual void		Unk_121(void);
	virtual void		Unk_122(void);	//	0122	Check for LevelUp
	virtual void		Unk_123(void);
	virtual void		Unk_124(void);
	virtual void		Unk_125(void);
	virtual void		SetPerkRank(BGSPerk* perk, UInt8 rank, bool alt);
	virtual void		Unk_127(void);
	virtual UInt8		GetPerkRank(BGSPerk* perk, bool alt);
	virtual void		Unk_129(void);
	virtual void		Unk_12A(void);
	virtual void		Unk_12B(void);
	virtual void		Unk_12C(void);
	virtual void		Unk_12D(void);
	virtual void		Unk_12E(void);
	virtual void		Unk_12F(void);
	virtual void		Unk_130(void);
	virtual void		Unk_131(void);
	virtual void		Unk_132(void);
	virtual void		Unk_133(void);
	virtual void		Unk_134(void);
	virtual void		Unk_135(void);
	virtual void		Unk_136(void);
	
	MagicCaster			magicCaster;			// 088
	MagicTarget			magicTarget;			// 094
	ActorValueOwner		avOwner;				// 0A4
	CachedValuesOwner	cvOwner;				// 0A8

	bhkRagdollController				* ragDollController;			// 0AC
	bhkRagdollPenetrationUtil			* ragDollPentrationUtil;		// 0B0
	UInt32								unk0B4[(0x104-0x0b4) >> 2];		// 0B4
	UInt8								unk104;							// 104 Is in combat
	UInt8								pad105[3];						// 105
	UInt32								lifeState;						// 108 saved as byte HasHealth = 1 or 2, optionally 6
	UInt32								unk10C[(0x140-0x10C) >> 2];		// 10B 12C is an array (of combat targets ?)
	UInt32								unk140;							// 140 looks like a flag. Bit31 used while checking if non essential can take damage
	UInt8                               unk144;							// 144
	UInt8                               unk145;							// 145 Has to do with package evaluation
	UInt8                               unk146;							// 146 Has to do with package evaluation
	UInt8                               unk147;							// 147
	UInt32								unk148;							// 148
	UInt8								unk14C;							// 14C
	UInt8								isSwimming;						// 14D
	UInt8								unk14E;							// 14E
	UInt8								unk14F;							// 14F
	UInt32								unk150[(0x190-0x150) >> 2];		// 150	-	if Actor and not player: 0160 receives a copy of Pos[X,Y,Z] and Rot if 0170 is null during Func011B. 0170 receive ParentCell if Interior, Worldspace otherwize
	ActorMover							* actorMover;					// 190

	UInt32								unk194[(0x1B0-0x194) >> 2];		// 184 Byt018D = PlayerTeammate, 1AC is a list (or not! I have it loaded from a byte)

	UInt8								unk1B0;							// 1B0
	UInt8								unk1B1;							// 1B1
	UInt8								unk1B2[2];						// 1B2

	// OBSE: unk1 looks like quantity, usu. 1; ignored for ammo (equips entire stack). In NVSE, pretty much always forced internally to 1 
	// OBSE: itemExtraList is NULL as the container changes entry is not resolved before the call
	// NVSE: Default values are those used by the vanilla script functions.
	void EquipItem(TESForm * objType, UInt32 equipCount = 1, ExtraDataList* itemExtraList = NULL, UInt32 unk3 = 1, bool lockEquip = false, UInt32 unk5 = 1);	// unk3 apply enchantment on player differently
	void UnequipItem(TESForm* objType, UInt32 unequipCount = 1, ExtraDataList* itemExtraList = NULL, UInt32 unk3 = 1, bool lockEquip = false, UInt32 unk5 = 1);

	EquippedItemsList	GetEquippedItems();
	ExtraContainerDataArray	GetEquippedEntryDataList();
	ExtraContainerExtendDataArray GetEquippedExtendDataList();
};

STATIC_ASSERT(offsetof(Actor, magicCaster) == 0x088);

class Character : public Actor
{
public:
	Character();
	~Character();

	virtual void		Unk_137(void);
	virtual void		Unk_138(void);

	ValidBip01Names	* validBip01Names;	// 1B4
	float			flt1B8;				// 1B8
	float			flt1BC;				// 1BC
	UInt8			byt1C0;				// 1C0
	UInt8			byt1C1;				// 1C1
	UInt16			unk1C2;				// 1C2
	float			flt1C4;				// 1C4
};

struct CombatActors;

typedef tList<BGSQuestObjective::Target> QuestObjectiveTargets;

// 9BC
class PlayerCharacter : public Character
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	// used to flag controls as disabled in disabledControlFlags
	enum {
		kControlFlag_Movement		= 1 << 0,
		kControlFlag_Look			= 1 << 1,
		kControlFlag_Pipboy			= 1 << 2,
		kControlFlag_Fight			= 1 << 3,
		kControlFlag_POVSwitch		= 1 << 4,
		kControlFlag_RolloverText	= 1 << 5,
		kControlFlag_Sneak			= 1 << 6,
	};

	virtual void		Unk_139(void);
	virtual void		Unk_13A(void);

	// lotsa data

	UInt32								unk1C8[(0x244-0x1C8) >> 2];		// 1C8	0224 is a package of type 1C, 208 could be a DialogPackage, 206 questObjectiveTargets is valid
	float								unk244[0x4D];					// 244	have to be a set of ActorValue
	float								unk378[0x4D];					// 378	have to be a set of ActorValue
	UInt32								unk4AC;							// 4AC
	float								unk4B0[0x4D];					// 4B0	have to be a set of ActorValue
	BGSNote								* note;							// 5E4
	UInt32								unk574;							// 5E8
	ImageSpaceModifierInstanceDOF		* unk5EC;						// 5EC
	ImageSpaceModifierInstanceDOF		* unk5F0;						// 5F0
	ImageSpaceModifierInstanceDRB		* unk5F4;						// 5F4
	UInt32								unk5F8;							// 5F8
	tList<Actor>						teammates;						// 5FC
	UInt32								unk604[(0x648 - 0x604) >> 2];	// 604
	UInt8								unk648;							// 648
	UInt8								unk649;							// 649
	bool								unk64A;							// 64A	= not FirstPerson
	UInt8								unk64B;							// 64B
	bool								bThirdPerson;					// 64C
	UInt8								unk64D[3];	
	UInt32								unk650[(0x680 - 0x650) >> 2];	// 650 
	UInt8								disabledControlFlags;			// 680 kControlFlag_xxx
	UInt8								unk0681[3];						// 681
	UInt32								unk684[(0x68C - 0x684) >> 2];	// 684
	ValidBip01Names						* playerVB01N;					// 68C
	ExtraAnim::Animation				* extraAnimation;				// 690 ExtraDataAnim::Data
	NiNode								* playerNode;					// 694 used as node if unk64A is true
	UInt32								unk698[(0x6A8-0x698) >> 2];		// 698
	TESTopic							* topic;						// 6A8
	UInt32								unk6AC[3];						// 6AC
	TESQuest							* quest;						// 6B8
	tList<BGSQuestObjective>			questObjectiveList;				// 6BC
	UInt32								unk6C4[39];				// 6C4
	TESRegion							*currentRegion;			// 760
	TESRegionList						regionsList;			// 764
	UInt32								unk770[18];				// 770
	UInt8								gameDifficulty;			// 7B8
	UInt8								pad7B9[3];				// 7B9
	bool								isHardcore;				// 7BC
	UInt8								pad7BD[3];				// 7BD
	UInt32								unk7C0[49];				// 7C0
	tList<BGSEntryPointPerkEntry>		perkEntries[74];		// 884
	UInt32								unkAD4[164];			// AD4
	CombatActors						*combatActors;			// D64
	UInt32								unkD68[3];				// D68
	UInt8								unkD74[96];				// D74
	UInt32								unkDD4[(0x0E50 - 0x0DD4) / 4];	// DD4

		// 7C4 is a byte used during Combat evaluation (Player is targetted ?), 
		// 7C6 is a boolean meaning toddler, 
		// 7C7 byte bool PCCanUsePowerArmor, Byt0E39 referenced during LoadGame
		// Used when entering FlyCam : 7E8/7EC/7F0 stores Pos, 7F0 adjusted by scaledHeight multiplied by 0698 , 7E0 stores RotZ, 7E4 RotX
		// Perks forms a list at 87C and AD4. Caravan Cards at 614 and 618. Quest Stage LogEntry at 6B0. Quest Objectives at 6BC.
		// Hardcore flag would be E38. Byte at DF0 seems to be PlayerIsInCombat
		// tList at 6C4 is cleared when there is no current quest. There is another NiNode at 069C
		// list of perk and perkRank are at 0x087C and 0x0AD4 (alt perks). 086C is cleared after equipement change.
		// D68 counts the Teammates.
		// D74: 96 bytes that are cleared when the 3D is cleared.

	bool IsThirdPerson() { return bThirdPerson ? true : false; }
	UInt32 GetMovementFlags() { return actorMover->Unk_08(); }	// 11: IsSwimming, 9: IsSneaking, 8: IsRunning, 7: IsWalking, 0: keep moving
	bool IsPlayerSwimming() { return (GetMovementFlags()  >> 11) & 1; }

	static PlayerCharacter*	GetSingleton();
	bool SetSkeletonPath(const char* newPath);
	bool SetSkeletonPath_v1c(const char* newPath);	// Less worse version as used by some modders
	static void UpdateHead(void);
	QuestObjectiveTargets* GetCurrentQuestObjectiveTargets();
};

extern PlayerCharacter** g_thePlayer;

STATIC_ASSERT(offsetof(PlayerCharacter, ragDollController) == 0x0AC);
STATIC_ASSERT(offsetof(PlayerCharacter, questObjectiveList) == 0x6BC);
STATIC_ASSERT(offsetof(PlayerCharacter, bThirdPerson) == 0x64C);
STATIC_ASSERT(offsetof(PlayerCharacter, actorMover) == 0x190);
STATIC_ASSERT(sizeof(PlayerCharacter) == 0xE50);
