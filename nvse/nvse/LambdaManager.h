#pragma once
#include "GameScript.h"
#include "ScriptUtils.h"

namespace LambdaManager
{
	struct ScriptData
	{
		UInt8* scriptData;
		UInt32 size;
		ScriptData(): scriptData(nullptr), size(0){}
		ScriptData(UInt8* scriptData, UInt32 size) : scriptData(scriptData), size(size){}
	};
	extern thread_local ScriptData g_lastScriptData;
	
	Script* CreateLambdaScript(UInt8* position, ScriptEventList* parentEventList, Script* parentScript);
	ScriptEventList* GetParentEventList(Script* scriptLambda);
	void MarkParentAsDeleted(ScriptEventList* parentEventList);
	bool IsScriptLambda(Script* script);
	void ClearCache();
}
