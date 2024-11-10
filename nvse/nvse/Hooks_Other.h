#pragma once
#include "FastStack.h"
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
		UInt32* curDataPtr = nullptr;
	};
	void CleanUpNVSEVars(ScriptEventList* eventList);
	void CleanUpNVSEVar(ScriptEventList* eventList, ScriptLocal* local);

	void DeleteEventList(ScriptEventList* eventList);
	
	void Hooks_Other_Init();

	CurrentScriptContext* GetExecutingScriptContext();

	void PushScriptContext(const CurrentScriptContext& ctx);
	void PopScriptContext();
}
