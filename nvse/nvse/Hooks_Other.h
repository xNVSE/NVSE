#pragma once
#include "GameAPI.h"

namespace OtherHooks
{
	struct QueuedEventList
	{
		ScriptEventList		*eventList;

		QueuedEventList(ScriptEventList *_eventList) : eventList(_eventList) {}
		~QueuedEventList();
	};

	extern Vector<QueuedEventList> s_eventListDestructionQueue;

	void CleanUpNVSEVars(const ScriptEventList* eventList);
	
	void Hooks_Other_Init();
}
