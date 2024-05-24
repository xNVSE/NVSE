#include "NVSETreePrinter.h"
#include <iostream>
#include "NVSEParser.h"

void NVSETreePrinter::printTabs() {
	for (int i = 0; i < curTab; i++) {
		printFn("  ");
	}
}

void NVSETreePrinter::visitNVSEScript(const NVSEScript* script) {
	printTabs();
	printFn(std::format("name: {}\n", script->name.lexeme));
	printTabs();
	if (!script->globalVars.empty()) {
		printFn("globals\n");
		curTab++;
		for (auto& g : script->globalVars) {
			g->accept(this);
		}
		curTab--;
	}
	printFn("blocks\n");
	curTab++;
	for (auto &b : script->blocks) {
		b->accept(this);
	}
	curTab--;
}

void NVSETreePrinter::visitBeginStatement(const BeginStmt* stmt) {
	printTabs();
	printFn(std::format("begin {}{}\n", stmt->name.lexeme, stmt->param.has_value() ? stmt->param->lexeme : ""));
	curTab++;
	printTabs();
	printFn("body\n");
	curTab++;
	stmt->block->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitFnDeclStmt(const FnDeclStmt* stmt) {
	
}

void NVSETreePrinter::visitVarDeclStmt(const VarDeclStmt* stmt) {
	printTabs();
	printFn("vardecl\n");
	curTab++;
	printTabs();
	printFn("name: " + stmt->name.lexeme + '\n');
	printTabs();
	printFn("type: " + stmt->type.lexeme + '\n');
	if (stmt->value) {
		printTabs();
		printFn("value\n");
		curTab++;
		stmt->value->accept(this);
		curTab--;
	}
	curTab--;
}

void NVSETreePrinter::visitExprStmt(const ExprStmt* stmt) {
	printTabs();
	printFn("exprstmt\n");
	curTab++;
	if (stmt->expr) {
		stmt->expr->accept(this);
	}
	curTab--;
}
void NVSETreePrinter::visitForStmt(const ForStmt* stmt) {
	printTabs();
	printFn("for\n");

	curTab++;

	printTabs();
	printFn("cond\n");
	curTab++;
	curTab--;

	curTab--;
}
void NVSETreePrinter::visitIfStmt(IfStmt* stmt) {
	printTabs();
	printFn("if\n");

	curTab++;

	printTabs();
	printFn("cond\n");
	curTab++;
	stmt->cond->accept(this);
	curTab--;

	printTabs();
	printFn("block\n");
	curTab++;
	stmt->block->accept(this);
	curTab--;

	if (stmt->elseBlock) {
		printTabs();
		printFn("else\n");
		curTab++;
		stmt->elseBlock->accept(this);
		curTab--;
	}

	curTab--;
}
void NVSETreePrinter::visitReturnStmt(const ReturnStmt* stmt) {
	printTabs();
	printFn("return\n");

	if (stmt->expr) {
		curTab++;
		printTabs();
		printFn("cond\n");
		curTab++;
		stmt->expr->accept(this);
		curTab--;
		curTab--;
	}
}
void NVSETreePrinter::visitWhileStmt(const WhileStmt* stmt) {
	printTabs();
	printFn("while\n");

	curTab++;

	printTabs();
	printFn("cond\n");
	curTab++;
	stmt->cond->accept(this);
	curTab--;

	printTabs();
	printFn("block\n");
	curTab++;
	stmt->block->accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::visitBlockStmt(const BlockStmt* stmt) {
	for (auto &stmt : stmt->statements) {
		stmt->accept(this);
	}
}

void NVSETreePrinter::visitAssignmentExpr(const AssignmentExpr* expr) {
	printTabs();
	printFn("assignment\n");
	curTab++;
	printTabs();
	printFn("name: " + expr->name.lexeme + '\n');
	printTabs();
	printFn("rhs\n");
	curTab++;
	expr->expr->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitTernaryExpr(const TernaryExpr* expr) {
	printTabs();
	printFn("ternary\n");
	curTab++;

	printTabs();
	printFn("cond\n");
	curTab++;
	expr->cond->accept(this);
	curTab--;

	if (expr->left) {
		printTabs();
		printFn("lhs\n");
		curTab++;
		expr->left->accept(this);
		curTab--;
	}

	printTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::visitLogicalExpr(const LogicalExpr* expr) {
	printTabs();
	printFn("logical: " + expr->op.lexeme + '\n');
	curTab++;
	printTabs();
	printFn("lhs\n");
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitBinaryExpr(const BinaryExpr* expr) {
	printTabs();
	printFn("binary: " + expr->op.lexeme + '\n');
	curTab++;
	printTabs();
	printFn("lhs\n");
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitUnaryExpr(const UnaryExpr* expr) {
	printTabs();
	printFn("unary: " + expr->op.lexeme + '\n');
	curTab++;
	expr->expr->accept(this);
	curTab--;
}

void NVSETreePrinter::visitCallExpr(const CallExpr* expr) {
	printTabs();
	printFn("call\n");
	curTab++;

	printTabs();
	printFn("expr\n");
	curTab++;
	expr->left->accept(this);
	curTab--;
	if (!expr->args.empty()) {
		printTabs();
		printFn("args\n");
		curTab++;
		for (const auto& exp : expr->args) {
			exp->accept(this);
		}
		curTab--;
	}

	curTab--;
}

void NVSETreePrinter::visitGetExpr(const GetExpr* expr) {
	printTabs();
	printFn("get\n");
	curTab++;
	printTabs();
	printFn("expr\n");
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	printFn("token: " + expr->token.lexeme + '\n');
	curTab--;
}

void NVSETreePrinter::visitSetExpr(const SetExpr* expr) {
	printTabs();
	printFn("set\n");
	curTab++;
	printTabs();
	printFn("lhs\n");
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	printFn("token: " + expr->token.lexeme + '\n');
	curTab--;
	curTab++;
	printTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitBoolExpr(const BoolExpr* expr) {
	printTabs();
	if (expr->value) {
		printFn("boolean: true\n");
	} else {
		printFn("boolean: false\n");
	}
}

void NVSETreePrinter::visitNumberExpr(const NumberExpr* expr) {
	printTabs();
	printFn("number: " + std::to_string(expr->value) + '\n');
}

void NVSETreePrinter::visitStringExpr(const StringExpr* expr) {
	printTabs();
	printFn("string: " + expr->value.lexeme + '\n');
}

void NVSETreePrinter::visitIdentExpr(const IdentExpr* expr) {
	printTabs();
	printFn("ident: " + expr->name.lexeme + '\n');
}

void NVSETreePrinter::visitGroupingExpr(const GroupingExpr* expr) {
	printTabs();
	printFn("grouping\n");
	curTab++;
	expr->expr->accept(this);
	curTab--;
}

void NVSETreePrinter::visitLambdaExpr(LambdaExpr* expr) {
	printTabs();
	printFn("lambda\n");
	curTab++;

	if (!expr->args.empty()) {
		printTabs();
		printFn("args\n");
		curTab++;
		for (auto &arg : expr->args) {
			arg->accept(this);
		}
		curTab--;
	}

	printTabs();
	printFn("body\n");
	curTab++;
	expr->body->accept(this);
	curTab--;

	curTab--;
}