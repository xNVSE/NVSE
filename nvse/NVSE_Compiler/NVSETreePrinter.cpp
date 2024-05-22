#include "NVSETreePrinter.h"
#include <iostream>
#include "NVSEParser.h"

void NVSETreePrinter::printTabs() {
	for (int i = 0; i < curTab; i++) {
		std::cout << "  ";
	}
}

void NVSETreePrinter::visitFnDeclStmt(const FnDeclStmt* stmt) {
	
}

void NVSETreePrinter::visitVarDeclStmt(const VarDeclStmt* stmt) {
	printTabs();
	std::cout << "vardecl" << std::endl;
	curTab++;
	printTabs();
	std::cout << "name: " << stmt->name.lexeme << std::endl;
	printTabs();
	std::cout << "type: " << stmt->type.lexeme << std::endl;
	if (stmt->value) {
		printTabs();
		std::cout << "value" << std::endl;
		curTab++;
		stmt->value->accept(this);
		curTab--;
	}
	curTab--;
}

void NVSETreePrinter::visitExprStmt(const ExprStmt* stmt) {
	printTabs();
	std::cout << "exprstmt" << std::endl;
	curTab++;
	if (stmt->expr) {
		stmt->expr->accept(this);
	}
	curTab--;
}
void NVSETreePrinter::visitForStmt(const ForStmt* stmt) {
	printTabs();
	std::cout << "for" << std::endl;

	curTab++;

	printTabs();
	std::cout << "cond" << std::endl;
	curTab++;
	curTab--;

	curTab--;
}
void NVSETreePrinter::visitIfStmt(const IfStmt* stmt) {
	printTabs();
	std::cout << "if" << std::endl;

	curTab++;

	printTabs();
	std::cout << "cond" << std::endl;
	curTab++;
	stmt->cond->accept(this);
	curTab--;

	printTabs();
	std::cout << "block" << std::endl;
	curTab++;
	stmt->block->accept(this);
	curTab--;

	if (stmt->elseBlock) {
		printTabs();
		std::cout << "else" << std::endl;
		curTab++;
		stmt->elseBlock->accept(this);
		curTab--;
	}

	curTab--;
}
void NVSETreePrinter::visitReturnStmt(const ReturnStmt* stmt) {
	printTabs();
	std::cout << "return" << std::endl;

	if (stmt->expr) {
		curTab++;
		printTabs();
		std::cout << "cond" << std::endl;
		curTab++;
		stmt->expr->accept(this);
		curTab--;
		curTab--;
	}
}
void NVSETreePrinter::visitWhileStmt(const WhileStmt* stmt) {
	printTabs();
	std::cout << "while" << std::endl;

	curTab++;

	printTabs();
	std::cout << "cond" << std::endl;
	curTab++;
	stmt->cond->accept(this);
	curTab--;

	printTabs();
	std::cout << "block" << std::endl;
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
	std::cout << "assignment" << std::endl;
	curTab++;
	printTabs();
	std::cout << "name: " << expr->name.lexeme << std::endl;
	printTabs();
	std::cout << "rhs" << std::endl;
	curTab++;
	expr->expr->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitTernaryExpr(const TernaryExpr* expr) {
	printTabs();
	std::cout << "ternary" << std::endl;
	curTab++;

	printTabs();
	std::cout << "cond" << std::endl;
	curTab++;
	expr->cond->accept(this);
	curTab--;

	if (expr->left) {
		printTabs();
		std::cout << "lhs" << std::endl;
		curTab++;
		expr->left->accept(this);
		curTab--;
	}

	printTabs();
	std::cout << "rhs" << std::endl;
	curTab++;
	expr->right->accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::visitLogicalExpr(const LogicalExpr* expr) {
	printTabs();
	std::cout << "logical: " << expr->op.lexeme << std::endl;
	curTab++;
	printTabs();
	std::cout << "lhs" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	std::cout << "rhs" << std::endl;
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitBinaryExpr(const BinaryExpr* expr) {
	printTabs();
	std::cout << "binary: " << expr->op.lexeme << std::endl;
	curTab++;
	printTabs();
	std::cout << "lhs" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	std::cout << "rhs" << std::endl;
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitUnaryExpr(const UnaryExpr* expr) {
	printTabs();
	std::cout << "unary: " << expr->op.lexeme << std::endl;
	curTab++;
	expr->expr->accept(this);
	curTab--;
}

void NVSETreePrinter::visitCallExpr(const CallExpr* expr) {
	printTabs();
	std::cout << "call" << std::endl;
	curTab++;

	printTabs();
	std::cout << "expr" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	if (!expr->args.empty()) {
		printTabs();
		std::cout << "args" << std::endl;
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
	std::cout << "get" << std::endl;
	curTab++;
	printTabs();
	std::cout << "expr" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	std::cout << "token: " << expr->token.lexeme << std::endl;
	curTab--;
}

void NVSETreePrinter::visitSetExpr(const SetExpr* expr) {
	printTabs();
	std::cout << "set" << std::endl;
	curTab++;
	printTabs();
	std::cout << "lhs" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	std::cout << "token: " << expr->token.lexeme << std::endl;
	curTab--;
	curTab++;
	printTabs();
	std::cout << "rhs" << std::endl;
	curTab++;
	expr->right->accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::visitBoolExpr(const BoolExpr* expr) {
	printTabs();
	std::cout << "boolean: " << (expr->value ? "true" : "false") << std::endl;
}

void NVSETreePrinter::visitNumberExpr(const NumberExpr* expr) {
	printTabs();
	std::cout << "number: " << expr->value << std::endl;
}

void NVSETreePrinter::visitStringExpr(const StringExpr* expr) {
	printTabs();
	std::cout << "string: " << expr->value.lexeme << std::endl;
}

void NVSETreePrinter::visitIdentExpr(const IdentExpr* expr) {
	printTabs();
	std::cout << "ident: " << expr->name.lexeme << std::endl;
}

void NVSETreePrinter::visitGroupingExpr(const GroupingExpr* expr) {
	printTabs();
	std::cout << "grouping" << std::endl;
	curTab++;
	expr->expr->accept(this);
	curTab--;
}

void NVSETreePrinter::visitLambdaExpr(const LambdaExpr* expr) {
	printTabs();
	std::cout << "lambda" << std::endl;
	curTab++;

	if (!expr->args.empty()) {
		printTabs();
		std::cout << "args" << std::endl;
		curTab++;
		for (auto &arg : expr->args) {
			arg->accept(this);
		}
		curTab--;
	}

	printTabs();
	std::cout << "body" << std::endl;
	curTab++;
	expr->body->accept(this);
	curTab--;

	curTab--;
}