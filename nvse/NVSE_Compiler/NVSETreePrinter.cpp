#include "NVSETreePrinter.h"
#include <iostream>
#include "NVSEParser.h"

void NVSETreePrinter::printTabs() {
	for (int i = 0; i < curTab; i++) {
		std::cout << "  ";
	}
}

void NVSETreePrinter::visitAssignmentExpr(const AssignmentExpr* expr)
{
}

void NVSETreePrinter::visitLogicalExpr(const LogicalExpr* expr)
{
	printTabs();
	std::cout << "logical: " << std::get<std::string>(expr->op.value) << std::endl;
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

void NVSETreePrinter::visitBinaryExpr(const BinaryExpr* expr)
{
	printTabs();
	std::cout << "binary: " << std::get<std::string>(expr->op.value) << std::endl;
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

void NVSETreePrinter::visitUnaryExpr(const UnaryExpr* expr)
{
	printTabs();
	std::cout << "unary: " << std::get<std::string>(expr->op.value) << std::endl;
	curTab++;
	expr->expr->accept(this);
	curTab--;
}

void NVSETreePrinter::visitCallExpr(const CallExpr* expr)
{
	printTabs();
	std::cout << "call" << std::endl;
	curTab++;
	printTabs();
	std::cout << "expr" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	if (!expr->args.empty()) {
		std::cout << "args" << std::endl;
		curTab++;
		for (const auto& exp : expr->args) {
			exp->accept(this);
		}
		curTab--;
	}
	curTab--;
}

void NVSETreePrinter::visitGetExpr(const GetExpr* expr)
{
	printTabs();
	std::cout << "get" << std::endl;
	curTab++;
	printTabs();
	std::cout << "expr" << std::endl;
	curTab++;
	expr->left->accept(this);
	curTab--;
	printTabs();
	std::cout << "token: " << std::get<std::string>(expr->token.value) << std::endl;
	curTab--;
}

void NVSETreePrinter::visitBoolExpr(const BoolExpr* expr)
{
	printTabs();
	std::cout << "boolean: " << (expr->value ? "true" : "false") << std::endl;
}

void NVSETreePrinter::visitNumberExpr(const NumberExpr* expr)
{
	printTabs();
	std::cout << "number: " << expr->value << std::endl;
}

void NVSETreePrinter::visitStringExpr(const StringExpr* expr)
{
	printTabs();
	std::cout << "string: " << std::get<std::string>(expr->value.value) << std::endl;
}

void NVSETreePrinter::visitIdentExpr(const IdentExpr* expr)
{
	printTabs();
	std::cout << "ident: " << std::get<std::string>(expr->name.value) << std::endl;
}

void NVSETreePrinter::visitGroupingExpr(const GroupingExpr* expr)
{
	printTabs();
	std::cout << "grouping" << std::endl;
	curTab++;
	expr->expr->accept(this);
	curTab--;
}
