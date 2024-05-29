#include "NVSECompilerUtils.h"

inline bool consoleAllocated = false;

void allocateConsole() {
#ifdef EDITOR
	if (!consoleAllocated) {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		consoleAllocated = true;
	}
#endif
}

void CompDbg(const char* fmt, ...) {
	allocateConsole();

	va_list argList;
	va_start(argList, fmt);
#if defined(EDITOR) && defined(_DEBUG)
	vprintf(fmt, argList);
#else
	char buf[1024];
	vsprintf(buf, fmt, argList);
    _DMESSAGE(buf, argList);
#endif
	va_end(argList);
}

void CompErr(const char* fmt, ...) {
	allocateConsole();

	va_list argList;
	va_start(argList, fmt);
#ifdef EDITOR
	vprintf(fmt, argList);
#else
    ShowRuntimeError(nullptr, fmt, argList);
#endif
	va_end(argList);
}

std::unordered_map<NVSETokenType, OperatorType> tokenOpToNVSEOpType{
	{NVSETokenType::EqEq, kOpType_Equals},
	{NVSETokenType::LogicOr, kOpType_LogicalOr},
	{NVSETokenType::LogicAnd, kOpType_LogicalAnd},
	{NVSETokenType::Greater, kOpType_GreaterThan},
	{NVSETokenType::GreaterEq, kOpType_GreaterOrEqual},
	{NVSETokenType::Less, kOpType_LessThan},
	{NVSETokenType::LessEq, kOpType_LessOrEqual},
	{NVSETokenType::Bang, kOpType_LogicalNot},
	{NVSETokenType::BangEq, kOpType_NotEqual},

	{NVSETokenType::Plus, kOpType_Add},
	{NVSETokenType::Minus, kOpType_Subtract},
	{NVSETokenType::Star, kOpType_Multiply},
	{NVSETokenType::Slash, kOpType_Divide},
	{NVSETokenType::Mod, kOpType_Modulo},
	{NVSETokenType::Pow, kOpType_Exponent},

	{NVSETokenType::Eq, kOpType_Assignment},
	{NVSETokenType::PlusEq, kOpType_PlusEquals},
	{NVSETokenType::MinusEq, kOpType_MinusEquals},
	{NVSETokenType::StarEq, kOpType_TimesEquals},
	{NVSETokenType::SlashEq, kOpType_DividedEquals},
	{NVSETokenType::ModEq, kOpType_ModuloEquals},
	{NVSETokenType::PowEq, kOpType_ExponentEquals},

	{NVSETokenType::MakePair, kOpType_MakePair},
	{NVSETokenType::Slice, kOpType_Slice},

	// Logical
	{NVSETokenType::BitwiseAnd, kOpType_BitwiseAnd},
	{NVSETokenType::BitwiseOr, kOpType_BitwiseOr},
	{NVSETokenType::BitwiseAndEquals, kOpType_BitwiseAndEquals},
	{NVSETokenType::BitwiseOrEquals, kOpType_BitwiseOrEquals},
	{NVSETokenType::LeftShift, kOpType_LeftShift},
	{NVSETokenType::RightShift, kOpType_RightShift},

	// Unary
	{NVSETokenType::Negate, kOpType_Negation},
	{NVSETokenType::Dollar, kOpType_ToString},
	{NVSETokenType::Pound, kOpType_ToNumber},
	{NVSETokenType::Box, kOpType_Box},
	{NVSETokenType::Unbox, kOpType_Dereference},
	{NVSETokenType::LeftBracket, kOpType_LeftBracket},
	{NVSETokenType::Dot, kOpType_Dot}
};

// Copied for testing from ScriptAnalyzer.cpp
const UInt32 g_gameParseCommands[] = { 0x5B1BA0, 0x5B3C70, 0x5B3CA0, 0x5B3C40, 0x5B3CD0, reinterpret_cast<UInt32>(Cmd_Default_Parse) };
const UInt32 g_messageBoxParseCmds[] = { 0x5B3CD0, 0x5B3C40, 0x5B3C70, 0x5B3CA0 };

bool isDefaultParse(Cmd_Parse parse) {
	return Contains(g_gameParseCommands, reinterpret_cast<UInt32>(parse)) || reinterpret_cast<UInt32>(parse) == 0x005C67E0;
}

