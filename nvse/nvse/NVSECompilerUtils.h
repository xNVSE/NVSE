#pragma once
#include <format>

#include "NVSELexer.h"
#include "ScriptTokens.h"
#include "ScriptUtils.h"

// Define tokens
extern std::unordered_map<NVSETokenType, OperatorType> tokenOpToNVSEOpType;

inline Script::VariableType GetScriptTypeFromTokenType(NVSETokenType t) {
	switch (t) {
	case NVSETokenType::DoubleType:
		return Script::eVarType_Float;
	case NVSETokenType::IntType:
		return Script::eVarType_Integer;
	case NVSETokenType::RefType:
		return Script::eVarType_Ref;
	case NVSETokenType::ArrayType:
		return Script::eVarType_Array;
	case NVSETokenType::StringType:
		return Script::eVarType_String;
	default:
		return Script::eVarType_Invalid;
	}
}

inline Script::VariableType GetScriptTypeFromToken(NVSEToken t) {
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

inline Token_Type GetDetailedTypeFromNVSEToken(NVSETokenType type) {
	switch (type) {
	case NVSETokenType::StringType:
		return kTokenType_StringVar;
	case NVSETokenType::ArrayType:
		return kTokenType_ArrayVar;
	case NVSETokenType::RefType:
		return kTokenType_RefVar;
		// Short
	case NVSETokenType::DoubleType:
	case NVSETokenType::IntType:
		return kTokenType_NumericVar;
	default:
		return kTokenType_Ambiguous;
	}
}

inline Token_Type GetDetailedTypeFromVarType(Script::VariableType type) {
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

inline Token_Type GetVariableTypeFromNonVarType(Token_Type type) {
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

inline NVSEParamType GetParamTypeFromBasicTokenType(Token_Type tt) {
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
	case kTokenType_String:
		return "String";
	}

	return "<Not implemented>";
}

void CompDbg(const char *fmt, ...);
void CompInfo(const char *fmt, ...);
void CompErr(const char *fmt, ...);

bool isDefaultParse(Cmd_Parse parse);

void resolveVanillaEnum(const ParamInfo* info, const char* str, uint32_t* val, uint32_t* len);
bool doesFormMatchParamType(TESForm* form, ParamType type);