#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"
#include "ScriptUtils.h"

DEFINE_COMMAND(IsScripted, returns 1 if the object or reference has a script attached to it., 0, 1, kParams_OneOptionalForm);
DEFINE_COMMAND(GetScript, returns the script of the reference or passed object., 0, 1, kParams_OneOptionalForm);
DEFINE_COMMAND(RemoveScript, removes the script of the reference or passed object., 0, 1, kParams_OneOptionalForm);
DEFINE_COMMAND(SetScript, sets the script of the reference or passed object., 0, 2, kParams_OneForm_OneOptionalForm);
DEFINE_COMMAND(IsFormValid, returns 1 if the reference or passed object is valid., 0, 1, kParams_OneOptionalForm);

static ParamInfo kParams_OneReference[1] =
{
	{	"reference",	kParamType_ObjectRef,	0	},
};

DEFINE_COMMAND(IsReference, returns 1 if the passed refVar is a reference., 0, 1, kParams_OneReference);

static ParamInfo kParams_GetVariable[2] =
{
	{	"variable name",	kParamType_String,	0	},
	{	"quest",			kParamType_Quest,	1	},
};

DEFINE_COMMAND(GetVariable, looks up the value of a variable by name, 0, 2, kParams_GetVariable);
DEFINE_COMMAND(HasVariable, returns true if the script has a variable with the specified name, 0, 2, kParams_GetVariable);
DEFINE_COMMAND(GetRefVariable, looks up the value of a ref variable by name, 0, 2, kParams_GetVariable);
DEFINE_CMD_ALT(GetArrayVariable, GetArrayVar, looks up an array variable by name on the calling object or specified quest, 0, 2, kParams_GetVariable);

static ParamInfo kParams_SetNumVariable[3] =
{
	{	"variable name",	kParamType_String,	0	},
	{	"variable value",	kParamType_Float,	0	},
	{	"quest",			kParamType_Quest,	1	},
};

static ParamInfo kParams_SetRefVariable[3] =
{
	{	"variable name",	kParamType_String,	0	},
	{	"variable value",	kParamType_AnyForm,	0	},
	{	"quest",			kParamType_Quest,	1	},
};

DEFINE_COMMAND(SetVariable, sets the value of a variable by name, 0, 3, kParams_SetNumVariable);
DEFINE_COMMAND(SetRefVariable, sets the value of a variable by name, 0, 3, kParams_SetRefVariable);

static ParamInfo kParams_CompareScripts[2] =
{
	{	"script",	kParamType_ObjectID,	0	},
	{	"script",	kParamType_ObjectID,	0	},
};

DEFINE_COMMAND(CompareScripts, returns true if the compiled scripts are identical, 0, 2, kParams_CompareScripts);

DEFINE_COMMAND(ResetAllVariables, sets all variables in a script to zero, 0, 0, NULL);

static ParamInfo kParams_GetFormFromMod[2] =
{
	{	"modName",	kParamType_String,	0	},
	{	"formID",	kParamType_String,	0	},
};

DEFINE_COMMAND(GetNumExplicitRefs, returns the number of literal references in a script, 0, 1, kParams_OneOptionalForm);

DEFINE_COMMAND(GetNthExplicitRef, returns the nth literal reference in a script, 0, 2, kParams_OneInt_OneOptionalForm);

DEFINE_COMMAND(RunScript, debug, 0, 1, kParams_OneForm);

DEFINE_COMMAND(GetCurrentScript, returns the calling script, 0, 0, NULL);
DEFINE_COMMAND(GetCallingScript, returns the script that called the executing function script, 0, 0, NULL);

static ParamInfo kNVSEParams_SetEventHandler[4] =
{
	{ "event name",			kNVSEParamType_String,	0 },
	{ "function script",	kNVSEParamType_Form,	0 },
	{ "filter",				kNVSEParamType_Pair,	1 },
	{ "filter",				kNVSEParamType_Pair,	1 },
};

DEFINE_COMMAND_EXP(SetEventHandler, defines a function script to serve as a callback for game events, 0, kNVSEParams_SetEventHandler);
DEFINE_COMMAND_EXP(RemoveEventHandler, "removes event handlers matching the event, script, and optional filters specified", 0, kNVSEParams_SetEventHandler);
DEFINE_CMD(GetCurrentEventName, returns the name of the event currently being processed by an event handler, 0, NULL);

