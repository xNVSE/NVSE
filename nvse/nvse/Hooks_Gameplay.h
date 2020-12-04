#pragma once
#include "CommandTable.h"
#include "Utilities.h"
#include "GameObjects.h"


class Menu;

struct QueuedScript
{
	Script			*script;
	UInt32			thisObj;
	ScriptEventList	*eventList;
	UInt32			containingObj;
	UInt8			arg5;
	UInt8			arg6;
	UInt8			arg7;
	UInt8			pad13;
	UInt32			arg8;

	QueuedScript(Script* _script, TESObjectREFR* _thisObj, ScriptEventList* _eventList, TESObjectREFR* _containingObj, UInt8 _arg5, UInt8 _arg6, UInt8 _arg7, UInt32 _arg8);

	void Execute();
};
extern Vector<QueuedScript> s_queuedScripts;

struct QueuedScript2
{
	Script			*script;
	UInt32			thisObj;
	ScriptEventList	*eventList;

	QueuedScript2(Script* _script, TESObjectREFR* _thisObj, ScriptEventList* _eventList);

	void Execute();
};
extern Vector<QueuedScript2> s_queuedScripts2;

void Hook_Gameplay_Init(void);
void ToggleUIMessages(bool enableSpam);
void ToggleConsoleOutput(bool enable);
bool RunCommand_NS(COMMAND_ARGS, Cmd_Execute cmd);

extern DWORD g_mainThreadID;

// this returns a refID rather than a TESObjectREFR* as dropped items are non-persistent references
UInt32 GetPCLastDroppedItemRef();
TESForm* GetPCLastDroppedItem();		// returns the base object

void SetRetainExtraOwnership(bool bRetain);