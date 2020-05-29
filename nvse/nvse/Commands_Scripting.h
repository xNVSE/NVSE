#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "GameAPI.h"

static const UInt32	kMaxSavedIPStack = 20;	// twice the supposed limit

struct SavedIPInfo
{
	UInt32	ip;
	UInt32	stackDepth;
	UInt32	stack[kMaxSavedIPStack];
};

struct ScriptRunner
{
	TESForm			*baseForm;			// 000
	UInt32			unk004;				// 004
	ScriptEventList	*eventList;			// 008
	UInt32			unk010;				// 010
	Script			*script;			// 014
	UInt32			unk018;				// 018	= 6 after failed to evaluate expression
	UInt32			unk01C;				// 01C
	UInt32			stackDepth;			// 020
	UInt32			stack[10];			// 024
	UInt32			stack2Depth;		// 04C
	UInt32			stack2[10];			// 050
	UInt32			stack3[10];			// 078
	UInt8			unk0A0;				// 0A0
	UInt8			unk0A1;				// 0A1	is set during runLine if CmdExecute.byt025 is not NULL
	UInt8			pad0A2[2];			// 0A2
};	// 0A4

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
