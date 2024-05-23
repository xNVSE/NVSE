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

std::vector<uint8_t> NVSECompiler::compile(StmtPtr &stmt) {
	stmt->accept(this);
	return data;
}

void NVSECompiler::visitFnDeclStmt(const FnDeclStmt* stmt) {}
void NVSECompiler::visitVarDeclStmt(const VarDeclStmt* stmt) {
	const auto token = stmt->name;
	const auto idx = addLocal(token.lexeme);

	int8_t varType = 0;
	switch (stmt->type.type) {
	case TokenType::DoubleType:
		varType = 0;
		break;
	case TokenType::IntType:
		varType = 1;
		break;
	case TokenType::RefType:
		varType = 4;
		break;
	case TokenType::ArrayType:
		varType = 3;
		break;
	case TokenType::StringType:
		varType = 2;
		break;
	}

	// OP_LET
	add_u16(0x1539);						

	//  Placeholder [expr_len]
	auto exprPatch = add_u16(0x0);  
	auto exprStart = data.size();

	// Num args
	add_u8(0x1);

	// Placeholder [arg_len]
	auto argStart = data.size();
	auto argPatch = add_u16(0x0);

	// Add arg to stack
	add_u8('V');
	add_u8(varType);
	add_u16(0); // SCRV
	add_u16(idx);     // SCDA

	// Build expression
	inNvseExpr = true;
	stmt->value->accept(this);               
	inNvseExpr = false;

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
	inNvseExpr = true;
	stmt->expr->accept(this);
	inNvseExpr = false;

	// Patch lengths
	set_u16(argPatch, data.size() - argStart);
	set_u16(exprPatch, data.size() - exprStart);
}

void NVSECompiler::visitForStmt(const ForStmt* stmt) {}
void NVSECompiler::visitIfStmt(const IfStmt* stmt) {
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
		inNvseExpr = true;
		stmt->cond->accept(this);
		inNvseExpr = false;

		// Patch OP_EVAL lengths
		set_u16(opEvalArgPatch, data.size() - opEvalArgStart);
		set_u16(opEvalPatch, data.size() - opEvalStart);
	}

	// Patch lengths
	set_u16(compPatch, data.size() - compStart);
	set_u16(exprPatch, data.size() - exprStart);

	// Compile block
	stmt->block->accept(this);

	// Patch JMP_OPS
	set_u16(jmpPatch, result);

	// Build OP_ELSE
	if (stmt->elseBlock) {
		// OP_ELSE
		add_u16(0x17);

		// OP_LEN
		add_u16(0x02);

		// Placeholder JMP_OPS
		auto elsePatch = add_u16(0x0);

		// Compile else block
		stmt->elseBlock->accept(this);
		set_u16(elsePatch, result);
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
	inNvseExpr = true;
	stmt->cond->accept(this);
	inNvseExpr = false;
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
	int len = 0;

	if (inNvseExpr) {
		expr->left->accept(this);
		len += result;

		expr->right->accept(this);
		len += result;

		add_u8(operatorMap[expr->op.lexeme]);
		len++;
	}

	result = len;
}
void NVSECompiler::visitUnaryExpr(const UnaryExpr* expr) {}
void NVSECompiler::visitCallExpr(const CallExpr* expr) {}
void NVSECompiler::visitGetExpr(const GetExpr* expr) {}
void NVSECompiler::visitSetExpr(const SetExpr* expr) {}
void NVSECompiler::visitBoolExpr(const BoolExpr* expr) {}
void NVSECompiler::visitNumberExpr(const NumberExpr* expr) {
	if (inNvseExpr) {
		if (expr->value <= UINT8_MAX) {
			add_u8('B');
			add_u8(expr->value);
			result = 2;
		}
		else if (expr->value <= UINT16_MAX) {
			add_u8('I');
			add_u16(expr->value);
			result = 3;
		}
		else {
			add_u8('L');
			add_u32(expr->value);
			result = 5;
		}
	}
}
void NVSECompiler::visitStringExpr(const StringExpr* expr) {}
void NVSECompiler::visitIdentExpr(const IdentExpr* expr) {}
void NVSECompiler::visitGroupingExpr(const GroupingExpr* expr) {}
void NVSECompiler::visitLambdaExpr(const LambdaExpr* expr) {}
