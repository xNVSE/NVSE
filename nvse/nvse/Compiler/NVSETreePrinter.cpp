#include "NVSETreePrinter.h"
#include <iostream>

#include "NVSECompilerUtils.h"
#include "Parser.h"
#include "AST/AST.h"

namespace Compiler {
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

	void NVSETreePrinter::Visit(AST* script) {
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
		for (auto& b : script->blocks) {
			b->Accept(this);
		}
		curTab--;
	}

	void NVSETreePrinter::VisitBeginStmt(Statements::Begin* stmt) {
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

	void NVSETreePrinter::VisitFnStmt(Statements::UDFDecl* stmt) {
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

	void NVSETreePrinter::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		PrintTabs();
		CompDbg("vardecl\n");

		curTab++;
		for (auto [name, value, info] : stmt->declarations) {
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

	void NVSETreePrinter::VisitExprStmt(const Statements::ExpressionStatement* stmt) {
		PrintTabs();
		CompDbg("exprstmt\n");
		curTab++;
		if (stmt->expr) {
			stmt->expr->Accept(this);
		}
		curTab--;
	}
	void NVSETreePrinter::VisitForStmt(Statements::For* stmt) {
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

	void NVSETreePrinter::VisitForEachStmt(Statements::ForEach* stmt) {
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

	void NVSETreePrinter::VisitIfStmt(Statements::If* stmt) {
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
	void NVSETreePrinter::VisitReturnStmt(Statements::Return* stmt) {
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

	void NVSETreePrinter::VisitContinueStmt(Statements::Continue* stmt) {
		PrintTabs();
		CompDbg("continue");
	}

	void NVSETreePrinter::VisitBreakStmt(Statements::Break* stmt) {
		PrintTabs();
		CompDbg("break");
	}

	void NVSETreePrinter::VisitWhileStmt(Statements::While* stmt) {
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

	void NVSETreePrinter::VisitBlockStmt(Statements::Block* stmt) {
		for (auto stmt : stmt->statements) {
			stmt->Accept(this);
		}
	}

	void NVSETreePrinter::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
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

	void NVSETreePrinter::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
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

	void NVSETreePrinter::VisitInExpr(Expressions::InExpr* expr) {
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
		}
		else {
			for (auto val : expr->values) {
				val->Accept(this);
			}
		}
		curTab--;
		curTab--;
	}

	void NVSETreePrinter::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));

		curTab--;
	}

	void NVSETreePrinter::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
		PrintTabs();
		CompDbg("unary: %s\n", expr->op.lexeme.c_str());
		curTab++;
		expr->expr->Accept(this);

		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));

		curTab--;
	}

	void NVSETreePrinter::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));

		curTab--;
	}

	void NVSETreePrinter::VisitCallExpr(Expressions::CallExpr* expr) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));

		curTab--;
	}

	void NVSETreePrinter::VisitGetExpr(Expressions::GetExpr* expr) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));

		curTab--;
	}

	void NVSETreePrinter::VisitBoolExpr(Expressions::BoolExpr* expr) {
		PrintTabs();
		if (expr->value) {
			CompDbg("boolean: true\n");
		}
		else {
			CompDbg("boolean: false\n");
		}

		curTab++;
		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitNumberExpr(Expressions::NumberExpr* expr) {
		PrintTabs();
		CompDbg("number: %0.5f\n", expr->value);

		curTab++;
		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitStringExpr(Expressions::StringExpr* expr) {
		PrintTabs();
		CompDbg("string: %s\n", expr->token.lexeme.c_str());

		curTab++;
		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitIdentExpr(Expressions::IdentExpr* expr) {
		PrintTabs();
		CompDbg("ident: %s\n", expr->token.lexeme.c_str());

		curTab++;
		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {}

	void NVSETreePrinter::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
		PrintTabs();
		CompDbg("grouping\n");
		curTab++;
		expr->expr->Accept(this);
		curTab--;

		curTab++;
		PrintTabs();
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}

	void NVSETreePrinter::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		PrintTabs();
		CompDbg("lambda\n");
		curTab++;

		if (!expr->args.empty()) {
			PrintTabs();
			CompDbg("args\n");
			curTab++;
			for (auto& arg : expr->args) {
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
		CompDbg("detailed type: %s\n", TokenTypeToString(expr->type));
		curTab--;
	}
}
