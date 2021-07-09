#include "ScriptAnalyzer.h"

#include <numeric>
#include <ranges>

#include "Commands_Scripting.h"
#include "ScriptUtils.h"

template <typename L, typename T>
bool Contains(const L& list, T t)
{
	for (auto i : list)
	{
		if (i == t)
			return true;
	}
	return false;
}

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

float ReadFloat(UInt8*& curData)
{
	const auto float_ = *reinterpret_cast<float*>(curData);
	curData += 4;
	return float_;
}

double ReadDouble(UInt8*& curData)
{
	const auto double_ = *reinterpret_cast<double*>(curData);
	curData += 8;
	return double_;
}

CommandInfo* GetScriptStatement(ScriptParsing::ScriptStatementCode code)
{
	return &g_scriptStatementCommandInfos[static_cast<unsigned int>(code) - 0x10];
}

UInt8 ScriptParsing::ScriptIterator::Read8()
{
	return ::Read8(curData);
}

UInt16 ScriptParsing::ScriptIterator::Read16()
{
	return ::Read16(curData);
}

UInt32 ScriptParsing::ScriptIterator::Read32()
{
	return ::Read32(curData);
}

UInt64 ScriptParsing::ScriptIterator::Read64()
{
	return ::Read64(curData);
}

float ScriptParsing::ScriptIterator::ReadFloat()
{
	return ::ReadFloat(curData);
}

double ScriptParsing::ScriptIterator::ReadDouble()
{
	return ::ReadDouble(curData);
}

void ScriptParsing::ScriptIterator::ReadLine()
{
	opcode = Read16();
	if (opcode == static_cast<UInt16>(ScriptStatementCode::ReferenceFunction))
	{
		// follows different rules than rest for some reason
		refIdx = Read16();
		opcode = Read16();
		length = Read16();
	}
	else
	{
		refIdx = 0;
		length = Read16();
	}
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script) : script(script), curData(static_cast<UInt8*>(script->data))
{
	ReadLine();
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script, UInt8* position) : script(script), curData(position)
{
	ReadLine();
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script, UInt16 opcode, UInt16 length, UInt16 refIdx, UInt8* data): opcode(opcode), length(length), refIdx(refIdx), script(script), curData(data)
{
}

void ScriptParsing::ScriptIterator::operator++()
{
	curData += length;
	ReadLine();
}

bool ScriptParsing::ScriptIterator::End() const
{
	return curData > static_cast<UInt8*>(script->data) + script->info.dataLength;
}

UInt8 ScriptParsing::ScriptLine::Read8()
{
	return context.Read8();
}

UInt16 ScriptParsing::ScriptLine::Read16()
{
	return context.Read16();
}

UInt32 ScriptParsing::ScriptLine::Read32()
{
	return context.Read32();
}

ScriptParsing::ScriptLine::ScriptLine(const ScriptIterator& context, CommandInfo* cmd): context(context), statementCmd(cmd)
{
}

std::string ScriptParsing::ScriptLine::ToString()
{
	return std::string(this->statementCmd->longName);
}

ScriptParsing::ScriptCommandCall::ScriptCommandCall(const ScriptIterator& contextParam):
    ScriptLine(contextParam, g_scriptCommands.GetByOpcode(contextParam.opcode)), commandCallToken(std::make_unique<CommandCallToken>(contextParam))
{
	if (context.length)
		commandCallToken->ParseCommandArgs(context);
}

std::string ScriptParsing::ScriptCommandCall::ToString()
{
	return commandCallToken->ToString();
}

ScriptParsing::ScriptStatement::ScriptStatement(const ScriptIterator& contextParam) : ScriptLine(contextParam, GetScriptStatement(static_cast<ScriptStatementCode>(contextParam.opcode)))
{
}

ScriptParsing::ScriptNameStatement::ScriptNameStatement(const ScriptIterator& contextParam): ScriptStatement(contextParam)
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

ScriptParsing::ExpressionToken::ExpressionToken(ExpressionCode code): code(code)
{
}

ScriptParsing::OperatorToken::OperatorToken(ScriptOperator* scriptOperator): scriptOperator(scriptOperator)
{
}

