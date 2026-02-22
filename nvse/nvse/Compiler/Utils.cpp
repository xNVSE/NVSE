#include "Utils.h"
#include "Lexer/Lexer.h"

#include <io.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>

#include "Parser/Parser.h"
#include "Passes/Passes.h"
#include "Passes/TreePrinter.h"

inline bool consoleAllocated = false;

// Forward declared, not referenced in editor code but needs to be declared for if constexpr compilation
void vShowRuntimeError(Script* script, const char* fmt, va_list args);

namespace Compiler {
#ifdef _DEBUG
#define IS_DEBUG 1
#else
#define IS_DEBUG 0
#endif

#ifdef EDITOR
#define IS_EDITOR 1
#else
#define IS_EDITOR 0
#endif

	void AllocateScriptConsole() {
#ifdef IS_EDITOR
		if (!consoleAllocated && AllocConsole()) {
			AttachConsole(GetCurrentProcessId());
			const auto hConOut
				= CreateFileA("CONOUT$", GENERIC_WRITE,
							  FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

			SetConsoleMode(hConOut, ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);

			SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
			SetStdHandle(STD_ERROR_HANDLE, hConOut);
			(void)freopen("CONOUT$", "w", stdout);
			(void)freopen("CONOUT$", "w", _fdopen(1, "w"));

			consoleAllocated = true;
		}
#endif
	}

	void CompClear() {
		if (consoleAllocated) {
			system("CLS");
		}
	}

	uint32_t indent_level = 0;

	void ResetIndent() {
		indent_level = 0;
	}

	void DbgIndent() {
		if constexpr (!IS_DEBUG) {
			return;
		}

		indent_level++;
	}

	void DbgOutdent() {
		if constexpr (!IS_DEBUG) {
			return;
		}

		if (indent_level > 0) {
			indent_level--;
		}
	}

	void ConsolePrint(const char* fmt, const va_list& argList) {
		for (size_t i = 0; i < indent_level; i++) {
			printf("  ");
		}

		(void)vprintf_s(fmt, argList);
	}

	void DbgPrintln(const char* fmt, ...) {
		if constexpr (!IS_DEBUG) {
			return;
		}

		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR && IS_DEBUG) {
			ConsolePrint(fmt, argList);
			printf("\n");
		}
		else {
			v_DMESSAGE(fmt, argList);
			_DMESSAGE("\n");
		}

		va_end(argList);
	}

	void DbgPrint(const char* fmt, ...) {
		if constexpr (!IS_DEBUG) {
			return;
		}

		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR && IS_DEBUG) {
			ConsolePrint(fmt, argList);
		}
		else {
			v_DMESSAGE(fmt, argList);
		}

