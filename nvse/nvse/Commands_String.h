#include "ParamInfos.h"
#include "CommandTable.h"
#include "GameScript.h"
#include "ScriptUtils.h"

static ParamInfo kParams_sv_Destruct[10] =
{
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
	{	"string_var",	kNVSEParamType_StringVar,	1	},
};

DEFINE_COMMAND_EXP(sv_Destruct, destroys one or more string variables, 0, kParams_sv_Destruct);

DEFINE_CMD(sv_Construct, returns a formatted string, 0, kParams_FormatString);

static ParamInfo kParams_sv_Set[22] =
{
	FORMAT_STRING_PARAMS,
	{	"stringVar",	kParamType_StringVar,	1	},
};

DEFINE_CMD(sv_Set, sets the contents of a string variable, 0, kParams_sv_Set);

static ParamInfo kParams_sv_Compare[23] =
{
	FORMAT_STRING_PARAMS,
	{	"stringVar",		kParamType_StringVar,	0	},
	{	"bCaseSensitive",	kParamType_Integer,		1	},
};

DEFINE_CMD(sv_Compare, compares two strings, 0, kParams_sv_Compare);

DEFINE_CMD(sv_Length, returns the number of characters in a string, 0, kParams_OneInt);

static ParamInfo kParams_SubString[3] =
{
	{	"stringVar",	kParamType_Integer,	0	},
	{	"startPos",		kParamType_Integer,	1	},
	{	"howMany",		kParamType_Integer,	1	},
};

DEFINE_CMD(sv_Erase, erases a portion of a string variable, 0, kParams_SubString);

DEFINE_CMD(sv_SubString, returns a substring of a string variable, 0, kParams_SubString);

static ParamInfo kParams_sv_ToNumeric[2] =
{
	{	"stringVar",	kParamType_Integer,	0	},
	{	"startPos",		kParamType_Integer,	1	},
};

DEFINE_CMD(sv_ToNumeric, converts a string variable to a numeric type, 0, kParams_sv_ToNumeric);

DEFINE_CMD(sv_Insert, inserts a substring in a string variable, 0, kParams_sv_Compare);

static ParamInfo kParams_sv_Find[25] =
{
	FORMAT_STRING_PARAMS,
	{	"stringVar",	kParamType_Integer,		0	},
	{	"startPos",		kParamType_Integer,		1	},
	{	"endPos",		kParamType_Integer,		1	},
	{	"bCaseSensitive", kParamType_Integer,	1	},
};

DEFINE_CMD(sv_Count, returns the number of occurences of a substring within a string variable, 0, kParams_sv_Find);

DEFINE_CMD(sv_Find, returns the position of a substring within a string variable or -1 if not found, 0, kParams_sv_Find);

static ParamInfo kParams_sv_Replace[26] =
{
	FORMAT_STRING_PARAMS,
	{	"stringVar",	kParamType_Integer,		0	},
	{	"startPos",		kParamType_Integer,		1	},
	{	"endPos",		kParamType_Integer,		1	},
	{	"bCaseSensitive", kParamType_Integer,		1	},
	{	"howMany",		kParamType_Integer,		1	},
};

static ParamInfo kParams_sv_Trim[] =
{
	{"stringVar", kNVSEParamType_StringVar, 0}
};

DEFINE_CMD(sv_Replace, replaces 1 or more occurences of a substring within a string variable, 0, kParams_sv_Replace);

DEFINE_CMD(IsLetter, returns 1 if the character is an alphabetic character, 0, kParams_OneInt);

DEFINE_CMD(IsDigit, returns 1 if the character is a numeric character, 0, kParams_OneInt);

DEFINE_CMD(IsPrintable, returns 1 if the character is a printable character, 0, kParams_OneInt);

DEFINE_CMD(IsPunctuation, returns 1 if the character is a punctuation character, 0, kParams_OneInt);

DEFINE_CMD(IsUpperCase, returns 1 if the character is an uppercase letter, 0, kParams_OneInt);

DEFINE_CMD(sv_GetChar, returns the ASCII code of the character at the specified position in a string variable, 0, kParams_TwoInts);

DEFINE_COMMAND_EXP(sv_Trim, "Trims the beginning and the end of a string variable", 0, kParams_sv_Trim);