std::string ScriptParsing::OperatorToken::ToString(std::vector<std::string>& stack)
{
	if (scriptOperator->code == ScriptOperatorCode::kOp_Tilde)
	{
		const auto topOp = stack.back();
		stack.pop_back();
		return '-' + topOp;
	}
	const auto rhOperand = stack.back();
	stack.pop_back();
	const auto lhOperand = stack.back();
	stack.pop_back();
	return lhOperand + " " + scriptOperator->operatorString + " " + rhOperand;
}

ScriptParsing::ScriptVariableToken::ScriptVariableToken(ExpressionCode varType, VariableInfo* info, TESForm* ref) : OperandToken(varType),
	ref(ref), varInfo(info)
{
}

std::string ScriptParsing::ScriptVariableToken::ToString()
{
	if (ref)
		return std::string(ref->GetName()) + '.' + std::string(varInfo->name.CStr());
	return varInfo->name.CStr();
}

ScriptParsing::StringLiteralToken::StringLiteralToken(const std::string_view& stringLiteral): stringLiteral(
	stringLiteral)
{
}

std::string ScriptParsing::StringLiteralToken::ToString()
{
	return std::string(this->stringLiteral);
}

ScriptParsing::NumericConstantToken::NumericConstantToken(double value): value(value)
{
}

std::string ScriptParsing::NumericConstantToken::ToString()
{
	return FormatString("%g", value);
}

ScriptParsing::RefToken::RefToken(Script* script, Script::RefVariable* refVariable): refVariable(refVariable), script(script)
{
}

std::string ScriptParsing::RefToken::ToString()
{
	if (refVariable->varIdx && script)
	{
		auto* varInfo = script->GetVariableInfo(refVariable->varIdx);
		if (!varInfo)
			return "";
		return varInfo->name.CStr();
	}
	if (refVariable->form)
	{
		if (refVariable->form->refID == 0x7)
			return "Player";
		if (refVariable->form->refID == 0x14)
			return "PlayerRef";
		return refVariable->form->GetName();
	}
	return "<failed to resolve ref list entry>";
}

ScriptParsing::GlobalVariableToken::GlobalVariableToken(Script::RefVariable* refVariableParam): RefToken(nullptr, refVariableParam),
                                                                                           global(DYNAMIC_CAST(refVariableParam->form, TESForm, TESGlobal))
{
}
std::string ScriptParsing::GlobalVariableToken::ToString()
{
	return global->name.CStr();
}

ScriptParsing::InlineExpressionToken::InlineExpressionToken(ScriptIterator& context)
{
	UInt32 offset = context.curData - static_cast<UInt8*>(context.script->data);
	this->eval = std::make_unique<ExpressionEvaluator>(nullptr, context.script->data, nullptr, nullptr, context.script, nullptr, nullptr, &offset);
	this->eval->m_inline = true;
	this->tokens = eval->GetTokens();
	context.curData = this->eval->m_data;
}

std::string ScriptParsing::InlineExpressionToken::ToString()
{
	return eval->GetLineText(*this->tokens, nullptr);
}

ScriptParsing::CommandCallToken::CommandCallToken(const ScriptIterator& iter): OperandToken(ExpressionCode::Command),
                                                                               cmdInfo(g_scriptCommands.GetByOpcode(iter.opcode)), callingReference(iter.GetCallingReference())
{
}

ScriptParsing::CommandCallToken::CommandCallToken(CommandInfo* cmdInfo, Script::RefVariable* callingRef) : cmdInfo(cmdInfo), callingReference(callingRef)
{
}

template <typename T, typename F>
std::string TokenListToString(std::vector<T>& tokens, F&& f)
{
	return std::accumulate(tokens.begin(), tokens.end(), std::string(), [&](const std::string& a, T& b)
	{
		return a + ' ' + f(b);
	});
}

