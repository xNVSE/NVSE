#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/ParamInfos.h"
#include "nvse/GameObjects.h"
#include <string>
//NoGore is unsupported in xNVSE

IDebugLog		gLog("nvse_plugin_example.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

NVSEMessagingInterface* g_messagingInterface{};
NVSEInterface* g_nvseInterface{};
NVSECommandTableInterface* g_cmdTableInterface{};

// RUNTIME = Is not being compiled as a GECK plugin.
#if RUNTIME
NVSEScriptInterface* g_script{};
NVSEStringVarInterface* g_stringInterface{};
NVSEArrayVarInterface* g_arrayInterface{};
NVSEDataInterface* g_dataInterface{};
NVSESerializationInterface* g_serializationInterface{};
NVSEConsoleInterface* g_consoleInterface{};
NVSEEventManagerInterface* g_eventInterface{};
bool (*ExtractArgsEx)(COMMAND_ARGS_EX, ...);

#define WantInventoryRefFunctions 0 // set to 1 if you want these PluginAPI functions
#if WantInventoryRefFunctions
_InventoryReferenceCreate InventoryReferenceCreate{};
_InventoryReferenceGetForRefID InventoryReferenceGetForRefID{};
_InventoryReferenceGetRefBySelf InventoryReferenceGetRefBySelf{};
_InventoryReferenceCreateEntry InventoryReferenceCreateEntry{};
#endif

#define WantLambdaFunctions 0 // set to 1 if you want these PluginAPI functions
#if WantLambdaFunctions
_LambdaDeleteAllForScript LambdaDeleteAllForScript{};
_LambdaSaveVariableList LambdaSaveVariableList{};
_LambdaUnsaveVariableList LambdaUnsaveVariableList{};
_IsScriptLambda IsScriptLambda{};
#endif

#define WantScriptFunctions 0 // set to 1 if you want these PluginAPI functions
#if WantScriptFunctions
_HasScriptCommand HasScriptCommand{};
_DecompileScript DecompileScript{};
#endif

#endif

/****************
 * Here we include the code + definitions for our script functions,
 * which are packed in header files to avoid lengthening this file.
 * Notice that these files don't require #include statements for globals/macros like ExtractArgsEx.
 * This is because the "fn_.h" files are only used here,
 * and they are included after such globals/macros have been defined.
 ***************/
#include "fn_intro_to_script_functions.h" 
#include "fn_typed_functions.h"


// Shortcut macro to register a script command (assigning it an Opcode).
#define RegisterScriptCommand(name) 	nvse->RegisterCommand(&kCommandInfo_ ##name)

// Short version of RegisterScriptCommand.
#define REG_CMD(name) RegisterScriptCommand(name)

// Use this when the function's return type is not a number (when registering array/form/string functions).
//Credits: taken from JohnnyGuitarNVSE.
#define REG_TYPED_CMD(name, type)	nvse->RegisterTypedCommand(&kCommandInfo_##name,kRetnType_##type)

// Allows having multiple different versions of commands that scripts can opt into by specifying a plugin version to compile with.
// Notably useful to not break JIP ScriptRunner (SR) scripts when replacing the interface of an existing function, 
// ..since SR will assume to compile the oldest version of a func unless a more recent plugin version is specified.
// 'requiredPluginVersion' should be in the same number format as the plugin's PluginInfo->version.
#define REG_TYPED_CMD_VER(name, type, requiredPluginVersion)	nvse->RegisterTypedCommandVersion(&kCommandInfo_##name,kRetnType_##type, requiredPluginVersion)
#define REG_CMD_VER(name, requiredPluginVersion)	nvse->RegisterTypedCommandVersion(&kCommandInfo_##name,kRetnType_Default, requiredPluginVersion)

// This is a message handler for nvse events
// With this, plugins can listen to messages such as whenever the game loads
void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case NVSEMessagingInterface::kMessage_PostLoad: break;
	case NVSEMessagingInterface::kMessage_ExitGame: break;
	case NVSEMessagingInterface::kMessage_ExitToMainMenu: break;
	case NVSEMessagingInterface::kMessage_LoadGame: break;
	case NVSEMessagingInterface::kMessage_SaveGame: break;
#if EDITOR
	case NVSEMessagingInterface::kMessage_ScriptEditorPrecompile: break;
#endif
	case NVSEMessagingInterface::kMessage_PreLoadGame: break;
	case NVSEMessagingInterface::kMessage_ExitGame_Console: break;
	case NVSEMessagingInterface::kMessage_PostLoadGame: break;
	case NVSEMessagingInterface::kMessage_PostPostLoad: break;
	case NVSEMessagingInterface::kMessage_RuntimeScriptError: break;
	case NVSEMessagingInterface::kMessage_DeleteGame: break;
	case NVSEMessagingInterface::kMessage_RenameGame: break;
	case NVSEMessagingInterface::kMessage_RenameNewGame: break;
	case NVSEMessagingInterface::kMessage_NewGame: break;
	case NVSEMessagingInterface::kMessage_DeleteGameName: break;
	case NVSEMessagingInterface::kMessage_RenameGameName: break;
	case NVSEMessagingInterface::kMessage_RenameNewGameName: break;
	case NVSEMessagingInterface::kMessage_DeferredInit: break;
	case NVSEMessagingInterface::kMessage_ClearScriptDataCache: break;
	case NVSEMessagingInterface::kMessage_MainGameLoop: break;
	case NVSEMessagingInterface::kMessage_ScriptCompile: break;
	case NVSEMessagingInterface::kMessage_EventListDestroyed: break;
	case NVSEMessagingInterface::kMessage_PostQueryPlugins: break;
	default: break;
	}
}

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

