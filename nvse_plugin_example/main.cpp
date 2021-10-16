#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/ParamInfos.h"
#include "nvse/GameObjects.h"
#include <string>
//NoGore is unsupported in xNVSE

#ifndef RegisterScriptCommand
#define RegisterScriptCommand(name) 	nvse->RegisterCommand(&kCommandInfo_ ##name);
#endif

IDebugLog		gLog("nvse_plugin_example.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

NVSEMessagingInterface* g_messagingInterface;
NVSEInterface* g_nvseInterface;
NVSECommandTableInterface* g_cmdTable;
const CommandInfo* g_TFC;

#if RUNTIME  //if non-GECK version (in-game)
NVSEScriptInterface* g_script;
#endif

bool (*ExtractArgsEx)(COMMAND_ARGS_EX, ...);

// This is a message handler for nvse events
// With this, plugins can listen to messages such as whenever the game loads
void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case NVSEMessagingInterface::kMessage_LoadGame:
		_MESSAGE("Received load game message with file path %s", msg->data);
		break;
	case NVSEMessagingInterface::kMessage_SaveGame:
		_MESSAGE("Received save game message with file path %s", msg->data);
		break;
	case NVSEMessagingInterface::kMessage_PreLoadGame:
		_MESSAGE("Received pre load game message with file path %s", msg->data);
		break;
	case NVSEMessagingInterface::kMessage_PostLoadGame:
		_MESSAGE("Received post load game message", msg->data ? "Error/Unkwown" : "OK");
		break;
	case NVSEMessagingInterface::kMessage_PostLoad:
		_MESSAGE("Received post load plugins message");
		break;
	case NVSEMessagingInterface::kMessage_PostPostLoad:
		_MESSAGE("Received post post load plugins message");
		break;
	case NVSEMessagingInterface::kMessage_ExitGame:
		_MESSAGE("Received exit game message");
		break;
	case NVSEMessagingInterface::kMessage_ExitGame_Console:
		_MESSAGE("Received exit game via console qqq command message");
		break;
	case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		_MESSAGE("Received exit game to main menu message");
		break;
	case NVSEMessagingInterface::kMessage_Precompile:
		_MESSAGE("Received precompile message with script");
		break;
	case NVSEMessagingInterface::kMessage_RuntimeScriptError:
		_MESSAGE("Received runtime script error message %s", msg->data);
		break;
	default:
		break;
	}
}

bool Cmd_ExamplePlugin_PluginTest_Execute(COMMAND_ARGS);

#if RUNTIME  //if non-GECK version (in-game)
//In here we define a script function
//Script functions must always follow the Cmd_FunctionName_Execute naming convention
bool Cmd_ExamplePlugin_PluginTest_Execute(COMMAND_ARGS)
{
	_MESSAGE("plugintest");

	*result = 42;

	Console_Print("plugintest running");

	return true;
}
#endif

//This defines a function without a condition, that does not take any arguments
DEFINE_COMMAND_PLUGIN(ExamplePlugin_PluginTest, "prints a string", false, NULL)

bool Cmd_ExamplePlugin_IsNPCFemale_Eval(COMMAND_ARGS_EVAL);

#if RUNTIME
//Conditions must follow the Cmd_FunctionName_Eval naming convention
bool Cmd_ExamplePlugin_IsNPCFemale_Eval(COMMAND_ARGS_EVAL)
{

	TESNPC* npc = (TESNPC*)arg1;
	*result = npc->baseData.IsFemale() ? 1 : 0;
	return true;
}
#endif

bool Cmd_ExamplePlugin_IsNPCFemale_Execute(COMMAND_ARGS);

#if RUNTIME
bool Cmd_ExamplePlugin_IsNPCFemale_Execute(COMMAND_ARGS)
{
	//Created a simple condition 
	//thisObj is what the script extracts as parent caller
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
DEFINE_COMMAND_PLUGIN(ExamplePlugin_IsNPCFemale, "Checks if npc is female", false, kParams_OneActorBase)

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	_MESSAGE("query");

	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "MyFirstPlugin";
	info->version = 2;

	// version checks
	if (nvse->nvseVersion < PACKED_NVSE_VERSION)
	{
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, PACKED_NVSE_VERSION);
		return false;
	}

	if (!nvse->isEditor)
	{
		if (nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525)
		{
			_ERROR("incorrect runtime version (got %08X need at least %08X)", nvse->runtimeVersion, RUNTIME_VERSION_1_4_0_525);
			return false;
		}

		if (nvse->isNogore)
		{
			_ERROR("NoGore is not supported");
			return false;
		}
	}
	else
	{
		if (nvse->editorVersion < CS_VERSION_1_4_0_518)
		{
			_ERROR("incorrect editor version (got %08X need at least %08X)", nvse->editorVersion, CS_VERSION_1_4_0_518);
			return false;
		}
	}

	// version checks pass
	// any version compatibility checks should be done here
	return true;
}

bool NVSEPlugin_Load(const NVSEInterface* nvse)
{
	_MESSAGE("load");

	if (!nvse->isEditor)
	{
		g_pluginHandle = nvse->GetPluginHandle();

		// save the NVSEinterface in cas we need it later
		g_nvseInterface = (NVSEInterface*)nvse;

		// register to receive messages from NVSE
		g_messagingInterface = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
		g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);

		g_script = (NVSEScriptInterface*)nvse->QueryInterface(kInterface_Script);
		ExtractArgsEx = g_script->ExtractArgsEx;
	}

	/***************************************************************************
	 *
	 *	READ THIS!
	 *
	 *	Before releasing your plugin, you need to request an opcode range from
	 *	the NVSE team and set it in your first SetOpcodeBase call. If you do not
	 *	do this, your plugin will create major compatibility issues with other
	 *	plugins, and will not load in release versions of NVSE. See
	 *	nvse_readme.txt for more information.
	 *
	 *	See https://geckwiki.com/index.php?title=NVSE_Opcode_Base
	 *
	 **************************************************************************/

	 // register commands
	nvse->SetOpcodeBase(0x2000);
	RegisterScriptCommand(ExamplePlugin_PluginTest);
	RegisterScriptCommand(ExamplePlugin_IsNPCFemale);

	return true;
}
