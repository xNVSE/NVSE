#pragma once
#include <format>

#include "Lexer/Lexer.h"
#include "../ScriptTokens.h"
#include "../ScriptUtils.h"
#include "Passes/Passes.h"

namespace Compiler {
	// Define tokens
	extern std::unordered_map<TokenType, OperatorType> tokenOpToNVSEOpType;

	inline Script::VariableType GetScriptTypeFromTokenType(TokenType t) {
		switch (t) {
		case TokenType::DoubleType:
			return Script::eVarType_Float;
		case TokenType::IntType:
			return Script::eVarType_Integer;
		case TokenType::RefType:
			return Script::eVarType_Ref;
		case TokenType::ArrayType:
			return Script::eVarType_Array;
		case TokenType::StringType:
			return Script::eVarType_String;
		default:
			return Script::eVarType_Invalid;
		}
	}

	inline Script::VariableType GetScriptTypeFromToken(Token t) {
		return GetScriptTypeFromTokenType(t.type);
	}

	inline Token_Type GetBasicTokenType(Token_Type type) {
		switch (type) {
		case kTokenType_Array:
		case kTokenType_ArrayVar:
			return kTokenType_Array;
		case kTokenType_Number:
		case kTokenType_NumericVar:
			return kTokenType_Number;
		case kTokenType_String:
		case kTokenType_StringVar:
			return kTokenType_String;
		case kTokenType_Ref:
		case kTokenType_RefVar:
			return kTokenType_Ref;
		}

		return type;
	}

	inline Token_Type NVSETokenType_To_TokenType(TokenType type) {
		switch (type) {
		case TokenType::StringType:
			return kTokenType_StringVar;
		case TokenType::ArrayType:
			return kTokenType_ArrayVar;
		case TokenType::RefType:
			return kTokenType_RefVar;
			// Short
		case TokenType::DoubleType:
		case TokenType::IntType:
			return kTokenType_NumericVar;
		default:
			return kTokenType_Ambiguous;
		}
	}

	inline Token_Type VariableType_To_TokenType(Script::VariableType type) {
		switch (type) {
		case Script::eVarType_Float:
		case Script::eVarType_Integer:
			return kTokenType_Number;
		case Script::eVarType_String:
			return kTokenType_String;
		case Script::eVarType_Array:
			return kTokenType_Array;
		case Script::eVarType_Ref:
			return kTokenType_Ref;
		case Script::eVarType_Invalid:
		default:
			return kTokenType_Invalid;
		}
	}

	inline Token_Type TokenType_To_Variable_TokenType(Token_Type type) {
		switch (type) {
		case kTokenType_Number:
			return kTokenType_NumericVar;
		case kTokenType_String:
			return kTokenType_StringVar;
		case kTokenType_Ref:
			return kTokenType_RefVar;
		case kTokenType_Array:
			return kTokenType_ArrayVar;
		default:
			return kTokenType_Invalid;
		}
	}

	inline NVSEParamType TokenType_To_ParamType(Token_Type tt) {
		switch (tt) {
		case kTokenType_Number:
			return kNVSEParamType_Number;
		case kTokenType_Form:
		case kTokenType_Ref:
			return kNVSEParamType_Form;
		case kTokenType_Array:
			return kNVSEParamType_Array;
		case kTokenType_String:
			return kNVSEParamType_String;
		}

		throw std::runtime_error(
			std::format("Type '{}' is not a basic type! Please report this as a bug.", TokenTypeToString(tt))
		);
	}

	inline bool CanUseDotOperator(Token_Type tt) {
		if (tt == kTokenType_Form) {
			return true;
		}

		if (GetBasicTokenType(tt) == kTokenType_Ref) {
			return true;
		}

		if (tt == kTokenType_Ambiguous) {
			return true;
		}

		if (tt == kTokenType_ArrayElement) {
			return true;
		}

		return false;
	}

	inline const char* GetBasicParamTypeString(NVSEParamType pt) {
		switch (pt) {
		case kNVSEParamType_Number:
			return "Number";
		case kNVSEParamType_Form:
			return "Form/Ref";
		case kNVSEParamType_Array:
			return "Array";
		case kNVSEParamType_String:
			return "String";
		default:
			return "<Not implemented>";
		}
	}

	void ResetIndent();
	void DbgIndent();
	void DbgOutdent();

	void DbgPrintln(const char* fmt, ...);
	void InfoPrintln(const char* fmt, ...);
	void ErrPrintln(const char* fmt, ...);

	void DbgPrint(const char* fmt, ...);
	void InfoPrint(const char* fmt, ...);
	void ErrPrint(const char* fmt, ...);

	void ReplaceChar(std::string& str, char charToReplace, const std::string& replacementString);;

	constexpr auto ESCAPE_RED = "\x1B[31m";
	constexpr auto ESCAPE_CYAN = "\x1B[36m";
	constexpr auto ESCAPE_UNDERLINE = "\x1B[4m";
	constexpr auto ESCAPE_RESET = "\x1B[0m";

	std::string HighlightSourceSpan(const std::vector<std::string>& lines, const std::string& msg, const SourceSpan& srcInfo, const char* colorEscape);

	bool IsDefaultParse(Cmd_Parse parse);

	void ResolveVanillaEum(const ParamInfo* info, const char* str, uint32_t* val, uint32_t* len);
	bool DoesFormTypeMatchParamType(TESForm* form, ParamType type);

	bool CompileNVSEScript(const std::string& script, Script* pScript, bool bPartialScript);
	std::optional<AST> PreProcessNVSEScript(const std::string& script, Script* pScript, bool bPartialScript);
}