bool NVSEPlugin_Load(NVSEInterface* nvse)
{
	_MESSAGE("load");

	g_pluginHandle = nvse->GetPluginHandle();

	// save the NVSE interface in case we need it later
	g_nvseInterface = nvse;

	// register to receive messages from NVSE
	g_messagingInterface = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);

	if (!nvse->isEditor)
	{
#if RUNTIME
		// script and function-related interfaces
		g_script = static_cast<NVSEScriptInterface*>(nvse->QueryInterface(kInterface_Script));
		g_stringInterface = static_cast<NVSEStringVarInterface*>(nvse->QueryInterface(kInterface_StringVar));
		g_arrayInterface = static_cast<NVSEArrayVarInterface*>(nvse->QueryInterface(kInterface_ArrayVar));
		g_dataInterface = static_cast<NVSEDataInterface*>(nvse->QueryInterface(kInterface_Data));
		g_eventInterface = static_cast<NVSEEventManagerInterface*>(nvse->QueryInterface(kInterface_EventManager));
		g_serializationInterface = static_cast<NVSESerializationInterface*>(nvse->QueryInterface(kInterface_Serialization));
		g_consoleInterface = static_cast<NVSEConsoleInterface*>(nvse->QueryInterface(kInterface_Console));
		ExtractArgsEx = g_script->ExtractArgsEx;

#if WantInventoryRefFunctions
		InventoryReferenceCreate = (_InventoryReferenceCreate)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceCreate);
		InventoryReferenceGetForRefID = (_InventoryReferenceGetForRefID)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceGetForRefID);
		InventoryReferenceGetRefBySelf = (_InventoryReferenceGetRefBySelf)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceGetRefBySelf);
		InventoryReferenceCreateEntry = (_InventoryReferenceCreateEntry)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceCreateEntry);
#endif

#if WantLambdaFunctions
		LambdaDeleteAllForScript = (_LambdaDeleteAllForScript)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_LambdaDeleteAllForScript);
		LambdaSaveVariableList = (_LambdaSaveVariableList)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_LambdaSaveVariableList);
		LambdaUnsaveVariableList = (_LambdaUnsaveVariableList)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_LambdaUnsaveVariableList);
		IsScriptLambda = (_IsScriptLambda)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_IsScriptLambda);
#endif

#if WantScriptFunctions
		HasScriptCommand = (_HasScriptCommand)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_HasScriptCommand);
		DecompileScript = (_DecompileScript)g_dataInterface->GetFunc(NVSEDataInterface::kNVSEData_DecompileScript);
#endif

#endif
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

	// Do NOT use this value when releasing your plugin; request your own opcode range.
	UInt32 const examplePluginOpcodeBase = 0x2000;
	
	 // register commands
	nvse->SetOpcodeBase(examplePluginOpcodeBase);
	
	/*************************
	 * The hexadecimal Opcodes are written as comments to the left of their respective functions.
	 * It's important to keep track of how many Opcodes are being used up,
	 * as each plugin is given a limited range which may need to be expanded at some point.
	 *
	 * === How Opcodes Work ===
	 * Each function is associated to an Opcode,
	 * which the game uses to look-up where to find your function's code.
	 * It is CRUCIAL to never change a function's Opcode once it is released to the public.
	 * This is because when compiling a script, each function being called are represented as Opcodes.
	 * So changing a function's Opcode will invalidate previously compiled scripts,
	 * as they will fail to look up that function properly, instead probably finding some other function.
	 *
	 * Example: say we compile a script that uses ExamplePlugin_IsNPCFemale.
	 * The compiled script will check for the Opcode 0x2002 to call that function, and should work fine.
	 * If we remove /REG_CMD(ExamplePlugin_CrashScript);/, and don't register a new function to replace it,
	 * then `REG_CMD(ExamplePlugin_IsNPCFemale);` now registers with Opcode #0x2001.
	 * When we test the script now, a bug/crash is bound to happen,
	 * since the script is looking for an Opcode which is no longer bound to the expected function.
	 ************************/
	
	/*2000*/ RegisterScriptCommand(ExamplePlugin_PluginTest);
	/*2001*/ REG_CMD(ExamplePlugin_CrashScript);
	/*2002*/ REG_CMD(ExamplePlugin_IsNPCFemale);
	/*2003*/ REG_CMD(ExamplePlugin_FunctionWithAnAlias);
	/*2004*/ REG_TYPED_CMD(ExamplePlugin_ReturnForm, Form);
	/*2005*/ REG_TYPED_CMD(ExamplePlugin_ReturnString, String);	// ignore the highlighting for String class, that's not being used here.
	/*2006*/ REG_TYPED_CMD(ExamplePlugin_ReturnArray, Array);
	
	return true;
}
