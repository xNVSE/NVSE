#include "NVSETreePrinter.h"
#include <iostream>

#include "NVSECompilerUtils.h"
#include "NVSEParser.h"

void NVSETreePrinter::PrintTabs(const bool debugOnly) {
	for (int i = 0; i < curTab; i++) {
		if (debugOnly) {
			CompDbg("  ");
		}
		else {
			CompInfo("  ");
		}
	}
}

void NVSETreePrinter::VisitNVSEScript(NVSEScript* script) {
	CompDbg("\n==== AST ====\n\n");
	PrintTabs();
	CompDbg("name: %s\n", script->name.lexeme.c_str());
	PrintTabs();
	if (!script->globalVars.empty()) {
		CompDbg("globals\n");
		curTab++;
		for (auto& g : script->globalVars) {
			g->Accept(this);
		}
		curTab--;
	}
	CompDbg("blocks\n");
	curTab++;
	for (auto &b : script->blocks) {
		b->Accept(this);
	}
	curTab--;
}

void NVSETreePrinter::VisitBeginStmt(BeginStmt* stmt) {
	PrintTabs();
	CompDbg("begin %s %s\n", stmt->name.lexeme.c_str(), stmt->param.has_value() ? stmt->param->lexeme.c_str() : "");
	curTab++;
	PrintTabs();
	CompDbg("body\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitFnStmt(FnDeclStmt* stmt) {
	PrintTabs();
	CompDbg("fn\n");
	curTab++;
	PrintTabs();
	CompDbg("args\n");
	curTab++;
	for (auto var_decl_stmt : stmt->args) {
		var_decl_stmt->Accept(this);
	}
	curTab--;
	PrintTabs();
	CompDbg("body\n");
	curTab++;
	stmt->body->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitVarDeclStmt(VarDeclStmt* stmt) {
	PrintTabs();
	CompDbg("vardecl\n");
	
	curTab++;
	for (auto [name, value] : stmt->values) {
		PrintTabs();
		CompDbg("%s\n", name.lexeme.c_str());
		if (value) {
			curTab++;
			value->Accept(this);
			curTab--;
		}
	}
	
	curTab--;
}

void NVSETreePrinter::VisitExprStmt(const ExprStmt* stmt) {
	PrintTabs();
	CompDbg("exprstmt\n");
	curTab++;
	if (stmt->expr) {
		stmt->expr->Accept(this);
	}
	curTab--;
}
void NVSETreePrinter::VisitForStmt(ForStmt* stmt) {
	PrintTabs();
	CompDbg("for\n");

	curTab++;

	if (stmt->init) {
		PrintTabs();
		CompDbg("init\n");
		curTab++;
		stmt->init->Accept(this);
		curTab--;
	}

	if (stmt->cond) {
		PrintTabs();
		CompDbg("cond\n");
		curTab++;
		stmt->cond->Accept(this);
		curTab--;
	}

	if (stmt->post) {
		PrintTabs();
		CompDbg("post\n");
		curTab++;
		stmt->post->Accept(this);
		curTab--;
	}

	PrintTabs();
	CompDbg("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitForEachStmt(ForEachStmt* stmt) {
	PrintTabs();
	CompDbg("foreach\n");

	curTab++;

	PrintTabs();
	CompDbg("elem\n");
	curTab++;
	for (auto decl : stmt->declarations) {
		if (decl) {
			decl->Accept(this);
		}
	}
	curTab--;
	
	PrintTabs();
	CompDbg("in\n");
	curTab++;
	stmt->rhs->Accept(this);
	curTab--;

	PrintTabs();
	CompDbg("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;
	
	curTab--;
}

void NVSETreePrinter::VisitIfStmt(IfStmt* stmt) {
	PrintTabs();
	CompDbg("if\n");

	curTab++;

	PrintTabs();
	CompDbg("cond\n");
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	CompDbg("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;

	if (stmt->elseBlock) {
		PrintTabs();
		CompDbg("else\n");
		curTab++;
		stmt->elseBlock->Accept(this);
		curTab--;
	}

	curTab--;
}
void NVSETreePrinter::VisitReturnStmt(ReturnStmt* stmt) {
	PrintTabs();
	CompDbg("return\n");

	if (stmt->expr) {
		curTab++;
		PrintTabs();
		CompDbg("value\n");
		curTab++;
		stmt->expr->Accept(this);
		curTab--;
		curTab--;
	}
}

void NVSETreePrinter::VisitContinueStmt(ContinueStmt* stmt) {
	PrintTabs();
	CompDbg("continue");
}

void NVSETreePrinter::VisitBreakStmt(BreakStmt* stmt) {
	PrintTabs();
	CompDbg("break");
}

void NVSETreePrinter::VisitWhileStmt(WhileStmt* stmt) {
	PrintTabs();
	CompDbg("while\n");

	curTab++;

	PrintTabs();
	CompDbg("cond\n");
	curTab++;
	stmt->cond->Accept(this);
	curTab--;

	PrintTabs();
	CompDbg("block\n");
	curTab++;
	stmt->block->Accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::VisitBlockStmt(BlockStmt* stmt) {
	for (auto stmt : stmt->statements) {
		stmt->Accept(this);
	}
}

void NVSETreePrinter::VisitAssignmentExpr(AssignmentExpr* expr) {
	PrintTabs();
	CompDbg("assignment\n");
	curTab++;
	PrintTabs();
	CompDbg("op: %s\n", expr->token.lexeme.c_str());
	PrintTabs();
	CompDbg("lhs:\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	CompDbg("rhs\n");
	curTab++;
	expr->expr->Accept(this);
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitTernaryExpr(TernaryExpr* expr) {
	PrintTabs();
	CompDbg("ternary\n");
	curTab++;

	PrintTabs();
	CompDbg("cond\n");
	curTab++;
	expr->cond->Accept(this);
	curTab--;

	if (expr->left) {
		PrintTabs();
		CompDbg("lhs\n");
		curTab++;
		expr->left->Accept(this);
		curTab--;
	}

	PrintTabs();
	CompDbg("rhs\n");
	curTab++;
	expr->right->Accept(this);
	curTab--;

	curTab--;
}

void NVSETreePrinter::VisitInExpr(InExpr* expr) {
	PrintTabs();
	CompDbg("inexpr\n");
	curTab++;
	PrintTabs();
	CompDbg("isNot: %s\n", expr->isNot ? "y" : "n");
	PrintTabs();
	CompDbg("expr\n");
	curTab++;
	expr->lhs->Accept(this);
	curTab--;
	PrintTabs();
	CompDbg("val\n");
	curTab++;
	if (expr->expression) {
		expr->expression->Accept(this);
	} else {
		for (auto val : expr->values) {
			val->Accept(this);
		}
	}
	curTab--;
	curTab--;
}

void NVSETreePrinter::VisitBinaryExpr(BinaryExpr* expr) {
	PrintTabs();
	CompDbg("binary: %s\n", expr->op.lexeme.c_str());
	curTab++;
	PrintTabs();
	CompDbg("lhs\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	CompDbg("rhs\n");
	curTab++;
	expr->right->Accept(this);
	curTab--;
	
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	
	curTab--;
}

void NVSETreePrinter::VisitUnaryExpr(UnaryExpr* expr) {
	PrintTabs();
	CompDbg("unary: %s\n", expr->op.lexeme.c_str());
	curTab++;
	expr->expr->Accept(this);
	
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	
	curTab--;
}

void NVSETreePrinter::VisitSubscriptExpr(SubscriptExpr* expr) {
	PrintTabs();
	CompDbg("subscript\n");

	curTab++;
	PrintTabs();
	CompDbg("expr\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	
	PrintTabs();
	CompDbg("[]\n");
	curTab++;
	expr->index->Accept(this);
	curTab--;
	
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	
	curTab--;
}

void NVSETreePrinter::VisitCallExpr(CallExpr* expr) {
	PrintTabs();
	CompDbg("call : %s\n", expr->token.lexeme.c_str());
	curTab++;

	if (expr->left) {
		PrintTabs();
		CompDbg("expr\n");
		curTab++;
		expr->left->Accept(this);
		curTab--;
	}

	if (!expr->args.empty()) {
		PrintTabs();
		CompDbg("args\n");
		curTab++;
		for (const auto& exp : expr->args) {
			exp->Accept(this);
		}
		curTab--;
	}
	
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));

	curTab--;
}

void NVSETreePrinter::VisitGetExpr(GetExpr* expr) {
	PrintTabs();
	CompDbg("get\n");
	curTab++;
	PrintTabs();
	CompDbg("expr\n");
	curTab++;
	expr->left->Accept(this);
	curTab--;
	PrintTabs();
	CompDbg("token: %s\n", expr->identifier.lexeme.c_str());
	
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	
	curTab--;
}

void NVSETreePrinter::VisitBoolExpr(BoolExpr* expr) {
	PrintTabs();
	if (expr->value) {
		CompDbg("boolean: true\n");
	} else {
		CompDbg("boolean: false\n");
	}

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitNumberExpr(NumberExpr* expr) {
	PrintTabs();
	CompDbg("number: %0.5f\n", expr->value);

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitStringExpr(StringExpr* expr) {
	PrintTabs();
	CompDbg("string: %s\n", expr->token.lexeme.c_str());

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitIdentExpr(IdentExpr* expr) {
	PrintTabs();
	CompDbg("ident: %s\n", expr->token.lexeme.c_str());

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitMapLiteralExpr(MapLiteralExpr* expr) {}

void NVSETreePrinter::VisitArrayLiteralExpr(ArrayLiteralExpr* expr) {
	PrintTabs();
	CompDbg("array literal\n");
	curTab++;
	PrintTabs();
	CompDbg("values\n");
	curTab++;
	for (auto val : expr->values) {
		val->Accept(this);
	}
	curTab--;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitGroupingExpr(GroupingExpr* expr) {
	PrintTabs();
	CompDbg("grouping\n");
	curTab++;
	expr->expr->Accept(this);
	curTab--;

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}

void NVSETreePrinter::VisitLambdaExpr(LambdaExpr* expr) {
	PrintTabs();
	CompDbg("lambda\n");
	curTab++;

	if (!expr->args.empty()) {
		PrintTabs();
		CompDbg("args\n");
		curTab++;
		for (auto &arg : expr->args) {
			arg->Accept(this);
		}
		curTab--;
	}

	PrintTabs();
	CompDbg("body\n");
	curTab++;
	expr->body->Accept(this);
	curTab--;

	curTab--;

	curTab++;
	PrintTabs();
	CompDbg("detailed type: %s\n", TokenTypeToString(expr->tokenType));
	curTab--;
}