#include "NVSECompiler.h"
#include "Commands_MiscRef.h"
#include "Commands_Scripting.h"
#include "NVSECompilerUtils.h"
#include "NVSEParser.h"


std::unordered_map<std::string, uint8_t> operatorMap {
	{"=", 0},
	{"||", 1},
	{"&&", 2},
	{":", 3},
	{"==", 4},
	{"!=", 5},
	{">", 6},
	{"<", 7},
	{">=", 8},
	{"<=", 9},
	{"|", 10},
	{"&", 11},
	{"<<", 12},
	{">>", 13},
	{"+", 14},
	{"-", 15},
	{"*", 16},
	{"/", 17},
	{"%", 18},
	{"^", 19},
	{"-", 20},
	{"!", 21},
	{"(", 22},
	{")", 23},
	{"[", 24},
	{"]", 25},
	{"<-", 26},
	{"$", 27},
	{"+=", 28},
	{"*=", 29},
	{"/=", 30},
	{"^=", 31},
	{"-=", 32},
	{"#", 33},
	{"*", 34},
	{"->", 35},
	{"::", 36},
	{"&", 37},
	{"{", 38},
	{"}", 39},
	{".", kOpType_Dot},
	{"|=", 41},
	{"&=", 42},
	{"%=", 43},
};

bool NVSECompiler::Compile() {
	insideNvseExpr.push(false);
	ast.Accept(this);

	script->SetEditorID(scriptName.c_str());

	script->info.compiled = true;
	script->info.dataLength = data.size();
	script->info.numRefs = script->refList.Count();
	script->info.varCount = script->varList.Count();
	script->info.unusedVariableCount = script->info.varCount - usedVars.size();
	script->data = static_cast<uint8_t*>(FormHeap_Allocate(data.size()));
	memcpy(script->data, data.data(), data.size());

	// Debug print local info
	printFn("[Locals]\n");
	for (int i = 0; i < script->varList.Count(); i++) {
		auto item = script->varList.GetNthItem(i);
		printFn(std::format("{}: {} {}\n", item->idx, item->name.CStr(), usedVars.contains(item->name.CStr()) ? "" : "(unused)"));
	}
	printFn("\n");

	// Refs
	printFn("[Refs]\n");
	for (int i = 0; i < script->refList.Count(); i++) {
		const auto ref = script->refList.GetNthItem(i);
		if (ref->varIdx) {
			printFn(std::format("{}: (Var {})\n", i, ref->varIdx));
		} else {
			printFn(std::format("{}: {}\n", i, ref->form->GetEditorID()));
		}
	}
	printFn("\n");

	// Script data
	printFn("[Data]\n");
	for (int i = 0; i < script->info.dataLength; i++) {
		printFn(std::format("{:02x} ", script->data[i]));
	}
	printFn("\n");

	printFn(std::format("\nNum compiled bytes: {}\n", script->info.dataLength));

	return true;
}

void NVSECompiler::VisitNVSEScript(const NVSEScript* nvScript) {
	// Compile the script name
	scriptName = nvScript->name.lexeme;

	// Dont allow naming script the same as another form, unless that form is the script itself
	auto comp = strcmp(scriptName.c_str(), originalScriptName);
	if (ResolveObjReference(scriptName, false) && comp) {
		throw std::runtime_error(std::format("Error: Form name '{}' is already in use.\n", scriptName.c_str()));
	}

	// SCN
	AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

	for (auto& global_var : nvScript->globalVars) {
		global_var->Accept(this);
	}

	for (auto& block : nvScript->blocks) {
		block->Accept(this);
	}
}

