#include "NVSECompiler.h"
#include "NVSEParser.h"

// Define tokens
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
	{".", 40},
	{"|=", 41},
	{"&=", 42},
	{"%=", 43},
};

uint8_t getArgType(NVSEToken t) {
	switch (t.type) {
	case TokenType::DoubleType:
		return 0;
	case TokenType::IntType:
		return 1;
	case TokenType::RefType:
		return 4;
	case TokenType::ArrayType:
		return 3;
	case TokenType::StringType:
		return 2;
	}

	return 0;
}

std::vector<uint8_t> NVSECompiler::compile(StmtPtr &stmt) {
	stmt->accept(this);
	return data;
}

void NVSECompiler::visitFnDeclStmt(const FnDeclStmt* stmt) {}
void NVSECompiler::visitVarDeclStmt(const VarDeclStmt* stmt) {
	uint8_t varType = getArgType(stmt->type);
	const auto token = stmt->name;
	const auto idx = addLocal(token.lexeme, varType);

	// OP_LET
	add_u16(0x1539);						

	//  Placeholder [expr_len]
	auto exprPatch = add_u16(0x0);  
	auto exprStart = data.size();

	// Num args
	add_u8(0x1);

	// Placeholder [arg_len]
	auto argPatch = add_u16(0x0);
	auto argStart = data.size();

	// Add arg to stack
	add_u8('V');
	add_u8(varType);
	add_u16(0); // SCRV
	add_u16(idx);     // SCDA

	// Build expression
	stmt->value->accept(this);    

	// OP_ASSIGN
	add_u8(0);                             

	// Patch lengths
	set_u16(argPatch, data.size() - argStart);
	set_u16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::visitExprStmt(const ExprStmt* stmt) {
	// OP_LET
	add_u16(0x1539);

	// Placeholder OP_LEN
	auto exprPatch = add_u16(0x0);
	auto exprStart = data.size();

	// Num args
	add_u8(0x1);

	// Placeholder [arg_len]
	auto argStart = data.size();
	auto argPatch = add_u16(0x0);

	// Build expression
	stmt->expr->accept(this);

	// Patch lengths
	set_u16(argPatch, data.size() - argStart);
	set_u16(exprPatch, data.size() - exprStart);
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
		stmt->cond->accept(this);

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

void NVSECompiler::visitReturnStmt(const ReturnStmt* stmt) {
	
}

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

uint32_t NVSECompiler::compileBlock(StmtPtr &stmt, bool incrementCurrent) {
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
	for (auto &statement : stmt->statements) {
		statement->accept(this);
	}

	result = stmt->statements.size();
}

void NVSECompiler::visitAssignmentExpr(const AssignmentExpr* expr) {}
void NVSECompiler::visitTernaryExpr(const TernaryExpr* expr) {}
void NVSECompiler::visitLogicalExpr(const LogicalExpr* expr) {}
void NVSECompiler::visitBinaryExpr(const BinaryExpr* expr) {
	expr->left->accept(this);
	expr->right->accept(this);
	add_u8(operatorMap[expr->op.lexeme]);
}
void NVSECompiler::visitUnaryExpr(const UnaryExpr* expr) {
	expr->expr->accept(this);
	add_u8(operatorMap[expr->op.lexeme]);
}

void NVSECompiler::visitCallExpr(const CallExpr* expr) {
	bool useStackRef = dynamic_cast<GetExpr*>(expr->left.get());
}

void NVSECompiler::visitGetExpr(const GetExpr* expr) {}

void NVSECompiler::visitSetExpr(const SetExpr* expr) {}

void NVSECompiler::visitBoolExpr(const BoolExpr* expr) {
	add_u8('B');
	add_u8(expr->value);
}

void NVSECompiler::visitNumberExpr(const NumberExpr* expr) {
	if (expr->isFp) {
		if (expr->value <= UINT8_MAX) {
			float value = expr->value;
			uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);

			add_u8('Z');
			add_u8(bytes[0]);
			add_u8(bytes[1]);
			add_u8(bytes[2]);
			add_u8(bytes[3]);
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
		compileBlock(expr->body, false);

		set_u32(scriptLenPatch, data.size() - scriptLenStart);

		// OP_END
		add_u32(0x11);
	}

	set_u32(argSizePatch, data.size() - argStart);
}