bool StringForNumericParam(ParamType typeID, int value, std::string& argStr)
{
	switch (typeID)
	{
	case kParamType_ActorValue:
		{
			argStr += g_actorValueInfoArray[value]->infoName;
			return true;
		}
	case kParamType_AnimationGroup:
		{
			argStr += TESAnimGroup::StringForAnimGroupCode(value);
			return true;
		}
	case kParamType_Axis:
		{
			argStr += value == 88 ? "X" : value == 89? "Y" : value == 90 ? "Z" : "<unknown axis>";
			return true;
		}
	case kParamType_Sex:
		{
			argStr += value == 0 ? "Male" : value == 1? "Female" : "<invalid sex>";
			return true;
		}
	default: 
		break;
	}
	return false;
}

std::string ScriptParsing::CommandCallToken::ToString()
{
	if (this->expressionEvaluator)
	{
		return cmdInfo->longName + TokenListToString(this->expressionEvalArgs, [&](CachedTokens* t)
		{
			return this->expressionEvaluator->GetLineText(*t, nullptr);
		});
	}
	std::string argStr;
	for (auto i = 0u; i < args.size(); ++i)
	{
		auto& arg = *args[i];
		const auto typeID = static_cast<ParamType>(cmdInfo->params[i].typeID);
		argStr += ' ';
		auto* numToken = dynamic_cast<NumericConstantToken*>(&arg);
		if (numToken)
		{
			const auto value = static_cast<int>(numToken->value);
			if (StringForNumericParam(typeID, value, argStr))
				continue;
		}
		
		argStr += arg.ToString();
	}
	return cmdInfo->longName + argStr;
}

ScriptOperator* ScriptParsing::Expression::ReadOperator()
{
	auto* prevPos = context.curData;
	for (auto& op : g_gameScriptOperators)
	{
		auto match = true;
		for (auto i = 0; i < 2; ++i)
		{
			const auto opChar = op.operatorString[i];
			const auto curChar = static_cast<char>(context.Read8());
			if (curChar <= 0x20 || isspace(curChar))
			{
				--context.curData;
				break;
			}
			if (opChar != curChar)
			{
				match = false;
				break;
			}
		}
		if (match)
			return &op;
		context.curData = prevPos;
	}
	return nullptr;
}

Script::RefVariable* ScriptParsing::ScriptIterator::ReadRefVar()
{
	return script->GetRefFromRefList(Read16());
}

VariableInfo* ScriptParsing::ScriptIterator::ReadVariable(TESForm*& refOut, ExpressionCode& typeOut)
{
	Script* lScript = this->script;
	if (*curData == 'r') //reference to var in another script
	{
		++curData;
		Script::RefVariable* refVar = lScript->GetRefFromRefList(Read16());
		if (!refVar)
			return nullptr;
		auto* form = refVar->form;
		if (!form)
			return nullptr;
		lScript = refVar->GetReferencedScript();
		if (!lScript)
			return nullptr;
		refOut = form;
	}
	const auto varType = Read8();
	typeOut = static_cast<ExpressionCode>(varType);
	if (varType == 'f' || varType == 's')
		return lScript->GetVariableInfo(Read16());
	return nullptr;
}

bool ScriptParsing::CommandCallToken::ReadVariableToken(ScriptIterator& context)
{
	TESForm* ref = nullptr;
	auto type = ExpressionCode::None;
	auto* var = context.ReadVariable(ref, type);
	if (!var)
		return false;
	args.push_back(std::make_unique<ScriptVariableToken>(type, var, ref));
	return true;
}

bool ScriptParsing::CommandCallToken::ReadFormToken(ScriptIterator& context)
{
	const auto code = context.Read8();
	if (code == 'r')
	{
		args.push_back(std::make_unique<RefToken>(context.script, context.script->GetRefFromRefList(context.Read16())));
	}
	else
	{
		if (!ReadVariableToken(context))
			return false;
	}
	return true;
}

bool ScriptParsing::CommandCallToken::ReadNumericToken(ScriptIterator& context)
{
	const auto code = context.Read8();
	switch (code)
	{
	case 'n':
		args.push_back(std::make_unique<NumericConstantToken>(context.Read32()));
		break;
	case 'z':
		args.push_back(std::make_unique<NumericConstantToken>(context.ReadDouble()));
		break;
	case 'G':
		args.push_back(std::make_unique<GlobalVariableToken>(context.ReadRefVar()));
		break;
	default:
		--context.curData;
		if (!ReadVariableToken(context))
			return false;
		break;
	}
	return true;
}

