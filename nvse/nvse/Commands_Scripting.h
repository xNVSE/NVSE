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
	UInt32				unk00;			// 00
	TESForm				*baseForm;		// 04
	ScriptEventList			*eventList;		// 08
	UInt32				unk0C;			// 0C
	UInt32				unk10;			// 10
	Script				*script;		// 14
	UInt32				unk18;			// 18	= 6 after failed to evaluate expression
	UInt32				unk1C;			// 1C
	UInt32				stackDepth;		// 20
	UInt32				stack[10];		// 24
	UInt32				stack2Depth;		// 4C
	UInt32				stack2[10];		// 50
	UInt32				stack3[10];		// 78
	UInt8				byteA0;			// A0
	UInt8				byteA1;			// A1	is set during runLine if CmdExecute.byt025 is not NULL
	UInt8				padA2[2];		// A2
};
STATIC_ASSERT(sizeof(ScriptRunner) == 0xA4);

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
