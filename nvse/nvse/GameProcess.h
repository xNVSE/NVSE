#pragma once

#include "NiNodes.h"
#include "GameForms.h"
#include "GameTasks.h"

class TESAmmo;
class TESObjectWEAP;

// Straight from OBSE. Needs to be debugged ! ! ! 
// This is used all over the game code to manage actors and occassionally other objects.
class ActorProcessManager
{
public:
	ActorProcessManager();
	~ActorProcessManager();

	struct ActorList {
		tList<Actor>			head;
		tList<Actor>::_Node		* tail;
	};

	ActorList				middleHighActors;		// 00
	ActorList				lowActors0C;			// 0C significantly smaller list than lowActors18. 
													//	  Plausible: actors currently scheduled for low processing
	ActorList				lowActors18;			// 18
	float					unk24;					// 24
	UInt32					unk28;					// 28
	void					* unk2C;				// 2C
	UInt32					unk30;					// 30
	void					* unk34;				// 34
	UInt32					unk38[0x0A];			// 38
	// almost confirmed to be at index 60
	tList<BSTempEffect>		tempEffects;			// 60
	// needs recalc of offsets
	UInt32					unk4C[4];				// 4C
	tList<Actor>			highActors;				// 5C
	Actor					* actor64;				// 64
	tList<Actor>::_Node		* unkNodes[3];			// 68 ##TODO: which lists do these belong to
	UInt32					unk74;					// 74 Possibly not a member. Definitely no more members following this.
};

extern ActorProcessManager * g_actorProcessManager;

class BaseProcess
{
public:
	BaseProcess();
	~BaseProcess();

	struct AmmoInfo
	{
		void*	unk00;	// 00
		UInt32	count;	// 04
		TESAmmo* ammo;	// 08
		UInt32	unk0C;	// 0C
		UInt32	unk10;	// 10
		UInt32	unk14;	// 14
		UInt32	unk18;	// 18
		UInt32	unk1C;	// 1C
		UInt32	unk20;	// 20
		UInt32	unk24;	// 24
		UInt32	unk28;	// 28
		UInt32	unk2C;	// 2C
		UInt32	unk30;	// 30
		UInt32	unk34;	// 34
		UInt32	unk38;	// 38
		UInt32	unk3C;	// 3C
		UInt32	unk40;	// 40
		TESObjectWEAP* weapon;	// 44
	};
	struct WeaponInfo
	{
		ExtraDataList	**xData;
		UInt32			unk04;
		TESObjectWEAP	*weapon;

		ExtraDataList *GetExtraData()
		{
			return xData ? *xData : NULL;
		}
	};

	struct Data004 {
		TESPackage		* package;		// 000
		TESPackageData	* packageData;	// 004
		TESObjectREFR	* targetRef;	// 008
		UInt32			unk00C;			// 00C	Initialized to 0FFFFFFFFh, set to 0 on start
		float			flt010;			// 010	Initialized to -1.0	. Set to GameHour on start so some time
		UInt32			flags;			// 014	Flags, bit0 would be not created and initialized
	};	// 018

	struct	Data02C {
		float	flt000;
		float	flt004;
		float	flt008;
		float	flt00C;
		float	flt010;
		float	flt014;
		float	flt018;
		float	flt01C;
		float	flt020;
		float	flt024;
		float	flt028;
		float	flt02C;
		UInt32	unk030;
		UInt32	unk034;
		float	flt038;
		float	flt03C;
		UInt8	byt040;
		UInt8	fil041[3];
		UInt32	unk044;			// 044	flags, bit28 = IsGhostn
	};

