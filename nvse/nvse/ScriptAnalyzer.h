#pragma once
#include "GameScript.h"

namespace ScriptParsing
{
	class ScriptIterator
	{
		void ReadLine();
	public:
		UInt16 Read16();
		UInt32 Read32();
		
		UInt16 opcode;
		UInt16 length;
		Script* script;
		UInt8* curData;

		explicit ScriptIterator(Script* script);
		void operator++();
		bool End() const;
	};
	extern std::span<ScriptOperator> g_operators;
	
	enum class ScriptStatementCode : UInt16
	{
		Begin = 0x10,
		End = 0x11,
		Short = 0x12,
		Long = 0x13,
		Float = 0x14,
		SetTo = 0x15,
		If = 0x16,
		Else = 0x17,
		ElseIf = 0x18,
		EndIf = 0x19,
		ReferenceFunction = 0x1C,
		ScriptName = 0x1D,
		Return = 0x1E,
		Ref = 0x1F,
	};

	// unfinished
	class ScriptLine
	{
	public:
		ScriptIterator context;
		CommandInfo* statementCmd;
		virtual ~ScriptLine() = default;

		ScriptLine(const ScriptIterator& context, CommandInfo* cmd);

		virtual std::string ToString();
	};

	class ScriptCommandCall : public ScriptLine
	{
	public:
		ScriptCommandCall(const ScriptIterator& contextParam);
	};

	class ScriptStatement : public ScriptLine
	{
	public:
		ScriptStatement(const ScriptIterator& contextParam);
	};
	
	class ScriptNameStatement : public ScriptStatement
	{
	public:
		ScriptNameStatement(const ScriptIterator& context)
			: ScriptStatement(context)
		{
		}

		std::string ToString() override;
	};

	class BeginStatement : public ScriptStatement
	{
	public:
		CommandInfo* eventBlockCmd;
		UInt32 endJumpLength;
		BeginStatement(const ScriptIterator& contextParam);

		std::string ToString() override;
	};

	class ScriptAnalyzer
	{
		ScriptIterator iter_;
		std::vector<std::unique_ptr<ScriptLine>> lines_;
		void AddLine(std::unique_ptr<ScriptLine> line);
	public:
		ScriptAnalyzer(Script* script);

		void ParseNext();
	};

}
