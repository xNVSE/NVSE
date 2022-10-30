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
		ScriptData(UInt8* scriptData, UInt32 size) : scriptData(scriptData), size(size) {}
	};

	class LambdaVariableContext
	{
		Script* scriptLambda = nullptr;
	public:

		LambdaVariableContext() = default;
		LambdaVariableContext(Script* scriptLambda);
		LambdaVariableContext(nullptr_t nullScript) {}

		LambdaVariableContext(const LambdaVariableContext& other) = delete;
		LambdaVariableContext& operator=(const LambdaVariableContext& other) = delete;

		bool operator==(const LambdaVariableContext& other) const { return scriptLambda == other.scriptLambda; }

		LambdaVariableContext(LambdaVariableContext&& other) noexcept;
		LambdaVariableContext& operator=(LambdaVariableContext&& other) noexcept;

		~LambdaVariableContext();
	};

	// Stores and gives access to a Script, but avoids needlessly capturing lambda var context until needed.
	class Maybe_Lambda
	{
		Script* m_script = nullptr;
		bool m_triedToSaveContext = false;
	public:

		Maybe_Lambda() = default;
		Maybe_Lambda(Script* script) : m_script(script) {}

		Maybe_Lambda(const Maybe_Lambda& other) = delete;
		Maybe_Lambda& operator=(const Maybe_Lambda& other) = delete;

		Maybe_Lambda(Maybe_Lambda&& other) noexcept;
		Maybe_Lambda& operator=(Maybe_Lambda&& other) noexcept;

		//Only compares the contained scripts.
		bool operator==(const Maybe_Lambda& other) const;
		bool operator==(const Script* other) const;
		operator bool() const { return m_script != nullptr; }

		[[nodiscard]] Script* Get() const { return m_script; }
		void TrySaveContext();

		~Maybe_Lambda();
	};
	
	Script* CreateLambdaScript(const ScriptData& scriptData, const Script* parentScript);

	Script* CreateLambdaScript(UInt8* position, const ScriptData& scriptData, const ExpressionEvaluator&);
	ScriptEventList* GetParentEventList(Script* scriptLambda);
	void MarkParentAsDeleted(ScriptEventList* parentEventList);
	void MarkScriptAsDeleted(Script* script);
	bool IsScriptLambda(Script* scriptLambda);
	void DeleteAllForParentScript(Script* parentScript);
	void ClearSavedDeletedEventLists();

	// makes sure that a variable list is not deleted by the game while a lambda is still pending execution
	void SaveLambdaVariables(Script* scriptLambda);
	void UnsaveLambdaVariables(Script* scriptLambda);

	void EraseUnusedSavedVariableLists();
}
