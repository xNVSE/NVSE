#include "ScriptAnalyzer.h"

#include <format>
#include <numeric>
#include <ranges>

#include "Commands_Scripting.h"
#include "FunctionScripts.h"
#include "ScriptUtils.h"

std::stack<ScriptParsing::ScriptAnalyzer*> g_analyzerStack;

ScriptParsing::ScriptAnalyzer* GetAnalyzer()
{
	if (g_analyzerStack.empty())
		return nullptr;
	return g_analyzerStack.top();
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
	if (code > ScriptParsing::ScriptStatementCode::Max)
		return nullptr;
	return &g_scriptStatementCommandInfos[static_cast<unsigned int>(code) - 0x10];
}

CommandInfo* GetEventCommandInfo(UInt16 opcode)
{
	if (opcode > 37)
		return nullptr;
	return &g_eventBlockCommandInfos[opcode];
}

void UnformatString(std::string& str)
{
	ReplaceAll(str, "\n", "%r");
	ReplaceAll(str, "\"", "%q");
}

bool ScriptParsing::ScriptContainsCommand(Script* script, CommandInfo* info, CommandInfo* eventBlock)
{
	ScriptAnalyzer analyzer(script);
	if (analyzer.error)
		return false;
	if (analyzer.CallsCommand(info, eventBlock))
		return true;
	return false;
}

bool ScriptParsing::PluginDecompileScript(Script* script, SInt32 lineNumber, char* buffer, UInt32 bufferSize)
{
	ScriptAnalyzer analyzer(script);
	if (analyzer.error)
		return false;
	if (lineNumber != -1)
	{
		if (lineNumber >= analyzer.lines.size())
			return false;
		auto str = analyzer.lines.at(lineNumber)->ToString();
		if (str.size() > bufferSize)
			return false;
		strcpy_s(buffer, bufferSize, str.c_str());
		return true;
	}
	auto str = analyzer.DecompileScript();
	if (str.size() > bufferSize)
		return false;
	strcpy_s(buffer, bufferSize, str.c_str());
	return true;

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
	if (!script->data)
		return;
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
	startData = curData;
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script) : script(script), curData(script->data), startData(curData)
{
	ReadLine();
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script, UInt8* position) : script(script), curData(position), startData(position)
{
	ReadLine();
}

ScriptParsing::ScriptIterator::ScriptIterator(Script* script, UInt16 opcode, UInt16 length, UInt16 refIdx, UInt8* data): opcode(opcode), length(length), refIdx(refIdx), script(script), curData(data), startData(data)
{
}

ScriptParsing::ScriptIterator::ScriptIterator(): script(nullptr), curData(nullptr), startData(nullptr)
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

ScriptParsing::ScriptLine::ScriptLine(const ScriptIterator& context, CommandInfo* cmd): context(context), cmdInfo(cmd)
{
	if (!cmd)
		error = true;
}

std::string ScriptParsing::ScriptLine::ToString()
{
	return std::string(this->cmdInfo->longName);
}

ScriptParsing::ScriptCommandCall::ScriptCommandCall(const ScriptIterator& contextParam):
    ScriptLine(contextParam, g_scriptCommands.GetByOpcode(contextParam.opcode)), commandCallToken(std::make_unique<CommandCallToken>(contextParam))
{
	if (error)
		return;
	if (context.length)
	{
		if (!commandCallToken->ParseCommandArgs(context, context.length))
			error = true;
	}
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
	if (error)
		return;
	const auto eventOpcode = this->context.Read16();
	this->eventBlockCmd = GetEventCommandInfo(eventOpcode);
	this->endJumpLength = this->context.Read32();
	
	this->commandCallToken = std::make_unique<CommandCallToken>(eventBlockCmd, eventBlockCmd ? eventBlockCmd->opcode : -1, nullptr, context.script);
	if (context.length > 6) // if more than eventOpcode and endJumpLength
	{
		const ScriptIterator iter(context.script, eventBlockCmd->opcode, context.length, 0, context.curData);
		if (!commandCallToken->ParseCommandArgs(iter, iter.length - 6))
			error = true;
	}
}


