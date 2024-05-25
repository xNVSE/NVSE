#include "NVSECompiler.h"
#include "Commands_MiscRef.h"
#include "Commands_Scripting.h"
#include "NVSECompilerUtils.h"
#include "NVSEParser.h"

bool NVSECompiler::compile(Script* script, NVSEScript& ast, std::function<void(std::string)> printFn) {
	this->script = script;
	originalScriptName = script->GetEditorID();
	this->printFn = printFn;

	insideNvseExpr.push(false);
	ast.accept(this);

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

void NVSECompiler::visitNVSEScript(const NVSEScript* nvScript) {
	// Compile the script name
	scriptName = nvScript->name.lexeme;

	// Dont allow naming script the same as another form, unless that form is the script itself
	auto comp = strcmp(scriptName.c_str(), originalScriptName);
	if (resolveObjectReference(scriptName, false) && comp) {
		throw std::runtime_error(std::format("Error: Form name '{}' is already in use.\n", scriptName.c_str()));
	}

	add_u32(0x1D);

	for (auto& global_var : nvScript->globalVars) {
		global_var->accept(this);
	}

	for (auto& block : nvScript->blocks) {
		block->accept(this);
	}
}

void NVSECompiler::visitBeginStatement(const BeginStmt* stmt) {
	// Get opcode
	auto beginInfo = mpBeginInfo[stmt->name.lexeme];

	// OP_BEGIN
	add_u16(0x10);

	auto beginPatch = add_u16(0x0);
	auto beginStart = data.size();

	add_u16(beginInfo.opcode);

	auto blockPatch = add_u32(0x0);

	// Add param shit here?
	if (stmt->param.has_value()) {
		auto param = stmt->param.value();

		add_u16(0xFFFF);
		auto varStart = data.size();
		auto varPatch = add_u16(0x0);

		if (beginInfo.argType == ArgType::Int) {
			add_u8('L');
			add_u32(static_cast<uint32_t>(std::get<double>(param.value)));
		}
		else if (beginInfo.argType == ArgType::Ref) {
			// Try to resolve the global ref
			auto ref = resolveObjectReference(param.lexeme);
			if (ref) {
				auto refInfo = script->refList.GetNthItem(ref - 1);
				printf("Resolved ref %s: %08x\n", param.lexeme.c_str(), refInfo->form->refID);
				add_u8('R');
				add_u16(ref);
			} else {
				throw std::runtime_error(std::format("Unable to resolve form '{}'.", param.lexeme));
			}
		}

		set_u16(varPatch, data.size() - varStart);
	}

	set_u16(beginPatch, data.size() - beginStart);
	auto blockStart = data.size();
	stmt->block->accept(this);

	// OP_END
	add_u32(0x11);

	set_u32(blockPatch, data.size() - blockStart);
}

void NVSECompiler::visitFnDeclStmt(FnDeclStmt* stmt) {
	// OP_BEGIN
	add_u16(0x10);

	// OP_MODE_LEN
	auto opModeLenPatch = add_u16(0x0);
	auto opModeStart = data.size();

	// OP_MODE
	add_u16(0x0D);

	// SCRIPT_LEN
	auto scriptLenPatch = add_u32(0x0);

	// BYTECODE VER
	add_u8(0x1);

	// arg count
	add_u8(stmt->args.size());

	// add args
	for (auto i = 0; i < stmt->args.size(); i++) {
		auto& arg = stmt->args[i];
		auto type = getArgType(arg->type);
		auto localIdx = addLocal(arg->name.lexeme, type);

		add_u16(localIdx);
		add_u8(type);
	}

	// NUM_ARRAY_ARGS, always 0
	add_u8(0x0);

	auto scriptLenStart = data.size();

	// Patch mode len
	set_u16(opModeLenPatch, data.size() - opModeStart);

	// Compile script
	compileBlock(stmt->body, false);

	// OP_END
	add_u32(0x11);

	set_u32(scriptLenPatch, data.size() - scriptLenStart);
}

void NVSECompiler::visitVarDeclStmt(const VarDeclStmt* stmt) {
	uint8_t varType = getArgType(stmt->type);
	const auto token = stmt->name;

	// TODO: Typecheker
	if (resolveObjectReference(token.lexeme)) {
		throw std::runtime_error(std::format("Error: Name '{}' is already in use by form.", token.lexeme));
	}

	// Compile lambdas differently
	// Does not affect params as they cannot have value specified
	if (stmt->value->isType<LambdaExpr>()) {
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

		expr->accept(this);
		return;
	}

	const auto idx = addLocal(token.lexeme, varType);

	if (!stmt->value) {
		return;
	}

	// OP_LET
	add_u16(0x1539);

	//  Placeholder [expr_len]
	auto exprPatch = add_u16(0x0);
	auto exprStart = data.size();

	// Num args
	add_u8(0x1);

	// Placeholder [param1_len]
	auto argStart = data.size();
	auto argPatch = add_u16(0x0);

	// Add arg to stack
	add_u8('V');
	add_u8(varType);
	add_u16(0); // SCRV
	add_u16(idx); // SCDA

	// Build expression
	insideNvseExpr.push(true);
	stmt->value->accept(this);
	insideNvseExpr.pop();

	// OP_ASSIGN
	add_u8(0);

	// Patch lengths
	set_u16(argPatch, data.size() - argStart);
	set_u16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::visitExprStmt(const ExprStmt* stmt) {
	// OP_EVAL
	// add_u16(0x153A);
	//
	// // Placeholder OP_LEN
	// auto exprPatch = add_u16(0x0);
	// auto exprStart = data.size();
	//
	// // Num args
	// add_u8(0x1);
	//
	// // Placeholder [arg_len]
	// auto argStart = data.size();
	// auto argPatch = add_u16(0x0);

	// Build expression
	//stmt->expr->accept(this);

	// // Patch lengths
	// set_u16(argPatch, data.size() - argStart);
	// set_u16(exprPatch, data.size() - exprStart);

	// Only compile calls and assignments?
	if (auto callStmt = dynamic_cast<CallExpr*>(stmt->expr.get())) {
		callStmt->accept(this);
	}

	if (auto assignmentStmt = dynamic_cast<AssignmentExpr*>(stmt->expr.get())) {
		assignmentStmt->accept(this);
	}
}

void NVSECompiler::visitForStmt(const ForStmt* stmt) {}

void NVSECompiler::visitIfStmt(IfStmt* stmt) {
	// OP_IF
	add_u16(0x16);

	// Placeholder OP_LEN
	auto exprPatch = add_u16(0x0);
	auto exprStart = data.size();

	// Placeholder JMP_OPS
	auto jmpPatch = add_u16(0x0);

	// Placeholder EXP_LEN
	auto compPatch = add_u16(0x0);
	auto compStart = data.size();

	// OP_PUSH
	add_u8(0x20);

	// Enter NVSE eval
	add_u8(0x58);
	{
		// OP_EVAL
		add_u16(0x153A);

		// Placeholder OP_EVAL_LEN
		auto opEvalPatch = add_u16(0x0);
		auto opEvalStart = data.size();

		// OP_EVAL num params
		add_u8(0x1);

		// Placeholder OP_EVAL_ARG_LEN
		auto opEvalArgStart = data.size();
		auto opEvalArgPatch = add_u16(0x0);

		// Compile cond
		insideNvseExpr.push(true);
		stmt->cond->accept(this);
		insideNvseExpr.push(false);

		// Patch OP_EVAL lengths
		set_u16(opEvalArgPatch, data.size() - opEvalArgStart);
		set_u16(opEvalPatch, data.size() - opEvalStart);
	}

	// Patch lengths
	set_u16(compPatch, data.size() - compStart);
	set_u16(exprPatch, data.size() - exprStart);

	// Patch JMP_OPS
	set_u16(jmpPatch, compileBlock(stmt->block, true));

	// Build OP_ELSE
	if (stmt->elseBlock) {
		// OP_ELSE
		add_u16(0x17);

		// OP_LEN
		add_u16(0x02);

		// Placeholder JMP_OPS
		auto elsePatch = add_u16(0x0);

		// Compile else block
		set_u16(elsePatch, compileBlock(stmt->elseBlock, true));
	}

	// OP_ENDIF
	add_u16(0x19);
	add_u16(0x0);
}

void NVSECompiler::visitReturnStmt(const ReturnStmt* stmt) {}

void NVSECompiler::visitWhileStmt(const WhileStmt* stmt) {
	// OP_WHILE
	add_u16(0x153B);

	// Placeholder OP_LEN
	auto exprPatch = add_u16(0x0);
	auto exprStart = data.size();

	auto jmpPatch = add_u32(0x0);

	// 1 param
	add_u8(0x1);

	// Compile / patch condition
	auto condStart = data.size();
	auto condPatch = add_u16(0x0);
	stmt->cond->accept(this);
	set_u16(condPatch, data.size() - condStart);

	// Patch OP_LEN
	set_u16(exprPatch, data.size() - exprStart);

	// Compile block
	stmt->block->accept(this);

	// OP_LOOP
	add_u16(0x153c);
	add_u16(0x0);

	// Patch jmp
	set_u32(jmpPatch, data.size());
}

uint32_t NVSECompiler::compileBlock(StmtPtr& stmt, bool incrementCurrent) {
	// Store old block size
	auto temp = result;

	stmt->accept(this);

	auto res = result;
	if (incrementCurrent) {
		temp += res;
	}

	result = temp;

	return res;
}

void NVSECompiler::visitBlockStmt(const BlockStmt* stmt) {
	for (auto& statement : stmt->statements) {
		statement->accept(this);
	}

	result = stmt->statements.size();
}

void NVSECompiler::visitAssignmentExpr(const AssignmentExpr* expr) {
	const auto varInfo = script->varList.GetVariableByName(expr->name.lexeme.c_str());
	if (!varInfo) {
		throw std::runtime_error(std::format("Cannot assign to '{}'. Variable with name not found.", expr->name.lexeme));
	}

	// OP_LET
	add_u16(0x1539);

	//  Placeholder [expr_len]
	auto exprPatch = add_u16(0x0);
	auto exprStart = data.size();

	// Num args
	add_u8(0x1);

	// Placeholder [param1_len]
	auto argStart = data.size();
	auto argPatch = add_u16(0x0);

	// Add arg to stack
	add_u8('V');
	add_u8(varInfo->type);
	add_u16(0); // SCRV
	add_u16(varInfo->idx); // SCDA

	// Build expression
	expr->expr->accept(this);

	// OP_ASSIGN
	add_u8(0);

	// Patch lengths
	set_u16(argPatch, data.size() - argStart);
	set_u16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::visitTernaryExpr(const TernaryExpr* expr) {
	
}

void NVSECompiler::visitBinaryExpr(const BinaryExpr* expr) {
	expr->left->accept(this);
	expr->right->accept(this);
	add_u8(operatorMap[expr->op.lexeme]);
}

void NVSECompiler::visitUnaryExpr(const UnaryExpr* expr) {
	if (expr->postfix) {
		expr->expr->accept(this);
		expr->expr->accept(this);
		add_u8('B');
		add_u8(1);
		if (expr->op.type == NVSETokenType::PlusPlus) {
			add_u8(operatorMap["+"]);
		} else {
			add_u8(operatorMap["-"]);
		}
		add_u8(operatorMap[":="]);
		
		add_u8('B');
		add_u8(1);
		if (expr->op.type == NVSETokenType::PlusPlus) {
			add_u8(operatorMap["-"]);
		} else {
			add_u8(operatorMap["+"]);
		}
	} else {
		expr->expr->accept(this);
		add_u8(operatorMap[expr->op.lexeme]);
	}
}

void NVSECompiler::visitSubscriptExpr(SubscriptExpr* expr) {
	expr->left->accept(this);
	expr->index->accept(this);
	add_u8(operatorMap["["]);
}

void NVSECompiler::visitCallExpr(const CallExpr* expr) {
	auto stackRefExpr = dynamic_cast<GetExpr*>(expr->left.get());
	auto identExpr = dynamic_cast<IdentExpr*>(expr->left.get());

	std::string name{};
	if (stackRefExpr) {
		name = stackRefExpr->token.lexeme;
	} else {
		name = identExpr->name.lexeme;
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
		throw std::runtime_error("Invalid function '" + identExpr->name.lexeme + "'.");
	}

	auto normalParse = cmd->parse != Cmd_Expression_Parse && cmd->parse != kCommandInfo_Call.parse;

	// If call command
	if (cmd->parse == kCommandInfo_Call.parse) {
		if (insideNvseExpr.top()) {
			add_u8('X');
			add_u16(0x0); // SCRV
		}

		add_u16(0x1545);
		auto callLengthStart = data.size();
		auto callLengthPatch = add_u16(0x0);

		// Compiled differently for some reason inside vs outside of calls
		if (!insideNvseExpr.top()) {
			callLengthStart = data.size();
		}
		insideNvseExpr.push(true);

		// Bytecode ver
		add_u8(1);

		auto exprLengthStart = data.size();
		auto exprLengthPatch = add_u16(0x0);

		// Resolve identifier
		expr->args[0]->accept(this);
		set_u16(exprLengthPatch, data.size() - exprLengthStart);

		// Add args
		add_u8(expr->args.size() - 1);
		for (int i = 1; i < expr->args.size(); i++) {
			auto argStartInner = data.size();
			auto argPatchInner = add_u16(0x0);
			expr->args[i]->accept(this);
			set_u16(argPatchInner, data.size() - argStartInner);
		}

		set_u16(callLengthPatch, data.size() - callLengthStart);

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
		add_u16(0x153A);

		//  Placeholder [expr_len]
		exprPatch = add_u16(0x0);
		exprStart = data.size();

		// Num args
		add_u8(0x1);

		// Placeholder [param1_len]
		argStart = data.size();
		argPatch = add_u16(0x0);

		insideNvseExpr.push(true);
	}

	// Put this on the stack
	if (insideNvseExpr.top()) {
		if (stackRefExpr) {
			stackRefExpr->left->accept(this);
			add_u8('x');
		}
		else {
			add_u8('X');
		}
		add_u16(0x0); // SCRV
	}
	add_u16(cmd->opcode);

	// Call size
	auto callStart = data.size();
	auto callPatch = add_u16(0x0);

	// Compiled differently for some reason inside vs outside of calls
	if (!insideNvseExpr.top()) {
		callStart = data.size();
	}

	insideNvseExpr.push(true);

	// Num args
	if (cmd->parse != Cmd_Expression_Parse) {
		// Count bytes after call length for vanilla expression parser
		//callStart = data.size();
		add_u16(expr->args.size());
	}
	else {
		add_u8(expr->args.size());
	}

	// Args
	for (int i = 0; i < expr->args.size(); i++) {
		if (cmd->parse != Cmd_Expression_Parse) {
			add_u16(0xFFFF);
		}

		auto argStartInner = data.size();
		auto argPatchInner = add_u16(0x0);
		
		expr->args[i]->accept(this);
		set_u16(argPatchInner, data.size() - argStartInner);
	}

	set_u16(callPatch, data.size() - callStart);

	// Add '.' token if stack expr
	if (stackRefExpr) {
		add_u8(operatorMap["."]);
	}

	insideNvseExpr.pop();

	// If we wrapped inside of let
	if (normalParse && stackRefExpr) {
		// Patch lengths
		set_u16(argPatch, data.size() - argStart);
		set_u16(exprPatch, data.size() - exprStart);
		insideNvseExpr.pop();
	}
}

void NVSECompiler::visitGetExpr(const GetExpr* expr) {

}

void NVSECompiler::visitSetExpr(const SetExpr* expr) {}

void NVSECompiler::visitBoolExpr(const BoolExpr* expr) {
	add_u8('B');
	add_u8(expr->value);
}

void NVSECompiler::visitNumberExpr(const NumberExpr* expr) {
	if (expr->isFp) {
		if (expr->value <= UINT8_MAX) {
			double value = expr->value;
			uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);

			add_u8('Z');
			for (int i = 0; i < 8; i++) {
				add_u8(bytes[i]);
			}
		}
	}
	else {
		if (expr->value <= UINT8_MAX) {
			add_u8('B');
			add_u8(static_cast<uint8_t>(expr->value));
		}
		else if (expr->value <= UINT16_MAX) {
			add_u8('I');
			add_u16(static_cast<uint16_t>(expr->value));
		}
		else {
			add_u8('L');
			add_u32(static_cast<uint32_t>(expr->value));
		}
	}
}

void NVSECompiler::visitStringExpr(const StringExpr* expr) {
	auto str = std::get<std::string>(expr->value.value);

	add_u8('S');
	add_u16(str.size());
	for (char c : str) {
		add_u8(c);
	}
}

void NVSECompiler::visitIdentExpr(const IdentExpr* expr) {
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

		getMLDCall->accept(this);
		return;
	}

	// Attempt to resolve as variable
	if (auto local = script->varList.GetVariableByName(name.c_str())) {
		add_u8('V');
		add_u8(local->type);
		add_u16(0);
		add_u16(local->idx);
		return;
	}

	// Try to look up as function

	// Try to look up as global
	auto scrv = resolveObjectReference(name);
	if (scrv) {
		add_u8('R');
		add_u16(scrv);
		return;
	}
}

void NVSECompiler::visitGroupingExpr(const GroupingExpr* expr) {
	expr->expr->accept(this);
}

void NVSECompiler::visitLambdaExpr(LambdaExpr* expr) {
	// PARAM_LAMBDA
	add_u8('F');

	// Arg size
	auto argSizePatch = add_u32(0x0);
	auto argStart = data.size();

	// Compile lambda
	{
		// SCN
		add_u32(0x1D);

		// OP_BEGIN
		add_u16(0x10);

		// OP_MODE_LEN
		auto opModeLenPatch = add_u16(0x0);
		auto opModeStart = data.size();

		// OP_MODE
		add_u16(0x0D);

		// SCRIPT_LEN
		auto scriptLenPatch = add_u32(0x0);

		// BYTECODE VER
		add_u8(0x1);

		// arg count
		add_u8(expr->args.size());

		// add args
		for (auto i = 0; i < expr->args.size(); i++) {
			auto& arg = expr->args[i];
			auto type = getArgType(arg->type);
			auto localIdx = addLocal(arg->name.lexeme, type);

			add_u16(localIdx);
			add_u8(type);
		}

		// NUM_ARRAY_ARGS, always 0
		add_u8(0x0);

		auto scriptLenStart = data.size();

		// Patch mode len
		set_u16(opModeLenPatch, data.size() - opModeStart);

		// Compile script
		insideNvseExpr.push(false);
		compileBlock(expr->body, false);
		insideNvseExpr.pop();

		set_u32(scriptLenPatch, data.size() - scriptLenStart);

		// OP_END
		add_u32(0x11);
	}

	set_u32(argSizePatch, data.size() - argStart);
}