bool ScriptParsing::ScriptIterator::ReadStringLiteral(std::string_view& out)
{
	const auto strLen = Read16();
	out = std::string_view(reinterpret_cast<char*>(curData), strLen);
	curData += strLen;
	return true;
}

static std::unordered_set<VariableInfo*> g_arrayVariables;
static std::unordered_set<VariableInfo*> g_stringVariables;

void RegisterNVSEVars(CachedTokens& tokens, Script* script)
{
	for (auto iter = tokens.Begin(); !iter.End(); ++iter)
	{
		auto* token = iter.Get().token;
		if (token->type == kTokenType_ArrayVar)
			g_arrayVariables.insert(script->GetVariableInfo(token->varIdx));
		else if (token->type == kTokenType_StringVar)
			g_stringVariables.insert(script->GetVariableInfo(token->varIdx));
	}
}

const UInt32 g_gameParseCommands[] = {0x5B1BA0, 0x5B3C70, 0x5B3CA0, 0x5B3C40, 0x5B3CD0, reinterpret_cast<UInt32>(Cmd_Default_Parse) };

bool ScriptParsing::CommandCallToken::ParseCommandArgs(ScriptIterator context)
{
	if (*reinterpret_cast<UInt16*>(context.curData) > 0x7FFF || !Contains(g_gameParseCommands, reinterpret_cast<UInt32>(cmdInfo->parse)))
	{
		UInt32 offset = context.curData - static_cast<UInt8*>(context.script->data);
		this->expressionEvaluator = std::make_unique<ExpressionEvaluator>(nullptr, context.script->data, nullptr, nullptr, context.script, nullptr, nullptr, &offset);
		const auto exprEvalNumArgs = this->expressionEvaluator->ReadByte();
		this->expressionEvalArgs.reserve(exprEvalNumArgs);
		for (auto i = 0u; i < exprEvalNumArgs; ++i)
			this->expressionEvalArgs.push_back(this->expressionEvaluator->GetTokens());
		for (auto* token : this->expressionEvalArgs)
			RegisterNVSEVars(*token, context.script);
		return true;
	}
	const auto numArgs = context.Read16();
	for (auto i = 0u; i < numArgs; ++i)
	{
		const auto typeID = static_cast<ExtractParamType>(kClassifyParamExtract[cmdInfo->params[i].typeID]);
		if (*reinterpret_cast<UInt16*>(context.curData) == 0xFFFF)
		{
			context.curData += 2;
			args.push_back(std::make_unique<InlineExpressionToken>(context));
			continue;
		}
		switch (typeID)
		{
		case kExtractParam_StringLiteral:
			{
				std::string_view stringLit;
				if (context.ReadStringLiteral(stringLit))
					args.push_back(std::make_unique<StringLiteralToken>(stringLit));
				break;
			}
		case kExtractParam_Int:
		case kExtractParam_Float:
		case kExtractParam_Double:
			ReadNumericToken(context);
			break;
		case kExtractParam_Short:
			args.push_back(std::make_unique<NumericConstantToken>(context.Read16()));
			break;
		case kExtractParam_Byte:
			args.push_back(std::make_unique<NumericConstantToken>(context.Read8()));
			break;
		case kExtractParam_Form:
			if (!ReadFormToken(context))
				return false;
			break;
		case kExtractParam_ScriptVariable:
			if (!ReadVariableToken(context))
				return false;
			break;
		}
	}
	return true;
}

bool ScriptParsing::ScriptIterator::ReadNumericConstant(double& result)
{
	char* endPtr;
	result = strtod(reinterpret_cast<char*>(curData), &endPtr);
	if (endPtr == reinterpret_cast<char*>(curData))
		return false;
	curData = reinterpret_cast<unsigned char*>(endPtr);
	return true;
}

Script::RefVariable* ScriptParsing::ScriptIterator::GetCallingReference() const
{
	if (!refIdx)
		return nullptr;
	return script->GetRefFromRefList(refIdx);
}