void NVSECompiler::VisitBeginStmt(const BeginStmt* stmt) {
	auto name = stmt->name.lexeme;

	// Shouldn't be null
	CommandInfo *beginInfo = nullptr;
	for (auto &info : g_eventBlockCommandInfos) {
		if (!strcmp(info.longName, name.c_str())) {
			beginInfo = &info;
		}
	}

	// OP_BEGIN
	AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

	auto beginPatch = AddU16(0x0);
	auto beginStart = data.size();

	AddU16(beginInfo->opcode);

	auto blockPatch = AddU32(0x0);

	// Add param shit here?
	if (stmt->param.has_value()) {
		auto param = stmt->param.value();

		AddU16(0xFFFF);
		auto varStart = data.size();
		auto varPatch = AddU16(0x0);

		if (beginInfo->params[0].typeID == kParamType_Integer) {
			AddU8('L');
			AddU32(static_cast<uint32_t>(std::get<double>(param.value)));
		}

		// All other instances use ref?
		else {
			// Try to resolve the global ref
			auto ref = ResolveObjReference(param.lexeme);
			if (ref) {
				auto refInfo = script->refList.GetNthItem(ref - 1);
				printf("Resolved ref %s: %08x\n", param.lexeme.c_str(), refInfo->form->refID);
				AddU8('R');
				AddU16(ref);
			} else {
				throw std::runtime_error(std::format("Unable to resolve form '{}'.", param.lexeme));
			}
		}

		SetU16(varPatch, data.size() - varStart);
	}

	SetU16(beginPatch, data.size() - beginStart);
	auto blockStart = data.size();
	stmt->block->Accept(this);

	// OP_END
	AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::End));

	SetU32(blockPatch, data.size() - blockStart);
}

void NVSECompiler::VisitFnStmt(FnDeclStmt* stmt) {
	// OP_BEGIN
	AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

	// OP_MODE_LEN
	auto opModeLenPatch = AddU16(0x0);
	auto opModeStart = data.size();

	// OP_MODE
	AddU16(0x0D);

	// SCRIPT_LEN
	auto scriptLenPatch = AddU32(0x0);

	// BYTECODE VER
	AddU8(0x1);

	// arg count
	AddU8(stmt->args.size());

	// add args
	for (auto i = 0; i < stmt->args.size(); i++) {
		auto& arg = stmt->args[i];
		auto type = GetScriptTypeFromToken(arg->type);
		auto localIdx = AddLocal(arg->name.lexeme, type);

		AddU16(localIdx);
		AddU8(type);
	}

	// NUM_ARRAY_ARGS, always 0
	AddU8(0x0);

	auto scriptLenStart = data.size();

	// Patch mode len
	SetU16(opModeLenPatch, data.size() - opModeStart);

	// Compile script
	CompileBlock(stmt->body, false);
	
	// OP_END
	AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::End));

	SetU32(scriptLenPatch, data.size() - scriptLenStart);
}

