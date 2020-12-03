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

	QueuedScript(Script* _script, TESObjectREFR* _thisObj, ScriptEventList* _eventList, TESObjectREFR* _containingObj, UInt8 _arg5, UInt8 _arg6, UInt8 _arg7, UInt32 _arg8) :
		script(_script), eventList(_eventList), arg5(_arg5), arg6(_arg6), arg7(_arg7), arg8(_arg8)
	{
		thisObj = _thisObj ? _thisObj->refID : 0;
		containingObj = _containingObj ? _containingObj->refID : 0;
	}

	void Execute();
};

extern Vector<QueuedScript> s_queuedScripts;


void Hook_Gameplay_Init(void);
void ToggleUIMessages(bool enableSpam);
void ToggleConsoleOutput(bool enable);
bool RunCommand_NS(COMMAND_ARGS, Cmd_Execute cmd);

extern DWORD g_mainThreadID;

// this returns a refID rather than a TESObjectREFR* as dropped items are non-persistent references
UInt32 GetPCLastDroppedItemRef();
TESForm* GetPCLastDroppedItem();		// returns the base object

void SetRetainExtraOwnership(bool bRetain);