bool ScriptParsing::Expression::ReadExpression()
{
	expressionLength = context.Read16();
	auto* endData = context.curData + expressionLength;
	Script::RefVariable* refVar = nullptr;
	while (context.curData < endData)
	{
		auto curChar = context.Read8();
		if (curChar <= 0x20 || isspace(curChar))
			continue;
		switch (curChar)
		{
		case 's':
		case 'l':
		case 'f':
			{
				const auto varIdx = context.Read16();
				auto* script = refVar ? refVar->GetReferencedScript() : context.script;
				this->stack.push_back(std::make_unique<ScriptVariableToken>(static_cast<ExpressionCode>(curChar),
					script->GetVariableInfo(varIdx), refVar ? refVar->form : nullptr));
				refVar = nullptr;
				continue;
			}
		case 'G':
			{
				this->stack.push_back(std::make_unique<GlobalVariableToken>(context.ReadRefVar()));
				continue;
			}
		case 'Z':
			{
				this->stack.push_back(std::make_unique<RefToken>(context.script, context.ReadRefVar()));
				continue;
			}
		case 'r':
			{
				refVar = context.ReadRefVar();
				continue;
			}
		case 'n':
		case 'z':
			// no idea what these do
			DebugBreak();
			break;
		case '"':
			{
				std::string_view strLit;
				if (context.ReadStringLiteral(strLit))
					this->stack.push_back(std::make_unique<StringLiteralToken>(strLit));
				break;
			}
		case 'X':
			{
				const auto opcode = context.Read16();
				auto token = std::make_unique<CommandCallToken>(g_scriptCommands.GetByOpcode(opcode), refVar);
				const auto dataLen = context.Read16();
				if (dataLen && !token->ParseCommandArgs(context))
					return false;
				this->stack.push_back(std::move(token));
				context.curData += dataLen;
				refVar = nullptr;
				continue;
			}
		default:
			break;
		}
		
		--context.curData;
		auto* scriptOperator = ReadOperator();
		if (scriptOperator)
		{
			stack.push_back(std::make_unique<OperatorToken>(scriptOperator));
			continue;
		}

		double constant;
		if (context.ReadNumericConstant(constant))
		{
			stack.push_back(std::make_unique<NumericConstantToken>(constant));
			continue;
		}
		return false;
	}
	return true;
}

ScriptParsing::Expression::Expression(const ScriptIterator& context): context(context)
{
}

std::string ScriptParsing::Expression::ToString()
{
	std::vector<std::string> operands;
	for (auto& iter : this->stack)
	{
		if (auto* operatorToken = dynamic_cast<OperatorToken*>(iter.get()))
			operands.push_back(operatorToken->ToString(operands));
		else if (auto* operandToken = dynamic_cast<OperandToken*>(iter.get()))
			operands.push_back(operandToken->ToString());
	}
	if (operands.size() != 1)
		return "";
	return operands.back();
}

ScriptParsing::SetToStatement::SetToStatement(const ScriptIterator& contextParam): ScriptStatement(contextParam)
{
	type = *context.curData;
	if (type == 'r' || type == 's' || type == 'f')
	{
		TESForm* ref = nullptr;
		auto varType = ExpressionCode::None;
		auto* var = context.ReadVariable(ref, varType);
		this->toVariable = std::make_unique<ScriptVariableToken>(varType, var, ref);
	}
	else if (type == 'G')
	{
		this->toVariable = std::make_unique<GlobalVariableToken>(context.ReadRefVar());
	}
	else
	{
		DebugBreak();
	}
	
	expression = Expression(context);
	expression.ReadExpression();
}

std::string ScriptParsing::SetToStatement::ToString()
{
	return "Set " + this->toVariable->ToString() + " To " + expression.ToString();
}

ScriptParsing::ConditionalStatement::ConditionalStatement(const ScriptIterator& contextParam): ScriptStatement(contextParam), expression(context)
{
	numBytesToJumpOnFalse = Read16();
	expression = Expression(context);
	if (context.opcode != static_cast<UInt16>(ScriptStatementCode::Else))
		expression.ReadExpression();
}