void NVSECompiler::VisitVarDeclStmt(const VarDeclStmt* stmt) {
	uint8_t varType = GetScriptTypeFromToken(stmt->type);
	const auto token = stmt->name;

	// TODO: Typecheker
	if (ResolveObjReference(token.lexeme)) {
		throw std::runtime_error(std::format("Error: Name '{}' is already in use by form.", token.lexeme));
	}

	// Compile lambdas differently
	// Does not affect params as they cannot have value specified
	if (stmt->value->IsType<LambdaExpr>()) {
		// To do a similar thing on access
		lambdaVars.insert(stmt->name.lexeme);
		
		// <scriptName>__lambdaName
		const auto name = scriptName + "__" + token.lexeme;

		// Build a call expr
		NVSEToken callToken{};
		callToken.lexeme = "SetModLocalData";
		auto ident = std::make_shared<IdentExpr>(callToken);
		
		NVSEToken nameToken{};
		nameToken.value = name;
		auto arg = std::make_shared<StringExpr>(nameToken);

		std::vector<ExprPtr> args{};
		args.emplace_back(arg);
		args.emplace_back(stmt->value);
		auto expr = std::make_shared<CallExpr>(ident, std::move(args));

		expr->Accept(this);
		return;
	}

	const auto idx = AddLocal(token.lexeme, varType);

	if (!stmt->value) {
		return;
	}

	// OP_LET
	AddU16(0x1539);

	//  Placeholder [expr_len]
	auto exprPatch = AddU16(0x0);
	auto exprStart = data.size();

	// Num args
	AddU8(0x1);

	// Placeholder [param1_len]
	auto argStart = data.size();
	auto argPatch = AddU16(0x0);

	// Add arg to stack
	AddU8('V');
	AddU8(varType);
	AddU16(0); // SCRV
	AddU16(idx); // SCDA

	// Build expression
	insideNvseExpr.push(true);
	stmt->value->Accept(this);
	insideNvseExpr.pop();

	// OP_ASSIGN
	AddU8(0);

	// Patch lengths
	SetU16(argPatch, data.size() - argStart);
	SetU16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::VisitExprStmt(const ExprStmt* stmt) {
	// These do not need to be wrapped in op_eval
	if (stmt->expr->IsType<CallExpr>() || stmt->expr->IsType<AssignmentExpr>()) {
		stmt->expr->Accept(this);
	} else {
		// OP_EVAL
		AddU16(0x153A);
		
		// Placeholder OP_LEN
		auto exprPatch = AddU16(0x0);
		auto exprStart = data.size();
	
		// Num args
		AddU8(0x1);
	
		// Placeholder [arg_len]
		auto argStart = data.size();
		auto argPatch = AddU16(0x0);

		//Build expression
	    stmt->expr->Accept(this);

		// Patch lengths
		SetU16(argPatch, data.size() - argStart);
		SetU16(exprPatch, data.size() - exprStart);
	}
}

void NVSECompiler::VisitForStmt(const ForStmt* stmt) {
	if (stmt->init) {
		stmt->init->Accept(this);
	}

	// Convert for into while
	if (stmt->post) {
		stmt->block->statements.push_back(std::make_shared<ExprStmt>(stmt->post));
	}
	auto whilePtr = std::make_shared<WhileStmt>(stmt->cond, stmt->block);
	whilePtr->Accept(this);
}

void NVSECompiler::VisitIfStmt(IfStmt* stmt) {
	// OP_IF
	AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::If));

	// Placeholder OP_LEN
	auto exprPatch = AddU16(0x0);
	auto exprStart = data.size();

	// Placeholder JMP_OPS
	auto jmpPatch = AddU16(0x0);

	// Placeholder EXP_LEN
	auto compPatch = AddU16(0x0);
	auto compStart = data.size();

	// OP_PUSH
	AddU8(0x20);

	// Enter NVSE eval
	AddU8(0x58);
	{
		// OP_EVAL
		AddU16(0x153A);

		// Placeholder OP_EVAL_LEN
		auto opEvalPatch = AddU16(0x0);
		auto opEvalStart = data.size();

		// OP_EVAL num params
		AddU8(0x1);

		// Placeholder OP_EVAL_ARG_LEN
		auto opEvalArgStart = data.size();
		auto opEvalArgPatch = AddU16(0x0);

		// Compile cond
		insideNvseExpr.push(true);
		stmt->cond->Accept(this);
		insideNvseExpr.push(false);

		// Patch OP_EVAL lengths
		SetU16(opEvalArgPatch, data.size() - opEvalArgStart);
		SetU16(opEvalPatch, data.size() - opEvalStart);
	}

	// Patch lengths
	SetU16(compPatch, data.size() - compStart);
	SetU16(exprPatch, data.size() - exprStart);

	// Patch JMP_OPS
	SetU16(jmpPatch, CompileBlock(stmt->block, true));

	// Build OP_ELSE
	if (stmt->elseBlock) {
		// OP_ELSE
		AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Else));

		// OP_LEN
		AddU16(0x02);

		// Placeholder JMP_OPS
		auto elsePatch = AddU16(0x0);

		// Compile else block
		SetU16(elsePatch, CompileBlock(stmt->elseBlock, true));
	}

	// OP_ENDIF
	AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::EndIf));
	AddU16(0x0);
}

void NVSECompiler::VisitReturnStmt(const ReturnStmt* stmt) {
	
}

void NVSECompiler::VisitWhileStmt(const WhileStmt* stmt) {
	// OP_WHILE
	AddU16(0x153B);

	// Placeholder OP_LEN
	auto exprPatch = AddU16(0x0);
	auto exprStart = data.size();

	auto jmpPatch = AddU32(0x0);

	// 1 param
	AddU8(0x1);

	// Compile / patch condition
	auto condStart = data.size();
	auto condPatch = AddU16(0x0);
	stmt->cond->Accept(this);
	SetU16(condPatch, data.size() - condStart);

	// Patch OP_LEN
	SetU16(exprPatch, data.size() - exprStart);

	// Compile block
	stmt->block->Accept(this);

	// OP_LOOP
	AddU16(0x153c);
	AddU16(0x0);

	// Patch jmp
	SetU32(jmpPatch, data.size());
}

uint32_t NVSECompiler::CompileBlock(StmtPtr& stmt, bool incrementCurrent) {
	// Store old statement count
	auto temp = statementCount;

	// Get sub-block statement count
	stmt->Accept(this);
	auto newStatementCount = statementCount;

	// This is optional because lambda compilation should
	//  not be counted in the parent blocks' statement count
	if (incrementCurrent) {
		temp += newStatementCount;
	}

	statementCount = temp;
	return newStatementCount;
}

