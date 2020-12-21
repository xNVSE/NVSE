#pragma once

class ActiveEffect
{
public:
	ActiveEffect();	// args are caster, magicItem, effectItem
	virtual ~ActiveEffect();

	virtual ActiveEffect *	Clone(void);
	virtual void			Unk_02(UInt32 arg);
	virtual void			Unk_03(UInt32 arg); // nullsub
	virtual void			SaveGame(UInt32 arg);
	virtual void			LoadGame(UInt32 arg);
	virtual void			Unk_06(UInt32 arg);
	virtual void			Unk_07(UInt32 arg);
	virtual void			Unk_08(UInt32 arg);
	virtual bool			UnregisterCaster(MagicCaster * _caster);	// returns 1 and clears caster if it matches the parameter, else returns 0
	virtual bool			Unk_0A(void);
	virtual void			Unk_0B(ActiveEffect * target);
	virtual bool			Unk_0C(UInt32 arg);
	virtual bool			Unk_0D(UInt32 arg);
	virtual void			Unk_0E(UInt32 arg);		// update/add effect?
	virtual void			Terminate(void);	// update/add effect?
	virtual void			Unk_10(UInt32 arg);
	virtual void			CopyTo(ActiveEffect* to);
	virtual void			Unk_12();
	virtual void			Unk_13();

//	void		** _vtbl;			// 00
	float		timeElapsed;		// 04
	MagicItem	*magicItem;			// 08
	EffectItem	* effectItem;		// 0C
	bool		bApplied;			// 10
	bool		bTerminated;		// 11 set to 1 when effect is to be removed
	UInt8		flags12;			// 12
	UInt8		pad13;				// 13
	UInt32		unk14;				// 14
	UInt32		unk18;				// 18 - flags
	float		magnitude;			// 1C - adjusted based on target?
	float		duration;			// 20 - adjusted based on target?
	MagicTarget	* target;			// 24
	MagicCaster	* caster;			// 28
	UInt32		spellType;			// 2C e.g. SpellItem::kType_Ability
	UInt32		sound;				// 30 Sound* in stewie's tweaks
	UInt32		unk34;				// 34
	UInt32		unk38;				// 38
	TESForm		* enchantObject;	// 3C enchanted obj responsible for effect
	tList<TESForm>	data;			// 40 - in ScriptEffect this is a Script *

	void Remove(bool bRemoveImmediately);
};
STATIC_ASSERT(sizeof(ActiveEffect) == 0x048);

class ValueModifierEffect : public ActiveEffect
{
public:
	ValueModifierEffect();
	~ValueModifierEffect();

	UInt32 actorVal;	// 48
};
STATIC_ASSERT(offsetof(ValueModifierEffect, actorVal) == 0x48);
STATIC_ASSERT(sizeof(ValueModifierEffect) == 0x04C);

class AssociatedItemEffect : public ActiveEffect
{
public:
	AssociatedItemEffect();
	~AssociatedItemEffect();

	TESObject	* item;	// 48 - creature, armor, weapon
};

class CommandEffect : public ActiveEffect
{
public:
	CommandEffect();
	~CommandEffect();
};

class AbsorbEffect : public ValueModifierEffect
{
public:
	AbsorbEffect();
	~AbsorbEffect();
};

class BoundItemEffect : public AssociatedItemEffect
{
public:
	BoundItemEffect();
	~BoundItemEffect();
};

class CalmEffect : public ValueModifierEffect
{
public:
	CalmEffect();
	~CalmEffect();
};

class ChameleonEffect : public ValueModifierEffect
{
public:
	ChameleonEffect();
	~ChameleonEffect();
};

class CommandCreatureEffect : public CommandEffect
{
public:
	CommandCreatureEffect();
	~CommandCreatureEffect();
};

class CommandHumanoidEffect : public CommandEffect
{
public:
	CommandHumanoidEffect();
	~CommandHumanoidEffect();
};

class ConcussionEffect : public ActiveEffect
{
public:
	ConcussionEffect();
	~ConcussionEffect();

	float	unk48;		//  48
};
STATIC_ASSERT(sizeof(ConcussionEffect) == 0x4C);

class CureEffect : public ActiveEffect
{
public:
	CureEffect();
	~CureEffect();
};

class DarknessEffect : public ValueModifierEffect
{
public:
	DarknessEffect();
	~DarknessEffect();
};

class DemoralizeEffect : public ActiveEffect
{
public:
	DemoralizeEffect();
	~DemoralizeEffect();
};

class DetectLifeEffect : public ValueModifierEffect
{
public:
	DetectLifeEffect();
	~DetectLifeEffect();
};

class DisintegrateArmorEffect : public ActiveEffect
{
public:
	DisintegrateArmorEffect();
	~DisintegrateArmorEffect();
};

class DisintegrateWeaponEffect : public ActiveEffect
{
public:
	DisintegrateWeaponEffect();
	~DisintegrateWeaponEffect();
};

class DispelEffect : public ActiveEffect
{
public:
	DispelEffect();
	~DispelEffect();
};

class FrenzyEffect : public ValueModifierEffect
{
public:
	FrenzyEffect();
	~FrenzyEffect();
};

class InvisibilityEffect : public ValueModifierEffect
{
public:
	InvisibilityEffect();
	~InvisibilityEffect();
};

class LightEffect : public ActiveEffect
{
public:
	LightEffect();
	~LightEffect();
};

class LimbConditionEffect : public ValueModifierEffect
{
public:
	LimbConditionEffect();
	~LimbConditionEffect();
};

class LockEffect : public ActiveEffect
{
public:
	LockEffect();
	~LockEffect();
};

class NightEyeEffect : public ValueModifierEffect
{
public:
	NightEyeEffect();
	~NightEyeEffect();
};

class OpenEffect : public ActiveEffect
{
public:
	OpenEffect();
	~OpenEffect();
};

class ParalysisEffect : public ValueModifierEffect
{
public:
	ParalysisEffect();
	~ParalysisEffect();
};

class ReanimateEffect : public ActiveEffect
{
public:
	ReanimateEffect();
	~ReanimateEffect();
};

class ScriptEffect : public ActiveEffect
{
public:
	ScriptEffect();
	~ScriptEffect();

	virtual void Unk_14();
	virtual void Unk_15();
	virtual void RunScriptAndDestroyEventList();
	
	Script* script;
	ScriptEventList* eventList;
};

class ShieldEffect : public ValueModifierEffect
{
public:
	ShieldEffect();
	~ShieldEffect();

	UInt32	unk48;		// 48
};

class SummonCreatureEffect : public AssociatedItemEffect
{
public:
	SummonCreatureEffect();
	~SummonCreatureEffect();
};

class SunDamageEffect : public ActiveEffect
{
public:
	SunDamageEffect();
	~SunDamageEffect();
};

class TelekinesisEffect : public ValueModifierEffect
{
public:
	TelekinesisEffect();
	~TelekinesisEffect();
};

class TurnUndeadEffect : public ActiveEffect
{
public:
	TurnUndeadEffect();
	~TurnUndeadEffect();
};

class ValueAndConditionsEffect : public ValueModifierEffect
{
public:
	ValueAndConditionsEffect();
	~ValueAndConditionsEffect();
};

class VampirismEffect : public ActiveEffect
{
public:
	VampirismEffect();
	~VampirismEffect();
};