ScriptParsing::ScriptAnalyzer::ScriptAnalyzer(Script* script) : iter_(script) {}



void ScriptParsing::ScriptAnalyzer::Parse()
{
	while (!this->iter_.End())
	{
		this->lines_.push_back(ParseLine(this->iter_));
		++this->iter_;
	}
}

std::unique_ptr<ScriptParsing::ScriptLine> ScriptParsing::ScriptAnalyzer::ParseLine(const ScriptIterator& iter)
{
	const auto opcode =  iter.opcode;
	if (opcode <= static_cast<UInt32>(ScriptStatementCode::Ref))
	{
		switch (static_cast<ScriptStatementCode>(opcode))
		{
		case ScriptStatementCode::Begin: 
			return std::make_unique<BeginStatement>(iter);
		case ScriptStatementCode::End: 
		case ScriptStatementCode::Short:
		case ScriptStatementCode::Long:
		case ScriptStatementCode::Float:
		case ScriptStatementCode::Return:
		case ScriptStatementCode::Ref:
		case ScriptStatementCode::EndIf:
		case ScriptStatementCode::ReferenceFunction: 
			return std::make_unique<ScriptStatement>(iter);
		case ScriptStatementCode::SetTo:
			return std::make_unique<SetToStatement>(iter);
		case ScriptStatementCode::If:
		case ScriptStatementCode::ElseIf:
		case ScriptStatementCode::Else:
			return std::make_unique<ConditionalStatement>(iter);
		case ScriptStatementCode::ScriptName: 
			return std::make_unique<ScriptNameStatement>(iter);
		}
	}
	return std::make_unique<ScriptCommandCall>(iter);
}

std::string ScriptParsing::ScriptAnalyzer::DecompileScript()
{
	auto* script = this->iter_.script;
	std::string scriptText;
	auto numTabs = 0;
	const auto nestAddOpcodes = {static_cast<UInt32>(ScriptStatementCode::If), static_cast<UInt32>(ScriptStatementCode::Begin), kCommandInfo_While.opcode, kCommandInfo_ForEach.opcode};
	const auto nestMinOpcodes = {static_cast<UInt32>(ScriptStatementCode::EndIf), static_cast<UInt32>(ScriptStatementCode::End), kCommandInfo_Loop.opcode};
	const auto nestNeutralOpcodes = {static_cast<UInt32>(ScriptStatementCode::Else), static_cast<UInt32>(ScriptStatementCode::ElseIf)};
	for (auto& iter : this->lines_)
	{
		const auto opcode = iter->statementCmd->opcode;
		const auto isMin = Contains(nestMinOpcodes, opcode);
		const auto isAdd = Contains(nestAddOpcodes, opcode);
		if (isAdd && !scriptText.ends_with("\n\n"))
			scriptText += '\n';
		if (isMin)
			--numTabs;
		const auto isNeutral = Contains(nestNeutralOpcodes, opcode);
		if (isNeutral)
			--numTabs;
		if (numTabs < 0)
			numTabs = 0;
		scriptText += std::string(numTabs, '\t') + iter->ToString();
		if (isMin && !scriptText.ends_with("\n\n"))
			scriptText += '\n';
		if (isNeutral || isAdd)
			++numTabs;

		scriptText += '\n';

		if (opcode == static_cast<UInt16>(ScriptStatementCode::ScriptName))
		{
			scriptText += '\n';
			for (auto* var : script->varList)
			{
				auto varName = std::string(var->name.CStr());
				if (g_arrayVariables.contains(var))
					scriptText += "Array_var " + varName;
				else if (g_stringVariables.contains(var))
					scriptText += "String_var " + varName;
				else
				{
					if (var->type == Script::VariableType::eVarType_Float)
					{
						if (var->IsReferenceType(script))
							scriptText += "Ref " + varName;
						else
							scriptText += "Float " + varName;
					}
					else
					{
						scriptText += "Int " + varName;
					}
				}
				
				scriptText += '\n';
			}
			scriptText += '\n';
			g_arrayVariables.clear();
			g_stringVariables.clear();
		}
	}
	return scriptText;
}
