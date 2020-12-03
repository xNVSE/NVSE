#pragma once
#include "GameAPI.h"

namespace OtherHooks
{
	extern Vector<ScriptEventList*> s_eventListDestructionQueue;

	void CleanUpNVSEVars(const ScriptEventList* eventList);
	
	void Hooks_Other_Init();
}