std::string ScriptParsing::BeginStatement::ToString()
{
	auto compilerOverrideEnabled = GetAnalyzer() ? GetAnalyzer()->compilerOverrideEnabled : false;
	return ScriptLine::ToString() + " " + (compilerOverrideEnabled ? "_" : "") + (this->commandCallToken ? this->commandCallToken->ToString() : "");
}

ScriptParsing::ExpressionToken::ExpressionToken(ExpressionCode code): code(code)
{
}

ScriptParsing::OperatorToken::OperatorToken(ScriptOperator* scriptOperator): scriptOperator(scriptOperator)
{
	if (!scriptOperator)
		error = true;
}

std::string ScriptParsing::OperatorToken::ToString(std::vector<std::string>& stack)
{
	if (scriptOperator->code == ScriptOperatorCode::kOp_Tilde)
	{
		if (stack.empty())
			return "<invalid number of operands on stack>";
		const auto topOp = stack.back();
		stack.pop_back();
		return '-' + topOp;
	}
	if (stack.size() < 2)
		return "<invalid number of operands on stack>";
	const auto rhOperand = stack.back();
	stack.pop_back();
	const auto lhOperand = stack.back();
	stack.pop_back();
	return lhOperand + " " + scriptOperator->operatorString + " " + rhOperand;
}

std::string ScriptParsing::ScriptVariableToken::GetBackupName()
{
	auto* analyzer = GetAnalyzer();
	std::string prefix = "i";
	if (analyzer && analyzer->arrayVariables.contains(this->varInfo))
		prefix = "a";
	else if (analyzer && analyzer->stringVariables.contains(this->varInfo))
		prefix = "s";
	else if (varInfo->idx == Script::eVarType_Float)
	{
		if (varInfo->IsReferenceType(this->script))
			prefix = "r";
		else
			prefix = "f";
	}
	return prefix + "Var" + std::to_string(varInfo->idx);
}

ScriptParsing::ScriptVariableToken::ScriptVariableToken(Script* script, ExpressionCode varType, VariableInfo* info, TESForm* ref) : OperandToken(varType), script(script),
                                                                                                                    ref(ref), varInfo(info)
{
	if (!this->varInfo || !this->script)
		error = true;
}

std::string ScriptParsing::ScriptVariableToken::ToString()
{
	if (!varInfo)
	{
		error = true;
		return "<failed to get variable>";
	}
	std::string varName = varInfo->name.CStr();
	if (varName.empty())
		varName = GetBackupName();
	if (ref)
		return std::string(ref->GetName()) + '.' + varName;
	return varName;
}

ScriptParsing::StringLiteralToken::StringLiteralToken(const std::string_view& stringLiteral): stringLiteral(
	stringLiteral)
{
}

std::string ScriptParsing::StringLiteralToken::ToString()
{
	std::string str(this->stringLiteral);
	UnformatString(str);
	return '"' + str + '"';
}

ScriptParsing::NumericConstantToken::NumericConstantToken(double value): value(value)
{
}

std::string DoubleToString(double x)
{
    char buf[0x100];
    sprintf(buf, "%.10f", x);
    char* p = buf + strlen(buf) - 1;
    while (*p == '0' && *p-- != '.');
    *(p+1) = '\0';
    if (*p == '.') *p = '\0';
    return buf;
}

std::string ScriptParsing::NumericConstantToken::ToString()
{
	return DoubleToString(value); // %g starts using scientific notation - bad!
}

ScriptParsing::RefToken::RefToken(Script* script, Script::RefVariable* refVariable): refVariable(refVariable), script(script)
{
	if (!refVariable)
		error = true;
}

std::string ScriptParsing::RefToken::ToString()
{
	if (refVariable->varIdx && script)
	{
		auto* varInfo = script->GetVariableInfo(refVariable->varIdx);
		if (!varInfo)
		{
			error = true;
			return FormatString("<failed to get var info index %d>", refVariable->varIdx);
		}
		ScriptVariableToken scriptVarToken(script, ExpressionCode::None, varInfo, nullptr);
		return scriptVarToken.ToString();
	}
	if (refVariable->form)
	{
		auto* editorId =  refVariable->form->GetEditorID();
		if (!editorId || StrLen(editorId) == 0)
			return FormatString("<%X>", refVariable->form->refID);
		return editorId;
	}
	error = true;
	return "<failed to resolve ref list entry>";
}