#ifdef EDITOR
constexpr uint32_t ACTOR_VALUE_ADDRESS = 0x491300;
constexpr uint32_t SEX_0 = 0xE9AB18;
constexpr uint32_t SEX_1 = 0xE9AB1C;
constexpr uint32_t FORM_TYPE = 0xEA7078;
constexpr uint32_t MISC_STAT = 0x52E790;
#else

#endif

uint32_t resolveVanillaEnum(const ParamInfo* info, const char* str) {
#ifdef EDITOR
	uint32_t i = -1;
	switch (info->typeID) {
	case kParamType_ActorValue:
		i = CdeclCall<uint32_t>(ACTOR_VALUE_ADDRESS, str);
		return i < 77 ? i : -1;
	case kParamType_Axis:
		if (strlen(str) == 1) {
			const auto c = str[0] & 0xDF;
			if (c < 'X' || c > 'Z') {
				return -1;
			}
			return c;
		}
		return -1;
	case kParamType_AnimationGroup:
		for (i = 0; i < 245 && StrCompare(g_animGroupInfos[i].name, str); i++) {}
		return i < 245 ? i : -1;
	case kParamType_Sex:
		if (!StrCompare(*reinterpret_cast<const char**>(SEX_0), str)) {
			return 0;
		}
		if (!StrCompare(*reinterpret_cast<const char**>(SEX_1), str)) {
			return 1;
		}
		return -1;
	case kParamType_CrimeType:
		if (IsStringInteger(str) && ((i = atoi(str)) <= 4)) {
			return i;
		}
		return -1;
	case kParamType_FormType:
		for (i = 0; i < 87 && StrCompare(g_formTypeNames[i], str); i++) {}
		if (i < 87) {
			return reinterpret_cast<uint8_t*>(FORM_TYPE)[i];
		}
		return -1;
	case kParamType_MiscellaneousStat:
		i = CdeclCall<uint32_t>(MISC_STAT, str);
		return i < 43 ? i : -1;
	case kParamType_Alignment:
		for (i = 0; (i < 5) && StrCompare(g_alignmentTypeNames[i], str); i++) {}
		return i < 5 ? i : -1;
	case kParamType_EquipType:
		for (i = 0; (i < 14) && StrCompare(g_equipTypeNames[i], str); i++) {}
		return i < 14 ? i : -1;
	case kParamType_CriticalStage:
		for (i = 0; (i < 5) && StrCompare(g_criticalStageNames[i], str); i++) {}
		return i == 5 ? -1 : i;
	default:
		return i;
	}
#else
	return -1;
#endif
}

