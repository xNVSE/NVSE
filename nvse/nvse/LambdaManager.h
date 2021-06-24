#pragma once
#include "GameScript.h"

class ExpressionEvaluator;

namespace LambdaManager
{
	extern ICriticalSection g_lambdaCs;
	struct ScriptData
	{
		UInt8* scriptData;
		UInt32 size;
		ScriptData() = default;
		ScriptData(UInt8* scriptData, UInt32 size) : scriptData(scriptData), size(size){}
	};
	
	Script* CreateLambdaScript(UInt8* position, const ScriptData& scriptData, const ExpressionEvaluator&);
	ScriptEventList* GetParentEventList(Script* scriptLambda);
	void MarkParentAsDeleted(ScriptEventList*& parentEventList, bool doFree);
	bool IsScriptLambda(Script* scriptLambda);
	void DeleteAllForParentScript(Script* parentScript);
	void ClearSavedDeletedEventLists();
}