		va_end(argList);
	}

	void InfoPrintln(const char* fmt, ...) {
		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR) {
			ConsolePrint(fmt, argList);
			printf("\n");
		}
		else {
			v_MESSAGE(fmt, argList);
			_MESSAGE("\n");
		}

		va_end(argList);
	}

	void InfoPrint(const char* fmt, ...) {
		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR) {
			ConsolePrint(fmt, argList);
		}
		else {
			v_MESSAGE(fmt, argList);
		}

		va_end(argList);
	}

	void ErrPrintln(const char* fmt, ...) {
		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR) {
			ConsolePrint(fmt, argList);
			printf("\n");
		}
		else {
			vShowRuntimeError(nullptr, fmt, argList);
		}

		va_end(argList);
	}

	void ErrPrint(const char* fmt, ...) {
		AllocateScriptConsole();

		va_list argList;
		va_start(argList, fmt);

		if constexpr (IS_EDITOR) {
			ConsolePrint(fmt, argList);
		}
		else {
			vShowRuntimeError(nullptr, fmt, argList);
		}

		va_end(argList);
	}

	void ReplaceChar(std::string& str, const char charToReplace, const std::string& replacementString) {
		size_t pos = 0;
		while ((pos = str.find(charToReplace, pos)) != std::string::npos) {
			str.replace(pos, 1, replacementString);
			pos += replacementString.length();
		}
	}

	std::string HighlightSourceSpan(const std::vector<std::string>& lines, const std::string& msg, const SourceSpan& srcInfo, const char* colorEscape) {
		std::stringstream ss;

		// Print message
		ss << "\r    | " << msg << "\n";

		// Highlight erroneous region
		const auto startLine = srcInfo.start.line;
		const auto endLine = srcInfo.end.line;

		// Error case
		if (srcInfo.start.column == 0 || srcInfo.end.column == 0) {
			ss << "    | " << ESCAPE_RED << "BUG: Failed to print source information" << ESCAPE_RESET << "\n\n";
			return ss.str();
		}

		for (auto i = startLine; i <= endLine; i++) {
			const auto& line = std::string_view(lines[i - 1]);

			ss << std::setw(3)
				<< std::setfill(' ')
				<< i
				<< " | ";

			// First line
			if (i == startLine) {
				// Single line error
				if (startLine == endLine) {
					const auto preceding = line.substr(0, srcInfo.start.column - 1);
					const auto highlight = line.substr(srcInfo.start.column - 1, srcInfo.end.column - srcInfo.start.column + 1);
					std::string_view remaining{};
					if (srcInfo.end.column < line.length()) {
						remaining = line.substr(srcInfo.end.column);
					}

					ss  << preceding
						<< colorEscape
						<< highlight
						<< ESCAPE_RESET
						<< remaining;
				}

				// First line of a multiline block
				else {
					const auto preceding = line.substr(0, srcInfo.start.column - 1);
					const auto highlight = line.substr(srcInfo.start.column - 1);

					ss  << preceding
						<< colorEscape
						<< highlight
						<< ESCAPE_RESET;
				}
			}

			// In-between lines - Highlight the entire line
			else if (i != startLine && i != endLine) {
				ss  << colorEscape
					<< line
					<< ESCAPE_RESET;
			}

			// Final line of a multi-line span
			else if (startLine != endLine && i == endLine) {
				const auto highlight = line.substr(0, srcInfo.end.column);
				const auto remaining = line.substr(srcInfo.end.column);

				ss  << colorEscape
					<< highlight
					<< ESCAPE_RESET
					<< remaining;
			}

			ss << "\n";
		}

		auto res = ss.str();
		ReplaceChar(res, '%', "%%");
		return res;
	}

	std::unordered_map<TokenType, OperatorType> tokenOpToNVSEOpType{
		{ TokenType::EqEq, kOpType_Equals },
		{ TokenType::LogicOr, kOpType_LogicalOr },
		{ TokenType::LogicAnd, kOpType_LogicalAnd },
		{ TokenType::Greater, kOpType_GreaterThan },
		{ TokenType::GreaterEq, kOpType_GreaterOrEqual },
		{ TokenType::Less, kOpType_LessThan },
		{ TokenType::LessEq, kOpType_LessOrEqual },
		{ TokenType::Bang, kOpType_LogicalNot },
		{ TokenType::BangEq, kOpType_NotEqual },

		{ TokenType::Plus, kOpType_Add },
		{ TokenType::Minus, kOpType_Subtract },
		{ TokenType::Star, kOpType_Multiply },
		{ TokenType::Slash, kOpType_Divide },
		{ TokenType::Mod, kOpType_Modulo },
		{ TokenType::Pow, kOpType_Exponent },

		{ TokenType::Eq, kOpType_Assignment },
		{ TokenType::PlusEq, kOpType_PlusEquals },
		{ TokenType::MinusEq, kOpType_MinusEquals },
		{ TokenType::StarEq, kOpType_TimesEquals },
		{ TokenType::SlashEq, kOpType_DividedEquals },
		{ TokenType::ModEq, kOpType_ModuloEquals },
		{ TokenType::PowEq, kOpType_ExponentEquals },

		{ TokenType::MakePair, kOpType_MakePair },
		{ TokenType::Slice, kOpType_Slice },

		// Logical
		{ TokenType::BitwiseAnd, kOpType_BitwiseAnd },
		{ TokenType::BitwiseOr, kOpType_BitwiseOr },
		{ TokenType::BitwiseAndEquals, kOpType_BitwiseAndEquals },
		{ TokenType::BitwiseOrEquals, kOpType_BitwiseOrEquals },
		{ TokenType::LeftShift, kOpType_LeftShift },
		{ TokenType::RightShift, kOpType_RightShift },

		// Unary
		{ TokenType::Negate, kOpType_Negation },
		{ TokenType::Dollar, kOpType_ToString },
		{ TokenType::Pound, kOpType_ToNumber },
		{ TokenType::Box, kOpType_Box },
		{ TokenType::Unbox, kOpType_Dereference },
		{ TokenType::LeftBracket, kOpType_LeftBracket },
		{ TokenType::Dot, kOpType_Dot },
		{ TokenType::BitwiseNot, kOpType_BitwiseNot }
	};

	// Copied for testing from ScriptAnalyzer.cpp
	const UInt32 g_gameParseCommands[] = {
		0x5B1BA0, 0x5B3C70, 0x5B3CA0, 0x5B3C40, 0x5B3CD0,
		reinterpret_cast<UInt32>(Cmd_Default_Parse)
	};
	constexpr UInt32 g_messageBoxParseCmds[] = {
		0x5B3CD0, 0x5B3C40, 0x5B3C70, 0x5B3CA0
	};

	bool IsDefaultParse(Cmd_Parse parse) {
		return Contains(g_gameParseCommands, reinterpret_cast<UInt32>(parse)) ||
			reinterpret_cast<UInt32>(parse) == 0x005C67E0;
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

	void ResolveVanillaEum(
		const ParamInfo* info,
		const char* str,
		uint32_t* val,
		uint32_t* len
	) {
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
				for (i = 0; i < 245 && StrCompare(
						 g_animGroupInfos[i].name,
						 str
					 ); i++) {
				}
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
				for (i = 0; (i < 5) && StrCompare(
						 g_alignmentTypeNames[i],
						 str
					 ); i++) {
				}
				*val = i < 5 ? i : -1;
				return;
			case kParamType_EquipType:
				for (i = 0; (i < 14) && StrCompare(g_equipTypeNames[i], str); i++) {
				}
				*val = i < 14 ? i : -1;
				return;
			case kParamType_CriticalStage:
				for (i = 0; (i < 5) && StrCompare(
						 g_criticalStageNames[i],
						 str
					 ); i++) {
				}
				*val = i == 5 ? -1 : i;
		}
	}