	virtual void	Destroy(bool noDealloc);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);
	virtual void	Unk_12(void);
	virtual void	Unk_13(void);
	virtual void	Unk_14(void);
	virtual void	Unk_15(void);
	virtual void	Unk_16(void);
	virtual void	Unk_17(void);
	virtual void	Unk_18(void);
	virtual void	Unk_19(void);
	virtual void	Unk_1A(void);
	virtual void	Unk_1B(void);
	virtual void	Unk_1C(void);
	virtual void	Unk_1D(void);
	virtual void	Unk_1E(void);
	virtual void	Unk_1F(void);
	virtual void	Unk_20(void);
	virtual void	Unk_21(void);
	virtual void	Unk_22(void);
	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);
	virtual void	Unk_2D(void);
	virtual void	Unk_2E(void);
	virtual void	Unk_2F(void);
	virtual void	Unk_30(void);
	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
	virtual void	Unk_35(void);
	virtual void	Unk_36(void);
	virtual void	Unk_37(void);
	virtual void	Unk_38(void);
	virtual void	Unk_39(void);
	virtual void	Unk_3A(void);
	virtual void	Unk_3B(void);
	virtual void	Unk_3C(void);
	virtual void	Unk_3D(void);
	virtual void	Unk_3E(void);
	virtual void	Unk_3F(void);
	virtual void	Unk_40(void);
	virtual void	Unk_41(void);
	virtual void	Unk_42(void);
	virtual void	Unk_43(void);
	virtual void	Unk_44(void);
	virtual void	Unk_45(void);
	virtual void	Unk_46(void);
	virtual void	Unk_47(void);
	virtual void	Unk_48(void);
	virtual void	Unk_49(void);
	virtual void	Unk_4A(void);		// Due to context, could be GetCombatTarget
	virtual void	Unk_4B(void);
	virtual void	Unk_4C(void);
	virtual void	Unk_4D(void);
	virtual void	Unk_4E(void);
	virtual void	Unk_4F(void);
	virtual void	Unk_50(void);
	virtual void	Unk_51(void);
	virtual WeaponInfo	*GetWeaponInfo();	// unk0114
	virtual AmmoInfo*	GetAmmoInfo();	// unk0118
	virtual void	Unk_54(void);
	virtual void	Unk_55(void);
	virtual void	Unk_56(void);
	virtual void	Unk_57(void);
	virtual void	Unk_58(void);
	virtual void	Unk_59(void);
	virtual void	Unk_5A(void);
	virtual void	Unk_5B(void);
	virtual void	Unk_5C(void);
	virtual void	Unk_5D(void);	// Called by 5E with count itemExtraList item
	virtual void	Unk_5E(void);	// EquipItem and UnEquipItem doEquip item count itemExtraList bytes = [equipArgC lockUnequip unk unEquipArcC lockEquip arg14 ] (arg as from Actor::(Un)EquipItem)
	virtual void	Unk_5F(void);
	virtual void	Unk_60(void);
	virtual void	Unk_61(void);
	virtual void	Unk_62(void);
	virtual void	Unk_63(void);

	virtual void	Unk_64(void);
	virtual void	Unk_65(void);
	virtual void	Unk_66(void);
	virtual void	Unk_67(void);
	virtual void	Unk_68(void);
	virtual void	Unk_69(void);
	virtual void	Unk_6A(void);
	virtual void	Unk_6B(void);
	virtual void	Unk_6C(void);
	virtual void	Unk_6D(void);
	virtual void	Unk_6E(void);
	virtual void	Unk_6F(void);
	virtual void	Unk_70(void);
	virtual void	Unk_71(void);
	virtual void	Unk_72(void);
	virtual void	Unk_73(void);
	virtual void	Unk_74(void);
	virtual void	Unk_75(void);
	virtual void	Unk_76(void);
	virtual void	Unk_77(void);
	virtual void	Unk_78(void);
	virtual void	Unk_79(void);
	virtual void	Unk_7A(void);
	virtual void	Unk_7B(void);
	virtual void	Unk_7C(void);
	virtual void	Unk_7D(void);
	virtual void	Unk_7E(void);
	virtual void	Unk_7F(void);
	virtual void	Unk_80(void);
	virtual void	Unk_81(void);
	virtual void	Unk_82(void);
	virtual void	Unk_83(void);	// 083 - GetInterruptPackage
	virtual void	Unk_84(void);	// 084 - SetInterruptPackage
	virtual void	Unk_85(void);	// 085 - StopInterruptPackage
	virtual void	Unk_86(void);	// 086 - SetInterruptPackageTargetRef
	virtual void	Unk_87(void);	// 087 - SetInterruptPackageTargetRef
	virtual void	Unk_88(void);	// 088 - IncreaseInterruptPackageUnk00C
	virtual void	Unk_89(void);
	virtual void	Unk_8A(void);
	virtual void	Unk_8B(void);	// 08B - GetStablePackage
	virtual void	Unk_8C(void);	// 08C - SetStablePackage
	virtual void	Unk_8D(void);	// 08D - StopStablePackage
	virtual void	Unk_8E(void);
	virtual void	Unk_8F(void);
	virtual void	Unk_90(void);
	virtual void	Unk_91(void);
	virtual void	Unk_92(void);	// Only HighProcess, get Unk0454
	virtual void	Unk_93(void);
	virtual void	Unk_94(void);
	virtual void	Unk_95(void);
	virtual void	Unk_96(void);
	virtual void	Unk_97(void);
	virtual void	Unk_98(void);
	virtual void	Unk_99(void);
	virtual void	Unk_9A(void);
	virtual void	Unk_9B(void);
	virtual void	Unk_9C(void);
	virtual void	Unk_9D(void);
	virtual void	Unk_9E(void);
	virtual TESPackage*	GetCurrentPackage(void);
	virtual void	Unk_A0(void);	// returns Interrupt package TargetRef or Data004::Unk00C
	virtual void	Unk_A1();
	virtual void	Unk_A2();
	virtual void	Unk_A3();		// returns the current animation from Unk0138 aparently.
	virtual void	Unk_A4();
	virtual void	Unk_A5();
	virtual void	Unk_A6();
	virtual void	Unk_A7();
	virtual void	Unk_A8();
	virtual void	Unk_A9();
	virtual void	Unk_AA();
	virtual void	Unk_AB();
	virtual void	Unk_AC();
	virtual void	Unk_AD();
	virtual void	Unk_AE();
	virtual void	Unk_AF();
	virtual void	Unk_B0();
	virtual void	Unk_B1();
	virtual void	Unk_B2();
	virtual void	Unk_B3();
	virtual void	Unk_B4();
	virtual void	Unk_B5();
	virtual void	Unk_B6();
	virtual void	Unk_B7();
	virtual void	Unk_B8();
	virtual void	Unk_B9();
	virtual void	Unk_BA();
	virtual void	Unk_BB();
	virtual void	Unk_BC();
	virtual void	Unk_BD();
	virtual void	Unk_BE();
	virtual void	Unk_BF();
	virtual void	Unk_C0();
	virtual void	Unk_C1();
	virtual void	Unk_C2();
	virtual void	Unk_C3();
	virtual void	Unk_C4();
	virtual void	Unk_C5();
	virtual void	Unk_C6();
	virtual void	Unk_C7();
	virtual void	Unk_C8();
	virtual void	Unk_C9();
	virtual void	Unk_CA();
	virtual void	Unk_CB();
	virtual void	Unk_CC();
	virtual void	Unk_CD();
	virtual void	Unk_CE();
	virtual void	Unk_CF();
	virtual void	Unk_D0();
	virtual void	Unk_D1();
	virtual void	Unk_D2();
	virtual void	Unk_D3();
	virtual void	Unk_D4();
	virtual void	Unk_D5();
	virtual void	Unk_D6();
	virtual void	Unk_D7();
	virtual void	Unk_D8();
	virtual void	Unk_D9();
	virtual void	Unk_DA();
	virtual void	Unk_DB();
	virtual void	Unk_DC();
	virtual void	Unk_DD();
	virtual void	Unk_DE();
	virtual void	Unk_DF();
	virtual void	Unk_E0();
	virtual void	Unk_E1();
	virtual void	Unk_E2();
	virtual void	Unk_E3();
	virtual void	Unk_E4();
	virtual void	Unk_E5();
	virtual void	Unk_E6();
	virtual void	Unk_E7();	// float GetActorValue
	virtual void	Unk_E8();
	virtual void	Unk_E9();
	virtual void	Unk_EA();
	virtual void	Unk_EB();
	virtual void	Unk_EC();
	virtual void	Unk_ED();
	virtual void	Unk_EE();
	virtual void	Unk_EF();
	virtual void	Unk_F0();
	virtual void	Unk_F1();
	virtual void	Unk_F2();
	virtual void	Unk_F3();
	virtual void	Unk_F4();
	virtual void	Unk_F5();
	virtual void	Unk_F6();
	virtual void	Unk_F7();
	virtual void	Unk_F8();
	virtual void	Unk_F9();
	virtual void	Unk_FA();
	virtual void	Unk_FB();
	virtual void	Unk_FC();
	virtual void	Unk_FD();
	virtual void	Unk_FE();
	virtual void	Unk_FF();
	virtual void	Unk_100();
	virtual void	Unk_101();
	virtual void	Unk_102();
	virtual void	Unk_103();
	virtual void	Unk_104();
	virtual void	Unk_105();
	virtual void	Unk_106();
	virtual void	Unk_107();
	virtual void	Unk_108();
	virtual void	Unk_109();
	virtual void	Unk_10A();
	virtual void	Unk_10B();
	virtual void	Unk_10C();
	virtual void	Unk_10D();
	virtual void	Unk_10E();
	virtual void	Unk_10F();
	virtual void	Unk_110();
	virtual void	Unk_111();
	virtual void	Unk_112();
	virtual void	Unk_113();
	virtual void	Unk_114();
	virtual void	Unk_115();
	virtual void	Unk_116();
	virtual void	Unk_117();
	virtual void	Unk_118();
	virtual void	Unk_119();
	virtual void	Unk_11A();
	virtual void	Unk_11B();
	virtual void	Unk_11C();
	virtual void	Unk_11D();
	virtual void	Unk_11E();
	virtual void	Unk_11F();
	virtual void	Unk_120();
	virtual void	Unk_121();
	virtual void	Unk_122();
	virtual void	Unk_123();
	virtual void	Unk_124();
	virtual void	Unk_125();
	virtual void	Unk_126();
	virtual void	Unk_127();
	virtual void	Unk_128();
	virtual void	Unk_129();
	virtual void	Unk_12A();
	virtual void	Unk_12B();
	virtual void	Unk_12C();
	virtual void	Unk_12D();
	virtual void	Unk_12E();
	virtual void	Unk_12F();
	virtual void	Unk_130();
	virtual void	Unk_131();
	virtual void	Unk_132();
	virtual void	Unk_133();
	virtual void	Unk_134();
	virtual void	Unk_135();
	virtual void	Unk_136();
	virtual void	Unk_137();
	virtual void	Unk_138();
	virtual void	Unk_139();
	virtual void	Unk_13A();
	virtual void	Unk_13B();
	virtual void	Unk_13C();
	virtual void	Unk_13D();
	virtual void	Unk_13E();
	virtual void	Unk_13F();
	virtual void	Unk_140();
	virtual void	Unk_141();
	virtual void	Unk_142();
	virtual void	Unk_143();
	virtual void	Unk_144();
	virtual void	Unk_145();
	virtual void	Unk_146();
	virtual void	Unk_147();
	virtual void	Unk_148();
	virtual void	Unk_149();
	virtual void	Unk_14A();
	virtual void	Unk_14B();
	virtual void	Unk_14C();
	virtual void	Unk_14D();
	virtual void	Unk_14E();
	virtual void	Unk_14F();
	virtual void	Unk_150();
	virtual void	Unk_151();
	virtual void	Unk_152();
	virtual void	Unk_153();
	virtual void	Unk_154();
	virtual void	Unk_155();
	virtual void	Unk_156();
	virtual void	Unk_157();
	virtual void	Unk_158();
	virtual void	Unk_159();
	virtual void	Unk_15A();
	virtual void	Unk_15B();
	virtual void	Unk_15C();
	virtual void	Unk_15D();
	virtual void	Unk_15E();
	virtual void	Unk_15F();
	virtual void	Unk_160();
	virtual void	Unk_161();
	virtual void	Unk_162();
	virtual void	Unk_163();
	virtual void	Unk_164();
	virtual void	Unk_165();
	virtual void	Unk_166();
	virtual void	Unk_167();
	virtual void	Unk_168();
	virtual void	Unk_169();
	virtual void	Unk_16A();
	virtual void	Unk_16B();
	virtual void	Unk_16C();
	virtual void	Unk_16D();
	virtual void	Unk_16E();
	virtual void	Unk_16F();
	virtual void	Unk_170();
	virtual void	Unk_171();
	virtual void	Unk_172();
	virtual void	Unk_173();
	virtual void	Unk_174();
	virtual void	Unk_175();
	virtual void	Unk_176();
	virtual void	Unk_177();
	virtual void	Unk_178();
	virtual void	Unk_179();
	virtual void	Unk_17A();
	virtual void	Unk_17B();
	virtual void	Unk_17C();
	virtual void	Unk_17D();
	virtual void	Unk_17E();
	virtual void	Unk_17F();
	virtual void	Unk_180();
	virtual void	Unk_181();
	virtual void	Unk_182();
	virtual void	Unk_183();
	virtual void	Unk_184();
	virtual void	Unk_185();
	virtual void	Unk_186();
	virtual void	Unk_187();
	virtual void	Unk_188();
	virtual void	Unk_189();
	virtual void	Unk_18A();
	virtual void	Unk_18B();
	virtual void	Unk_18C();
	virtual void	Unk_18D();
	virtual void	Unk_18E();
	virtual void	Unk_18F();
	virtual void	Unk_190();
	virtual void	Unk_191();
	virtual void	Unk_192();
	virtual void	Unk_193();
	virtual void	Unk_194();
	virtual void	Unk_195();
	virtual void	Unk_196();
	virtual void	Unk_197();
	virtual void	Unk_198();
	virtual void	Unk_199();
	virtual void	Unk_19A();
	virtual void	Unk_19B();
	virtual void	Unk_19C();
	virtual void	Unk_19D();
	virtual void	Unk_19E();
	virtual void	Unk_19F();
	virtual void	Unk_1A0();
	virtual void	Unk_1A1();
	virtual void	Unk_1A2();
	virtual void	Unk_1A3();
	virtual void	Unk_1A4();
	virtual void	Unk_1A5();
	virtual void	Unk_1A6();
	virtual void	Unk_1A7();
	virtual void	Unk_1A8();
	virtual void	Unk_1A9();
	virtual void	Unk_1AA();
	virtual void	Unk_1AB();
	virtual void	Unk_1AC();
	virtual void	Unk_1AD();
	virtual void	Unk_1AE();
	virtual void	Unk_1AF();
	virtual void	Unk_1B0();
	virtual void	Unk_1B1();
	virtual void	Unk_1B2();
	virtual void	Unk_1B3();
	virtual void	Unk_1B4();
	virtual void	Unk_1B5();
	virtual void	Unk_1B6();
	virtual void	Unk_1B7();
	virtual void	Unk_1B8();
	virtual void	Unk_1B9();
	virtual void	Unk_1BA();
	virtual void	Unk_1BB();
	virtual void	Unk_1BC();
	virtual void	Unk_1BD();
	virtual void	Unk_1BE();
	virtual void	Unk_1BF();
	virtual void	Unk_1C0();
	virtual void	Unk_1C1();
	virtual void	Unk_1C2();
	virtual void	Unk_1C3();
	virtual void	Unk_1C4();
	virtual void	Unk_1C5();
	virtual void	Unk_1C6();
	virtual void	Unk_1C7();
	virtual void	Unk_1C8();
	virtual void	Unk_1C9();
	virtual void	Unk_1CA();
	virtual void	Unk_1CB();
	virtual void	Unk_1CC();
	virtual void	Unk_1CD();
	virtual void	Unk_1CE();
	virtual void	Unk_1CF();
	virtual void	Unk_1D0();
	virtual void	Unk_1D1();
	virtual void	Unk_1D2();
	virtual void	Unk_1D3();
	virtual void	Unk_1D4();
	virtual void	Unk_1D5();
	virtual void	Unk_1D6();
	virtual void	Unk_1D7();
	virtual void	Unk_1D8();
	virtual void	Unk_1D9();
	virtual void	Unk_1DA();
	virtual void	Unk_1DB();
	virtual void	Unk_1DC();
	virtual void	Unk_1DD();
	virtual void	Unk_1DE();
	virtual void	Unk_1DF();
	virtual void	Unk_1E0();
	virtual void	Unk_1E1();
	virtual void	Unk_1E2();
	virtual void	Unk_1E3();
	virtual void	Unk_1E4();
	virtual void	Unk_1E5();
	virtual void	Unk_1E6();
	virtual void	Unk_1E7();
	virtual void	Unk_1E8();
	virtual void	Unk_1E9();
	virtual void	Unk_1EA();
	virtual void	Unk_1EB();
	virtual void	Unk_1EC();
	virtual void	Unk_1ED();	// Leads to Last Target

	// data

	Data004		data004;		// current package ?
	float		flt01C;			// not initialized, only by descendant!
	float		flt020;			// not initialized, only by descendant to -1.0! flt020 gets set to GameHour minus one on package evaluation
	UInt32		unk024;			// not initialized, only by descendant!
	UInt32		processLevel;	// not initialized, only by descendant to 3 for Low, 2 for MidlleLow, 1 MiddleHighProcess and 0 for HigProcess
	Data02C*	unk02C;
};

