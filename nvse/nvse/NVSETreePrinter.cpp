#include "NVSETreePrinter.h"
#include <iostream>
#include "NVSEParser.h"

void NVSETreePrinter::PrintTabs() {
	for (int i = 0; i < curTab; i++) {
		printFn("  ", true);
	}
}

void NVSETreePrinter::VisitNVSEScript(const NVSEScript* script) {
	printFn("\n==== AST ====\n\n", true);
	
	PrintTabs();
	printFn(std::format("name: {}\n", script->name.lexeme), true);
	PrintTabs();
	if (!script->globalVars.empty()) {
		printFn("globals\n", true);
		curTab++;
		for (auto& g : script->globalVars) {
			g->Accept(this);
		}
		curTab--;
	}
	printFn("blocks\n", true);
	curTab++;
	for (auto &b : script->blocks) {
		b->Accept(this);
	}
	curTab--;
}

void NVSETreePrinter::VisitBeginStmt(const BeginStmt* stmt) {
	PrintTabs();
	printFn(std::format("begin {} {}\n", stmt->name.lexeme, stmt->param.has_value() ? stmt->param->lexeme : ""), true);
	curTab++;
	PrintTabs();
	printFn("body\n", true);
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitFnStmt(FnDeclStmt* stmt) {
	PrintTabs();
	printFn("fn\n", true);
	curTab++;
	PrintTabs();
	printFn("args\n", true);
	curTab++;
	for (auto var_decl_stmt : stmt->args) {
		var_decl_stmt->Accept(this);
	}
	curTab--;
	PrintTabs();
	printFn("body\n", true);
	curTab++;
	stmt->body->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitVarDeclStmt(const VarDeclStmt* stmt) {
	PrintTabs();
	printFn("vardecl\n", true);
	curTab++;
	PrintTabs();
	printFn("name: " + stmt->name.lexeme + '\n', true);
	PrintTabs();
	printFn("type: " + stmt->type.lexeme + '\n', true);
	if (stmt->value) {
		PrintTabs();
		printFn("value\n", true);
		curTab++;
		stmt->value->Accept(this);
		curTab--;
	}
	curTab--;
}

void NVSETreePrinter::VisitExprStmt(const ExprStmt* stmt) {
	PrintTabs();
	printFn("exprstmt\n", true);
	curTab++;
	if (stmt->expr) {
		stmt->expr->Accept(this);
	}
	curTab--;
}
void NVSETreePrinter::VisitForStmt(ForStmt* stmt) {
	PrintTabs();
	printFn("for\n", true);

	curTab++;

	if (stmt->init) {
		PrintTabs();
		printFn("init\n", true);
		curTab++;
		stmt->init->Accept(this);
		curTab--;
	}

	if (stmt->cond) {
		PrintTabs();
		printFn("cond\n", true);
		curTab++;
		stmt->cond->Accept(this);
		curTab--;
	}

	if (stmt->post) {
		PrintTabs();
		printFn("post\n", true);
		curTab++;
		stmt->post->Accept(this);
		curTab--;
	}

	PrintTabs();
	printFn("block\n", true);
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitForEachStmt(ForEachStmt* stmt) {
	PrintTabs();
	printFn("foreach\n", true);

	curTab++;

	PrintTabs();
	printFn("elem\n", true);
	curTab++;
	stmt->lhs->Accept(this);
	curTab--;
	
	PrintTabs();
	printFn("in\n", true);
	curTab++;
	stmt->rhs->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n", true);
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitIfStmt(IfStmt* stmt) {
	PrintTabs();
	printFn("if\n", true);

	curTab++;

	PrintTabs();
	printFn("cond\n", true);
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n", true);
	curTab++;
	stmt->block->Accept(this);
	curTab--;

	if (stmt->elseBlock) {
		PrintTabs();
		printFn("else\n", true);
		curTab++;
		stmt->elseBlock->Accept(this);
		curTab--;
	}

	curTab--;
}
void NVSETreePrinter::VisitReturnStmt(ReturnStmt* stmt) {
	PrintTabs();
	printFn("return\n", true);

	if (stmt->expr) {
		curTab++;
		PrintTabs();
		printFn("value\n", true);
		curTab++;
		stmt->expr->Accept(this);
		curTab--;
		curTab--;
	}
}

void NVSETreePrinter::VisitContinueStmt(ContinueStmt* stmt) {
	PrintTabs();
	printFn("continue", true);
}

void NVSETreePrinter::VisitBreakStmt(BreakStmt* stmt) {
	PrintTabs();
	printFn("break", true);
}

void NVSETreePrinter::VisitWhileStmt(const WhileStmt* stmt) {
	PrintTabs();
	printFn("while\n", true);

	curTab++;

	PrintTabs();
	printFn("cond\n", true);
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n", true);
	curTab++;
	stmt->block->Accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::VisitBlockStmt(BlockStmt* stmt) {
	for (auto &stmt : stmt->statements) {
		stmt->Accept(this);
	}
}

void NVSETreePrinter::VisitAssignmentExpr(const AssignmentExpr* expr) {
	PrintTabs();
	printFn("assignment\n", true);
	curTab++;
	PrintTabs();
	printFn("op: " + expr->token.lexeme + "\n", true);
	PrintTabs();
	printFn("lhs:\n", true);
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("rhs\n", true);
	curTab++;
	expr->expr->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitTernaryExpr(const TernaryExpr* expr) {
	PrintTabs();
	printFn("ternary\n", true);
	curTab++;

	PrintTabs();
	printFn("cond\n", true);
	curTab++;
	expr->cond->Accept(this);
	curTab--;

	if (expr->left) {
		PrintTabs();
		printFn("lhs\n", true);
		curTab++;
		expr->left->Accept(this);
		curTab--;
	}

	PrintTabs();
	printFn("rhs\n", true);
	curTab++;
	expr->right->Accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::VisitBinaryExpr(BinaryExpr* expr) {
	PrintTabs();
	printFn("binary: " + expr->op.lexeme + '\n', true);
	curTab++;
	PrintTabs();
	printFn("lhs\n", true);
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("rhs\n", true);
	curTab++;
	expr->right->Accept(this);
	curTab--;
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	
	curTab--;
}

void NVSETreePrinter::VisitUnaryExpr(UnaryExpr* expr) {
	PrintTabs();
	printFn("unary: " + expr->op.lexeme + '\n', true);
	curTab++;
	expr->expr->Accept(this);
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	
	curTab--;
}

void NVSETreePrinter::VisitSubscriptExpr(SubscriptExpr* expr) {
	PrintTabs();
	printFn("subscript\n", true);

	curTab++;
	PrintTabs();
	printFn("expr\n", true);
	curTab++;
	expr->left->Accept(this);
	curTab--;
	
	PrintTabs();
	printFn("[]\n", true);
	curTab++;
	expr->index->Accept(this);
	curTab--;
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	
	curTab--;
}

void NVSETreePrinter::VisitCallExpr(CallExpr* expr) {
	PrintTabs();
	printFn("call\n", true);
	curTab++;

	PrintTabs();
	printFn("expr\n", true);
	curTab++;
	expr->left->Accept(this);
	curTab--;
	if (!expr->args.empty()) {
		PrintTabs();
		printFn("args\n", true);
		curTab++;
		for (const auto& exp : expr->args) {
			exp->Accept(this);
		}
		curTab--;
	}
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);

	curTab--;
}

void NVSETreePrinter::VisitGetExpr(GetExpr* expr) {
	PrintTabs();
	printFn("get\n", true);
	curTab++;
	PrintTabs();
	printFn("expr\n", true);
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("token: " + expr->identifier.lexeme + '\n', true);
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	
	curTab--;
}

void NVSETreePrinter::VisitBoolExpr(BoolExpr* expr) {
	PrintTabs();
	if (expr->value) {
		printFn("boolean: true\n", true);
	} else {
		printFn("boolean: false\n", true);
	}

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}

void NVSETreePrinter::VisitNumberExpr(NumberExpr* expr) {
	PrintTabs();
	printFn("number: " + std::to_string(expr->value) + '\n', true);

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}

void NVSETreePrinter::VisitStringExpr(StringExpr* expr) {
	PrintTabs();
	printFn("string: " + expr->token.lexeme + '\n', true);

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}

void NVSETreePrinter::VisitIdentExpr(IdentExpr* expr) {
	PrintTabs();
	printFn("ident: " + expr->token.lexeme + '\n', true);

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}

void NVSETreePrinter::VisitGroupingExpr(GroupingExpr* expr) {
	PrintTabs();
	printFn("grouping\n", true);
	curTab++;
	expr->expr->Accept(this);
	curTab--;

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}

void NVSETreePrinter::VisitLambdaExpr(LambdaExpr* expr) {
	PrintTabs();
	printFn("lambda\n", true);
	curTab++;

	if (!expr->args.empty()) {
		PrintTabs();
		printFn("args\n", true);
		curTab++;
		for (auto &arg : expr->args) {
			arg->Accept(this);
		}
		curTab--;
	}

	PrintTabs();
	printFn("body\n", true);
	curTab++;
	expr->body->Accept(this);
	curTab--;

	curTab--;

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n", true);
	curTab--;
}