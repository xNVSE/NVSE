#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "GameAPI.h"
#include "ScriptUtils.h"

static const UInt32	kMaxSavedIPStack = 20;	// twice the supposed limit

struct SavedIPInfo
{
	UInt32	ip;
	UInt32	stackDepth;
	UInt32	stack[kMaxSavedIPStack];
};

extern ScriptRunner * GetScriptRunner(UInt32 * opcodeOffsetPtr);
extern SInt32 * GetCalculatedOpLength(UInt32 * opcodeOffsetPtr);

DEFINE_COMMAND(Label, set a label, 0, 1, kParams_OneOptionalInt);
DEFINE_COMMAND(Goto, branch to a label, 0, 1, kParams_OneOptionalInt);

// Not using DEFINE_COMMAND due to non standard parsers

extern CommandInfo kCommandInfo_Let;
extern CommandInfo kCommandInfo_eval;
extern CommandInfo kCommandInfo_While;
extern CommandInfo kCommandInfo_Loop;
extern CommandInfo kCommandInfo_ForEach;
extern CommandInfo kCommandInfo_ForEachAlt;
extern CommandInfo kCommandInfo_Continue;
extern CommandInfo kCommandInfo_Break;
extern CommandInfo kCommandInfo_ToString;
extern CommandInfo kCommandInfo_Print;
extern CommandInfo kCommandInfo_PrintDebug;
extern CommandInfo kCommandInfo_PrintF;
extern CommandInfo kCommandInfo_PrintDebugF;
extern CommandInfo kCommandInfo_testexpr;
extern CommandInfo kCommandInfo_TypeOf;

extern CommandInfo kCommandInfo_Function;
extern CommandInfo kCommandInfo_Call;

static ParamInfo kParams_CallFunctionCond[2] =
{
	{	"UDF FormList", kParamType_FormList,	0		},
	{	"bBreakIfFalse", kParamType_Integer,	1		},
};

DEFINE_CMD_COND(CallFunctionCond, "calls every UDF in a formlist and returns the result of the last one.", 
	false, kParams_CallFunctionCond);

extern CommandInfo kCommandInfo_SetFunctionValue;

DEFINE_COMMAND(GetUserTime, returns the users local time and date as a stringmap, 0, 0, NULL);
DEFINE_COMMAND(GetAllModLocalData, returns a StringMap containing all local data for the calling mod, 0, 0, NULL);

extern CommandInfo kCommandInfo_GetModLocalData;
extern CommandInfo kCommandInfo_SetModLocalData;
extern CommandInfo kCommandInfo_ModLocalDataExists;
extern CommandInfo kCommandInfo_RemoveModLocalData;

extern CommandInfo kCommandInfo_GetAllModLocalData;

extern CommandInfo kCommandInfo_Internal_PushExecutionContext;
extern CommandInfo kCommandInfo_Internal_PopExecutionContext;

// Up to 15 variables
static ParamInfo kParams_OneNVSEVariable_OptionalNVSEVariables[] =
{
	{"variable", kNVSEParamType_Variable, 0},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},

	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},

	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
	{"variable", kNVSEParamType_Variable, 1},
};

static ParamInfo kParams_OneBoolean[] =
{
	{	"boolean expression",	kNVSEParamType_Boolean,	0	},
};

DEFINE_CMD_ALT_EXP(PrintVar, PrintV, , false, kParams_OneNVSEVariable_OptionalNVSEVariables);
DEFINE_CMD_ALT_EXP(DebugPrintVar, DPrintV, , false, kParams_OneNVSEVariable_OptionalNVSEVariables);

DEFINE_CMD_ALT_EXP(Assert, AssertTrue, , false, kParams_OneBoolean);
DEFINE_CMD_ALT_EXP(GetSelfAlt_OLD, ThisAlt_OLD, 
	"Unlike GetSelf, will return ThisObj even if it isn't Persistent and is clutter.",
	false, nullptr);
DEFINE_CMD_ALIAS(GetSelfAlt, ThisAlt,
	"Unlike GetSelf, will return ThisObj even if it isn't Persistent and is clutter.",
	false, nullptr);


static ParamInfo kNVSEParams_PluginVersion[] =
{
	{	"plugin name",	kNVSEParamType_String,	0	},
	{	"plugin version",	kNVSEParamType_Number,	0	},
};
DEFINE_COMMAND_EXP(PluginVersion, "Internal only - declares a nvse / plugin requirement for a script.", false, kNVSEParams_PluginVersion)