ScriptParsing::GlobalVariableToken::GlobalVariableToken(Script::RefVariable* refVariableParam): RefToken(nullptr, refVariableParam),
                                                                                           global(DYNAMIC_CAST(refVariableParam ? refVariableParam->form : nullptr, TESForm, TESGlobal))
{
	if (!global)
		error = true;
		
}
std::string ScriptParsing::GlobalVariableToken::ToString()
{
	return global->name.CStr();
}

ScriptParsing::InlineExpressionToken::~InlineExpressionToken() = default;

ScriptParsing::InlineExpressionToken::InlineExpressionToken(ScriptIterator& context)
{
	UInt32 offset = context.curData - context.script->data;
	this->eval = std::make_unique<ExpressionEvaluator>(context.script->data, context.script, &offset);
	this->eval->m_inline = true;
	this->tokens = eval->GetTokens(nullptr);
	if (!tokens)
		error = true;
	context.curData = this->eval->m_data;
}

std::string ScriptParsing::InlineExpressionToken::ToString()
{
	if (!this->tokens)
	{
		error = true;
		return "<failed to decompile inline expression>";
	}
	return eval->GetLineText(*this->tokens, nullptr);
}

ScriptParsing::CommandCallToken::CommandCallToken(CommandInfo* cmdInfo, UInt32 opcode, Script::RefVariable* callingRef, Script* script) :
	opcode(opcode), cmdInfo(cmdInfo)
{
	if (!cmdInfo)
		error = true;
	if (callingRef)
		callingReference = std::make_unique<RefToken>(script, callingRef);
}

ScriptParsing::CommandCallToken::CommandCallToken(const ScriptIterator& iter): CommandCallToken(g_scriptCommands.GetByOpcode(iter.opcode), iter.opcode, iter.GetCallingReference(), iter.script)
{
	opcode = iter.opcode;
}

template <typename T, typename F>
std::string TokenListToString(std::vector<T>& tokens, F&& f)
{
	auto i = 0u;
	return std::accumulate(tokens.begin(), tokens.end(), std::string(), [&](const std::string& a, T& b)
	{
		return a + ' ' + f(b, i++);
	});
}

std::string StringForNumericParam(ParamType typeID, int value)
{
	switch (typeID)
	{
	case kParamType_ActorValue:
		{
			return g_actorValues[value]->infoName;
		}
	case kParamType_Axis:
		{
			return value == 'X' ? "X" : value == 'Y' ? "Y" : value == 'Z' ? "Z" : "<unknown axis>";
		}
	case kParamType_AnimationGroup:
		{
			return TESAnimGroup::StringForAnimGroupCode(value);
		}
	case kParamType_Sex:
		{
			return value == 0 ? "Male" : value == 1 ? "Female" : "<invalid sex>";
		}
	case kParamType_MiscellaneousStat:
		{	
			if (value < 43)
				return FormatString("\"%s\"", ((const char**)0x1189280)[value]);
			break;
		}
	case kParamType_CriticalStage:
		{
			if (value < 5)
				return ((const char**)0x119BBB0)[value];
			break;
		}
	default:
		break;
	}
	return "";
}

std::string ScriptParsing::CommandCallToken::ToString()
{
	if (!cmdInfo)
		return FormatString("UNK_OPCODE_%04X", this->opcode);
	std::string refStr;
	if (callingReference)
		refStr = callingReference->ToString() + '.';
	std::string cmdName = cmdInfo->longName[0] ? cmdInfo->longName : FormatString("UNK_OPCODE_%04X", this->opcode);
	if (this->cmdInfo->execute == kCommandInfo_Function.execute)
	{
		return cmdName + " {" + TokenListToString(this->args, [&](auto& token, UInt32 i)
		{
			return token->ToString() + (i != args.size() - 1 ? "," : "");
		}) + " }";
	}
	refStr += cmdName;
	if (this->expressionEvaluator)
	{
		return refStr + TokenListToString(this->expressionEvalArgs, [&](CachedTokens* t, UInt32 callCount)
		{
			auto text = this->expressionEvaluator->GetLineText(*t, nullptr);
			//if (expressionEvalArgs.size() > 1 && t->Size() > 1)
			//	text = '(' + text + ')';
			return text;
		});
	}
	return refStr + TokenListToString(this->args, [&](auto& token, UInt32 i)
	{
		auto& arg = *token;
		const auto typeID = static_cast<ParamType>(cmdInfo->params[i].typeID);
		if (auto* numToken = dynamic_cast<NumericConstantToken*>(&arg))
		{
			const auto value = static_cast<int>(numToken->value);
			if (auto str = StringForNumericParam(typeID, value); !str.empty())
				return str;
		}
		return arg.ToString();
	});
}

