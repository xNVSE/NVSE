#include "nvse/PluginAPI.h"

#include "Hooks_ExpressionEvalOptimized.h"

//NoGore is unsupported in this fork

#ifndef RegisterScriptCommand
#define RegisterScriptCommand(name) 	nvse->RegisterCommand(&kCommandInfo_ ##name);
#endif


extern "C" {

bool NVSEPlugin_Query(const NVSEInterface * nvse, PluginInfo * info)
{
	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "ShortCircuitScript";
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
		if(nvse->isNogore)
		{
			_ERROR("NoGore is not supported");
			return false;
		}
		return true;
	}
	return false;
}

bool NVSEPlugin_Load(const NVSEInterface * nvse)
{
	Hook_Evaluator();
	return true;
}

};