DEFINE_CMD(CharToAscii, converts a single character to its equivalent ASCII code, 0, kParams_OneString);

DEFINE_CMD(ToUpper, converts a character to uppercase, 0, kParams_OneInt);

DEFINE_CMD(ToLower, converts a character to lowercase, 0, kParams_OneInt);

DEFINE_CMD(GetNthModName, returns the name of the nth active mod, 0, kParams_OneInt);

DEFINE_CMD(GetName, returns the name of an object, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetStringGameSetting, returns the value of a string game setting, 0, kParams_OneString);

DEFINE_CMD(GetStringIniSetting, returns the value of a string ini setting, 0, kParams_OneString);

DEFINE_CMD(SetStringGameSettingEX_DEPRECATED, sets a string game setting, 0, kParams_FormatString);

DEFINE_CMD(SetStringIniSetting_DEPRECATED, sets a string ini setting, 0, kParams_FormatString);
DEFINE_CMD(SetStringIniSetting, sets a string ini setting, 0, kParams_TwoStrings);

DEFINE_CMD(GetModelPath, returns the model path of an object, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetIconPath, returns the icon path of an object, 0, kParams_OneOptionalForm);

DEFINE_CMD(GetBipedModelPath, returns a model path, 0, kParams_OneInt_OneOptionalForm);

DEFINE_CMD(GetBipedIconPath, returns an icon path, 0, kParams_OneInt_OneOptionalForm);

static ParamInfo kParams_SetPathEX[SIZEOF_FMT_STRING_PARAMS + 1] =
{
	FORMAT_STRING_PARAMS,
	{	"object" ,	kParamType_AnyForm,	1	},
};

DEFINE_CMD(SetModelPathEX, sets a simple model path, 0, kParams_SetPathEX);

DEFINE_CMD(SetIconPathEX, sets a simple icon path, 0, kParams_SetPathEX);

static ParamInfo kParams_SetBipedPathEX[SIZEOF_FMT_STRING_PARAMS + 2] =
{
	FORMAT_STRING_PARAMS,
	{	"whichPath",	kParamType_Integer,			0	},
	{	"item",			kParamType_AnyForm,	1	},
};

DEFINE_CMD(SetBipedIconPathEX, sets a biped icon path, 0, kParams_SetBipedPathEX);

DEFINE_CMD(SetBipedModelPathEX, sets a biped model path, 0, kParams_SetBipedPathEX);

DEFINE_CMD(GetTexturePath, "returns the texture path of an object. This command is identical to GetIconPath, but also works for other object types such as skills, classes, and miscellaneous objects.",
			   0, kParams_OneOptionalForm);

static ParamInfo kNVSEParams_OneString_OneOptionalForm[] =
{
	{	"string",	kNVSEParamType_String,	0	},
	{	"object",	kNVSEParamType_Form,	1	},
};

DEFINE_COMMAND_EXP(SetTexturePath, sets the texture path of an object. This command works for a broader set of objects than SetIconPathEX., 0, kNVSEParams_OneString_OneOptionalForm);

#if 0

static ParamInfo kParams_GetNthEffectItemScriptName[2] =
{
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};

DEFINE_CMD(GetNthEffectItemScriptName, returns the name of the nth scripted effect item, 0, kParams_GetNthEffectItemScriptName);

static ParamInfo kParams_SetNthEffectItemScriptNameEX[SIZEOF_FMT_STRING_PARAMS + 2] =
{
	FORMAT_STRING_PARAMS,
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};

DEFINE_CMD_ALT_EXP(SetNthEffectItemScriptNameEX, SetNthEIScriptNameEX, sets the name of the nth effect item script effect, 0, kParams_SetNthEffectItemScriptNameEX);

#endif

static ParamInfo kParams_GetNthRankName[3] =
{
	{	"faction",		kParamType_Faction,	0	},
	{	"rank",			kParamType_Integer,	0	},
	{	"bFemale",		kParamType_Integer,	1	},
};

