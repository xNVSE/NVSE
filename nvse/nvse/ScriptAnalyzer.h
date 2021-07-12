#pragma once
#include <unordered_set>
#include "GameObjects.h"
#include "GameScript.h"

class ExpressionEvaluator;
class CachedTokens;
CommandInfo* GetEventCommandInfo(UInt16 opcode);

namespace ScriptParsing
{
	class ExpressionToken;
	class CommandCallToken;
	enum class ExpressionCode : UInt8;

	class ScriptIterator
	{
		void ReadLine();
	public:
		UInt8 Read8();
		UInt16 Read16();
		UInt32 Read32();
		UInt64 Read64();
		float ReadFloat();
		double ReadDouble();
		Script::RefVariable* ReadRefVar();
		VariableInfo* ReadVariable(TESForm*& refOut, ExpressionCode& typeOut);
		bool ReadStringLiteral(std::string_view& out);
		bool ReadNumericConstant(double& result);
		Script::RefVariable* GetCallingReference() const;

		UInt16 opcode{};
		UInt16 length{};
		UInt16 refIdx{};
		Script* script;
		UInt8* curData;

		explicit ScriptIterator(Script* script);
		ScriptIterator(Script* script, UInt8* position);
		ScriptIterator(Script* script, UInt16 opcode, UInt16 length, UInt16 refIdx, UInt8* data);
		ScriptIterator() : script(nullptr), curData(nullptr) {}
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
		Max = Ref,
	};

	enum class ExpressionCode : UInt8
	{
		None = '\0',
		Int = 's',
		FloatOrRef = 'f',
		Global = 'G',
		Command = 'X'
	};

	class Expression
	{
	public:
		ScriptIterator context;
		UInt16 expressionLength = 0;
		std::vector<std::unique_ptr<ExpressionToken>> stack;

		ScriptOperator* ReadOperator();

		bool ReadExpression();

		Expression() = default;
		explicit Expression(const ScriptIterator& context);

		std::string ToString();
	};

	class ScriptLine
	{
	public:
		ScriptIterator context;
		CommandInfo* cmdInfo;
		bool error = false;
		virtual ~ScriptLine() = default;

		UInt8 Read8();
		UInt16 Read16();
		UInt32 Read32();

		ScriptLine(const ScriptIterator& context, CommandInfo* cmd);

		virtual std::string ToString();
	};

	class ScriptCommandCall : public ScriptLine
	{
	public:
		ScriptCommandCall(const ScriptIterator& contextParam);
		std::unique_ptr<CommandCallToken> commandCallToken = nullptr;

		std::string ToString() override;
	};

	class ScriptStatement : public ScriptLine
	{
	public:
		ScriptStatement(const ScriptIterator& contextParam);
	};
	
	class ScriptNameStatement : public ScriptStatement
	{
	public:
		ScriptNameStatement(const ScriptIterator& context);

		std::string ToString() override;
	};

	class BeginStatement : public ScriptStatement
	{
	public:
		CommandInfo* eventBlockCmd;
		UInt32 endJumpLength;
		std::unique_ptr<CommandCallToken> commandCallToken = nullptr;;
		BeginStatement(const ScriptIterator& contextParam);

		std::string ToString() override;
	};

	class ExpressionToken
	{
	public:
		ExpressionCode code = ExpressionCode::None;
		bool error = false;
		virtual ~ExpressionToken() = default;

		ExpressionToken() = default;

		explicit ExpressionToken(ExpressionCode code);

	};

	class OperatorToken : public ExpressionToken
	{
	public:
		ScriptOperator* scriptOperator;

		explicit OperatorToken(ScriptOperator* scriptOperator);

		std::string ToString(std::vector<std::string>& stack);
	};

	class OperandToken : public ExpressionToken
	{
	public:
		explicit OperandToken(ExpressionCode code)
			: ExpressionToken(code)
		{
		}

		OperandToken() = default;
		virtual std::string ToString() = 0;

	};

	class ScriptVariableToken : public OperandToken
	{
		std::string GetBackupName();
	public:
		Script* script;
		TESForm* ref = nullptr;
		VariableInfo* varInfo;

		ScriptVariableToken(Script* script, ExpressionCode varType, VariableInfo* info, TESForm* ref = nullptr);

		std::string ToString() override;
	};

	class StringLiteralToken : public OperandToken
	{
	public:
		std::string_view stringLiteral;

		explicit StringLiteralToken(const std::string_view& stringLiteral);

		std::string ToString() override;
	};

	class NumericConstantToken : public OperandToken
	{
	public:
		double value;

		explicit NumericConstantToken(double value);

		std::string ToString() override;
	};

	class RefToken : public OperandToken
	{
	public:
		Script::RefVariable* refVariable;
		Script* script;

		explicit RefToken(Script* script, Script::RefVariable* refVariable);

		std::string ToString() override;
	};

	class GlobalVariableToken : public RefToken
	{
	public:
		TESGlobal* global;

		explicit GlobalVariableToken(Script::RefVariable* refVariable);

		std::string ToString() override;
	};

	class InlineExpressionToken : public OperandToken
	{
	public:
		std::unique_ptr<ExpressionEvaluator> eval = nullptr;
		CachedTokens* tokens;

		InlineExpressionToken(ScriptIterator& context);

		std::string ToString() override;
	};

	class CommandCallToken : public OperandToken
	{
	public:
		CommandInfo* cmdInfo;
		std::unique_ptr<RefToken> callingReference = nullptr;
		
		std::vector<std::unique_ptr<OperandToken>> args;

		std::unique_ptr<ExpressionEvaluator> expressionEvaluator = nullptr;
		std::vector<CachedTokens*> expressionEvalArgs;

		CommandCallToken(const ScriptIterator& context);
		CommandCallToken(CommandInfo* cmdInfo, Script::RefVariable* callingRef, Script* script);

		std::string ToString() override;

		bool ReadVariableToken(ScriptIterator& context);

		bool ReadFormToken(ScriptIterator& context);

		bool ReadNumericToken(ScriptIterator& context);

		bool ParseCommandArgs(ScriptIterator context);
	};

	class ExpressionStatement
	{
	public:
		virtual ~ExpressionStatement() = default;
		virtual Expression& GetExpression() = 0;
	};

	class SetToStatement : public ScriptStatement, public ExpressionStatement
	{
	public:
		char type = 0;
		std::unique_ptr<OperandToken> toVariable = nullptr;
		Expression expression;

		explicit SetToStatement(const ScriptIterator& contextParam);

		std::string ToString() override;
		Expression& GetExpression() override;
	};

	class ConditionalStatement : public ScriptStatement, public ExpressionStatement
	{
	public:
		UInt16 numBytesToJumpOnFalse;
		Expression expression;
		explicit ConditionalStatement(const ScriptIterator& contextParam);
		std::string ToString() override;
		Expression& GetExpression() override;
	};
	
	class ScriptAnalyzer
	{
		ScriptIterator iter_;
		std::vector<std::unique_ptr<ScriptLine>> lines_;
		void Parse();
	public:
		bool isLambdaScript = false;
		bool compilerOverrideEnabled = false;
		bool error = false;
		std::unordered_set<VariableInfo*> arrayVariables;
		std::unordered_set<VariableInfo*> stringVariables;
		Script* script;

		bool CallsCommand(CommandInfo* cmd, CommandInfo* eventBlockInfo);

		ScriptAnalyzer(Script* script);
		~ScriptAnalyzer();

		static std::unique_ptr<ScriptLine> ParseLine(const ScriptIterator& iter);
		std::string DecompileScript();
	};

}