ScriptOperator* ScriptParsing::Expression::ReadOperator()
{
	auto* prevPos = context.curData;
	const auto isNonOperatorChar = [](unsigned char c) { return c <= 0x20 || isspace(c) || !ispunct(c); };
	if (isNonOperatorChar(*context.curData))
		return nullptr;
	ScriptOperator* result = nullptr;
	auto maxCharsMatched = 0u;
	auto* retPos = prevPos;
	for (auto& op : g_gameScriptOperators)
	{
		auto curCharsMatched = 0u;
		const auto len = strlen(op.operatorString);
		bool match = true;
		for (auto i = 0u; i < len; ++i)
		{
			const auto opChar = op.operatorString[i];
			const auto curChar = context.Read8();
			if (opChar != static_cast<char>(curChar))
			{
				match = false;
				break;
			}
			++curCharsMatched;
		}
		if (match && curCharsMatched > maxCharsMatched) // match >= over > if input is that
		{
			result = &op;
			maxCharsMatched = curCharsMatched;
			retPos = context.curData;
		}
		context.curData = prevPos;
	}
	context.curData = retPos;
	return result;
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
	args.push_back(std::make_unique<ScriptVariableToken>(context.script, type, var, ref));
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
		args.push_back(std::make_unique<NumericConstantToken>(static_cast<int>(context.Read32())));
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

bool ScriptParsing::CommandCallToken::ParseGameArgs(ScriptIterator& context, UInt32 numArgs)
{
	if (!cmdInfo->params)
		return true;
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
		if (args.back()->error)
			return false;
	}
	return true;
}

ScriptParsing::CommandCallToken::~CommandCallToken() = default;

bool ScriptParsing::ScriptIterator::ReadStringLiteral(std::string_view& out)
{
	const auto strLen = Read16();
	if (strLen > 0x200)
		return false;
	out = std::string_view(reinterpret_cast<char*>(curData), strLen);
	curData += strLen;
	return true;
}

void RegisterNVSEVar(VariableInfo* info, Script::VariableType type)
{
	if (auto* analyzer = GetAnalyzer())
	{
		if (type == Script::eVarType_Array)
			analyzer->arrayVariables.insert(info);
		else if (type == Script::eVarType_String)
			analyzer->stringVariables.insert(info);
	}

}

void RegisterNVSEVars(CachedTokens& tokens, Script* script)
{
	if (auto* analyzer = GetAnalyzer())
	{
		for (auto iter = tokens.Begin(); !iter.End(); ++iter)
		{
			auto* token = iter.Get().token;
			if (token->type == kTokenType_ArrayVar)
				analyzer->arrayVariables.insert(script->GetVariableInfo(token->varIdx));
			else if (token->type == kTokenType_StringVar)
				analyzer->stringVariables.insert(script->GetVariableInfo(token->varIdx));
			else if (token->type == kTokenType_Command)
			{
				auto cmdCallToken = token->GetCallToken(script);
				for (auto* arg : cmdCallToken.expressionEvalArgs)
					RegisterNVSEVars(*arg, script);
			}
		}

	}
}

