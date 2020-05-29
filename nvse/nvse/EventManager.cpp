#include <list>
#include <stdarg.h>
#include "EventManager.h"
#include "ArrayVar.h"
#include "PluginAPI.h"
#include "GameAPI.h"
#include "Utilities.h"
#include "ScriptUtils.h"
#include "SafeWrite.h"
#include "FunctionScripts.h"
#include "GameObjects.h"
#include "ThreadLocal.h"
#include "common/ICriticalSection.h"
#include "Hooks_Gameplay.h"
#include "GameOSDepend.h"
//#include "InventoryReference.h"
#include "GameData.h"
#include "GameRTTI.h"

namespace EventManager {

void __stdcall HandleEventForCallingObject(UInt32 id, TESObjectREFR* callingObj, void* arg0, void* arg1);

static ICriticalSection s_criticalSection;

//////////////////////
// Event definitions
/////////////////////

// Hook routines need to be forward declared so they can be used in EventInfo structs.
// ###TODO: Would be nice to move hooks out into a separate file
static void  InstallHook();
static void  InstallActivateHook();
//static void  InstallOnVampireFeedHook();
//static void  InstallOnSkillUpHook();
//static void  InstallModPCSHook();
//static void  InstallMapMarkerHook();
//static void  InstallOnSpellCastHook();
//static void  InstallOnFallImpactHook();
//static void  InstallOnDrinkPotionHook();
//static void  InstallOnEatIngredientHook();
static void	 InstallOnActorEquipHook();
//static void  InstallOnHealthDamageHook();
//static void  InstallOnMeleeAttackHook();
//static void  InstallOnMeleeReleaseHook();
//static void  InstallOnBowAttackHook();
//static void  InstallOnBowReleaseHook();
//static void  InstallOnBlockHook();
//static void  InstallOnRecoilHook();
//static void  InstallOnStaggerHook();
//static void  InstallOnDodgeHook();
//static void  InstallOnSoulTrapHook();
//static void	 InstallIniHook();
//static void  InstallOnRaceSelectionCompleteHook();
//static void  InstallOnQuestCompleteHook();
//static void  InstallOnMagicCastHook();
//static void  InstallOnMagicApplyHook();
//static void  InstallSwimmingBreathHook();

enum {
	kEventMask_OnActivate		= 0xFFFFFFFF,		// special case as OnActivate has no event mask
};

typedef void (* EventHookInstaller)();

typedef std::list<EventCallback>	CallbackList;

struct EventInfo
{
	EventInfo (std::string const& name_, UInt8* params_, UInt8 nParams_, bool defer_, EventHookInstaller* installer_)
		: name(name_), paramTypes(params_), numParams(nParams_), isDeferred(defer_), callbacks(NULL), installHook(installer_)
		{ MakeLower (name); }
	EventInfo (std::string const& name_, UInt8 * params_, UInt8 numParams_) : name(name_), paramTypes(params_), numParams(numParams_), isDeferred(false), callbacks(NULL), installHook(NULL)
		{ MakeLower (name); }
	EventInfo () : name(""), paramTypes(NULL), numParams(0), isDeferred(false), callbacks(NULL), installHook(NULL)
		{ ; }
	EventInfo (const EventInfo& other) :
		name(other.name), 
		paramTypes(other.paramTypes),
		numParams(other.numParams), 
		callbacks(other.callbacks ? new CallbackList(*(other.callbacks)) : NULL),
		isDeferred(other.isDeferred), 
		installHook(other.installHook)
		{ 
		}
	~EventInfo();
	EventInfo& operator=(const EventInfo& other) {
		name = other.name;
		paramTypes = other.paramTypes;
		numParams = other.numParams;
		callbacks = other.callbacks ? new CallbackList(*(other.callbacks)) : NULL;
		isDeferred = other.isDeferred;
		installHook = other.installHook;
		return *this;
	};

	std::string			name;			// must be lowercase
	UInt8				* paramTypes;
	UInt8				numParams;
	bool				isDeferred;		// dispatch event in Tick() instead of immediately - currently unused
	CallbackList		* callbacks;
	EventHookInstaller	* installHook;	// if a hook is needed for this event type, this will be non-null. 
										// install it once and then set *installHook to NULL. Allows multiple events
										// to use the same hook, installing it only once.
	
};

// hook installers
static EventHookInstaller s_MainEventHook = InstallHook;
static EventHookInstaller s_ActivateHook = InstallActivateHook;
//static EventHookInstaller s_VampireHook = InstallOnVampireFeedHook;
//static EventHookInstaller s_SkillUpHook = InstallOnSkillUpHook;
//static EventHookInstaller s_ModPCSHook = InstallModPCSHook;
//static EventHookInstaller s_MapMarkerHook = InstallMapMarkerHook;
//static EventHookInstaller s_SpellScrollHook = InstallOnSpellCastHook;
//static EventHookInstaller s_FallImpactHook = InstallOnFallImpactHook;
//static EventHookInstaller s_DrinkHook = InstallOnDrinkPotionHook;
//static EventHookInstaller s_EatIngredHook = InstallOnEatIngredientHook;
static EventHookInstaller s_ActorEquipHook = InstallOnActorEquipHook;
//static EventHookInstaller s_HealthDamageHook = InstallOnHealthDamageHook;
//static EventHookInstaller s_MeleeAttackHook = InstallOnMeleeAttackHook;
//static EventHookInstaller s_MeleeReleaseHook = InstallOnMeleeReleaseHook;
//static EventHookInstaller s_BowAttackHook = InstallOnBowAttackHook;
//static EventHookInstaller s_BowReleaseHook = InstallOnBowReleaseHook;
//static EventHookInstaller s_BlockHook = InstallOnBlockHook;
//static EventHookInstaller s_RecoilHook = InstallOnRecoilHook;
//static EventHookInstaller s_StaggerHook = InstallOnStaggerHook;
//static EventHookInstaller s_DodgeHook = InstallOnDodgeHook;
//static EventHookInstaller s_SoulTrapHook = InstallOnSoulTrapHook;
//static EventHookInstaller s_IniHook = InstallIniHook;
//static EventHookInstaller s_OnRaceSelectionCompleteHook = InstallOnRaceSelectionCompleteHook;
//static EventHookInstaller s_OnQuestCompleteHook = InstallOnQuestCompleteHook;
//static EventHookInstaller s_OnMagicCastHook = InstallOnMagicCastHook;
//static EventHookInstaller s_OnMagicApplyHook = InstallOnMagicApplyHook;
//static EventHookInstaller s_OnWaterDiveSurfaceHook = InstallSwimmingBreathHook;


// event handler param lists
static UInt8 kEventParams_GameEvent[2] =
{
	Script::eVarType_Ref, Script::eVarType_Ref
};

static UInt8 kEventParams_OneRef[1] =
{
	Script::eVarType_Ref,
};

//static UInt8 kEventParams_GameMGEFEvent[2] =
//{
//	// MGEF gets converted to effect code when passed to scripts
//	Script::eVarType_Ref, Script::eVarType_Integer
//};

static UInt8 kEventParams_OneString[1] =
{
	Script::eVarType_String
};

static UInt8 kEventParams_OneInteger[1] =
{
	Script::eVarType_Integer
};

static UInt8 kEventParams_TwoIntegers[2] =
{
	Script::eVarType_Integer, Script::eVarType_Integer
};

static UInt8 kEventParams_OneFloat_OneRef[2] =
{
	 Script::eVarType_Float, Script::eVarType_Ref
};

static UInt8 kEventParams_OneRef_OneInt[2] =
{
	Script::eVarType_Ref, Script::eVarType_Integer
};

static UInt8 kEventParams_OneArray[1] =
{
	Script::eVarType_Array
};

///////////////////////////
// internal functions
//////////////////////////

void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1);
void __stdcall HandleGameEvent(UInt32 eventMask, TESObjectREFR* source, TESForm* object);

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kVtbl_PlayerCharacter = 0x0108AA3C;
static const UInt32 kVtbl_Character = 0x01086A6C;
static const UInt32 kVtbl_Creature = 0x010870AC;
static const UInt32 kVtbl_ArrowProjectile = 0x01085954;
static const UInt32 kVtbl_MagicBallProjectile = 0x0107A554;
static const UInt32 kVtbl_MagicBoltProjectile = 0x0107A8F4;
static const UInt32 kVtbl_MagicFogProjectile = 0x0107AD84;
static const UInt32 kVtbl_MagicSprayProjectile = 0x0107B8C4;
static const UInt32 kVtbl_TESObjectREFR = 0x0102F55C;

static const UInt32 kMarkEvent_HookAddr = 0x005AC750;
static const UInt32 kMarkEvent_RetnAddr = 0x005AC756;

static const UInt32 kActivate_HookAddr = 0x00573183;
static const UInt32 kActivate_RetnAddr = 0x00573194;

//static UInt32 s_PlayerCharacter_SetVampireHasFed_OriginalFn = 0x0066B120;

static const UInt32 kDestroyCIOS_HookAddr = 0x008232B5;
static const UInt32 kDestroyCIOS_RetnAddr = 0x008232EF;

