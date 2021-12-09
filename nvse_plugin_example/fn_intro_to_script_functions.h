#pragma once


//In here we define a script function
//Script functions must always follow the Cmd_FunctionName_Execute naming convention

#if RUNTIME	// unnecessary if we only compile in non-GECK mode.
// Otherwise, GECK plugins do not need to know a function's code.
bool Cmd_ExamplePlugin_PluginTest_Execute(COMMAND_ARGS)
{
	_MESSAGE("plugintest");

	*result = 42;	// what the function returns.

	Console_Print("plugintest running");

	return true;
}
#endif

//This defines a function without a condition, that does not take any arguments.
DEFINE_COMMAND_PLUGIN(ExamplePlugin_PluginTest, "prints a string and returns 42", false, NULL)


/**************** Notes On DEFINE_ Functions *********************
 *
 * === Valid DEFINE_ Functions For Use In NVSE Plugins ====
 * In the "nvse/CommandTable.h" file, there are multiple DEFINE_ functions.
 * However, only a few of these are accessible to plugins.
 * Only use the DEFINE_s that have "_PLUGIN" in the name,
 * since the parsers that the other DEFINE_s use are not defined for NVSE Plugins.
 *
 * === Defining Condition Functions ===
 * Condition functions must be defined using a DEFINE_ that has "COND" in the name.
 * This way, we enforce having to provide code for when the function is called as a condition,
 * on top of having to write code for using it as a script function.
 * There must be two implementations, since both types of functions call pass their arguments in different ways.
 * Code for condition functions has "_Eval" at the end of the C++ function name, and has the COMMAND_ARGS_EVAL arguments.
 * Example: see ExamplePlugin_IsNPCFemale.
 *
 * === Having an Alias for a Function ===
 * Any type of function can be granted an alias, although this alias is only useful in Scripts.
 * The DEFINE_s with "_ALT" in the name can be used to this end.
 * They simply have an extra argument that allows specifying an "altName" for the function.
 *****************************************************************/


//We can also define the arguments for the function before implementing the _Execute function.
DEFINE_COMMAND_PLUGIN(ExamplePlugin_CrashScript, "Crashes the script", false, NULL)

#if RUNTIME
bool Cmd_ExamplePlugin_CrashScript_Execute(COMMAND_ARGS)
{
	_MESSAGE("Crashscript");

	Console_Print("Crashscript running");

	// NOTE: returning false is not recommended, as it crashes the script that called the function.
	return false;	// will crash the script.
}
#endif

#if RUNTIME
//Conditions must follow the Cmd_FunctionName_Eval naming convention
bool Cmd_ExamplePlugin_IsNPCFemale_Eval(COMMAND_ARGS_EVAL)
{
	TESNPC* npc = (TESNPC*)arg1;
	*result = npc->baseData.IsFemale() ? 1 : 0;
	return true;
}

bool Cmd_ExamplePlugin_IsNPCFemale_Execute(COMMAND_ARGS)
{
	*result = 0;	// If ExtractArgsEx fails, then we want to give a default return value.
	
	//Created a simple condition 
	//thisObj is what the script/condition extracts as parent caller
	//EG, Ref.IsFemale would make thisObj = ref
	//We are using actor bases though, so the function is called as such: ExamplePlugin_IsNPCFemale baseForm
	TESNPC* npc = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &npc))
	{
		Cmd_ExamplePlugin_IsNPCFemale_Eval(thisObj, npc, NULL, result);
	}

	return true;
}
#endif
DEFINE_CMD_COND_PLUGIN(ExamplePlugin_IsNPCFemale, "Checks if npc is female", false, kParams_OneActorBase)


// Define a function with an alias.
// It can be called in scripts/console via either typing "ExamplePlugin_FunctionWithAnAlias", or "ExPlug_FnWithAlias".
DEFINE_COMMAND_ALT_PLUGIN(ExamplePlugin_FunctionWithAnAlias, ExPlug_FnWithAlias, "Prints a message in console, has an alias.", false, NULL)

#if RUNTIME
bool Cmd_ExamplePlugin_FunctionWithAnAlias_Execute(COMMAND_ARGS)
{
	_MESSAGE("FunctionWithAnAlias");
	Console_Print("FunctionWithAnAlias running");
	return true;
}
#endif