const UInt32 g_gameParseCommands[] = {0x5B1BA0, 0x5B3C70, 0x5B3CA0, 0x5B3C40, 0x5B3CD0, reinterpret_cast<UInt32>(Cmd_Default_Parse) };
const UInt32 g_messageBoxParseCmds[] = { 0x5B3CD0, 0x5B3C40, 0x5B3C70, 0x5B3CA0 };
bool ScriptParsing::CommandCallToken::ParseCommandArgs(ScriptIterator context, UInt32 dataLen)
{
	if (!cmdInfo)
		return false;
	if (cmdInfo->execute == kCommandInfo_Function.execute)
	{
		FunctionInfo info(context.script);
		for (auto& param : info.m_userFunctionParams)
		{
			auto* varInfo = context.script->GetVariableInfo(param.varIdx);
			RegisterNVSEVar(varInfo, param.varType);
			args.push_back(std::make_unique<ScriptVariableToken>(context.script, ExpressionCode::None, varInfo));
			if (args.back()->error)
				return false;
		}
		return true;
	}
	const auto defaultParse = Contains(g_gameParseCommands, reinterpret_cast<UInt32>(cmdInfo->parse));
	auto compilerOverride = false;
	if (defaultParse && *reinterpret_cast<UInt16*>(context.curData) > 0x7FFF) // compiler override
	{
		compilerOverride = true;
		context.curData += 2;
		if (auto* analyzer = GetAnalyzer())
			analyzer->compilerOverrideEnabled = true;
	}
	if (!defaultParse || compilerOverride)
	{
		if (cmdInfo->parse == kCommandInfo_While.parse)
			context.curData += 4;
		UInt32 offset = context.curData - context.script->data;
		this->expressionEvaluator = std::make_unique<ExpressionEvaluator>(context.script->data, context.script, &offset);
		if (cmdInfo->parse == kCommandInfo_Call.parse)
		{
			const auto callerVersion = this->expressionEvaluator->ReadByte();
			auto* tokens = this->expressionEvaluator->GetTokens(nullptr);
			if (!tokens)
				return false;
			this->expressionEvalArgs.push_back(tokens);
		}
		const auto exprEvalNumArgs = this->expressionEvaluator->ReadByte();
		for (auto i = 0u; i < exprEvalNumArgs; ++i)
		{
			auto* tokens = this->expressionEvaluator->GetTokens(nullptr);
			if (!tokens)
				return false;
			this->expressionEvalArgs.push_back(tokens);
		}
		for (auto* token : this->expressionEvalArgs)
			RegisterNVSEVars(*token, context.script);
		return true;
	}
	const auto numArgs = context.Read16();
	if (!ParseGameArgs(context, numArgs))
		return false;
	if (context.curData < context.startData + dataLen && Contains(g_messageBoxParseCmds, reinterpret_cast<UInt32>(cmdInfo->parse)))
	{
		// message box args?
		const auto numArgs = context.Read16();
		if (numArgs > 9)
			return false;
		for (auto i = 0u; i < numArgs; ++i)
		{
			if (!ReadNumericToken(context))
				return false;
		}
	}
	return true;
}