void NVSECompiler::VisitBlockStmt(const BlockStmt* stmt) {
	for (auto& statement : stmt->statements) {
		statement->Accept(this);
	}

	statementCount = stmt->statements.size();
}

void NVSECompiler::VisitAssignmentExpr(const AssignmentExpr* expr) {
	// OP_LET
	AddU16(0x1539);

	//  Placeholder [expr_len]
	auto exprPatch = AddU16(0x0);
	auto exprStart = data.size();

	// Num args
	AddU8(0x1);

	// Placeholder [param1_len]
	auto argStart = data.size();
	auto argPatch = AddU16(0x0);

	// Add LHS to stack
	insideNvseExpr.push(true);
	expr->left->Accept(this);
	insideNvseExpr.push(false);

	// Build expression
	expr->expr->Accept(this);

	// OP_ASSIGN
	AddU8(0);

	// Patch lengths
	SetU16(argPatch, data.size() - argStart);
	SetU16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::VisitTernaryExpr(const TernaryExpr* expr) {
	// TODO
}

void NVSECompiler::VisitBinaryExpr(const BinaryExpr* expr) {
	expr->left->Accept(this);
	expr->right->Accept(this);
	AddU8(operatorMap[expr->op.lexeme]);
}

void NVSECompiler::VisitUnaryExpr(const UnaryExpr* expr) {
	if (expr->postfix) {
		// Slight hack to get postfix operators working
		expr->expr->Accept(this);
		expr->expr->Accept(this);
		AddU8('B');
		AddU8(1);
		if (expr->op.type == NVSETokenType::PlusPlus) {
			AddU8(operatorMap["+"]);
		} else {
			AddU8(operatorMap["-"]);
		}
		AddU8(operatorMap[":="]);
		
		AddU8('B');
		AddU8(1);
		if (expr->op.type == NVSETokenType::PlusPlus) {
			AddU8(operatorMap["-"]);
		} else {
			AddU8(operatorMap["+"]);
		}
	} else {
		expr->expr->Accept(this);
		AddU8(operatorMap[expr->op.lexeme]);
	}
}

void NVSECompiler::VisitSubscriptExpr(SubscriptExpr* expr) {
	expr->left->Accept(this);
	expr->index->Accept(this);
	AddU8(operatorMap["["]);
}

void NVSECompiler::VisitCallExpr(const CallExpr* expr) {
	auto stackRefExpr = dynamic_cast<GetExpr*>(expr->left.get());
	auto identExpr = dynamic_cast<IdentExpr*>(expr->left.get());

	std::string name{};
	if (stackRefExpr) {
		name = stackRefExpr->token.lexeme;
	} else if (identExpr) {
		name = identExpr->name.lexeme;
	} else {
		// Shouldn't happen I don't think
		throw std::runtime_error("Unable to compile call expression, expected stack reference or identifier.");
	}

	// Try to get the script command by lexeme
	auto cmd = g_scriptCommands.GetByName(name.c_str());

	// Handle lambda NON dot expressions
	// myLambda(), not player.myLambda();
	// bool inLambda = false;
	// if (identExpr){
	// 	if (auto var = script->varList.GetVariableByName(name.c_str())) {
	// 		if (var->type == Script::eVarType_Ref) {
	// 			inLambda = true;
	//
	// 			// Wrap in Call
	// 			add_u8('X');
	// 			add_u16(0x1545);
	//
	// 			// Call size
	// 			auto callStart = data.size();
	// 			auto callPatch = add_u16(0x0);
	//
	// 			// Add ref
	// 			add_u8('R');
	//
	// 			// Args
	// 			for (int i = 0; i < expr->args.size(); i++) {
	// 				auto argStartInner = data.size();
	// 				auto argPatchInner = add_u16(0x0);
	// 				expr->args[i]->accept(this);
	// 				set_u16(argPatchInner, data.size() - argStartInner);
	// 			}
	//
	// 			set_u16(callPatch, data.size() - callStart);
	// 		}
	// 	}
	// }

	// Didn't resolve a ref var or command
	if (!cmd /*&& !inLambda*/) {
		throw std::runtime_error("Invalid function '" + name + "'.");
	}

	auto normalParse = cmd->parse != Cmd_Expression_Parse && cmd->parse != kCommandInfo_Call.parse;

	// If call command
	if (cmd->parse == kCommandInfo_Call.parse) {
		if (insideNvseExpr.top()) {
			AddU8('X');
			AddU16(0x0); // SCRV
		}

		AddU16(0x1545);
		auto callLengthStart = data.size();
		auto callLengthPatch = AddU16(0x0);

		// Compiled differently for some reason inside vs outside of calls
		if (!insideNvseExpr.top()) {
			callLengthStart = data.size();
		}
		insideNvseExpr.push(true);

		// Bytecode ver
		AddU8(1);

		auto exprLengthStart = data.size();
		auto exprLengthPatch = AddU16(0x0);

		// Resolve identifier
		expr->args[0]->Accept(this);
		SetU16(exprLengthPatch, data.size() - exprLengthStart);

		// Add args
		AddU8(expr->args.size() - 1);
		for (int i = 1; i < expr->args.size(); i++) {
			auto argStartInner = data.size();
			auto argPatchInner = AddU16(0x0);
			expr->args[i]->Accept(this);
			SetU16(argPatchInner, data.size() - argStartInner);
		}

		SetU16(callLengthPatch, data.size() - callLengthStart);

		insideNvseExpr.pop();
		return;
	}

	// See if we should wrap inside let
	// Need to do this in the case of something like
	// player.AddItem(Caps001, 10) to pass player on stack and use 'x'
	// annoying but want to keep the way these were passed consistent, especially in case of
	// Quest.target.AddItem()
	size_t exprPatch = 0;
	size_t exprStart = 0;
	size_t argStart = 0;
	size_t argPatch = 0;
	if (normalParse && stackRefExpr) {
		// OP_EVAL
		AddU16(0x153A);

		//  Placeholder [expr_len]
		exprPatch = AddU16(0x0);
		exprStart = data.size();

		// Num args
		AddU8(0x1);

		// Placeholder [param1_len]
		argStart = data.size();
		argPatch = AddU16(0x0);

		insideNvseExpr.push(true);
	}

	// Put this on the stack
	if (insideNvseExpr.top()) {
		if (stackRefExpr) {
			stackRefExpr->left->Accept(this);
			AddU8('x');
		}
		else {
			AddU8('X');
		}
		AddU16(0x0); // SCRV
	}
	AddU16(cmd->opcode);

	// Call size
	auto callStart = data.size();
	auto callPatch = AddU16(0x0);

	// Compiled differently for some reason inside vs outside of calls
	if (!insideNvseExpr.top()) {
		callStart = data.size();
	}

	insideNvseExpr.push(true);

	// Num args
	if (cmd->parse != Cmd_Expression_Parse) {
		// Count bytes after call length for vanilla expression parser
		//callStart = data.size();
		AddU16(expr->args.size());
	}
	else {
		AddU8(expr->args.size());
	}

	// Args
	for (int i = 0; i < expr->args.size(); i++) {
		if (cmd->parse != Cmd_Expression_Parse) {
			AddU16(0xFFFF);
		}

		auto argStartInner = data.size();
		auto argPatchInner = AddU16(0x0);
		
		expr->args[i]->Accept(this);
		SetU16(argPatchInner, data.size() - argStartInner);
	}

	SetU16(callPatch, data.size() - callStart);

	// Add '.' token if stack expr
	if (stackRefExpr) {
		AddU8(operatorMap["."]);
	}

	insideNvseExpr.pop();

	// If we wrapped inside of let
	if (normalParse && stackRefExpr) {
		// Patch lengths
		SetU16(argPatch, data.size() - argStart);
		SetU16(exprPatch, data.size() - exprStart);
		insideNvseExpr.pop();
	}
}

void NVSECompiler::VisitGetExpr(const GetExpr* expr) {
	// Try to resolve lhs reference
	const auto ident = dynamic_cast<IdentExpr*>(expr->left.get());
	if (!ident) {
		throw std::runtime_error("Left side of a get expression must be an object reference.");
	}
	
	const auto lhsName = ident->name.lexeme;
	const auto rhsName = expr->token.lexeme;
	
	const auto refIdx = ResolveObjReference(lhsName);
	if (!refIdx) {
		throw std::runtime_error(std::format("Unable to resolve reference '{}'.", lhsName));
	}

	// Ensure it is of type quest
	TESForm *form = script->refList.GetNthItem(refIdx - 1)->form;
	TESScriptableForm *scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
	if (!scriptable) {
		throw std::runtime_error(std::format("Form '{}' is not referenceable from scripts.", lhsName));
	}

	// Try to find variable
	Script *questScript = scriptable->script;
	if (!questScript) {
		throw std::runtime_error(std::format("Form '{}' does not have a script.", lhsName));
	}

	const VariableInfo *info = nullptr;
	for (int i = 0; i < questScript->varList.Count(); i++) {
		const auto curVar = questScript->varList.GetNthItem(i);
		if (!strcmp(curVar->name.CStr(), rhsName.c_str())) {
			info = curVar;
		}
	}

	if (!info) {
		throw std::runtime_error(std::format("Form '{}' does not have variable with name '{}'.", lhsName, rhsName));
	}

	// Put ref on stack
	AddU8('V');
	AddU8(info->type);
	AddU16(refIdx);
	AddU16(info->idx);
}

void NVSECompiler::VisitBoolExpr(const BoolExpr* expr) {
	AddU8('B');
	AddU8(expr->value);
}

void NVSECompiler::VisitNumberExpr(const NumberExpr* expr) {
	if (expr->isFp) {
		AddF64(expr->value);
	}
	else {
		if (expr->value <= UINT8_MAX) {
			AddU8('B');
			AddU8(static_cast<uint8_t>(expr->value));
		}
		else if (expr->value <= UINT16_MAX) {
			AddU8('I');
			AddU16(static_cast<uint16_t>(expr->value));
		}
		else {
			AddU8('L');
			AddU32(static_cast<uint32_t>(expr->value));
		}
	}
}

void NVSECompiler::VisitStringExpr(const StringExpr* expr) {
	AddString(std::get<std::string>(expr->value.value));
}

void NVSECompiler::VisitIdentExpr(const IdentExpr* expr) {
	auto name = expr->name.lexeme;

	// If this is a lambda var, inline it as a call to GetModLocalData
	if (lambdaVars.contains(name)) {
		// <scriptName>__lambdaName
		const auto lambdaName = scriptName + "__" + name;

		// Build a call expr
		NVSEToken callToken{};
		callToken.lexeme = "GetModLocalData";
		auto ident = std::make_shared<IdentExpr>(callToken);
		
		NVSEToken nameToken{};
		nameToken.value = lambdaName;
		auto arg = std::make_shared<StringExpr>(nameToken);

		std::vector<ExprPtr> args{};
		args.emplace_back(arg);
		auto getMLDCall = std::make_shared<CallExpr>(ident, std::move(args));

		getMLDCall->Accept(this);
		return;
	}

	// Attempt to resolve as variable
	if (auto local = script->varList.GetVariableByName(name.c_str())) {
		AddU8('V');
		AddU8(local->type);
		AddU16(0);
		AddU16(local->idx);
		return;
	}

	// Try to look up as global
	auto scrv = ResolveObjReference(name);
	if (scrv) {
		AddU8('R');
		AddU16(scrv);
		return;
	}
}

void NVSECompiler::VisitGroupingExpr(const GroupingExpr* expr) {
	expr->expr->Accept(this);
}

void NVSECompiler::VisitLambdaExpr(LambdaExpr* expr) {
	// PARAM_LAMBDA
	AddU8('F');

	// Arg size
	auto argSizePatch = AddU32(0x0);
	auto argStart = data.size();

	// Compile lambda
	{
		// SCN
		AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

		// OP_BEGIN
		AddU16(0x10);

		// OP_MODE_LEN
		auto opModeLenPatch = AddU16(0x0);
		auto opModeStart = data.size();

		// OP_MODE
		AddU16(0x0D);

		// SCRIPT_LEN
		auto scriptLenPatch = AddU32(0x0);

		// BYTECODE VER
		AddU8(0x1);

		// arg count
		AddU8(expr->args.size());

		// add args
		for (auto i = 0; i < expr->args.size(); i++) {
			auto& arg = expr->args[i];
			auto type = GetScriptTypeFromToken(arg->type);
			auto localIdx = AddLocal(arg->name.lexeme, type);

			AddU16(localIdx);
			AddU8(type);
		}

		// NUM_ARRAY_ARGS, always 0
		AddU8(0x0);

		auto scriptLenStart = data.size();

		// Patch mode len
		SetU16(opModeLenPatch, data.size() - opModeStart);

		// Compile script
		insideNvseExpr.push(false);
		CompileBlock(expr->body, false);
		insideNvseExpr.pop();

		SetU32(scriptLenPatch, data.size() - scriptLenStart);

		// OP_END
		AddU32(0x11);
	}

	SetU32(argSizePatch, data.size() - argStart);
}