class LowProcess : public BaseProcess
{
public:
	LowProcess();
	~LowProcess();

	struct FloatPair {
		float	flt000;
		float	flt004;
	};

	struct	ActorValueModifier
	{
		UInt8	actorValue;	// 00 Might allow for other values
		UInt8	pad[3];		// 01
		float	damage;		// 04
	};

	struct ActorValueModifiers
	{
		tList<ActorValueModifier>	avModifierList;	// 00
		UInt8						unk008;			// 08
		UInt8						pad009[3];		// 09
		void*						** modifiedAV;	// 0C	array of damaged actorValue
	};	// 10

	virtual void	Unk_1EE();
	virtual void	Unk_1EF();
	virtual void	Unk_1F0();
	virtual void	Unk_1F1();
	virtual void	Unk_1F2();
	virtual void	Unk_1F3();
	virtual void	Unk_1F4();
	virtual void	Unk_1F5();
	virtual void	Unk_1F6();
	virtual void	Unk_1F7();
	virtual void	Unk_1F8();
	virtual void	Unk_1F9();
	virtual void	Unk_1FA();
	virtual void	Unk_1FB();
	virtual void	Unk_1FC();
	virtual void	Unk_1FD();
	virtual void	Unk_1FE();
	virtual void	Unk_1FF();
	virtual void	Unk_200();
	virtual void	Unk_201();
	virtual void	Unk_202();
	virtual void	Unk_203();
	virtual void	Unk_204();
	virtual void	Unk_205();
	virtual void	Unk_206();
	virtual void	Unk_207();