#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
static const UInt32 kVtbl_PlayerCharacter = 0x0108AA3C;
static const UInt32 kVtbl_Character = 0x01086A6C;
static const UInt32 kVtbl_Creature = 0x010870AC;
static const UInt32 kVtbl_ArrowProjectile = 0x01085954;
static const UInt32 kVtbl_MagicBallProjectile = 0x0107A554;
static const UInt32 kVtbl_MagicBoltProjectile = 0x0107A8F4;
static const UInt32 kVtbl_MagicFogProjectile = 0x0107AD84;
static const UInt32 kVtbl_MagicSprayProjectile = 0x0107B8C4;
static const UInt32 kVtbl_TESObjectREFR = 0x0102F54C;

static const UInt32 kMarkEvent_HookAddr = 0x005AC900;
static const UInt32 kMarkEvent_RetnAddr = 0x005AC906;

static const UInt32 kActivate_HookAddr = 0x00573443;
static const UInt32 kActivate_RetnAddr = 0x00573454;

//static UInt32 s_PlayerCharacter_SetVampireHasFed_OriginalFn = 0x0066B120;

static const UInt32 kDestroyCIOS_HookAddr = 0x00823265;
static const UInt32 kDestroyCIOS_RetnAddr = 0x0082329F;

#else
#error
#endif

// cheap check to prevent duplicate events being processed in immediate succession
// (e.g. game marks OnHitWith twice per event, this way we only process it once)
static TESObjectREFR* s_lastObj = NULL;
static TESForm* s_lastTarget = NULL;
static UInt32 s_lastEvent = NULL;

// OnHitWith often gets marked twice per event. If weapon enchanted, may see:
//  OnHitWith >> OnMGEFHit >> ... >> OnHitWith. 
// Prevent OnHitWith being reported more than once by recording OnHitWith events processed
static TESObjectREFR* s_lastOnHitWithActor = NULL;
static TESForm* s_lastOnHitWithWeapon = NULL;

// And it turns out OnHit is annoyingly marked several times per frame for spells/enchanted weapons
static TESObjectREFR* s_lastOnHitVictim = NULL;
static TESForm* s_lastOnHitAttacker = NULL;

//////////////////////////////////
// Hooks
/////////////////////////////////

static __declspec(naked) void MarkEventHook(void)
{
	// volatile: ecx, edx, eax

	__asm {
		// overwritten code
		push    ebp
		mov     ebp, esp
		sub     esp, 8

		// grab args
		pushad
		mov	eax, [ebp+0x0C]			// ExtraDataList*
		test eax, eax
		jz	XDataListIsNull

	//XDataListIsNotNull:
		sub eax, 0x44				// TESObjectREFR* thisObj
		mov ecx, [ebp+0x10]			// event mask
		mov edx, [ebp+0x08]			// target

		push edx
		push eax
		push ecx
		call HandleGameEvent

	XDataListIsNull:
		popad
		jmp [kMarkEvent_RetnAddr]
	}
}		

void InstallHook()
{
	WriteRelJump(kMarkEvent_HookAddr, (UInt32)&MarkEventHook);
}

static __declspec(naked) void DestroyCIOSHook(void)
{
	__asm {
		pushad
		cmp ecx, 0
		jz doHook
		popad
		mov edx, [ecx]
		ret

doHook:
		popad
		fldz
		jmp	[kDestroyCIOS_RetnAddr]
	}
}

static void InstallDestroyCIOSHook()
{
	WriteRelCall(kDestroyCIOS_HookAddr, (UInt32)&DestroyCIOSHook);
}

static __declspec(naked) void OnActorEquipHook(void)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_callAddr = 0x004BF0E0;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	static const UInt32 s_callAddr = 0x004BFC10;
#else
#error unsupported Oblivion version
#endif

	static const UInt32 kEventMask = ScriptEventList::kEvent_OnActorEquip;

	__asm {
		pushad
		push [ebp+0x08]
		push [ebp+0x10]
		push [kEventMask]
		mov [s_InsideOnActorEquipHook], 0x0FFFFFFFF
		call HandleGameEvent
		mov [s_InsideOnActorEquipHook], 0x0
		popad

		jmp	[s_callAddr]
	}
}

static void InstallOnActorEquipHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 kOnActorEquipHookAddr = 0x004C0032;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	static const UInt32 kOnActorEquipHookAddr = 0x004C0B62;
	static const UInt32 patchAddr = 0x00489C6E;
#else
#error unsupported Oblivion version
#endif
	if (s_MainEventHook) {
		// OnActorEquip event also (unreliably) passes through main hook, so install that
		s_MainEventHook();
		// since it's installed, prevent it from being installed again
		s_MainEventHook = NULL;
	}

	// additional hook to overcome game's failure to consistently mark this event type

	// OBSE: The issue is that our Console_Print routine interacts poorly with the game's debug text (turned on with TDT console command)
	// OBSE: when called from a background thread.
	// OBSE: So if the handler associated with this event calls Print, PrintC, etc, there is a chance it will crash.
	//
	// Fix:
	// Added s_InsideOnActorEquipHook to neutralize Print during OnEquip events, with an optional s_CheckInsideOnActorEquipHook to bypass it for testing.
	WriteRelCall(kOnActorEquipHookAddr, (UInt32)&OnActorEquipHook);
}

static __declspec(naked) void TESObjectREFR_ActivateHook(void)
{
	__asm {
		pushad
		push [ebp+0x08]			// activating refr
		push ecx				// this
		mov eax, kEventMask_OnActivate
		push eax
		call HandleGameEvent
		popad

		// overwritten code
		push esi
		mov	[ebp-0x012C], ecx
		mov	byte ptr [ebp-1], 0

		jmp	[kActivate_RetnAddr]
	}
}

void InstallActivateHook()
{
	WriteRelJump(kActivate_HookAddr, (UInt32)&TESObjectREFR_ActivateHook);
}

#if 0

void __stdcall OnVampireFeedHook(bool bHasFed)
{
	ASSERT(s_PlayerCharacter_SetVampireHasFed_OriginalFn != 0);

	if (bHasFed) {
		HandleEvent(kEventID_OnVampireFeed, NULL, NULL);
	}

	ThisStdCall(s_PlayerCharacter_SetVampireHasFed_OriginalFn, *g_thePlayer, bHasFed);
}

void InstallOnVampireFeedHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 vtblEntry = 0x00A73C70;
	SafeWrite32(vtblEntry, (UInt32)OnVampireFeedHook);
#else
#error unsupported Oblivion version
#endif
}

static __declspec(naked) void OnSkillUpHook(void)
{
	// on entry: edi = TESSkill*
	// retn addr determined by zero flag (we're overwriting a jnz rel32 instruction)
	// ecx, eax volatile
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_jnzAddr = 0x00668129;		// if zero flag set
	static const UInt32 s_jzAddr = 0x006680A6;		// if zero flag not set
#else
#error unsupported Oblivion version
#endif

	__asm {
		jnz	ZeroFlagSet
		mov	ecx, [s_jzAddr]
		jmp DoHook
	ZeroFlagSet:
		mov ecx, [s_jnzAddr]
	DoHook:
		pushad
		push 0
		mov eax, [edi+0x2C]		// skill->skill actor value code
		push eax
		push kEventID_OnSkillUp
		call HandleEvent		// HandleEvent(kEventID_OnSkillUp, (void*)skill->skill, NULL)
		popad
		jmp ecx
	}
}

void InstallOnSkillUpHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 hookAddr = 0x006680A0;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(hookAddr, (UInt32)&OnSkillUpHook);
}

static __declspec(naked) void ModPCSHook(void)
{
	// on entry: esi = TESSkill*, [esp+0x21C-0x20C] = amount. amount may be zero or negative.
	// hook overwrites a jz instruction following a comparison of amount to zero
	// eax, edx volatile
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 jz_retnAddr = 0x0050D1CA;
	static const UInt32 jnz_retnAddr = 0x0050D0ED;
	static const UInt32 amtStackOffset = 0x10;
#else
#error unsupported Oblivion version
#endif

	__asm {
		mov edx, esp
		jz _JZ
		mov eax, [jnz_retnAddr]
		jmp DoHook
	_JZ:
		mov eax, [jz_retnAddr]
	DoHook:
		push eax				// retn addr
		pushad

		// grab amt, skill
		mov eax, [amtStackOffset]
		mov eax, [eax+edx]
		push eax				// amount
		mov eax, [esi+0x2C]		// TESSkill* skill->skill
		push eax
		push kEventID_OnScriptedSkillUp
		call HandleEvent		// HandleEvent(kEventID_OnScriptedSkillUp, skill->skill, amount)

		popad
		// esp now points to saved retn addr
		retn
	}
}
	
void InstallModPCSHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 hookAddr = 0x0050D0E7;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(hookAddr, (UInt32)&ModPCSHook);
}

