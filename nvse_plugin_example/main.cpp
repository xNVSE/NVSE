#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/ParamInfos.h"
#include "nvse/GameObjects.h"
#include <string>

#ifdef NOGORE
IDebugLog		gLog("nvse_plugin_example_ng.log");
#else
IDebugLog		gLog("nvse_plugin_example.log");
#endif

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

NVSEMessagingInterface* g_msg;
NVSEInterface * SaveNVSE;
NVSECommandTableInterface * g_cmdTable;
const CommandInfo * g_TFC;
NVSEScriptInterface* g_script;
#define ExtractArgsEx(...) g_script->ExtractArgsEx(__VA_ARGS__)
#define ExtractFormatStringArgs(...) g_script->ExtractFormatStringArgs(__VA_ARGS__)

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
			_MESSAGE("Received precompile message with script at %08x", msg->data);
			break;
		case NVSEMessagingInterface::kMessage_RuntimeScriptError:
			_MESSAGE("Received runtime script error message %s", msg->data);
			break;
		default:
			_MESSAGE("Plugin Example received unknown message");
			break;
	}
}

#ifdef RUNTIME

bool Cmd_ExamplePlugin_PluginTest_Execute(COMMAND_ARGS)
{
	_MESSAGE("plugintest");

	*result = 42;

	Console_Print("plugintest running");

	return true;
}

#endif

DEFINE_COMMAND_PLUGIN(ExamplePlugin_PluginTest, "prints a string", 0, 0, NULL)

extern "C" {

bool NVSEPlugin_Query(const NVSEInterface * nvse, PluginInfo * info)
{
	_MESSAGE("query");

	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "nvse_plugin_example";
	info->version = 2;

	// version checks
	if(nvse->nvseVersion < NVSE_VERSION_INTEGER)
	{
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, NVSE_VERSION_INTEGER);
		return false;
	}

	if(!nvse->isEditor)
	{
		if(nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525)
		{
			_ERROR("incorrect runtime version (got %08X need at least %08X)", nvse->runtimeVersion, RUNTIME_VERSION_1_4_0_525);
			return false;
		}

#ifdef NOGORE
		if(!nvse->isNogore)
		{
			_ERROR("incorrect runtime edition (got %08X need %08X (nogore))", nvse->isNogore, 1);
			return false;
		}
#else
		if(nvse->isNogore)
		{
			_ERROR("incorrect runtime edition (got %08X need %08X (standard))", nvse->isNogore, 0);
			return false;
		}
#endif
	}
	else
	{
		if(nvse->editorVersion < CS_VERSION_1_4_0_518)
		{
			_ERROR("incorrect editor version (got %08X need at least %08X)", nvse->editorVersion, CS_VERSION_1_4_0_518);
			return false;
		}
#ifdef NOGORE
		_ERROR("Editor only uses standard edition, closing.");
		return false;
#endif
	}

	// version checks pass

	return true;
}

bool NVSEPlugin_Load(const NVSEInterface * nvse)
{
	_MESSAGE("load");

	g_pluginHandle = nvse->GetPluginHandle();

	// save the NVSEinterface in cas we need it later
	SaveNVSE = (NVSEInterface *)nvse;

	// register to receive messages from NVSE
	NVSEMessagingInterface* msgIntfc = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
	msgIntfc->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
	g_msg = msgIntfc;

	g_script = (NVSEScriptInterface*)nvse->QueryInterface(kInterface_Script);

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
	 **************************************************************************/

	// register commands
	nvse->SetOpcodeBase(0x2000);
	nvse->RegisterCommand(&kCommandInfo_ExamplePlugin_PluginTest);

	return true;
}

};