#if EDITOR
	constexpr UInt32 p_mapMarker = 0xEDDA34;
	constexpr UInt32 g_isContainer = 0x63D740;
#else
	constexpr UInt32 p_mapMarker = 0x11CA224;
	constexpr UInt32 g_isContainer = 0x55D310;
#endif

	bool DoesFormTypeMatchParamType(TESForm* form, const ParamType type) {
		if (!form)
			return false;

		switch (type) {
			case kParamType_ObjectID:
				if (!form || !kInventoryType[form->typeID]) {
					return false;
				}
				break;
			case kParamType_ObjectRef:
				if (!form || !DYNAMIC_CAST(form, TESForm, TESObjectREFR)) {
					return false;
				}
				break;
			case kParamType_Actor:
#if EDITOR
				if (!form || !form->IsActor_InEditor()) {
					return false;
				}
#else
				if (!form || !form->IsActor_Runtime()) {
					return false;
				}
#endif
				break;
			case kParamType_MapMarker:
				if (!form || NOT_ID(form, TESObjectREFR) || (static_cast<TESObjectREFR*>(form)
					->baseForm != *(TESForm**)p_mapMarker)) {
					return false;
				}
				break;
			case kParamType_Container:
				if (!form || !DYNAMIC_CAST(form, TESForm, TESObjectREFR) || !
					ThisStdCall<TESContainer*>(g_isContainer, form)) {
					return false;
				}
				break;
			case kParamType_SpellItem:
				if (!form || (NOT_ID(form, SpellItem) &&
					NOT_ID(form, TESObjectBOOK))) {
					return false;
				}
				break;
			case kParamType_Cell:
				if (!form || NOT_ID(form, TESObjectCELL) || !(static_cast<TESObjectCELL*>(form)
					->cellFlags & 1)) {
					return false;
				}
				break;
			case kParamType_MagicItem:
				if (!form || !DYNAMIC_CAST(form, TESForm, MagicItem)) {
					return false;
				}
				break;
			case kParamType_Sound:
				if (!form || NOT_ID(form, TESSound)) {
					return false;
				}
				break;
			case kParamType_Topic:
				if (!form || NOT_ID(form, TESTopic)) {
					return false;
				}
				break;
			case kParamType_Quest:
				if (!form || NOT_ID(form, TESQuest)) {
					return false;
				}
				break;
			case kParamType_Race:
				if (!form || NOT_ID(form, TESRace)) {
					return false;
				}
				break;
			case kParamType_Class:
				if (!form || NOT_ID(form, TESClass)) {
					return false;
				}
				break;
			case kParamType_Faction:
				if (!form || NOT_ID(form, TESFaction)) {
					return false;
				}
				break;
			case kParamType_Global:
				if (!form || NOT_ID(form, TESGlobal)) {
					return false;
				}
				break;
			case kParamType_Furniture:
				if (!form || (NOT_ID(form, TESFurniture) && NOT_ID(
					form,
					BGSListForm
				))) {
					return false;
				}
				break;
			case kParamType_TESObject:
				if (!form || !DYNAMIC_CAST(form, TESForm, TESObject)) {
					return false;
				}
				break;
			case kParamType_ActorBase:
				if (!form || (NOT_ID(form, TESNPC) && NOT_ID(form, TESCreature))) {
					return false;
				}
				break;
			case kParamType_WorldSpace:
				if (!form || NOT_ID(form, TESWorldSpace)) {
					return false;
				}
				break;
			case kParamType_AIPackage:
				if (!form || NOT_ID(form, TESPackage)) {
					return false;
				}
				break;
			case kParamType_CombatStyle:
				if (!form || NOT_ID(form, TESCombatStyle)) {
					return false;
				}
				break;
			case kParamType_MagicEffect:
				if (!form || NOT_ID(form, EffectSetting)) {
					return false;
				}
				break;
			case kParamType_WeatherID:
				if (!form || NOT_ID(form, TESWeather)) {
					return false;
				}
				break;
			case kParamType_NPC:
				if (!form || NOT_ID(form, TESNPC)) {
					return false;
				}
				break;
			case kParamType_Owner:
				if (!form || (NOT_ID(form, TESFaction) && NOT_ID(form, TESNPC))) {
					return false;
				}
				break;
			case kParamType_EffectShader:
				if (!form || NOT_ID(form, TESEffectShader)) {
					return false;
				}
				break;
			case kParamType_FormList:
				if (!form || NOT_ID(form, BGSListForm)) {
					return false;
				}
				break;
			case kParamType_MenuIcon:
				if (!form || NOT_ID(form, BGSMenuIcon)) {
					return false;
				}
				break;
			case kParamType_Perk:
				if (!form || NOT_ID(form, BGSPerk)) {
					return false;
				}
				break;
			case kParamType_Note:
				if (!form || NOT_ID(form, BGSNote)) {
					return false;
				}
				break;
			case kParamType_ImageSpaceModifier:
				if (!form || NOT_ID(form, TESImageSpaceModifier)) {
					return false;
				}
				break;
			case kParamType_ImageSpace:
				if (!form || NOT_ID(form, TESImageSpace)) {
					return false;
				}
				break;
			case kParamType_EncounterZone:
				if (!form || NOT_ID(form, BGSEncounterZone)) {
					return false;
				}
				break;
			case kParamType_IdleForm:
				if (!form || NOT_ID(form, TESIdleForm)) {
					return false;
				}
				break;
			case kParamType_Message:
				if (!form || NOT_ID(form, BGSMessage)) {
					return false;
				}
				break;
			case kParamType_InvObjOrFormList:
				if (!form || (NOT_ID(form, BGSListForm) && !kInventoryType[form->
					typeID])) {
					return false;
				}
				break;
			case kParamType_NonFormList:
#if RUNTIME
				if (!form || NOT_ID(form, BGSListForm)) {
					return false;
				}
#else
				if (!form || (NOT_ID(form, BGSListForm) && !form->Unk_33())) {
					return false;
				}
#endif
				break;
			case kParamType_SoundFile:
				if (!form || NOT_ID(form, BGSMusicType)) {
					return false;
				}
				break;
			case kParamType_LeveledOrBaseChar:
				if (!form || (NOT_ID(form, TESNPC) && NOT_ID(
					form,
					TESLevCharacter
				))) {
					return false;
				}
				break;
			case kParamType_LeveledOrBaseCreature:
				if (!form || (NOT_ID(form, TESCreature) && NOT_ID(
					form,
					TESLevCreature
				))) {
					return false;
				}
				break;
			case kParamType_LeveledChar:
				if (!form || NOT_ID(form, TESLevCharacter)) {
					return false;
				}
				break;
			case kParamType_LeveledCreature:
				if (!form || NOT_ID(form, TESLevCreature)) {
					return false;
				}
				break;
			case kParamType_LeveledItem:
				if (!form || NOT_ID(form, TESLevItem)) {
					return false;
				}
				break;
			case kParamType_AnyForm:
				if (!form) {
					return false;
				}
				break;
			case kParamType_Reputation:
				if (!form || NOT_ID(form, TESReputation)) {
					return false;
				}
				break;
			case kParamType_Casino:
				if (!form || NOT_ID(form, TESCasino)) {
					return false;
				}
				break;
			case kParamType_CasinoChip:
				if (!form || NOT_ID(form, TESCasinoChips)) {
					return false;
				}
				break;
			case kParamType_Challenge:
				if (!form || NOT_ID(form, TESChallenge)) {
					return false;
				}
				break;
			case kParamType_CaravanMoney:
				if (!form || NOT_ID(form, TESCaravanMoney)) {
					return false;
				}
				break;
			case kParamType_CaravanCard:
				if (!form || NOT_ID(form, TESCaravanCard)) {
					return false;
				}
				break;
			case kParamType_CaravanDeck:
				if (!form || NOT_ID(form, TESCaravanDeck)) {
					return false;
				}
				break;
			case kParamType_Region:
				if (!form || NOT_ID(form, TESRegion)) {
					return false;
				}
				break;
		}

		return true;
	}

	template <typename T> requires std::is_base_of_v<Visitor, T>
	bool RunPass(T&& visitor, AST* ast) {
		visitor.Visit(ast);
		return !visitor.HadError();
	}

	bool CompileNVSEScript(const std::string& script, Script* pScript, bool bPartialScript) {
		ResetIndent();
		DbgPrintln("[New Compiler]");
		DbgIndent();

		Parser parser{ script };
		const auto astOpt = parser.Parse();
		if (!astOpt || parser.HadError()) {
			return false;
		}

		auto ast = *astOpt;

		if (
			!RunPass(Passes::MatchTransformer   {&ast		  }, &ast) ||
			!RunPass(Passes::VariableResolution {&ast, pScript}, &ast) ||
			!RunPass(Passes::CallTransformer    {&ast         }, &ast) || 
			!RunPass(Passes::TypeChecker        {&ast, pScript}, &ast) || 
			!RunPass(Passes::LoopTransformer    {&ast         }, &ast) || 
			!RunPass(Passes::LambdaTransformer  {&ast         }, &ast)
		) {
			return false;
		}

		TreePrinter tp{};
		ast.Accept(&tp);

		Passes::Compiler compiler{ ast, pScript, bPartialScript };
		compiler.Visit(&ast);
		if (compiler.HadError()) {
			return false;
		}

		return true;
	}

	std::optional<AST> PreProcessNVSEScript(const std::string& script, Script* pScript, bool bPartialScript) {
		Parser parser{ script };
		const auto astOpt = parser.Parse();
		if (!astOpt || parser.HadError()) {
			return std::nullopt;
		}

		auto ast = *astOpt;

		Passes::MatchTransformer transformation{ &ast };
		transformation.Visit(&ast);
		if (transformation.HadError()) {
			return std::nullopt;
		}

		Passes::VariableResolution resolver{ &ast, pScript };
		resolver.Visit(&ast);
		if (resolver.HadError()) {
			return std::nullopt;
		}

		return astOpt;
	}
}