static __declspec(naked) void OnMapMarkerAddHook(void)
{
	// on entry, we know the marker is being set as visible
	// ecx: ExtraMapMarker::Data* mapMarkerData
	// Only report event if marker was not *already* visible
	// This can be called from 3 locations in game code, 2 of which we're interested in
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_HUDMainMenuRetnAddr = 0x005A7058;		// from HUDMainMenu when player discovers a new marker
	static const UInt32 s_ShowMapRetnAddr = 0x0050AD95;			// from Cmd_ShowMap_Execute
#else
#error unsupported Oblivion  version
#endif

	__asm {
		// is marker already visible?
		test byte ptr [ecx+0x0C], 1				// flags, bit 1 is "visible"
		jnz Done

		// not visible, mark it
		or byte ptr [ecx+0x0C], 1

		// get the map marker refr based on calling code
		mov eax, [s_HUDMainMenuRetnAddr]
		cmp [esp], eax
		jnz CheckShowMapRetnAddr
		mov eax, [edi+0x4]
		jmp GotRefr

	CheckShowMapRetnAddr:
		mov eax, [s_ShowMapRetnAddr]
		cmp [esp], eax
		jnz Done			// unknown caller, so don't handle the event since we can't get the refr
		mov eax, [esp+0x0C]
		
	GotRefr:
		// we have a mapmarker refr, report the event
		pushad
		push 0
		push eax			// TESObjectREFR* marker
		push kEventID_OnMapMarkerAdd
		call HandleEvent
		popad

	Done:
		retn 0x4;
	}
}

void InstallMapMarkerHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 patchAddr = 0x0042B327;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(patchAddr, (UInt32)&OnMapMarkerAddHook);
}

static void DoSpellCastHook(MagicCaster* caster)
{
	MagicItemForm* magicItemForm = RUNTIME_CAST(caster->GetQueuedMagicItem(), MagicItem, MagicItemForm);
	TESObjectREFR* casterRef = RUNTIME_CAST(caster, MagicCaster, TESObjectREFR);
	if (magicItemForm && casterRef) {
		UInt32 eventID = RUNTIME_CAST(magicItemForm, MagicItemForm, EnchantmentItem) ? kEventID_OnScrollCast : kEventID_OnSpellCast;
		HandleEvent(eventID, casterRef, magicItemForm);
	}
}

static __declspec(naked) void OnSpellCastHook(void)
{
	// on entry, we know the spell is valid to cast
	// edi: MagicCaster
	// spell can be obtained from MagicCaster::GetQueuedMagicItem()
	static const UInt32 s_retnAddr = 0x005F3F04;

	__asm {
		pushad
		push edi
		call DoSpellCastHook
		pop edi
		popad
		jmp [s_retnAddr]
	}
}

static void InstallOnSpellCastHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	// overwriting jnz rel32 when MagicCaster->CanCast() returns true
	static const UInt32 s_patchAddr = 0x005F3E71;
#else
#error unsupported Oblivion version
#endif
	
	WriteRelJnz(s_patchAddr, (UInt32)&OnSpellCastHook);
}

static __declspec(naked) void OnFallImpactHook(void)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_retnAddr = 0x005EFD57;
#else
#error unsupported Oblivion version
#endif

	// on entry: esi=Actor*
	__asm {
		pushad
		push 0
		push esi
		push kEventID_OnFallImpact
		call HandleEvent
		popad

		// overwritten code
		and dword ptr [ecx+0x1F4], 0xFFFFFF7F
		jmp [s_retnAddr]
	}
}

static void InstallOnFallImpactHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_patchAddr = 0x005EFD4D;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(s_patchAddr, (UInt32)&OnFallImpactHook);
}

static __declspec(naked) void OnDrinkPotionHook(void)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_arg2StackOffset = 0x18;
#else
#error unsupported Oblivion version
#endif

	// hooks bool Actor::UseAlchemyItem(AlchemyItem*, UInt32, bool)
	// is called recursively for player - on second call boolean arg is true
	// boolean arg always true for non-player actor
	// returns true if successfully used potion (false e.g. if max potion count exceeded)

	// on entry:
	//	bl: retn value (bool)
	//	esi: Actor*
	//	edi: AlchemyItem*
	
	__asm {
		// make sure retn value is true
		test bl, bl
		jz Done

		// make sure arg2 is true
		mov eax, esp
		add eax, s_arg2StackOffset
		mov al, byte ptr [eax]
		test al, al
		jz Done

		// invoke the handler
		pushad
		push edi
		push esi
		push kEventID_OnDrinkPotion
		call HandleEvent
		popad

	Done:
		// overwritten code
		mov al, bl
		pop ebx
		pop edi
		pop esi
		retn 0x0C
	}
}

static void InstallOnDrinkPotionHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_hookAddr = 0x005E0968;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(s_hookAddr, (UInt32)&OnDrinkPotionHook);
}

static __declspec(naked) void OnEatIngredientHook(void)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 arg2StackOffset = 0x00000024;
#else
#error unsupported Oblivion version
#endif

	// on entry:
	//	esi: 'eater' refr Actor*
	//	edi: IngredientItem*
	//	boolean arg2 must be true as this is called recursively for player (once with arg=false, then arg=true)

	__asm {
		// check boolean arg, make sure it's true
		mov eax, esp
		add eax, arg2StackOffset
		mov al, byte ptr [eax]
		test al, al
		jz Done

		// handle event
		pushad
		push edi			// ingredient
		push esi			// actor
		push kEventID_OnEatIngredient
		call HandleEvent
		popad

	Done:
		// overwritten code
		jmp MarkBaseExtraListScriptEvent
	}
}

static void InstallOnEatIngredientHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 s_hookAddr = 0x005E4515;	// overwrite call to MarkScriptEventList(actor, baseExtraList, kEvent_Equip)
#else
#error unsupported Oblivion version
#endif

	WriteRelCall(s_hookAddr, (UInt32)&OnEatIngredientHook);
}

static __declspec(naked) void OnHealthDamageHook(void)
{
	// hooks Actor::OnHealthDamage virtual fn
	// only runs if actor is not already dead. Runs *before* damage has a chance to kill actor, so possible to prevent death
	// overwrites a virtual call to Actor::GetCurAV(health)

	// on entry:
	//	edx: virtual fn addr GetCurAV()
	//	esi: Actor* this (actor taking damage)
	//	arg0: Actor* attacker (may be null)
	//	arg1: float damage (has been modified for game difficulty if applicable)

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 argsOffset = 0x00000008;
	static const UInt32 retnAddr = 0x006034D1;
#else
#error unsupported Oblivion version
#endif

	__asm {
		mov eax, esp
		add eax, [argsOffset]
		pushad
		push [eax]					// attacker
		add eax, 4
		push [eax]					// damage
		push esi					// this
		push kEventID_OnHealthDamage
		call HandleEventForCallingObject
		popad

		// overwritten code
		push 8						// kActorVal_Health
		mov ecx, esi
		call edx

		jmp [retnAddr];
	}
}

static void InstallOnHealthDamageHook()
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 patchAddr = 0x006034CB;
#else
#error unsupported Oblivion version
#endif

	WriteRelJump(patchAddr, (UInt32)&OnHealthDamageHook);
}

// bitfield, set bit (1 << HighProcess::kAction_XXX) for actions which have event handlers registered
static UInt32 s_registeredActions = 0;

static __declspec(naked) void OnActionChangeHook(void)
{
	// overwrites call to HighProcess::SetCurrentAction(UInt16 action, BSAnimGroupSequence*)
	//	esi: Actor*
	//	eax: one of HighProcess::kAction_XXX
	//	ebp: BSAnimGroupSequence*
	//	edx: virtual fn address
	// volatile: ebp, esi (both popped after call), eax

	__asm {
		// -1 == no action
		cmp eax, 0xFFFFFFFF
		jz Done

		push ecx						// actor->process
		add ecx, 0x1F4					// process->currentAction
		mov cl, byte ptr [ecx]
		cmp cl, al						// is new action same as current action?
		jz NotInterested				// if we're interested, we've already reported it, so ignore.
		
		mov ecx, eax					// action
		mov ebp, 1
		shl ebp, cl						// bit for this action
		test [s_registeredActions], ebp	// are we interested in this action?
		jz NotInterested

		// k, we're interested, so invoke the handler
		pushad

		// this supports a linear subset of HighProcess::kAction_XXX, from kAction_Attack through kAction_Dodge
		// so we can calculate the event ID easily
		sub ecx, 2			// kAction_Attack
		add ecx, kEventID_OnMeleeAttack

		push 0
		push esi
		push ecx
		call HandleEvent
		popad

	NotInterested:
		pop ecx

	Done:
		// overwritten code
		call edx
		pop esi
		pop ebp
		retn 8
	}
}

static void InstallOnActionChangeHook(UInt32 action)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	static const UInt32 patchAddr = 0x005F01A5;
#else
#error unsupported Oblivion version
#endif

	ASSERT_STR((action >= HighProcess::kAction_Attack && action <= HighProcess::kAction_Dodge),
		"Invalid action supplied to InstallOnActionChangeHook()");

	// same hook used by multiple events, only install once
	static bool s_installed = false;
	if (!s_installed) {
		WriteRelJump(patchAddr, (UInt32)&OnActionChangeHook);
		s_installed = true;
	}

	// record our interest in this action
	s_registeredActions |= (1 << action);
}

static void InstallOnMeleeAttackHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_Attack);
}

static void InstallOnMeleeReleaseHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_AttackFollowThrough);
}

static void InstallOnBowAttackHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_AttackBow);
}

static void InstallOnBowReleaseHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_AttackBowArrowAttached);
}

static void InstallOnBlockHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_Block);
}

static void InstallOnRecoilHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_Recoil);
}

static void InstallOnStaggerHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_Stagger);
}