bool doesFormMatchParamType(TESForm* form, const ParamType type)
{
	if (!form)
		return false;

	switch (type)
	{
	case kParamType_ObjectID:
		if (!kInventoryType[form->typeID])
			return false;
		break;
	case kParamType_ObjectRef:
		if ((form->typeID < kFormType_TESObjectREFR) || ((form->typeID > kFormType_FlameProjectile) && NOT_ID(form, ContinuousBeamProjectile)))
			return false;
		break;
	case kParamType_Actor:
		if (NOT_ID(form, Character) && NOT_ID(form, Creature))
			return false;
		break;
	case kParamType_SpellItem:
		if (NOT_ID(form, SpellItem) && NOT_ID(form, TESObjectBOOK))
			return false;
		break;
	case kParamType_Cell:
		if NOT_ID(form, TESObjectCELL)
			return false;
		break;
	case kParamType_MagicItem:
	{
		MagicItem* magicItem = DYNAMIC_CAST(form, TESForm, MagicItem);
		if (!magicItem)
			return false;
		break;
	}
	case kParamType_Sound:
		if NOT_ID(form, TESSound)
			return false;
		break;
	case kParamType_Topic:
		if NOT_ID(form, TESTopic)
			return false;
		break;
	case kParamType_Quest:
		if NOT_ID(form, TESQuest)
			return false;
		break;
	case kParamType_Race:
		if NOT_ID(form, TESRace)
			return false;
		break;
	case kParamType_Class:
		if NOT_ID(form, TESClass)
			return false;
		break;
	case kParamType_Faction:
		if NOT_ID(form, TESFaction)
			return false;
		break;
	case kParamType_Global:
		if NOT_ID(form, TESGlobal)
			return false;
		break;
	case kParamType_Furniture:
		if (NOT_ID(form, TESFurniture) && NOT_ID(form, BGSListForm))
			return false;
		break;
	case kParamType_TESObject:
		if (!form->IsBoundObject())
			return false;
		break;
	case kParamType_MapMarker:
		if (NOT_ID(form, TESObjectREFR) || (((TESObjectREFR*)form)->baseForm->refID != 0x10))
			return false;
		break;
	case kParamType_ActorBase:
		if (NOT_ID(form, TESNPC) && NOT_ID(form, TESCreature))
			return false;
		break;
	case kParamType_Container:
		if (NOT_ID(form, Character) && NOT_ID(form, Creature) && (NOT_ID(form, TESObjectREFR) || NOT_ID(((TESObjectREFR*)form)->baseForm, TESObjectCONT)))
			return false;
		break;
	case kParamType_WorldSpace:
		if NOT_ID(form, TESWorldSpace)
			return false;
		break;
	case kParamType_AIPackage:
		if NOT_ID(form, TESPackage)
			return false;
		break;
	case kParamType_CombatStyle:
		if NOT_ID(form, TESCombatStyle)
			return false;
		break;
	case kParamType_MagicEffect:
		if NOT_ID(form, EffectSetting)
			return false;
		break;
	case kParamType_WeatherID:
		if NOT_ID(form, TESWeather)
			return false;
		break;
	case kParamType_NPC:
		if NOT_ID(form, TESNPC)
			return false;
		break;
	case kParamType_Owner:
		if (NOT_ID(form, TESFaction) && NOT_ID(form, TESNPC))
			return false;
		break;
	case kParamType_EffectShader:
		if NOT_ID(form, TESEffectShader)
			return false;
		break;
	case kParamType_FormList:
		if NOT_ID(form, BGSListForm)
			return false;
		break;
	case kParamType_MenuIcon:
		if NOT_ID(form, BGSMenuIcon)
			return false;
		break;
	case kParamType_Perk:
		if NOT_ID(form, BGSPerk)
			return false;
		break;
	case kParamType_Note:
		if NOT_ID(form, BGSNote)
			return false;
		break;
	case kParamType_ImageSpaceModifier:
		if NOT_ID(form, TESImageSpaceModifier)
			return false;
		break;
	case kParamType_ImageSpace:
		if NOT_ID(form, TESImageSpace)
			return false;
		break;
	case kParamType_EncounterZone:
		if NOT_ID(form, BGSEncounterZone)
			return false;
		break;
	case kParamType_IdleForm:
		if NOT_ID(form, TESIdleForm)
			return false;
		break;
	case kParamType_Message:
		if NOT_ID(form, BGSMessage)
			return false;
		break;
	case kParamType_InvObjOrFormList:
		if (NOT_ID(form, BGSListForm) && !kInventoryType[form->typeID])
			return false;
		break;
	case kParamType_NonFormList:
		if (IS_ID(form, BGSListForm) || !form->IsBoundObject())
			return false;
		break;
	case kParamType_SoundFile:
		if NOT_ID(form, BGSMusicType)
			return false;
		break;
	case kParamType_LeveledOrBaseChar:
		if (NOT_ID(form, TESNPC) && NOT_ID(form, TESLevCharacter))
			return false;
		break;
	case kParamType_LeveledOrBaseCreature:
		if (NOT_ID(form, TESCreature) && NOT_ID(form, TESLevCreature))
			return false;
		break;
	case kParamType_LeveledChar:
		if NOT_ID(form, TESLevCharacter)
			return false;
		break;
	case kParamType_LeveledCreature:
		if NOT_ID(form, TESLevCreature)
			return false;
		break;
	case kParamType_LeveledItem:
		if NOT_ID(form, TESLevItem)
			return false;
		break;
	case kParamType_Reputation:
		if NOT_ID(form, TESReputation)
			return false;
		break;
	case kParamType_Casino:
		if NOT_ID(form, TESCasino)
			return false;
		break;
	case kParamType_CasinoChip:
		if NOT_ID(form, TESCasinoChips)
			return false;
		break;
	case kParamType_Challenge:
		if NOT_ID(form, TESChallenge)
			return false;
		break;
	case kParamType_CaravanMoney:
		if NOT_ID(form, TESCaravanMoney)
			return false;
		break;
	case kParamType_CaravanCard:
		if NOT_ID(form, TESCaravanCard)
			return false;
		break;
	case kParamType_CaravanDeck:
		if NOT_ID(form, TESCaravanDeck)
			return false;
		break;
	case kParamType_Region:
		if NOT_ID(form, TESRegion)
			return false;
		break;
	}

	return true;
}