bool ScriptParsing::ScriptIterator::ReadNumericConstant(double& result, UInt8* endData)
{
	char buf[0x201];
	const auto len = endData - curData;
	if (len > 0x200)
		return false;
	std::memcpy(buf, curData, len);
	buf[len] = 0;
	char* endPtr;
	result = strtod(buf, &endPtr);
	if (endPtr == buf)
		return false;
	curData += endPtr - buf;
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
				this->stack.push_back(std::make_unique<ScriptVariableToken>(script, static_cast<ExpressionCode>(curChar),
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
			return false;
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
				auto token = std::make_unique<CommandCallToken>(g_scriptCommands.GetByOpcode(opcode), opcode, refVar, context.script);
				const auto dataLen = context.Read16();
				if (!token->error && dataLen && !token->ParseCommandArgs(context, dataLen))
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
		if (context.ReadNumericConstant(constant, endData))
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
	std::vector<ScriptOperator*> operatorStack;
	std::unordered_set<std::string> composites;
	for (auto iter = this->stack.begin(); iter != this->stack.end(); ++iter)
	{
		auto& token = *iter;
		if (auto* operatorToken = dynamic_cast<OperatorToken*>(token.get()))
		{
			if (!operatorStack.empty() && !operands.empty())
			{
				auto* lastOp = operatorStack.back();
				operatorStack.pop_back();
				if (operatorToken->scriptOperator->precedence > lastOp->precedence)
				{
					auto& rhOperand = operands.back();
					if (composites.contains(rhOperand))
						rhOperand = '(' + rhOperand + ')';
					if (operands.size() > 1)
					{
						auto& lhOperand = operands.at(operands.size() - 2);
						if (composites.contains(lhOperand))
							lhOperand = '(' + lhOperand + ')';
					}
				}
			}
			operatorStack.push_back(operatorToken->scriptOperator);
			auto result = operatorToken->ToString(operands);
			composites.insert(result);
			operands.push_back(result);
		}
		else if (auto* operandToken = dynamic_cast<OperandToken*>(token.get()))
		{
			operands.push_back(operandToken->ToString());
			CommandCallToken* callToken;
			// if it has optional params, wrap in parenthesis
			if (iter+1 != stack.end() && (callToken = dynamic_cast<CommandCallToken*>(token.get())))
			{
				std::span paramInfo{callToken->cmdInfo->params, callToken->cmdInfo->numParams};
				if (std::ranges::any_of(paramInfo, [](auto& p) {return p.isOptional;}))
				{
					operands.back() = '(' + operands.back() + ')';
				}
			}
		}
	}
	if (operands.size() != 1)
	{
		return "; failed to decompile expression, bad expression stack:" + TokenListToString(operands, [&](const std::string& s , unsigned i)
		{
			return i == operands.size() - 1 ? "'" + s + "'" : "'" + s + "',";
		});
	}
	return operands.back();
}

ScriptParsing::SetToStatement::SetToStatement(const ScriptIterator& contextParam): ScriptStatement(contextParam)
{
	if (error)
		return;
	type = *context.curData;
	if (type == 'r' || type == 's' || type == 'f')
	{
		TESForm* ref = nullptr;
		auto varType = ExpressionCode::None;
		auto* var = context.ReadVariable(ref, varType);
		this->toVariable = std::make_unique<ScriptVariableToken>(context.script, varType, var, ref);
	}
	else if (type == 'G')
	{
		++context.curData;
		this->toVariable = std::make_unique<GlobalVariableToken>(context.ReadRefVar());
	}
	else
	{
		error = true;
		return;
	}
	if (this->toVariable->error)
	{
		error = true;
		return;
	}
	expression = Expression(context);
	if (!expression.ReadExpression())
		error = true;
}

std::string ScriptParsing::SetToStatement::ToString()
{
	return "Set " + this->toVariable->ToString() + " To " + expression.ToString();
}

ScriptParsing::Expression& ScriptParsing::SetToStatement::GetExpression()
{
	return expression;
}

ScriptParsing::ConditionalStatement::ConditionalStatement(const ScriptIterator& contextParam): ScriptStatement(contextParam), expression(context)
{
	if (error)
		return;
	numBytesToJumpOnFalse = Read16();
	expression = Expression(context);
	if (!expression.ReadExpression())
			error = true;
}

std::string ScriptParsing::ConditionalStatement::ToString()
{
	return ScriptLine::ToString() + " " + expression.ToString();
}

ScriptParsing::Expression& ScriptParsing::ConditionalStatement::GetExpression()
{
	return expression;
}

bool DoAnyExpressionEvalTokensCallCmd(const std::vector<CachedTokens*>& tokenList, CommandInfo* cmd);

bool DoExpressionEvalTokensCallCmd(CachedTokens* tokens, CommandInfo* cmd)
{
	std::span exprEvalArgs {tokens->DataBegin(), tokens->Size()};
	return std::ranges::any_of(exprEvalArgs, [&](TokenCacheEntry& entry)
	{
		if (entry.token->type == kTokenType_Command)
		{
			if (entry.token->value.cmd == cmd)
				return true;
			if (auto* analyzer = GetAnalyzer())
			{
				const auto callToken = entry.token->GetCallToken(analyzer->script);
				if (DoAnyExpressionEvalTokensCallCmd(callToken.expressionEvalArgs, cmd))
					return true;
			}

		}
		return false;
	});
}

bool DoAnyExpressionEvalTokensCallCmd(const std::vector<CachedTokens*>& tokenList, CommandInfo* cmd)
{
	return std::ranges::any_of(tokenList, [&](CachedTokens* tokens)
	{
		return DoExpressionEvalTokensCallCmd(tokens, cmd);
	});
}

bool DoesTokenCallCmd(ScriptParsing::CommandCallToken& token, CommandInfo* cmd)
{
	if (token.cmdInfo == cmd)
		return true;
	const auto argsCallCmd = std::ranges::any_of(token.args, [&](auto& tokenPtr)
	{
		ScriptParsing::OperandToken& token = *tokenPtr;
		if (auto* inlineExpression = dynamic_cast<ScriptParsing::InlineExpressionToken*>(&token))
			if (DoExpressionEvalTokensCallCmd(inlineExpression->tokens, cmd))
				return true;
		return false;
	});
	if (argsCallCmd)
		return true;
	if (DoAnyExpressionEvalTokensCallCmd(token.expressionEvalArgs, cmd))
		return true;
	return false;
}

bool ScriptParsing::ScriptAnalyzer::CallsCommand(CommandInfo* cmd, CommandInfo* eventBlockInfo = nullptr)
{
	CommandInfo* lastEventBlock = nullptr;
	return std::ranges::any_of(this->lines, [&](std::unique_ptr<ScriptLine>& line)
	{
		BeginStatement* beginStatement;
		if (eventBlockInfo && (beginStatement = dynamic_cast<BeginStatement*>(line.get())))
			lastEventBlock = beginStatement->eventBlockCmd;
		if (eventBlockInfo && lastEventBlock && lastEventBlock != eventBlockInfo)
			return false;
		if (line->cmdInfo == cmd)
			return true;
		ScriptCommandCall* cmdCall; CommandCallToken* argsToken;
		if ((cmdCall = dynamic_cast<ScriptCommandCall*>(line.get())) && (argsToken = cmdCall->commandCallToken.get()) && DoesTokenCallCmd(*argsToken, cmd))
			return true;
		if ((beginStatement = dynamic_cast<BeginStatement*>(line.get())) && beginStatement->eventBlockCmd == cmd)
			return true;
		if (auto* expressionStatement = dynamic_cast<ExpressionStatement*>(line.get()))
		{
			auto& expression = expressionStatement->GetExpression();
			for (auto& token : expression.stack)
			{
				CommandCallToken* cmdCallToken;
				if (!(cmdCallToken = dynamic_cast<CommandCallToken*>(token.get())))
					continue;
				if (DoesTokenCallCmd(*cmdCallToken, cmd))
					return true;
			}
		}
		return false;
	});
}

ScriptParsing::ScriptAnalyzer::ScriptAnalyzer(Script* script, bool parse) : iter(script), script(script)
{
	if (!script->data)
	{
		this->error = true;
	}
	g_analyzerStack.push(this);
	if (parse && !this->error)
	{
		Parse();
	}
}

ScriptParsing::ScriptAnalyzer::~ScriptAnalyzer()
{
	if (!g_analyzerStack.empty())
		g_analyzerStack.pop();
}


void ScriptParsing::ScriptAnalyzer::Parse()
{
	while (!this->iter.End())
	{
		this->lines.push_back(ParseLine(this->iter));
		if (lines.back()->error)
			error = true;
		++this->iter;
	}
}

std::unique_ptr<ScriptParsing::ScriptLine> ScriptParsing::ScriptAnalyzer::ParseLine(const ScriptIterator& iter)
{
	const auto opcode =  iter.opcode;
	if (opcode <= static_cast<UInt32>(ScriptStatementCode::Ref))
	{
		const auto statementCode = static_cast<ScriptStatementCode>(opcode);
		switch (statementCode)
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
		case ScriptStatementCode::Else:
			return std::make_unique<ScriptStatement>(iter);
		case ScriptStatementCode::SetTo:
			return std::make_unique<SetToStatement>(iter);
		case ScriptStatementCode::If:
		case ScriptStatementCode::ElseIf:
			return std::make_unique<ConditionalStatement>(iter);
		case ScriptStatementCode::ScriptName: 
			return std::make_unique<ScriptNameStatement>(iter);
		}
	}
	return std::make_unique<ScriptCommandCall>(iter);
}

std::unique_ptr<ScriptParsing::ScriptLine> ScriptParsing::ScriptAnalyzer::ParseLine(UInt32 line)
{
	auto count = 0u;
	while (count < line && !this->iter.End())
	{
		++this->iter;
		++count;
	}
	if (line != count)
		return nullptr;
	return ParseLine(this->iter);
}

std::string GetTimeString()
{
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,sizeof buffer,"%d-%m-%Y %H:%M:%S", timeinfo);
	return buffer;
}