static void InstallOnDodgeHook()
{
	InstallOnActionChangeHook(HighProcess::kAction_Dodge);
}

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	// when player successfully traps a soul
	static const UInt32 s_soulTrapPatchAddr = 0x006A4EC8;	

	// when an existing EntryExtendData for a soulgem is populated with a newly captured soul
	static const UInt32 s_createExtraSoulPatchAddr1 = 0x00484D14;

	// when a new EntryExtendData for a soulgem is created for a newly captured soul
	static const UInt32 s_createExtraSoulPatchAddr2 = 0x00484D47;
	static const UInt32 s_createExtraSoulRetnAddr2 = 0x00484D4D;

	// void tList<T>::AddEntry (T* data), prepends new entry to list
	static const UInt32 s_BSSimpleList_AddEntry = 0x00446CB0;

	// void BaseExtraList::SetExtraSoulLevel (UInt32 soulLevel)
	static const UInt32 s_BaseExtraList_SetExtraSoulLevel = 0x0041EF30;
#else
#error unsupported Oblivion version
#endif

// temp ref (InventoryReference) created for most recently populated soul gem in player's inventory, valid only for a single frame
static TESObjectREFR* s_lastFilledSoulgem = NULL;

static void __stdcall SetLastFilledSoulgem (ExtraContainerChanges::EntryData* entryData, ExtraContainerChanges::EntryExtendData* extendData)
{
	// locate ExtraContainerChanges::Entry for this EntryData
	TESObjectREFR* owner = *g_thePlayer;
	ExtraContainerChanges::Entry* entry = ExtraContainerChanges::GetForRef (*g_thePlayer)->data->objList;
	while (entry && entry->data != entryData)
		entry = entry->next;

	// create temp InventoryReference for soulgem
	InventoryReference::Data irefData (entryData->type, entry, extendData);
	InventoryReference* iref = InventoryReference::Create (owner, irefData, false);
	s_lastFilledSoulgem = iref->GetRef ();
}
	
static __declspec(naked) void CreateExtraSoulHook1 (void)
{
	__asm {
		pushad
		push esi		// EntryExtendData
		push ebp		// EntryData
		call SetLastFilledSoulgem

		popad
		jmp [s_BaseExtraList_SetExtraSoulLevel]		// overwritten function call
	}
}

static __declspec(naked) void CreateExtraSoulHook2 (void)
{
	__asm {
		push esi
		mov esi, ecx						// List
		call [s_BSSimpleList_AddEntry]		// overwritten function call, returns EntryExtendData*
		pushad
		mov ecx, esi
		push ecx		// EntryExtendData
		push ebp		// EntryData
		call SetLastFilledSoulgem

		popad
		jmp [s_createExtraSoulRetnAddr2]
	}
}
		
static __declspec(naked) void OnSoulTrapHook(void)
{
	__asm {
		pushad
		mov eax, [s_lastFilledSoulgem]
		push eax
		push esi		// actor whose soul was captured
		push kEventID_OnSoulTrap
		call HandleEvent
		popad

		// we overwrote a call to QueueUIMessage, jump there and it'll return to hook location
		jmp QueueUIMessage
	}
}

static void InstallOnSoulTrapHook()
{
	WriteRelCall(s_soulTrapPatchAddr, (UInt32)&OnSoulTrapHook);
	WriteRelJump(s_createExtraSoulPatchAddr1, (UInt32)&CreateExtraSoulHook1);
	WriteRelJump(s_createExtraSoulPatchAddr2, (UInt32)&CreateExtraSoulHook2);
}
	
// hook overwrites IniSettingCollection::Write() virtual function
static UInt32 s_IniSettingCollection_Write = 0;
static void SaveIniHook()
{
	// Ini is saved when game exits. We cannot invoke a function script at that time, so check
	OSGlobals* globals = *g_osGlobals;
	if (NULL == globals || 0 != globals->quitGame)
		return;

	// check if ini can be written at this time; if so dispatch pre-save event
	IniSettingCollection* settings = IniSettingCollection::GetSingleton();	// aka 'this', since this is a virtual method
	if (NULL != settings->writeInProgressCollection)
		HandleEvent(kEventID_SaveIni, 0, NULL);

	// just in case I've screwed something up, let the vanilla Write() method run even if we've determined above that it shouldn't
	bool bWritten = ThisStdCall(s_IniSettingCollection_Write, settings) ? true : false;
	
	// if successful, dispatch post-save event
	if (bWritten)
		HandleEvent(kEventID_SaveIni, (void*)1, NULL);
}

static void InstallIniHook()
{
	IniSettingCollection* settings = IniSettingCollection::GetSingleton();
	UInt32* vtbl = *((UInt32**)settings);
	s_IniSettingCollection_Write = vtbl[7];
	SafeWrite32((UInt32)(vtbl+7), (UInt32)(&SaveIniHook));
}

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kChargenPatchAddr = 0x005C2B36;
static const UInt32 kChargenCallAddr  = 0x0066C580;
static const UInt32 kChargenRetnAddr  = 0x005C2B3B;
#else
#error unsupported Oblivion version
#endif

static __declspec (naked) void OnRaceSelectionCompleteHook (void)
{
	__asm {
		pushad
		push 0
		push 0
		push kEventID_OnRaceSelectionComplete
		call HandleEvent
		popad
		call [kChargenCallAddr]
		jmp  [kChargenRetnAddr]
	}
}

static void InstallOnRaceSelectionCompleteHook()
{
	WriteRelJump (kChargenPatchAddr, (UInt32)&OnRaceSelectionCompleteHook);
}

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kQuestCompletePatchAddr = 0x00529847;
static const UInt32 kQuestCompleteRetnAddr  = 0x00529851;
#else
#error unsupported Oblivion version
#endif

static __declspec (naked) void OnQuestCompleteHook (void)
{
	__asm {
		pushad
		push 0
		push ecx
		push kEventID_OnQuestComplete
		call HandleEvent
		popad
		
		or	 byte ptr [ecx + 0x3C], 2
		jmp  [kQuestCompleteRetnAddr]
	}
}

static void InstallOnQuestCompleteHook()
{
	WriteRelJump (kQuestCompletePatchAddr, (UInt32)&OnQuestCompleteHook);
}

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kMagicCasterCastMagicItemFnAddr = 0x00699190;
static const UInt32 kMagicCasterCastMagicItemCallSites[13]  = 
{
	0x005020D6, 0x0050212F, 0x00514942, 0x005E4496,
	0x00601439, 0x006033F2, 0x006174C2, 0x0062B3F3,
	0x0062B539, 0x00634FAE, 0x0064AE06, 0x0064D786,
	0x006728FC
};

static const UInt32 kMagicTargetAddEffectFnAddr = 0x006A27F0;
static const UInt32 kMagicTargetAddEffectCallSites[13]  = 
{
	0x005E560F, 0x006A2D7F
};
#else
#error unsupported Oblivion version
#endif

static bool PerformMagicCasterTargetHook(UInt32 eventID, MagicCaster* caster, MagicItem* magicItem, MagicTarget* target, UInt32 noHitVFX, ActiveEffect* av)
{
	bool result = false;
	
	if (eventID == kEventID_OnMagicCast) {
		result = ThisStdCall(kMagicCasterCastMagicItemFnAddr, caster, magicItem, target, noHitVFX);
	} else {
		result = ThisStdCall(kMagicTargetAddEffectFnAddr, target, caster, magicItem, av);
	}

	if (result) {
		TESObjectREFR* casterRef = OBLIVION_CAST(caster, MagicCaster, TESObjectREFR);
		TESObjectREFR* targetRef = OBLIVION_CAST(target, MagicTarget, TESObjectREFR);
		TESForm* magicItemForm = OBLIVION_CAST(magicItem, MagicItem, TESForm);

		if (casterRef == NULL && caster)
			casterRef = caster->GetParentRefr();

		if (targetRef == NULL && target)
			targetRef = target->GetParent();

		if (magicItemForm) {
			if (eventID == kEventID_OnMagicCast) {
				HandleEventForCallingObject(kEventID_OnMagicCast, casterRef, magicItemForm, targetRef);
			} else {
				HandleEventForCallingObject(kEventID_OnMagicApply, targetRef, magicItemForm, casterRef);
			}
		}
	}

	return result;
}

static bool __stdcall DoOnMagicCastHook(MagicCaster* caster, MagicItem* magicItem, MagicTarget* target, UInt32 noHitVFX)
{
	return PerformMagicCasterTargetHook(kEventID_OnMagicCast, caster, magicItem, target, noHitVFX, NULL);
}

static __declspec(naked) void OnMagicCastHook(void)
{
	__asm {
		push [esp + 0xC]
		push [esp + 0xC]
		push [esp + 0xC]
		push ecx
		xor eax, eax
		call DoOnMagicCastHook
		retn 0xC
	} 
}

static void InstallOnMagicCastHook()
{
	for (int i = 0; i < 13; i++) {
		WriteRelCall(kMagicCasterCastMagicItemCallSites[i], (UInt32)OnMagicCastHook);
	}
}

static bool __stdcall DoOnMagicApplyHook(MagicTarget* target, MagicCaster* caster, MagicItem* magicItem, ActiveEffect* av)
{
	return PerformMagicCasterTargetHook(kEventID_OnMagicApply, caster, magicItem, target, 0, av);
}

