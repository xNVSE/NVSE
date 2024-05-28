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