	UInt8		byt030;		// Flags, used before being initialized . Ok, the initial value is zeroed out by a "and 0" but ???
	UInt8		pad031[3];
	UInt32		unk034;
	FloatPair	unk038;
	UInt32		unk03C;		// not initialized!
	UInt32		unk040;		// not initialized!	refr, expected actor, might be CombatTarget
	UInt32		unk044;
	UInt32		unk048;
	UInt32		unk04C;
	UInt32		unk050;		// not initialized!
	UInt32		unk054;
	UInt32		unk058;
	tList<UInt32>	unk05C;		// List
	UInt32		unk064;
	UInt32		unk068;
	tList<TESForm>	unk06C;
	tList<UInt32>	unk074;
	tList<UInt32>	unk07C;
	UInt32		unk084;
	UInt32		unk088;
	UInt32		unk08C;
	UInt32		unk090;
	ActorValueModifiers	damageModifiers;
	UInt32		unk098;		// not initialized!
	UInt32		unk0A0;		// not initialized!
	UInt32		unk0A4;		// not initialized!
	float		flt0A8;
	float		flt0AC;
	UInt8		byt0B0;
	UInt8		pad0B1[3];	// Filler
};
// LowProcess has 207 virtual func


class MiddleLowProcess : public LowProcess
{
public:
	MiddleLowProcess();
	~MiddleLowProcess();