static __declspec(naked) void OnMagicApplyHook(void)
{
	__asm {
		push [esp + 0xC]
		push [esp + 0xC]
		push [esp + 0xC]
		push ecx
		xor eax, eax
		call DoOnMagicApplyHook
		retn 0xC
	} 
}

static void InstallOnMagicApplyHook()
{
	for (int i = 0; i < 2; i++) {
		WriteRelCall(kMagicTargetAddEffectCallSites[i], (UInt32)OnMagicApplyHook);
	}
}

//max swimming breath is calculated each frame based on actor's endurance
//hook the two calls to the function that does this
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kActorSwimBreath_CalcMax_CallAddr	= 0x00548960;	// original function for calculating max breath 
static const UInt32 kActorSwimBreath_CalcMax1_PatchAddr = 0x00604559;
static const UInt32 kActorSwimBreath_CalcMax1_RetnAddr	= 0x0060455E;
static const UInt32 kActorSwimBreath_CalcMax2_PatchAddr = 0x005E01C4;
static const UInt32 kActorSwimBreath_CalcMax2_RetnAddr	= 0x005E01C9;
#else
#error unsupported Oblivion version
#endif
static __declspec(naked) void Hook_ActorSwimBreath_CalcMax1()
{
	__asm
	{
		pushad
		push	ebp
		call	GetActorMaxSwimBreath
		popad
		jmp		[kActorSwimBreath_CalcMax1_RetnAddr]
	}
}
static __declspec(naked) void Hook_ActorSwimBreath_CalcMax2()
{
	__asm
	{
		pushad
		push	esi
		call	GetActorMaxSwimBreath
		popad
		jmp		[kActorSwimBreath_CalcMax2_RetnAddr]
	}
}

typedef std::map<Actor*,float> MaxBreathOverrideMapT;
MaxBreathOverrideMapT s_MaxSwimmingBreathOverrideMap;

void SetActorMaxSwimBreath(Actor* actor, float nuMax)
{
	static bool s_hooked = false;
	if (!s_hooked)
	{
		s_hooked = true;
		WriteRelJump(kActorSwimBreath_CalcMax1_PatchAddr, (UInt32)Hook_ActorSwimBreath_CalcMax1);
		WriteRelJump(kActorSwimBreath_CalcMax2_PatchAddr, (UInt32)Hook_ActorSwimBreath_CalcMax2);
	}

	MaxBreathOverrideMapT::iterator it = s_MaxSwimmingBreathOverrideMap.find(actor);
	if (nuMax > 0)
	{
		if (it != s_MaxSwimmingBreathOverrideMap.end())
		{
			it->second = nuMax;
		}
		else
		{
			s_MaxSwimmingBreathOverrideMap[ actor ] = nuMax;
		}
	}
	else
	{
		if (it != s_MaxSwimmingBreathOverrideMap.end())
		{
			s_MaxSwimmingBreathOverrideMap.erase(it);
		}
	}
}

float __stdcall GetActorMaxSwimBreath(Actor* actor)
{
	HighProcess* highProcess = (HighProcess*)actor->process;
	MaxBreathOverrideMapT::iterator it = s_MaxSwimmingBreathOverrideMap.find(actor);

	if (it != s_MaxSwimmingBreathOverrideMap.end())
	{
		return it->second;
	}
	else
	{
		typedef float (* _fn)(UInt32 Endurance);
		const _fn fn = (_fn)kActorSwimBreath_CalcMax_CallAddr;
		return fn(actor->GetActorValue(5));
	}
}


#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 kActorSwimBreath_Override_PatchAddr = 0x006045CA;
static const UInt32 kActorSwimBreath_Override_RetnCanBreathAddr = 0x006045DF; // code for actor that can breath, sets currentBreath to maxBreath
static const UInt32 kActorSwimBreath_Override_RetnNoBreathAddr	= 0x006045F9; // code for actor that cannot breath 
static const UInt32 kActorSwimBreath_Override_RetnNoTickAddr	= 0x00604635; // skips code that ticks the current breath when underwater while keeping the rest
static const UInt32 kActorSwimBreath_Override_RetnSkipAddr		= 0x00604763; // jumps to the end of the breath code
static const UInt32 kActorSwimBreath_Override_RetnSkip2Addr		= 0x00604879; // also skips breathing menu code if actor is player
#else
#error unsupported Oblivion version
#endif

enum 
{
	kActorSwimBreath_IsUnderWater	= 1,	 // we'll treat the boolean as a flag for more compact code
	// the rest are possible override states  
	kActorSwimBreath_CanBreath		= 1 << 1, // forces code to think the actor can breath, no other changes so standard behaviour of setting 'curBreath' to 'maxBreath' applies
	kActorSwimBreath_NoBreath		= 2 << 1, // forces the code to think the actor cannot breath, no other changes so standard behaviour applies
	kActorSwimBreath_NoTick			= 3 << 1, // stops the game from changing 'curBreath' each frame (when underwater) but still causes health damage when 'curBreath' is set below 0
	kActorSwimBreath_SkipBreath		= 4 << 1, // completely skips breath code (BreathMenu not included)
	kActorSwimBreath_SkipBreath2	= 5 << 1, // completely skips breath code (BreathMenu included)
};
typedef std::map<Actor*,UInt32> ActorSwimmingBreathMapT;
ActorSwimmingBreathMapT s_ActorSwimmingBreathMap;

bool SetActorSwimBreathOverride(Actor* actor, UInt32 state)
{
	if (state >= 0 && state < 4)
	{
		ActorSwimmingBreathMapT::iterator it = s_ActorSwimmingBreathMap.find(actor);
		if (it != s_ActorSwimmingBreathMap.end())
		{
			it->second = ((it->second & kActorSwimBreath_IsUnderWater) | (state << 1));
		}
		return true;
	}
	return false;
}

UInt32 __stdcall HandleActorSwimBreath(Actor* actor, HighProcess* process, bool canBreath, bool isUnderWater, float* curBreath, float* maxBreath)
{
	UInt32 retnAddr = 0;
	//ensure curBreath is initialized
	*curBreath = process->swimBreath;

	//find & update state
	ActorSwimmingBreathMapT::iterator it = s_ActorSwimmingBreathMap.find(actor);
	//only fire events when actor is already registered
	if (it != s_ActorSwimmingBreathMap.end())
	{
		if ( (it->second & kActorSwimBreath_IsUnderWater) != isUnderWater )
		{
			//Console_Print("OnWater%s for (%08X)", isUnderWater ? "Dive" : "Surface", actor->refID);
			//_MESSAGE("OnWater%s for '%s' (%08X)", isUnderWater ? "Dive" : "Surface", GetActorFullName(actor), actor->refID);
			HandleEvent(EventManager::kEventID_OnWaterSurface + isUnderWater, actor, NULL);
		}
	}
	UInt32 breathState = s_ActorSwimmingBreathMap[actor];
	breathState = (breathState & ~kActorSwimBreath_IsUnderWater) | (isUnderWater != 0);
	s_ActorSwimmingBreathMap[actor] = breathState;

	if ( actor == *g_thePlayer && IsGodMode() )
	{
		// GodMode overrides everything
		retnAddr = kActorSwimBreath_Override_RetnCanBreathAddr;
	}
	else if ( breathState > 1 )
	{
		// override is in place
		switch ( breathState & ~kActorSwimBreath_IsUnderWater )
		{
		case kActorSwimBreath_CanBreath:
			retnAddr = kActorSwimBreath_Override_RetnCanBreathAddr;
			break;
		case kActorSwimBreath_NoBreath:
			retnAddr = kActorSwimBreath_Override_RetnNoBreathAddr;
			break;
		case kActorSwimBreath_NoTick:
			retnAddr = kActorSwimBreath_Override_RetnNoTickAddr;
			break;
		case kActorSwimBreath_SkipBreath:
			retnAddr = kActorSwimBreath_Override_RetnSkipAddr;
			break;
		default:
			//_MESSAGE("Invalid swim breath override for '%s' (%08X)", GetActorFullName(actor), actor->refID);
			retnAddr = canBreath ? kActorSwimBreath_Override_RetnCanBreathAddr : kActorSwimBreath_Override_RetnNoBreathAddr;
			break;
		}
	}
	else
	{
		retnAddr = canBreath ? kActorSwimBreath_Override_RetnCanBreathAddr : kActorSwimBreath_Override_RetnNoBreathAddr;
	}

	// update stack variables in case SetActor(Max)SwimBreath was called inside any of the event handlers
	*curBreath = process->swimBreath;
	*maxBreath = GetActorMaxSwimBreath(actor);

	//ASSERT(retnAddr != NULL);

	return retnAddr;
}
static __declspec(naked) void Hook_ActorSwimBreath_Override()
{
	//TESObjectREFR::IsUnderWater(Vector3& pos, TESObjectCELL* cell, float thresholdFactor); =0x005E06C0
	static UInt32 retnAddr;
	__asm
	{
		// ebx = canBreath (bool)				determined based on being underwater or not and if the actor breaths air or water
		// esp+17h = isSwimming (bool)			from TESObjectREFR::IsUnderWater(...) w/ threshold of 70%
		// esp+15h = isUnderWater (bool)		from TESObjectREFR::IsUnderWater(...) w/ threshold of 87.5%
		// esp+28h = currentBreath (float)		not yet set at this point, used to change/update stack variable with our handler (if needed)
		// esp+24h = maxBreath (float)			already calculated at this point (possibly by our other hook), may also need to be changed/updated with our handler
		// 
		mov		al, [esp+15h]
		lea		edx, [esp+24h]
		lea		ecx, [esp+28h]
		pushad
		push	edx		// float*
		push	ecx		// float*
		push	eax		// bool
		push	ebx		// bool
		mov		eax, [ebp+58h]
		push	eax		// HighProcess*
		push	ebp		// Actor*
		call	HandleActorSwimBreath
		mov		[retnAddr], eax
		popad
		mov		eax, [retnAddr]
		jmp		eax
	}
}

