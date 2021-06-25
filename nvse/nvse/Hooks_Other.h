#pragma once
#include "GameAPI.h"

namespace OtherHooks
{
	extern thread_local TESObjectREFR* g_lastScriptOwnerRef;
	void CleanUpNVSEVars(ScriptEventList* eventList);

	void DeleteEventList(ScriptEventList* eventList);
	
	void Hooks_Other_Init();
}
