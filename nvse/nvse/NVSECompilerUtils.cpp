#include "NVSECompilerUtils.h"

inline bool consoleAllocated = false;

void allocateConsole() {
#ifdef EDITOR
	if (!consoleAllocated) {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		consoleAllocated = true;

		HWND hwnd = GetConsoleWindow();
		if (hwnd != NULL)
		{
			HMENU hMenu = GetSystemMenu(hwnd, FALSE);
			if (hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
		}
	}
#endif
}

void CompClear() {
	if (consoleAllocated) {
		system("CLS");
	}
}

void CompDbg(const char* fmt, ...) {
	allocateConsole();

	va_list argList;
	va_start(argList, fmt);
#if defined(EDITOR) && defined(_DEBUG)
	vprintf(fmt, argList);
#else
	v_DMESSAGE(fmt, argList);
#endif
	va_end(argList);
}

void CompInfo(const char* fmt, ...) {
	allocateConsole();

	va_list argList;
	va_start(argList, fmt);
#if defined(EDITOR)
	vprintf(fmt, argList);
#else
	v_MESSAGE(fmt, argList);
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
	vShowRuntimeError(nullptr, fmt, argList);
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
	{NVSETokenType::Dot, kOpType_Dot},
	{NVSETokenType::BitwiseNot, kOpType_BitwiseNot}
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
constexpr uint32_t MISC_STAT = 0x52E790;
#else
constexpr uint32_t ACTOR_VALUE_ADDRESS = 0x66EB40;
constexpr uint32_t SEX_0 = 0x104A0EC;
constexpr uint32_t SEX_1 = 0x104A0F4;
constexpr uint32_t MISC_STAT = 0x4D5EB0;
#endif

void resolveVanillaEnum(const ParamInfo* info, const char* str, uint32_t* val, uint32_t* len) {
	uint32_t i = -1;
	*val = -1;
	*len = 2;
	switch (info->typeID) {
	case kParamType_ActorValue:
		i = CdeclCall<uint32_t>(ACTOR_VALUE_ADDRESS, str);
		*val = i < 77 ? i : -1;
		return;
	case kParamType_Axis:
		if (strlen(str) == 1) {
			const auto c = str[0] & 0xDF;
			if (c < 'X' || c > 'Z') {
				return;
			}
			*val = c;
			*len = 1;
		}
		return;
	case kParamType_AnimationGroup:
		for (i = 0; i < 245 && StrCompare(g_animGroupInfos[i].name, str); i++) {}
		*val = i < 245 ? i : -1;
		return;
	case kParamType_Sex:
		if (!StrCompare(*reinterpret_cast<const char**>(SEX_0), str)) {
			*val = 0;
		}
		if (!StrCompare(*reinterpret_cast<const char**>(SEX_1), str)) {
			*val = 1;
		}
		return;
	case kParamType_CrimeType:
		if (IsStringInteger(str) && ((i = atoi(str)) <= 4)) {
			*val = i;
		}
		return;
	case kParamType_FormType:
		for (auto& [formId, name] : g_formTypeNames) {
			if (!StrCompare(name, str)) {
				*val = formId;
			}
		}
		return;
	case kParamType_MiscellaneousStat:
		i = CdeclCall<uint32_t>(MISC_STAT, str);
		*val = i < 43 ? i : -1;
		return;
	case kParamType_Alignment:
		for (i = 0; (i < 5) && StrCompare(g_alignmentTypeNames[i], str); i++) {}
		*val = i < 5 ? i : -1;
		return;
	case kParamType_EquipType:
		for (i = 0; (i < 14) && StrCompare(g_equipTypeNames[i], str); i++) {}
		*val = i < 14 ? i : -1;
		return;
	case kParamType_CriticalStage:
		for (i = 0; (i < 5) && StrCompare(g_criticalStageNames[i], str); i++) {}
		*val = i == 5 ? -1 : i;
		return;
	}
}

#if EDITOR
constexpr UInt32 p_mapMarker = 0xEDDA34;
constexpr UInt32 g_isContainer = 0x63D740;
#else
constexpr UInt32 p_mapMarker = 0x11CA224;
constexpr UInt32 g_isContainer = 0x55D310;
#endif

bool doesFormMatchParamType(TESForm* form, const ParamType type)
{
	if (!form)
		return false;

	switch (type)
	{
	case kParamType_ObjectID:
		if (!form || !kInventoryType[form->typeID])
		{
			return false;
		}
		break;
	case kParamType_ObjectRef:
		if (!form || !DYNAMIC_CAST(form, TESForm, TESObjectREFR))
		{
			return false;
		}
		break;
	case kParamType_Actor:
#if EDITOR
		if (!form || !form->IsActor_InEditor())
		{
			return false;
		}
#else
		if (!form || !form->IsActor_Runtime())
		{
			return false;
		}
#endif
		break;
	case kParamType_MapMarker:
		if (!form || NOT_ID(form, TESObjectREFR) || (((TESObjectREFR*)form)->baseForm != *(TESForm**)p_mapMarker))
		{
			return false;
		}
		break;
	case kParamType_Container:
		if (!form || !DYNAMIC_CAST(form, TESForm, TESObjectREFR) || !ThisStdCall<TESContainer*>(g_isContainer, form))
		{
			return false;
		}
		break;
	case kParamType_SpellItem:
		if (!form || (NOT_ID(form, SpellItem) && NOT_ID(form, TESObjectBOOK)))
		{
			return false;
		}
		break;
	case kParamType_Cell:
		if (!form || NOT_ID(form, TESObjectCELL) || !(((TESObjectCELL*)form)->cellFlags & 1))
		{
			return false;
		}
		break;
	case kParamType_MagicItem:
		if (!form || !DYNAMIC_CAST(form, TESForm, MagicItem))
		{
			return false;
		}
		break;
	case kParamType_Sound:
		if (!form || NOT_ID(form, TESSound))
		{
			return false;
		}
		break;
	case kParamType_Topic:
		if (!form || NOT_ID(form, TESTopic))
		{
			return false;
		}
		break;
	case kParamType_Quest:
		if (!form || NOT_ID(form, TESQuest))
		{
			return false;
		}
		break;
	case kParamType_Race:
		if (!form || NOT_ID(form, TESRace))
		{
			return false;
		}
		break;
	case kParamType_Class:
		if (!form || NOT_ID(form, TESClass))
		{
			return false;
		}
		break;
	case kParamType_Faction:
		if (!form || NOT_ID(form, TESFaction))
		{
			return false;
		}
		break;
	case kParamType_Global:
		if (!form || NOT_ID(form, TESGlobal))
		{
			return false;
		}
		break;
	case kParamType_Furniture:
		if (!form || (NOT_ID(form, TESFurniture) && NOT_ID(form, BGSListForm)))
		{
			return false;
		}
		break;
	case kParamType_TESObject:
		if (!form || !DYNAMIC_CAST(form, TESForm, TESObject))
		{
			return false;
		}
		break;
	case kParamType_ActorBase:
		if (!form || (NOT_ID(form, TESNPC) && NOT_ID(form, TESCreature)))
		{
			return false;
		}
		break;
	case kParamType_WorldSpace:
		if (!form || NOT_ID(form, TESWorldSpace))
		{
			return false;
		}
		break;
	case kParamType_AIPackage:
		if (!form || NOT_ID(form, TESPackage))
		{
			return false;
		}
		break;
	case kParamType_CombatStyle:
		if (!form || NOT_ID(form, TESCombatStyle))
		{
			return false;
		}
		break;
	case kParamType_MagicEffect:
		if (!form || NOT_ID(form, EffectSetting))
		{
			return false;
		}
		break;
	case kParamType_WeatherID:
		if (!form || NOT_ID(form, TESWeather))
		{
			return false;
		}
		break;
	case kParamType_NPC:
		if (!form || NOT_ID(form, TESNPC))
		{
			return false;
		}
		break;
	case kParamType_Owner:
		if (!form || (NOT_ID(form, TESFaction) && NOT_ID(form, TESNPC)))
		{
			return false;
		}
		break;
	case kParamType_EffectShader:
		if (!form || NOT_ID(form, TESEffectShader))
		{
			return false;
		}
		break;
	case kParamType_FormList:
		if (!form || NOT_ID(form, BGSListForm))
		{
			return false;
		}
		break;
	case kParamType_MenuIcon:
		if (!form || NOT_ID(form, BGSMenuIcon))
		{
			return false;
		}
		break;
	case kParamType_Perk:
		if (!form || NOT_ID(form, BGSPerk))
		{
			return false;
		}
		break;
	case kParamType_Note:
		if (!form || NOT_ID(form, BGSNote))
		{
			return false;
		}
		break;
	case kParamType_ImageSpaceModifier:
		if (!form || NOT_ID(form, TESImageSpaceModifier))
		{
			return false;
		}
		break;
	case kParamType_ImageSpace:
		if (!form || NOT_ID(form, TESImageSpace))
		{
			return false;
		}
		break;
	case kParamType_EncounterZone:
		if (!form || NOT_ID(form, BGSEncounterZone))
		{
			return false;
		}
		break;
	case kParamType_IdleForm:
		if (!form || NOT_ID(form, TESIdleForm))
		{
			return false;
		}
		break;
	case kParamType_Message:
		if (!form || NOT_ID(form, BGSMessage))
		{
			return false;
		}
		break;
	case kParamType_InvObjOrFormList:
		if (!form || (NOT_ID(form, BGSListForm) && !kInventoryType[form->typeID]))
		{
			return false;
		}
		break;
	case kParamType_NonFormList:
#if RUNTIME
		if (!form || NOT_ID(form, BGSListForm))
		{
			return false;
		}
#else
		if (!form || (NOT_ID(form, BGSListForm) && !form->Unk_33()))
		{
			return false;
		}
#endif
		break;
	case kParamType_SoundFile:
		if (!form || NOT_ID(form, BGSMusicType))
		{
			return false;
		}
		break;
	case kParamType_LeveledOrBaseChar:
		if (!form || (NOT_ID(form, TESNPC) && NOT_ID(form, TESLevCharacter)))
		{
			return false;
		}
		break;
	case kParamType_LeveledOrBaseCreature:
		if (!form || (NOT_ID(form, TESCreature) && NOT_ID(form, TESLevCreature)))
		{
			return false;
		}
		break;
	case kParamType_LeveledChar:
		if (!form || NOT_ID(form, TESLevCharacter))
		{
			return false;
		}
		break;
	case kParamType_LeveledCreature:
		if (!form || NOT_ID(form, TESLevCreature))
		{
			return false;
		}
		break;
	case kParamType_LeveledItem:
		if (!form || NOT_ID(form, TESLevItem))
		{
			return false;
		}
		break;
	case kParamType_AnyForm:
		if (!form)
		{
			return false;
		}
		break;
	case kParamType_Reputation:
		if (!form || NOT_ID(form, TESReputation))
		{
			return false;
		}
		break;
	case kParamType_Casino:
		if (!form || NOT_ID(form, TESCasino))
		{
			return false;
		}
		break;
	case kParamType_CasinoChip:
		if (!form || NOT_ID(form, TESCasinoChips))
		{
			return false;
		}
		break;
	case kParamType_Challenge:
		if (!form || NOT_ID(form, TESChallenge))
		{
			return false;
		}
		break;
	case kParamType_CaravanMoney:
		if (!form || NOT_ID(form, TESCaravanMoney))
		{
			return false;
		}
		break;
	case kParamType_CaravanCard:
		if (!form || NOT_ID(form, TESCaravanCard))
		{
			return false;
		}
		break;
	case kParamType_CaravanDeck:
		if (!form || NOT_ID(form, TESCaravanDeck))
		{
			return false;
		}
		break;
	case kParamType_Region:
		if (!form || NOT_ID(form, TESRegion))
		{
			return false;
		}
		break;
	}

	return true;
}