	virtual void	Unk_207();

	UInt32				unk0B4;			// 0B4
	ActorValueModifiers	tempModifiers;	// 0B8
};	// 0C8

// MiddleLowProcess has 208 virtual func, 208 would be SetAnimation

class QueuedFile;
class bhkCharacterController;
class BSFaceGenAnimationData;
class BSBound;
class NiTriShape;

// 25C
class MiddleHighProcess : public MiddleLowProcess
{
public:
	MiddleHighProcess();
	~MiddleHighProcess();

	struct	Unk138 {
		UInt32	unk000;
		UInt32	unk004;		// Semaphore
			// ... ?
	};

	struct	Unk148 {
		UInt32	unk000;
		UInt32	unk004;
		UInt32	unk008;
		UInt16	unk00C;
		UInt8	unk00E;
		UInt8	fil00F;
	};

	virtual void	SetAnimation(UInt32 newAnimation);
	virtual void	Unk_209();
	virtual void	Unk_20A();
	virtual void	Unk_20B();
	virtual void	Unk_20C();
	virtual void	Unk_20D();
	virtual void	Unk_20E();
	virtual void	Unk_20F();
	virtual void	Unk_210();
	virtual void	Unk_211();
	virtual void	Unk_212();
	virtual void	Unk_213();
	virtual void	Unk_214();
	virtual void	Unk_215();
	virtual void	Unk_216();
	virtual void	Unk_217();
	virtual void	Unk_218();
	virtual void	Unk_219();
	virtual void	Unk_21A();
	virtual void	Unk_21B();