void InstallSwimmingBreathHook()
{
	WriteRelJump(kActorSwimBreath_Override_PatchAddr, (UInt32)Hook_ActorSwimBreath_Override);
}

#endif

UInt32 EventIDForMask(UInt32 eventMask)
{
	switch (eventMask) {
		case ScriptEventList::kEvent_OnAdd:
			return kEventID_OnAdd;
		case ScriptEventList::kEvent_OnActorEquip:
			return kEventID_OnActorEquip;
		case ScriptEventList::kEvent_OnDrop:
			return kEventID_OnDrop;
		case ScriptEventList::kEvent_OnActorUnequip:
			return kEventID_OnActorUnequip;
		case ScriptEventList::kEvent_OnDeath:
			return kEventID_OnDeath;
		case ScriptEventList::kEvent_OnMurder:
			return kEventID_OnMurder;
		case ScriptEventList::kEvent_OnCombatEnd:
			return kEventID_OnCombatEnd;
		case ScriptEventList::kEvent_OnHit:
			return kEventID_OnHit;
		case ScriptEventList::kEvent_OnHitWith:
			return kEventID_OnHitWith;
		case ScriptEventList::kEvent_OnPackageStart:
			return kEventID_OnPackageStart;
		case ScriptEventList::kEvent_OnPackageDone:
			return kEventID_OnPackageDone;
		case ScriptEventList::kEvent_OnPackageChange:
			return kEventID_OnPackageChange;
		case ScriptEventList::kEvent_OnLoad:
			return kEventID_OnLoad;
		case ScriptEventList::kEvent_OnMagicEffectHit:
			return kEventID_OnMagicEffectHit;
		case ScriptEventList::kEvent_OnSell:
			return kEventID_OnSell;
		case ScriptEventList::kEvent_OnStartCombat:
			return kEventID_OnStartCombat;
		case ScriptEventList::kEvent_SayToDone:
			return kEventID_SayToDone;
		case ScriptEventList::kEvent_0x00400000:
			return kEventID_0x00400000;
		case ScriptEventList::kEvent_OnOpen:
			return kEventID_OnOpen;
		case ScriptEventList::kEvent_OnClose:
			return kEventID_OnClose;
		case ScriptEventList::kEvent_OnOpen2:
			return kEventID_OnOpen;
		case ScriptEventList::kEvent_OnClose2:
			return kEventID_OnClose;
		case ScriptEventList::kEvent_0x00080000:
			return kEventID_0x00080000;
		case ScriptEventList::kEvent_OnTrigger:
			return kEventID_OnTrigger;
		case ScriptEventList::kEvent_OnTriggerEnter:
			return kEventID_OnTriggerEnter;
		case ScriptEventList::kEvent_OnTriggerLeave:
			return kEventID_OnTriggerLeave;
		case ScriptEventList::kEvent_OnReset:
			return kEventID_OnReset;
		case kEventMask_OnActivate:
			return kEventID_OnActivate;
		default:
			return kEventID_INVALID;
	}
}

UInt32 EventIDForMessage(UInt32 msgID)
{
	switch (msgID) {
		case NVSEMessagingInterface::kMessage_ExitGame:
			return kEventID_ExitGame;
		case NVSEMessagingInterface::kMessage_ExitToMainMenu:
			return kEventID_ExitToMainMenu;
		case NVSEMessagingInterface::kMessage_LoadGame:
			return kEventID_LoadGame;
		case NVSEMessagingInterface::kMessage_SaveGame:
			return kEventID_SaveGame;
		case NVSEMessagingInterface::kMessage_ExitGame_Console:
			return kEventID_QQQ;
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			return kEventID_PostLoadGame;
		case NVSEMessagingInterface::kMessage_RuntimeScriptError:
			return kEventID_RuntimeScriptError;
		case NVSEMessagingInterface::kMessage_DeleteGame:
			return kEventID_DeleteGame;
		case NVSEMessagingInterface::kMessage_RenameGame:
			return kEventID_RenameGame;
		case NVSEMessagingInterface::kMessage_RenameNewGame:
			return kEventID_RenameNewGame;
		case NVSEMessagingInterface::kMessage_NewGame:
			return kEventID_NewGame;
		case NVSEMessagingInterface::kMessage_DeleteGameName:
			return kEventID_DeleteGameName;
		case NVSEMessagingInterface::kMessage_RenameGameName:
			return kEventID_RenameGameName;
		case NVSEMessagingInterface::kMessage_RenameNewGameName:
			return kEventID_RenameNewGameName;
		default:
			return kEventID_INVALID;
	}
}

typedef std::vector<EventInfo> EventInfoList;
static EventInfoList s_eventInfos;

UInt32 EventIDForString(const char* eventStr)
{
	std::string name(eventStr);
	MakeLower(name);
	eventStr = name.c_str();
	UInt32 numEventInfos = s_eventInfos.size ();
	for (UInt32 i = 0; i < numEventInfos; i++) {
		if (!strcmp(eventStr, s_eventInfos[i].name.c_str())) {
			return i;
		}
	}

	return kEventID_INVALID;
}

bool EventCallback::Equals(const EventCallback& rhs) const
{
	return (script == rhs.script &&
		object == rhs.object &&
		source == rhs.source &&
		callingObj == rhs.callingObj);
}

bool RemoveHandler(UInt32 id, EventCallback& handler);
bool RemoveHandler(UInt32 id, Script* fnScript);

class EventHandlerCaller : public InternalFunctionCaller
{
public:
	EventHandlerCaller(Script* script, EventInfo* eventInfo, void* arg0, void* arg1, TESObjectREFR* callingObj = NULL)
		: InternalFunctionCaller(script, callingObj), m_eventInfo(eventInfo)
	{
		UInt8 numArgs = 2;
		if (!arg1)
			numArgs = 1;
		if (!arg0)
			numArgs = 0;

		SetArgs(numArgs, arg0, arg1);
	}

	virtual bool ValidateParam(UserFunctionParam* param, UInt8 paramIndex)
	{
		return param->varType == m_eventInfo->paramTypes[paramIndex];
	}

	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) {
		// make sure we've got the same # of args as expected by event handler
		DynamicParamInfo& dParams = info->ParamInfo();
		if (dParams.NumParams() != m_eventInfo->numParams || dParams.NumParams() > 2) {
			ShowRuntimeError(m_script, "Number of arguments to function script does not match those expected for event");
			return false;
		}

		return InternalFunctionCaller::PopulateArgs(eventList, info);
	}

private:
	EventInfo		* m_eventInfo;
};