static ParamInfo kParams_SetNthRankNameEX[SIZEOF_FMT_STRING_PARAMS + 3] =
{
	FORMAT_STRING_PARAMS,
	{	"faction",		kParamType_Faction,		0	},
	{	"rank",			kParamType_Integer,		0	},
	{	"bFemale",		kParamType_Integer,		1	},
};

DEFINE_CMD(GetNthFactionRankName, returns the name of the faction rank, 0, kParams_GetNthRankName);

DEFINE_CMD(SetNthFactionRankNameEX, sets the name of the nth faction rank, 0, kParams_SetNthRankNameEX);

DEFINE_CMD(GetKeyName, returns the name of a key given a scan code, 0, kParams_OneInt);
DEFINE_CMD(AsciiToChar, returns a single character string given an ASCII code, 0, kParams_OneInt);
DEFINE_CMD(GetFormIDString, returns a formID of a form as a hex string, 0, kParams_OneOptionalForm);
DEFINE_CMD(NumToHex_OLD, returns an int as a hex string of the specified width, 0, kParams_OneInt_OneOptionalInt);
DEFINE_CMD_ALT(NumToHex, IntToHex, returns an int as a hex string of the specified width, false, 3, kParams_OneInt_TwoOptionalInts);
DEFINE_CMD(IntToBin, returns an int as a binary string of the specified width, false, kParams_OneInt_TwoOptionalInts); 

static ParamInfo kNVSEParams_OneString_TwoOptionalInts[3] =
{
	{	"string",	kNVSEParamType_String,		0	},
	{	"bHex",		kNVSEParamType_Number,		1	},
	{	"bReportError",		kNVSEParamType_Number,		1	},
};

DEFINE_COMMAND_EXP(ToNumber, translates a string to a number, 0, kNVSEParams_OneString_TwoOptionalInts);

static ParamInfo kNVSEParams_TwoStrings_OneOptionalBool[3] =
{
	{	"string",		kNVSEParamType_String,	0	},
	{	"string",		kNVSEParamType_String,	0	},
	{	"bool",			kNVSEParamType_Boolean,	1	},
};

DEFINE_COMMAND_EXP(sv_Split, "split a string into substrings returning an array", 0, kNVSEParams_TwoStrings_OneOptionalBool);

DEFINE_CMD_ALT_EXP(GetFalloutDirectory, GetFalloutDir, returns the path to the Fallout directory, 0, NULL);

static ParamInfo kParams_OneNVSEString[] =
{
	{	"string",	kNVSEParamType_String,	0	},
};

DEFINE_COMMAND_EXP(sv_Percentify, converts all percent signs in a string to double percent signs, 0, kParams_OneNVSEString);

static ParamInfo kNVSEParams_OneForm[1] =
{
	{	"object",	kNVSEParamType_Form,	0	},
};

DEFINE_CMD_ALT_EXP(GetRawFormIDString, GetFormIDString2, returns the form ID stored in an array element or ref variable as a string regardless of whether or not the formID is valid, 
				   0, kNVSEParams_OneForm);

DEFINE_COMMAND_EXP(sv_ToLower, converts all characters in the string to lowercase, 0, kParams_OneNVSEString);

DEFINE_COMMAND_EXP(sv_ToUpper, converts all characters in the string to uppercase, 0, kParams_OneNVSEString);

DEFINE_CMD_ALT(ActorValueToString, AVString, returns the localized string corresponding to an actor value, 0, 1, kParams_OneActorValue);
DEFINE_CMD_ALT(ActorValueToStringC, AVStringC, returns the localized string corresponding to an actor value code, 0, 2, kParams_OneInt_OneOptionalInt);

DEFINE_COMMAND(GetScopeModelPath, "Get the path to the scope model of a weapon", 0, 1, kParams_OneOptionalObjectID)
DEFINE_COMMAND(SetScopeModelPath, "Set the path to the scope model of a weapon", 0, 2, kParams_OneString_OneOptionalObjectID)

static ParamInfo kParams_ValidateRegex[] =
{
	{"regexToValidate", kNVSEParamType_String, 0},
	{"outErrStrVar", kNVSEParamType_StringVar, 1}
};

DEFINE_COMMAND_EXP(ValidateRegex, "Returns 0 for no errors, otherwise returns int codes indicating the error cause.", 
	0, kParams_ValidateRegex);

