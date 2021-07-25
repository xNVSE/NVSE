#pragma once
#include "GameAPI.h"

namespace OtherHooks
{
	struct CurrentScriptContext
	{
		Script* script = nullptr;
		ScriptRunner* scriptRunner = nullptr;
		UInt32* lineNumberPtr = nullptr;
		TESObjectREFR* scriptOwnerRef = nullptr;
		CommandInfo* command = nullptr;
	};
	extern thread_local CurrentScriptContext g_currentScriptContext;
	void CleanUpNVSEVars(ScriptEventList* eventList);

	void DeleteEventList(ScriptEventList* eventList);
	
	void Hooks_Other_Init();
}