bool TryGetReference(TESObjectREFR* refr)
{
	// ### HACK HACK HACK
	// MarkEventList() may have been called for a BaseExtraList not associated with a TESObjectREFR
	bool bIsRefr = false;
	__try 
	{
		switch (*((UInt32*)refr)) {
			case kVtbl_PlayerCharacter:
			case kVtbl_Character:
			case kVtbl_Creature:
			case kVtbl_ArrowProjectile:
			case kVtbl_MagicBallProjectile:
			case kVtbl_MagicBoltProjectile:
			case kVtbl_MagicFogProjectile:
			case kVtbl_MagicSprayProjectile:
			case kVtbl_TESObjectREFR:
				bIsRefr = true;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
		bIsRefr = false;
	}

	return bIsRefr;
}

// stack of event names pushed when handler invoked, popped when handler returns
// used by GetCurrentEventName
std::stack<std::string> s_eventStack;

// some events are best deferred until Tick() invoked rather than being handled immediately
// this stores info about such an event. Currently unused.
struct DeferredCallback
{
	DeferredCallback(CallbackList::iterator& _iter, TESObjectREFR* _callingObj, void* _arg0, void* _arg1, EventInfo* _eventInfo)
		: iterator(_iter), callingObj(_callingObj), arg0(_arg0), arg1(_arg1), eventInfo(_eventInfo) { }

	CallbackList::iterator	iterator;
	TESObjectREFR			* callingObj;
	void					* arg0;
	void					* arg1;
	EventInfo				* eventInfo;
};

typedef std::list<DeferredCallback> DeferredCallbackList;
DeferredCallbackList s_deferredCallbacks;

void __stdcall HandleEventForCallingObject(UInt32 id, TESObjectREFR* callingObj, void* arg0, void* arg1)
{
	ScopedLock lock(s_criticalSection);

	EventInfo* eventInfo = &s_eventInfos[id];
	if (eventInfo->callbacks) {
		for (CallbackList::iterator iter = eventInfo->callbacks->begin(); iter != eventInfo->callbacks->end(); ) {
			if (iter->IsRemoved()) {
				if (!iter->IsInUse()) {
					iter = eventInfo->callbacks->erase(iter);
				}
				else {
					++iter;
				}

				continue;
			}

			// Check filters
			if (iter->source) {
				bool match = ((TESForm*)arg0 == iter->source);
				if (!match && TryGetReference((TESObjectREFR*)arg0))
				{
					TESObjectREFR* source = (TESObjectREFR*)arg0;
					if (source->baseForm == iter->source)
						match = true;
				}
				//// special-case - check the source filter against the second arg, the attacker
				//if (!match && (id == kEventID_OnHealthDamage))
				//{
				//	if ((TESForm*)arg1 == iter->source)
				//		match = true;
				//	if (!match && TryGetReference((TESObjectREFR*)arg1))
				//	{
				//		TESObjectREFR* source = (TESObjectREFR*)arg1;
				//		if (source->baseForm == iter->source)
				//			match = true;
				//	}
				//}
				if (!match)
				{
					++iter;
					continue;
				}
			}

			if (iter->callingObj && !(callingObj == iter->callingObj)) {
				++iter;
				continue;
			}

			if (iter->object) {
				//if (id == kEventID_OnMagicEffectHit) {
				//	EffectSetting* setting = DYNAMIC_CAST(iter->object, TESForm, EffectSetting);
				//	if (setting && setting->effectCode != (UInt32)arg1) {
				//		++iter;
				//		continue;
				//	}
				//}
				//else
				if (!(iter->object == (TESForm*)arg1)) {
					++iter;
					continue;
				}
			}

			if (eventInfo->isDeferred || GetCurrentThreadId() != g_mainThreadID) {
				// avoid potential issues with invoking handlers outside of main thread by deferring event handling
				if (!iter->IsRemoved()) {
					iter->SetInUse(true);
					s_deferredCallbacks.push_back(DeferredCallback(iter, callingObj, arg0, arg1, eventInfo));
					++iter;
				}
			}
			else {
				// handle immediately
				bool bWasInUse = iter->IsInUse();
				iter->SetInUse(true);
				s_eventStack.push(eventInfo->name);
				ScriptToken* result = UserFunctionManager::Call(EventHandlerCaller(iter->script, eventInfo, arg0, arg1, callingObj));
				s_eventStack.pop();
				iter->SetInUse(bWasInUse);

				// it's possible the handler decided to remove itself, so take care of that, being careful
				// not to remove a callback that is needed for deferred invocation
				if (!bWasInUse && iter->IsRemoved()) {
					iter = eventInfo->callbacks->erase(iter);
				}
				else {
					++iter;
				}

				// result is unused
				delete result;
			}
		}
	}
}

void __stdcall HandleEvent(UInt32 id, void * arg0, void * arg1)
{
	// initial implementation didn't support a calling object; pass through to impl which does
	HandleEventForCallingObject(id, NULL, arg0, arg1);
}

////////////////
// public API
///////////////

std::string GetCurrentEventName()
{
	ScopedLock lock(s_criticalSection);

	return s_eventStack.size() ? s_eventStack.top() : "";
}

static UInt32 recursiveLevel = 0;

bool SetHandler(const char* eventName, EventCallback& handler)
{
	UInt32 setted = 0;

	// trying to use a FormList to specify the source filter
	if (handler.source && handler.source->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.source;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, iter.Get(), handler.object, handler.callingObj);
			recursiveLevel++;
			if (SetHandler(eventName, listHandler)) setted++;
			recursiveLevel--;
		}
		return setted>0;
	}

	// trying to use a FormList to specify the object filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
			recursiveLevel++;
			if (SetHandler(eventName, listHandler)) setted++;
			recursiveLevel--;
		}
		return setted>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 id = EventIDForString (eventName);
	if (kEventID_INVALID == id)
	{
		// have to assume registering for a user-defined event which has not been used before this point
		id = s_eventInfos.size();
		s_eventInfos.push_back (EventInfo (eventName, kEventParams_OneArray, 1));
	}

	if (id < s_eventInfos.size()) {
		EventInfo* info = &s_eventInfos[id];
		// is hook installed for this event type?
		if (info->installHook) {
			if (*(info->installHook)) {
				// if this hook is used by more than one event type, prevent it being installed a second time
				(*info->installHook)();
				*(info->installHook) = NULL;
			}
			// mark event as having had its hook installed
			info->installHook = NULL;
		}

		if (!info->callbacks) {
			info->callbacks = new CallbackList();
		}
		else {
			// if an existing handler matches this one exactly, don't duplicate it
			for (CallbackList::iterator iter = info->callbacks->begin(); iter != info->callbacks->end(); ++iter) {
				if (iter->Equals(handler)) {
					// may be re-adding a previously removed handler, so clear the Removed flag
					iter->SetRemoved(false);
					return false;
				}
			}
		}

		info->callbacks->push_back(handler);
		return true;
	}
	else {
		return false; 
	}
}

bool RemoveHandler(const char* id, EventCallback& handler)
{
	UInt32 removed = 0;

	// trying to use a FormList to specify the filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
			recursiveLevel++;
			if (RemoveHandler(id, listHandler)) removed++;
			recursiveLevel--;
		}
		return removed>0;
	}

	// trying to use a FormList to specify the object filter
	if (handler.object && handler.object->GetTypeID()==0x055 && recursiveLevel < 100)
	{
		BGSListForm* formList = (BGSListForm*)handler.object;
		for (tList<TESForm>::Iterator iter = formList->list.Begin(); !iter.End(); ++iter) {
			EventCallback listHandler(handler.script, handler.source, iter.Get(), handler.callingObj);
			recursiveLevel++;
			if (RemoveHandler(id, listHandler)) removed++;
			recursiveLevel--;
		}
		return removed>0;
	}

	ScopedLock lock(s_criticalSection);

	UInt32 eventType = EventIDForString(id);
	bool bRemovedAtLeastOne = false;
	if (eventType < s_eventInfos.size() && s_eventInfos[eventType].callbacks) {
		CallbackList* callbacks = s_eventInfos[eventType].callbacks;
		for (CallbackList::iterator iter = callbacks->begin(); iter != callbacks->end(); ) {
			if (iter->script == handler.script) {
				bool bMatches = true;
				/* if ((eventType == kEventID_OnHealthDamage && handler.callingObj && handler.callingObj != iter->object) ||	// OnHealthDamage special-casing
					handler.object && handler.object != iter->object) {
					bMatches = false;
				}
				else */
				if (handler.source && handler.source != iter->source) {
					bMatches = false;
				}

				if (bMatches) {
					if (iter->IsInUse()) {
						// this handler is currently active, flag it for later removal
						iter->SetRemoved(true);
						++iter;
					}
					else {
						iter = callbacks->erase(iter);
					}

					bRemovedAtLeastOne = true;
				}
				else {
					++iter;
				}
			}
			else {
				++iter;
			}
		}
	}
	
	return bRemovedAtLeastOne;
}

void __stdcall HandleGameEvent(UInt32 eventMask, TESObjectREFR* source, TESForm* object)
{
	if (!TryGetReference(source)) {
		return;
	}

	ScopedLock lock(s_criticalSection);

	// ScriptEventList can be marked more than once per event, cheap check to prevent sending duplicate events to scripts
	if (source != s_lastObj || object != s_lastTarget || eventMask != s_lastEvent) {
		s_lastObj = source;
		s_lastEvent = eventMask;
		s_lastTarget = object;
	}
	else {
		// duplicate event, ignore it
		return;
	}

	UInt32 eventID = EventIDForMask(eventMask);
	if (eventID != kEventID_INVALID) {
		//// special-case OnMagicEffectHit // there isn't an effect code in Fallout
		//if (eventID == kEventID_OnMagicEffectHit) {
		//	EffectSetting* setting = DYNAMIC_CAST(object, TESForm, EffectSetting);
		//	HandleEvent(eventID, source, setting ? (void*)setting->effectCode : 0);
		//}
		//else
		if (eventID == kEventID_OnHitWith) {
			// special check for OnHitWith, since it gets called redundantly
			if (source != s_lastOnHitWithActor || object != s_lastOnHitWithWeapon) {
				s_lastOnHitWithActor = source;
				s_lastOnHitWithWeapon = object;
				HandleEvent(eventID, source, object);
			}
		}
		else if (eventID == kEventID_OnHit) {
			if (source != s_lastOnHitVictim || object != s_lastOnHitAttacker) {
				s_lastOnHitVictim = source;
				s_lastOnHitAttacker = object;
				HandleEvent(eventID, source, object);
			}
		}
		else
			HandleEvent(eventID, source, object);
	}
}

void HandleNVSEMessage(UInt32 msgID, void* data)
{
	UInt32 eventID = EventIDForMessage(msgID);
	if (eventID != kEventID_INVALID)
		HandleEvent(eventID, data, NULL);
}

