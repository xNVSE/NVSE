#include "NVSETreePrinter.h"
#include <iostream>
#include "NVSEParser.h"

void NVSETreePrinter::PrintTabs() {
	for (int i = 0; i < curTab; i++) {
		printFn("  ");
	}
}

void NVSETreePrinter::VisitNVSEScript(const NVSEScript* script) {
	printFn("\n==== AST ====\n\n");
	
	PrintTabs();
	printFn(std::format("name: {}\n", script->name.lexeme));
	PrintTabs();
	if (!script->globalVars.empty()) {
		printFn("globals\n");
		curTab++;
		for (auto& g : script->globalVars) {
			g->Accept(this);
		}
		curTab--;
	}
	printFn("blocks\n");
	curTab++;
	for (auto &b : script->blocks) {
		b->Accept(this);
	}
	curTab--;
}

void NVSETreePrinter::VisitBeginStmt(const BeginStmt* stmt) {
	PrintTabs();
	printFn(std::format("begin {} {}\n", stmt->name.lexeme, stmt->param.has_value() ? stmt->param->lexeme : ""));
	curTab++;
	PrintTabs();
	printFn("body\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitFnStmt(FnDeclStmt* stmt) {
	PrintTabs();
	printFn("fn\n");
	curTab++;
	PrintTabs();
	printFn("args\n");
	curTab++;
	for (auto var_decl_stmt : stmt->args) {
		var_decl_stmt->Accept(this);
	}
	curTab--;
	PrintTabs();
	printFn("body\n");
	curTab++;
	stmt->body->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitVarDeclStmt(const VarDeclStmt* stmt) {
	PrintTabs();
	printFn("vardecl\n");
	curTab++;
	PrintTabs();
	printFn("name: " + stmt->name.lexeme + '\n');
	PrintTabs();
	printFn("type: " + stmt->type.lexeme + '\n');
	if (stmt->value) {
		PrintTabs();
		printFn("value\n");
		curTab++;
		stmt->value->Accept(this);
		curTab--;
	}
	curTab--;
}

void NVSETreePrinter::VisitExprStmt(const ExprStmt* stmt) {
	PrintTabs();
	printFn("exprstmt\n");
	curTab++;
	if (stmt->expr) {
		stmt->expr->Accept(this);
	}
	curTab--;
}
void NVSETreePrinter::VisitForStmt(ForStmt* stmt) {
	PrintTabs();
	printFn("for\n");

	curTab++;

	if (stmt->init) {
		PrintTabs();
		printFn("init\n");
		curTab++;
		stmt->init->Accept(this);
		curTab--;
	}

	if (stmt->cond) {
		PrintTabs();
		printFn("cond\n");
		curTab++;
		stmt->cond->Accept(this);
		curTab--;
	}

	if (stmt->post) {
		PrintTabs();
		printFn("post\n");
		curTab++;
		stmt->post->Accept(this);
		curTab--;
	}

	PrintTabs();
	printFn("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitForEachStmt(ForEachStmt* stmt) {
	PrintTabs();
	printFn("foreach\n");

	curTab++;

	PrintTabs();
	printFn("elem\n");
	curTab++;
	stmt->lhs->Accept(this);
	curTab--;
	
	PrintTabs();
	printFn("in\n");
	curTab++;
	stmt->rhs->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitIfStmt(IfStmt* stmt) {
	PrintTabs();
	printFn("if\n");

	curTab++;

	PrintTabs();
	printFn("cond\n");
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;

	if (stmt->elseBlock) {
		PrintTabs();
		printFn("else\n");
		curTab++;
		stmt->elseBlock->Accept(this);
		curTab--;
	}

	curTab--;
}
void NVSETreePrinter::VisitReturnStmt(ReturnStmt* stmt) {
	PrintTabs();
	printFn("return\n");

	if (stmt->expr) {
		curTab++;
		PrintTabs();
		printFn("value\n");
		curTab++;
		stmt->expr->Accept(this);
		curTab--;
		curTab--;
	}
}

void NVSETreePrinter::VisitContinueStmt(ContinueStmt* stmt) {
	PrintTabs();
	printFn("continue");
}

void NVSETreePrinter::VisitBreakStmt(BreakStmt* stmt) {
	PrintTabs();
	printFn("break");
}

void NVSETreePrinter::VisitWhileStmt(const WhileStmt* stmt) {
	PrintTabs();
	printFn("while\n");

	curTab++;

	PrintTabs();
	printFn("cond\n");
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	printFn("block\n");
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
	printFn("assignment\n");
	curTab++;
	PrintTabs();
	printFn("op: " + expr->token.lexeme + "\n");
	PrintTabs();
	printFn("lhs:\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("rhs\n");
	curTab++;
	expr->expr->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitTernaryExpr(const TernaryExpr* expr) {
	PrintTabs();
	printFn("ternary\n");
	curTab++;

	PrintTabs();
	printFn("cond\n");
	curTab++;
	expr->cond->Accept(this);
	curTab--;

	if (expr->left) {
		PrintTabs();
		printFn("lhs\n");
		curTab++;
		expr->left->Accept(this);
		curTab--;
	}

	PrintTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->Accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::VisitBinaryExpr(BinaryExpr* expr) {
	PrintTabs();
	printFn("binary: " + expr->op.lexeme + '\n');
	curTab++;
	PrintTabs();
	printFn("lhs\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("rhs\n");
	curTab++;
	expr->right->Accept(this);
	curTab--;
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	
	curTab--;
}

void NVSETreePrinter::VisitUnaryExpr(UnaryExpr* expr) {
	PrintTabs();
	printFn("unary: " + expr->op.lexeme + '\n');
	curTab++;
	expr->expr->Accept(this);
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	
	curTab--;
}

void NVSETreePrinter::VisitSubscriptExpr(SubscriptExpr* expr) {
	PrintTabs();
	printFn("subscript\n");

	curTab++;
	PrintTabs();
	printFn("expr\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	
	PrintTabs();
	printFn("[]\n");
	curTab++;
	expr->index->Accept(this);
	curTab--;
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	
	curTab--;
}

void NVSETreePrinter::VisitCallExpr(CallExpr* expr) {
	PrintTabs();
	printFn("call\n");
	curTab++;

	PrintTabs();
	printFn("expr\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	if (!expr->args.empty()) {
		PrintTabs();
		printFn("args\n");
		curTab++;
		for (const auto& exp : expr->args) {
			exp->Accept(this);
		}
		curTab--;
	}
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");

	curTab--;
}

void NVSETreePrinter::VisitGetExpr(GetExpr* expr) {
	PrintTabs();
	printFn("get\n");
	curTab++;
	PrintTabs();
	printFn("expr\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	printFn("token: " + expr->identifier.lexeme + '\n');
	
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	
	curTab--;
}

void NVSETreePrinter::VisitBoolExpr(BoolExpr* expr) {
	PrintTabs();
	if (expr->value) {
		printFn("boolean: true\n");
	} else {
		printFn("boolean: false\n");
	}

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}

void NVSETreePrinter::VisitNumberExpr(NumberExpr* expr) {
	PrintTabs();
	printFn("number: " + std::to_string(expr->value) + '\n');

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}

void NVSETreePrinter::VisitStringExpr(StringExpr* expr) {
	PrintTabs();
	printFn("string: " + expr->token.lexeme + '\n');

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}

void NVSETreePrinter::VisitIdentExpr(IdentExpr* expr) {
	PrintTabs();
	printFn("ident: " + expr->token.lexeme + '\n');

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}

void NVSETreePrinter::VisitGroupingExpr(GroupingExpr* expr) {
	PrintTabs();
	printFn("grouping\n");
	curTab++;
	expr->expr->Accept(this);
	curTab--;

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}

void NVSETreePrinter::VisitLambdaExpr(LambdaExpr* expr) {
	PrintTabs();
	printFn("lambda\n");
	curTab++;

	if (!expr->args.empty()) {
		PrintTabs();
		printFn("args\n");
		curTab++;
		for (auto &arg : expr->args) {
			arg->Accept(this);
		}
		curTab--;
	}

	PrintTabs();
	printFn("body\n");
	curTab++;
	expr->body->Accept(this);
	curTab--;

	curTab--;

	curTab++;
	PrintTabs();
	std::string detailedTypeStr = TokenTypeToString(expr->detailedType);
	printFn("detailed type: " + detailedTypeStr + "\n");
	curTab--;
}