	tList<TESForm*>			unk0C8;			// 0C8
	tList<UInt32>			unk0D0;			// 0D0
	UInt32					unk0D8[(0xE4-0xD8) >> 2];
	BaseProcess::Data004	unk0E4;			// 0E4	I suspect interrupt package

	UInt8					unk0FC[0x0C];	// 0FC	Saved as one, might be Pos/Rot given size

	UInt32					unk108;			// 108
	TESIdleForm				*idleForm;		// 10C
	UInt32					unk110;			// 110  EntryData, also handled as part of weapon code. AmmoInfo.
	WeaponInfo*				weaponInfo;		// 114
	AmmoInfo*				ammoInfo;		// 118
	QueuedFile*				unk11C;			// 11C
	RefNiRefObject			unk120;			// 120
	UInt8					byt124;			// 124	OneHandGrenade equipped
	UInt8					byt125;			// 125	OneHandMine equipped
	UInt8					byt126;			// 126	OneHandThrown equipped
	UInt8					byt127;			// 127
	UInt8					byt128;			// 128
	UInt8					byt129;			// 129
	UInt8					byt12A;			// 12A
	UInt8					fil12B;			// 12B
	UInt32					unk12C;			// 12C
	UInt32					unk130;			// 130 Gets copied over during TESNPC.CopyFromBase
	UInt8					byt134;			// 134
	UInt8					byt135;			// 135
	UInt8					byt136;			// 136
	UInt8					byt137;			// 137

