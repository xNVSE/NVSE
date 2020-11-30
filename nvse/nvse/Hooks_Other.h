#pragma once
#include "GameAPI.h"

namespace OtherHooks
{
	struct QueuedEventListDestruction
	{
		ScriptEventList* eventList;
		bool doFree;

		QueuedEventListDestruction(ScriptEventList* eventList, bool doFree) : eventList(eventList), doFree(doFree)
		{}

		void Destroy();
	};
	
	extern Vector<QueuedEventListDestruction> s_eventListDestructionQueue;

	void CleanUpNVSEVars(const ScriptEventList* eventList);
	
	void Hooks_Other_Init();
}
