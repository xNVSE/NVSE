#pragma once
#include "NVSELexer.h"
#include "ScriptTokens.h"

// Define tokens
extern std::unordered_map<NVSETokenType, OperatorType> tokenOpToNVSEOpType;

inline Script::VariableType GetScriptTypeFromToken(NVSEToken t) {
	switch (t.type) {
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

void CompDbg(const char *fmt, ...);
void CompInfo(const char *fmt, ...);
void CompErr(const char *fmt, ...);

bool isDefaultParse(Cmd_Parse parse);

void resolveVanillaEnum(const ParamInfo* info, const char* str, uint32_t* val, uint32_t* len);
bool doesFormMatchParamType(TESForm* form, ParamType type);