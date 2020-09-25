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

// represents the currently executing script context
class ScriptRunner
{
public:
	static const UInt32	kStackDepth = 10;

	enum
	{
		kStackFlags_IF = 1 << 0,
		kStackFlags_ELSEIF = 1 << 1,
		/* ELSE and ENDIF modify the above flags*/
	};

	// members
	/*00*/ TESObjectREFR* containingObj; // set when executing scripts on inventory objects
	/*04*/ TESForm* callingRefBaseForm;
	/*08*/ ScriptEventList* eventList;
	/*0C*/ UInt32 unk0C;
	/*10*/ UInt32 unk10; // pointer? set to NULL before executing an instruction
	/*14*/ Script* script;
	/*18*/ UInt32 unk18; // set to 6 after a failed expression evaluation
	/*1C*/ UInt32 unk1C; // set to Expression::errorCode
	/*20*/ UInt32 ifStackDepth;
	/*24*/ UInt32 ifStack[kStackDepth];	// stores flags
	/*4C*/ UInt32 unk4C[(0xA0 - 0x4C) >> 2];
	/*A0*/ UInt8 invalidReferences;	// set when the dot operator fails to resolve a reference (inside the error message handler)
	/*A1*/ UInt8 unkA1;	// set when the executing CommandInfo's 2nd flag bit (+0x25) is set
	/*A2*/ UInt16 padA2;
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