bool DispatchUserDefinedEvent (const char* eventName, Script* sender, UInt32 argsArrayId, const char* senderName)
{
	ScopedLock lock(s_criticalSection);

	// does an EventInfo entry already exist for this event?
	UInt32 eventID = EventIDForString (eventName);
	if (kEventID_INVALID == eventID)
		return true;

	//{
	//	// create new entry for this event
	//	eventID = s_eventInfos.size ();
	//	s_eventInfos.push_back (EventInfo (eventName, kEventParams_OneArray, 1));
	//}

	// get or create args array
	if (argsArrayId == 0)
		argsArrayId = g_ArrayMap.Create (kDataType_String, false, sender->GetModIndex ());
	else if (!g_ArrayMap.Exists (argsArrayId) || g_ArrayMap.GetKeyType (argsArrayId) != kDataType_String)
		return false;

	// populate args array
	g_ArrayMap.SetElementString (argsArrayId, "eventName", eventName);
	if (NULL == senderName)
		if (sender)
			senderName = DataHandler::Get()->GetNthModName (sender->GetModIndex ());
		else
			senderName = "NVSE";

	g_ArrayMap.SetElementString (argsArrayId, "eventSender", senderName);

	// dispatch
	HandleEvent (eventID, (void*)argsArrayId, NULL);
	return true;
}

EventInfo::~EventInfo()
{
	if (callbacks) {
		delete callbacks;
		callbacks = NULL;
	}
}

typedef std::list< DeferredCallbackList::iterator > DeferredCallbackListIteratorList;

void Tick()
{
	ScopedLock lock(s_criticalSection);

	// handle deferred events
	if (s_deferredCallbacks.size()) {
		DeferredCallbackListIteratorList s_removedCallbacks;

		DeferredCallbackList::iterator iter = s_deferredCallbacks.begin();
		while (iter != s_deferredCallbacks.end()) {
			if (!iter->iterator->IsRemoved()) {
				s_eventStack.push(iter->eventInfo->name);
				ScriptToken* result = UserFunctionManager::Call(
					EventHandlerCaller(iter->iterator->script, iter->eventInfo, iter->arg0, iter->arg1, iter->callingObj));
				s_eventStack.pop();

				if (iter->iterator->IsRemoved()) {
					s_removedCallbacks.push_back(iter);
					++iter;
				}
				else {
					iter = s_deferredCallbacks.erase(iter);
				}

				// result is unused
				delete result;
			}
		}

		// get rid of any handlers removed during processing above
		while (s_removedCallbacks.size()) {
			(*s_removedCallbacks.begin())->eventInfo->callbacks->erase(iter->iterator);
			s_removedCallbacks.pop_front();
		}

		s_deferredCallbacks.clear();
	}


	// cleanup temporary hook data
	//for (MaxBreathOverrideMapT::iterator itr = s_MaxSwimmingBreathOverrideMap.begin(); itr != s_MaxSwimmingBreathOverrideMap.end();)
	//{
	//	if (g_actorProcessManager->highActors.IndexOf(itr->first) == -1 && itr->first != (*g_thePlayer))
	//		itr = s_MaxSwimmingBreathOverrideMap.erase(itr);						// remove the actor from the map if not in high process
	//	else
	//		itr++;
	//}

	//for (ActorSwimmingBreathMapT::iterator itr = s_ActorSwimmingBreathMap.begin(); itr != s_ActorSwimmingBreathMap.end();)
	//{
	//	if (g_actorProcessManager->highActors.IndexOf(itr->first) == -1 && itr->first != (*g_thePlayer))
	//		itr = s_ActorSwimmingBreathMap.erase(itr);
	//	else
	//		itr++;
	//}

	s_lastObj = NULL;
	s_lastTarget = NULL;
	s_lastEvent = NULL;
	s_lastOnHitWithActor = NULL;
	s_lastOnHitWithWeapon = NULL;
	s_lastOnHitVictim = NULL;
	s_lastOnHitAttacker = NULL;
}

void Init()
{
#define EVENT_INFO(name, params, hookInstaller, deferred) s_eventInfos.push_back (EventInfo (name, params, params ? sizeof(params) : 0, deferred, hookInstaller));


	EVENT_INFO("onadd", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onactorequip", kEventParams_GameEvent, &s_ActorEquipHook, false)
	EVENT_INFO("ondrop", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onactorunequip", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ondeath", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onmurder", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("oncombatend", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onhit", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onhitwith", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagechange", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagestart", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onpackagedone", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onload", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onmagiceffecthit", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onsell", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onstartcombat", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("saytodone", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("on0x0080000", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onopen", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onclose", kEventParams_GameEvent, &s_MainEventHook, false)
	// Needs investigation:
	//EVENT_INFO("onopen2", kEventParams_GameEvent, &s_MainEventHook, false)
	//EVENT_INFO("onclose2", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("on0x00400000", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontrigger", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontriggerenter", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("ontriggerleave", kEventParams_GameEvent, &s_MainEventHook, false)
	EVENT_INFO("onreset", kEventParams_GameEvent, &s_MainEventHook, false)

	EVENT_INFO("onactivate", kEventParams_GameEvent, &s_ActivateHook, false)
	EVENT_INFO("ondropitem", kEventParams_GameEvent, &s_MainEventHook, false)

	//EVENT_INFO("onvampirefeed", NULL, &s_VampireHook, false)
	//EVENT_INFO("onskillup", kEventParams_OneInteger, &s_SkillUpHook, false)
	//EVENT_INFO("onscriptedskillup", kEventParams_TwoIntegers, &s_ModPCSHook, false)
	//EVENT_INFO("onmapmarkeradd", kEventParams_OneRef, &s_MapMarkerHook, false)
	//EVENT_INFO("onspellcast", kEventParams_GameEvent, &s_SpellScrollHook, false)
	//EVENT_INFO("onscrollcast", kEventParams_GameEvent, &s_SpellScrollHook, false)
	//EVENT_INFO("onfallimpact", kEventParams_OneRef, &s_FallImpactHook, false)
	//EVENT_INFO("onactordrop", kEventParams_GameEvent, NULL, false)
	//EVENT_INFO("ondrinkpotion", kEventParams_GameEvent, &s_DrinkHook, false)
	//EVENT_INFO("oneatingredient", kEventParams_GameEvent, &s_EatIngredHook, false)
	//EVENT_INFO("onnewgame", NULL, NULL, false)
	//EVENT_INFO("onhealthdamage", kEventParams_OneFloat_OneRef, &s_HealthDamageHook, false)
	//EVENT_INFO("onsoultrap", kEventParams_GameEvent, &s_SoulTrapHook, false)
	//EVENT_INFO("onraceselectioncomplete", NULL, &s_OnRaceSelectionCompleteHook, false)

	//EVENT_INFO("onattack", kEventParams_OneRef, &s_MeleeAttackHook, false)
	//EVENT_INFO("onrelease", kEventParams_OneRef, &s_MeleeReleaseHook, false)
	//EVENT_INFO("onbowattack", kEventParams_OneRef, &s_BowAttackHook, false)
	//EVENT_INFO("onbowarrowattach", kEventParams_OneRef, &s_BowReleaseHook, false)	// undocumented, not hugely useful
	//EVENT_INFO("onblock", kEventParams_OneRef, &s_BlockHook, false)
	//EVENT_INFO("onrecoil", kEventParams_OneRef, &s_RecoilHook, false)
	//EVENT_INFO("onstagger", kEventParams_OneRef, &s_StaggerHook, false)
	//EVENT_INFO("ondodge", kEventParams_OneRef, &s_DodgeHook, false)

	//EVENT_INFO("onenchant", kEventParams_OneRef, NULL, false)
	//EVENT_INFO("oncreatespell", kEventParams_OneRef, NULL, false)
	//EVENT_INFO("oncreatepotion", kEventParams_OneRef_OneInt, NULL, false)

	//EVENT_INFO("onquestcomplete", kEventParams_OneRef, &s_OnQuestCompleteHook, false)
	//EVENT_INFO("onmagiccast", kEventParams_GameEvent, &s_OnMagicCastHook, false)
	//EVENT_INFO("onmagicapply", kEventParams_GameEvent, &s_OnMagicApplyHook, false)
	//EVENT_INFO("onwatersurface", kEventParams_OneRef, &s_OnWaterDiveSurfaceHook, false)
	//EVENT_INFO("onwaterdive", kEventParams_OneRef, &s_OnWaterDiveSurfaceHook, false)

	EVENT_INFO("exitgame", NULL, NULL, false)
	EVENT_INFO("exittomainmenu", NULL, NULL, false)
	EVENT_INFO("loadgame", kEventParams_OneString, NULL, false)
	EVENT_INFO("savegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("qqq", NULL, NULL, false)
	EVENT_INFO("postloadgame", kEventParams_OneInteger, NULL, false)
	EVENT_INFO("runtimescripterror", kEventParams_OneString, NULL, false)
	EVENT_INFO("deletegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamegame", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamenewgame", kEventParams_OneString, NULL, false)
	EVENT_INFO("newgame", NULL, NULL, false)
	EVENT_INFO("deletegamename", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamegamename", kEventParams_OneString, NULL, false)
	EVENT_INFO("renamenewgamename", kEventParams_OneString, NULL, false)

	//EVENT_INFO("saveini", kEventParams_OneInteger, &s_IniHook, false)

	ASSERT (kEventID_InternalMAX == s_eventInfos.size());

#undef EVENT_INFO

	InstallDestroyCIOSHook();	// handle a missing parameter value check.

}


};	// namespace