auto GetNVSEVersionString()
{
	return FormatString("%d.%d.%d", NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA);
}

std::string ScriptParsing::ScriptAnalyzer::DecompileScript()
{
	if (this->error)
		return "";
	auto* script = this->iter.script;
	std::string scriptText;
	auto numTabs = 0;
	const auto nestAddOpcodes = {static_cast<UInt32>(ScriptStatementCode::If), static_cast<UInt32>(ScriptStatementCode::Begin), kCommandInfo_While.opcode, kCommandInfo_ForEach.opcode};
	const auto nestMinOpcodes = {static_cast<UInt32>(ScriptStatementCode::EndIf), static_cast<UInt32>(ScriptStatementCode::End), kCommandInfo_Loop.opcode};
	const auto nestNeutralOpcodes = {static_cast<UInt32>(ScriptStatementCode::Else), static_cast<UInt32>(ScriptStatementCode::ElseIf)};
	UInt32 lastOpcode = 0;
	for (auto& it : this->lines)
	{
		const auto opcode = it->cmdInfo->opcode;
		if (opcode == kCommandInfo_Internal_PushExecutionContext.opcode || opcode == kCommandInfo_Internal_PopExecutionContext.opcode
			|| opcode == static_cast<UInt16>(ScriptStatementCode::ScriptName) && isLambdaScript)
			continue;
		const auto isMin = Contains(nestMinOpcodes, opcode);
		const auto isAdd = Contains(nestAddOpcodes, opcode);
		if (isAdd && !scriptText.ends_with("\n\n") && !Contains(nestAddOpcodes, lastOpcode) && !Contains(nestNeutralOpcodes, lastOpcode))
			scriptText += '\n';
		if (isMin)
			--numTabs;
		const auto isNeutral = Contains(nestNeutralOpcodes, opcode);
		if (isNeutral)
			--numTabs;
		if (numTabs < 0)
			numTabs = 0;

		if (!scriptText.ends_with("\n\n") && Contains(nestMinOpcodes, lastOpcode) && !Contains(nestMinOpcodes, opcode) && !Contains(nestNeutralOpcodes, opcode))
			scriptText += '\n';
		
		auto nextLine = it->ToString();
		if (it->error)
			nextLine += " ; there was an error decompiling this line\n";
		// adjust lambda script indent
		auto tabStr = std::string(numTabs, '\t');
		ReplaceAll(nextLine, "\n", '\n' + tabStr);
		scriptText += tabStr;

		if (!lastOpcode && !script->varList.Empty())
		{
			if (opcode == static_cast<UInt16>(ScriptStatementCode::ScriptName))
				scriptText += nextLine + '\n' + '\n';
			for (auto* var : script->varList)
			{
				if (!var)
					continue;
				ScriptVariableToken token(script, ExpressionCode::None, var, nullptr);
				if (arrayVariables.contains(var))
					scriptText += "Array_var ";
				else if (stringVariables.contains(var))
					scriptText += "String_var ";
				else if (var->IsReferenceType(script))
					scriptText += "Ref ";
				else if (var->type == Script::VariableType::eVarType_Float)
					scriptText += "Float ";
				else
					scriptText += "Int ";
				scriptText += token.ToString() + '\n';
			}
			scriptText += '\n';
			if (opcode != static_cast<UInt16>(ScriptStatementCode::ScriptName))
				scriptText += nextLine + '\n';
		}
		else scriptText += nextLine + '\n';

		if (isNeutral || isAdd)
			++numTabs;
		
		lastOpcode = opcode;
	}
	return scriptText;
}

size_t __stdcall ScriptParsing::DecompileToBuffer(Script* pScript, FILE* pStream, char* pBuffer)
{
	if (ScriptAnalyzer analyzer(pScript); !analyzer.error)
		if (auto compiledSrc = analyzer.DecompileScript(); !compiledSrc.empty())
		{
			compiledSrc += '\n';
			if (pStream)
				fwrite(compiledSrc.c_str(), compiledSrc.length(), 1, pStream);
			if (pBuffer)
				memcpy(pBuffer, compiledSrc.c_str(), compiledSrc.length() + 1);
			return compiledSrc.length();
		}
	if (pBuffer)
		*pBuffer = 0;
	return 0;
}