	RefNiObject				unk138;			// 138	its an animation/bhkCharacterController. Current Animation?

	UInt8					byt13C;			// 13C
	UInt8					byt13D;			// 13D

	UInt8					unk13E[(0x148-0x13E)];

	Unk148					unk148;			// 148
	
	UInt32					unk150[8];		// 150
	//UInt32				unk158;			
	//UInt32				unk15C;
	//UInt32				unk160;		// get/set.
	//UInt8					byt168;

	float					actorAlpha;		// 170
	UInt32					unk174;			// 174
	BSFaceGenAnimationData	*unk178;		// 178
	UInt32					unk17C[24];		// 17C
	//UInt8					byt18C;
	//tList<UInt32>			unk1B0;
	//tList<UInt32>			* unk1B8;		// get.
	//UInt32				animation;	// 1C0
	//UInt8					unk1C4[12];	// Cleared at the same time as the animation
	//UInt32				unk1DA;

	NiNode					*unk1DC;		// 1DC
	NiNode					*unk1E0;		// 1E0
	UInt32					unk1E4;			// 1E4
	NiNode					*unk1E8;		// 1E8
	UInt32					unk1EC;			// 1EC
	NiNode					*unk1F0;		// 1F0
	UInt32					unk1F4;			// 1F4
	NiNode					*unk1F8;		// 1F8
	UInt32					unk1FC[2];		// 1FC
	NiNode					*unk204;		// 204
	UInt32					unk208[4];		// 208
	NiNode					*unk218;		// 218
	NiNode					*unk21C;		// 21C
	RefNiObject				unk220;			// 220
	BSBound					*boundingBox;	// 224
	UInt32					unk228[8];		// 228
	BSFaceGenNiNode			*unk248;		// 248
	BSFaceGenNiNode			*unk24C;		// 24C
	NiTriShape				*unk250;		// 250
	UInt32					unk254[2];		// 254

