#include "ScriptAnalyzer.h"


UInt8 Read8(UInt8*& curData)
{
	const auto byte = *curData;
	++curData;
	return byte;
}

UInt16 Read16(UInt8*& curData)
{
	const auto word = *reinterpret_cast<UInt16*>(curData);
	curData += 2;
	return word;
}

UInt32 Read32(UInt8*& curData)
{
	const auto dword = *reinterpret_cast<UInt32*>(curData);
	curData += 4;
	return dword;
}

UInt64 Read64(UInt8*& curData)
{
	const auto qword = *reinterpret_cast<UInt64*>(curData);
	curData += 8;
	return qword;
}

CommandInfo* GetScriptStatement(ScriptParsing::ScriptStatementCode code)
{
	return &g_scriptStatementCommandInfos[static_cast<unsigned int>(code) - 0x10];
}

UInt16 ScriptParsing::ScriptIterator::Read16()
{
	return ::Read16(curData);
}

UInt32 ScriptParsing::ScriptIterator::Read32()
{
	return ::Read32(curData);
}

void ScriptParsing::ScriptIterator::ReadLine()
{
	opcode = Read16();
	length = Read16();
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script): curData(static_cast<UInt8*>(script->data))
{
	ReadLine();
}

void ScriptParsing::ScriptIterator::operator++()
{
	curData += length;
	ReadLine();
}

bool ScriptParsing::ScriptIterator::End() const
{
	return curData >= static_cast<UInt8*>(script->data) + script->info.dataLength;
}

ScriptParsing::ScriptLine::ScriptLine(const ScriptIterator& context, CommandInfo* cmd): context(context), statementCmd(cmd)
{
}

std::string ScriptParsing::ScriptLine::ToString()
{
	return std::string(this->statementCmd->longName);
}

ScriptParsing::ScriptCommandCall::ScriptCommandCall(const ScriptIterator& contextParam):
    ScriptLine(contextParam, g_scriptCommands.GetByOpcode(contextParam.opcode))
{
}

ScriptParsing::ScriptStatement::ScriptStatement(const ScriptIterator& contextParam) : ScriptLine(contextParam, GetScriptStatement(static_cast<ScriptStatementCode>(contextParam.opcode)))
{
}

std::string ScriptParsing::ScriptNameStatement::ToString()
{
	return ScriptLine::ToString() + " " + std::string(context.script->GetName());
}

ScriptParsing::BeginStatement::BeginStatement(const ScriptIterator& contextParam) : ScriptStatement(contextParam)
{
	const auto eventOpcode = this->context.Read16();
	this->eventBlockCmd = &g_eventBlockCommandInfos[eventOpcode];
	this->endJumpLength = this->context.Read32();
}


std::string ScriptParsing::BeginStatement::ToString()
{
	return ScriptLine::ToString() + " " + std::string(this->eventBlockCmd->longName);
}

void ScriptParsing::ScriptAnalyzer::AddLine(std::unique_ptr<ScriptLine> line)
{
	this->lines_.emplace_back(std::move(line));
}

ScriptParsing::ScriptAnalyzer::ScriptAnalyzer(Script* script) : iter_(script) {}

void ScriptParsing::ScriptAnalyzer::ParseNext()
{
	const auto opcode =  this->iter_.opcode;
	if (opcode <= static_cast<UInt32>(ScriptStatementCode::Ref))
	{
		switch (static_cast<ScriptStatementCode>(opcode))
		{
		case ScriptStatementCode::Begin: 
			AddLine(std::make_unique<BeginStatement>(this->iter_));
			break;
		case ScriptStatementCode::End: 
		case ScriptStatementCode::Short:
		case ScriptStatementCode::Long:
		case ScriptStatementCode::Float:
		case ScriptStatementCode::SetTo:
		case ScriptStatementCode::If:
		case ScriptStatementCode::Else:
		case ScriptStatementCode::ElseIf:
		case ScriptStatementCode::EndIf:
		case ScriptStatementCode::ReferenceFunction: 
			AddLine(std::make_unique<ScriptStatement>(this->iter_));
			break;
		case ScriptStatementCode::ScriptName: 
			AddLine(std::make_unique<ScriptNameStatement>(this->iter_));
			break;
		case ScriptStatementCode::Return:
		case ScriptStatementCode::Ref: 
			AddLine(std::make_unique<ScriptStatement>(this->iter_));
			break;
		}
	}
	else
	{
		AddLine(std::make_unique<ScriptCommandCall>(this->iter_));
	}
	++this->iter_;
}