static ParamInfo kNVSEParams_DispatchEvent[3] =
{
	{	"eventName",			kNVSEParamType_String,	0	},
	{	"args",					kNVSEParamType_Array,	1	},
	{	"sender",				kNVSEParamType_String,	1	}
};

DEFINE_COMMAND_EXP(DispatchEvent, dispatches a user-defined event to any registered listeners, 0, kNVSEParams_DispatchEvent);

static ParamInfo kParams_CallAfter[2] =
{
	{	"seconds",	kParamType_Float,	0	},
	{	"function",	kParamType_AnyForm,0	},
};

static ParamInfo kParams_CallWhile[2] =
{
	{	"function",	kParamType_AnyForm,	0	},
	{	"condition",	kParamType_AnyForm,0	},
};

DEFINE_CMD_ALT(CallAfterSeconds, CallAfter, "calls UDF after argument number of seconds", 0, 2, kParams_CallAfter);
DEFINE_COMMAND(CallWhile, "calls UDF each frame while condition is met", 0, 2, kParams_CallWhile);
DEFINE_CMD_ALT(CallForSeconds, CallFor, "calls UDF each frame for argument number of seconds", 0, 2, kParams_CallAfter);
DEFINE_COMMAND(CallWhen, "calls UDF once when a condition is met which is polled each frame", 0, 2, kParams_CallWhile);

using CallArgs = std::vector<VarValue>;

struct DelayedCallInfo
{
	Script* script;
	float time;
	TESObjectREFR* thisObj;
	LambdaManager::LambdaVariableContext lambdaVariableContext;
	CallArgs args;

	DelayedCallInfo(Script* script, float time, TESObjectREFR* thisObj, CallArgs args = {})
		: script(script),
		  time(time),
		  thisObj(thisObj),
		  lambdaVariableContext(script),
		  args(std::move(args))
	{
	}
};

struct CallWhileInfo
{
	Script* callFunction;
	Script* condition;
	TESObjectREFR* thisObj;
	LambdaManager::LambdaVariableContext callFnLambdaCtx;
	LambdaManager::LambdaVariableContext condFnLambdaCtx;
	enum eFlagVals : UInt8 {
		kPassArgs_ToCallFunc = 1 << 0,
		kPassArgs_ToConditionFunc = 1 << 1,
	} flags;
	CallArgs args;

	CallWhileInfo(Script* callFunction, Script* condition, TESObjectREFR* thisObj, UInt8 flags, CallArgs args = {})
		: callFunction(callFunction),
		  condition(condition),
		  thisObj(thisObj),
		  callFnLambdaCtx(callFunction),
		  condFnLambdaCtx(condition),
		  flags(static_cast<eFlagVals>(flags)),
		  args(std::move(args))
	{
	}
};

extern std::list<DelayedCallInfo> g_callForInfos;
extern std::list<CallWhileInfo> g_callWhileInfos;
extern std::list<DelayedCallInfo> g_callAfterInfos;
extern std::list<CallWhileInfo> g_callWhenInfos;

extern ICriticalSection g_callForInfosCS;
extern ICriticalSection g_callWhileInfosCS;
extern ICriticalSection g_callAfterInfosCS;
extern ICriticalSection g_callWhenInfosCS;



static ParamInfo kParams_HasScriptCommand[3] =
{
	{	"command opcode",	kParamType_Integer,			0	},
	{	"form",			kParamType_AnyForm,	1	},
	{	"event block id",			kParamType_Integer,	1	}
};


DEFINE_COMMAND(DecompileScript, decompiles a script to file, false, 2, kParams_OneForm_OneOptionalString);
DEFINE_COMMAND(HasScriptCommand, returns 1 if script contains call to a command, false, 3, kParams_HasScriptCommand);
DEFINE_COMMAND(GetCommandOpcode, gets opcode for command name, false, 1, kParams_OneString);

static ParamInfo kParams_Ternary[] =
{
	{	"value",			kNVSEParamType_BasicType,	0	},
	{	"callIfTrue (UDF)",	kNVSEParamType_Form,	0	},
	{	"callIfFalse (UDF)",	kNVSEParamType_Form,	0	},
};

DEFINE_COMMAND_EXP(Ternary, "The ternary operator as a function.", false, kParams_Ternary);