	//TESPackage*			unk224;	// Set during StartConversation, package used by subject.

	//Unk0254*				contains lastTarget at 0x04;	// 0254
};
// MiddleHighProcess has 21B virtual func

// HighProcess: 	// something in wrd2EC and unk2F0, Unk039C count the actor targeting the player (in the player process).
	// Byt0410[6] presence of data in Unk03F8[6], referenced during StopCombat, Byt413, Byt0420, Unk0413, Unk041C

	////virtual void	Unk_21C();
	////virtual void	Unk_21D();
class AnimSequenceBase;

// 100+
struct AnimData
{
	struct Unk124
	{
		struct Unk18
		{
			UInt32			unk00[9];
			UInt32			unk24;
		};

		UInt32			unk00[6];
		Unk18* unk18;
	};

	struct Unk128
	{
		UInt32			unk00[11];
		TESIdleForm* idle2C;
	};

	UInt32							unk000;				// 000
	Actor* actor;				// 004
	NiNode* nSceneRoot;		// 008
	NiNode* nBip01;			// 00C
	UInt32							unk010;				// 010
	float							flt014;				// 014
	float							flt018;				// 018
	UInt32							unk01C;				// 01C
	float							flt020;				// 020
	UInt32							unk024;				// 024
	NiNode* nPelvis;			// 028
	NiNode* nBip01Copy;		// 02C
	NiNode* nLForearm;			// 030
	NiNode* nHead;				// 034
	NiNode* nWeapon;			// 038
	UInt32							unk03C[2];			// 03C
	NiNode* nNeck1;			// 044
	UInt32							unk048[5];			// 048
	SInt32							sequenceState1[8];	// 05C
	SInt32							sequenceState2[8];	// 07C
	UInt32							unk09C[12];			// 09C
	float							flt0CC;				// 0CC
	float							flt0D0;				// 0D0
	UInt32							unk0D4;				// 0D4
	NiControllerManager* unk0D8;			// 0D8
	NiTPointerMap<AnimSequenceBase>* unk0DC;			// 0DC
	BSAnimGroupSequence* animSequence[8];	// 0E0
	BSAnimGroupSequence* animSeq100;		// 100
	UInt32							unk104;				// 104
	UInt32							unk108;				// 108
	float							flt10C;				// 10C
	float							flt110;				// 110
	float							flt114;				// 114
	float							flt118;				// 118
	float							flt11C;				// 11C
	UInt8							byte120;			// 120
	UInt8							byte121;			// 121
	UInt16							word122;			// 122
	Unk124* unk124;			// 124
	Unk128* unk128;			// 128
};
STATIC_ASSERT(sizeof(AnimData) == 0x12C);