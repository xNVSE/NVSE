#include "TreePrinter.h"
#include <iostream>

#include "../Utils.h"
#include "Parser.h"
#include "../AST/AST.h"

namespace Compiler {
	void TreePrinter::PrintTabs(const bool debugOnly) {
		for (int i = 0; i < curTab; i++) {
			if (debugOnly) {
				DbgPrint("  ");
			}
			else {
				InfoPrint("  ");
			}
		}
	}

	void TreePrinter::Visit(AST* script) {
		DbgPrintln("[AST]");
		DbgIndent();

		DbgPrintln("name: %s", script->name.c_str());
		if (!script->globalVars.empty()) {
			DbgPrintln("globals");
			DbgIndent();
			for (auto& g : script->globalVars) {
				g->Accept(this);
			}
			DbgOutdent();
		}
		DbgPrintln("blocks");
		DbgIndent();
		for (auto& b : script->blocks) {
			b->Accept(this);
		}
		DbgOutdent();

		DbgOutdent();
	}

	void TreePrinter::VisitBeginStmt(Statements::Begin* stmt) {
		DbgPrintln("begin %s %s", stmt->name, stmt->param.has_value() ? stmt->param->lexeme.c_str() : "");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		
		DbgPrintln("body");
		DbgIndent();
		stmt->block->Accept(this);
		DbgOutdent();
		DbgOutdent();
	}

	void TreePrinter::VisitFnStmt(Statements::UDFDecl* stmt) {
		DbgPrintln("fn");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		
		DbgPrintln("args");
		DbgIndent();
		for (const auto& declStmt : stmt->args) {
			declStmt->Accept(this);
		}
		DbgOutdent();
		
		DbgPrintln("body");
		DbgIndent();
		stmt->body->Accept(this);
		DbgOutdent();
		DbgOutdent();
	}

	void TreePrinter::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		
		DbgPrintln("vardecl");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]", 
			stmt->sourceInfo.start.line, 
			stmt->sourceInfo.start.column, 
			stmt->sourceInfo.end.line, 
			stmt->sourceInfo.end.column
		);

		for (const auto& [name, value, info] : stmt->declarations) {
			
			DbgPrintln("%s", name->str.c_str());
			if (value) {
				DbgIndent();
				value->Accept(this);
				DbgOutdent();
			}
		}

		DbgOutdent();
	}

	void TreePrinter::VisitExprStmt(Statements::ExpressionStatement* stmt) {
		
		DbgPrintln("exprstmt");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);

		if (stmt->expr) {
			stmt->expr->Accept(this);
		}
		DbgOutdent();
	}

	void TreePrinter::VisitForStmt(Statements::For* stmt) {
		DbgPrintln("for");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);

		if (stmt->init) {
			
			DbgPrintln("init");
			DbgIndent();
			stmt->init->Accept(this);
			DbgOutdent();
		}

		if (stmt->cond) {
			
			DbgPrintln("cond");
			DbgIndent();
			stmt->cond->Accept(this);
			DbgOutdent();
		}

		if (stmt->post) {
			
			DbgPrintln("post");
			DbgIndent();
			stmt->post->Accept(this);
			DbgOutdent();
		}

		
		DbgPrintln("block");
		DbgIndent();
		stmt->block->Accept(this);
		DbgOutdent();

		DbgOutdent();
	}

	void TreePrinter::VisitForEachStmt(Statements::ForEach* stmt) {
		
		DbgPrintln("foreach");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		
		DbgPrintln("elem");
		DbgIndent();
		for (auto decl : stmt->declarations) {
			if (decl) {
				decl->Accept(this);
			}
		}
		DbgOutdent();

		
		DbgPrintln("in");
		DbgIndent();
		stmt->rhs->Accept(this);
		DbgOutdent();

		
		DbgPrintln("block");
		DbgIndent();
		stmt->block->Accept(this);
		DbgOutdent();

		DbgOutdent();
	}

	void TreePrinter::VisitIfStmt(Statements::If* stmt) {
		DbgPrintln("if");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		
		DbgPrintln("cond");
		DbgIndent();
		stmt->cond->Accept(this);
		DbgOutdent();

		
		DbgPrintln("block");
		DbgIndent();
		stmt->block->Accept(this);
		DbgOutdent();

		if (stmt->elseBlock) {
			
			DbgPrintln("else");
			DbgIndent();
			stmt->elseBlock->Accept(this);
			DbgOutdent();
		}

		DbgOutdent();
	}
	void TreePrinter::VisitReturnStmt(Statements::Return* stmt) {
		DbgPrintln("return");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);

		if (stmt->expr) {
			DbgPrintln("value");
			DbgIndent();
			stmt->expr->Accept(this);
			DbgOutdent();
		}
		DbgOutdent();
	}

	void TreePrinter::VisitContinueStmt(Statements::Continue* stmt) {
		DbgPrintln("continue");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		DbgOutdent();
	}

	void TreePrinter::VisitBreakStmt(Statements::Break* stmt) {
		DbgPrintln("break");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		DbgOutdent();
	}

	void TreePrinter::VisitWhileStmt(Statements::While* stmt) {
		DbgPrintln("while");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			stmt->sourceInfo.start.line,
			stmt->sourceInfo.start.column,
			stmt->sourceInfo.end.line,
			stmt->sourceInfo.end.column
		);
		
		DbgPrintln("cond");
		DbgIndent();
		stmt->cond->Accept(this);
		DbgOutdent();

		
		DbgPrintln("block");
		DbgIndent();
		stmt->block->Accept(this);
		DbgOutdent();

		DbgOutdent();
	}

	void TreePrinter::VisitBlockStmt(Statements::Block* stmt) {
		for (auto stmt : stmt->statements) {
			stmt->Accept(this);
		}
	}

	void TreePrinter::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
		DbgPrintln("assignment");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("op: %s", expr->token.lexeme.c_str());
		
		DbgPrintln("lhs:");
		DbgIndent();
		expr->left->Accept(this);
		DbgOutdent();
		
		DbgPrintln("rhs");
		DbgIndent();
		expr->expr->Accept(this);
		DbgOutdent();
		DbgOutdent();
	}

	void TreePrinter::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
		DbgPrintln("ternary");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("cond");
		DbgIndent();
		expr->cond->Accept(this);
		DbgOutdent();

		if (expr->left) {
			
			DbgPrintln("lhs");
			DbgIndent();
			expr->left->Accept(this);
			DbgOutdent();
		}

		
		DbgPrintln("rhs");
		DbgIndent();
		expr->right->Accept(this);
		DbgOutdent();

		DbgOutdent();
	}

	void TreePrinter::VisitInExpr(Expressions::InExpr* expr) {
		DbgPrintln("inexpr");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("isNot: %s", expr->isNot ? "y" : "n");
		
		DbgPrintln("expr");
		DbgIndent();
		expr->lhs->Accept(this);
		DbgOutdent();
		
		DbgPrintln("val");
		DbgIndent();
		if (expr->expression) {
			expr->expression->Accept(this);
		}
		else {
			for (auto val : expr->values) {
				val->Accept(this);
			}
		}
		DbgOutdent();
		DbgOutdent();
	}

	void TreePrinter::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
		DbgPrintln("binary: %s", expr->op.lexeme.c_str());
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("lhs");
		DbgIndent();
		expr->left->Accept(this);
		DbgOutdent();
		
		DbgPrintln("rhs");
		DbgIndent();
		expr->right->Accept(this);
		DbgOutdent();

		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));

		DbgOutdent();
	}

	void TreePrinter::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
		DbgPrintln("unary: %s", expr->op.lexeme.c_str());
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);

		expr->expr->Accept(this);

		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));

		DbgOutdent();
	}

	void TreePrinter::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
		DbgPrintln("subscript");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("expr");
		DbgIndent();
		expr->left->Accept(this);
		DbgOutdent();

		
		DbgPrintln("[]");
		DbgIndent();
		expr->index->Accept(this);
		DbgOutdent();

		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));

		DbgOutdent();
	}

	void TreePrinter::VisitCallExpr(Expressions::CallExpr* expr) {
		DbgPrintln("call : %s", expr->identifier->str.c_str());
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);

		if (expr->left) {
			
			DbgPrintln("expr");
			DbgIndent();
			expr->left->Accept(this);
			DbgOutdent();
		}

		if (!expr->args.empty()) {
			
			DbgPrintln("args");
			DbgIndent();
			for (const auto& exp : expr->args) {
				exp->Accept(this);
			}
			DbgOutdent();
		}
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));

		DbgOutdent();
	}

	void TreePrinter::VisitGetExpr(Expressions::GetExpr* expr) {
		DbgPrintln("get");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("expr");
		DbgIndent();
		expr->left->Accept(this);
		DbgOutdent();
		
		DbgPrintln("token: %s", expr->identifier->str.c_str());
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));

		DbgOutdent();
	}

	void TreePrinter::VisitBoolExpr(Expressions::BoolExpr* expr) {
		if (expr->value) {
			DbgPrintln("boolean: true");
		}
		else {
			DbgPrintln("boolean: false");
		}

		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitNumberExpr(Expressions::NumberExpr* expr) {
		DbgPrintln("number: %0.5f", expr->value);
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitStringExpr(Expressions::StringExpr* expr) {
		DbgPrintln("string: %s", expr->value.c_str());
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitIdentExpr(Expressions::IdentExpr* expr) {
		DbgPrintln("ident: %s", expr->str.c_str());
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {}

	void TreePrinter::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
		DbgPrintln("array literal");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);
		
		DbgPrintln("values");
		DbgIndent();
		for (auto val : expr->values) {
			val->Accept(this);
		}
		DbgOutdent();
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
		DbgPrintln("grouping");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);

		expr->expr->Accept(this);
		DbgOutdent();

		DbgIndent();
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}

	void TreePrinter::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		DbgPrintln("lambda");
		DbgIndent();
		DbgPrintln(
			"source info: [%d:%d] -> [%d:%d]",
			expr->sourceInfo.start.line,
			expr->sourceInfo.start.column,
			expr->sourceInfo.end.line,
			expr->sourceInfo.end.column
		);

		if (!expr->args.empty()) {
			DbgPrintln("args");
			DbgIndent();
			for (auto& arg : expr->args) {
				arg->Accept(this);
			}
			DbgOutdent();
		}

		
		DbgPrintln("body");
		DbgIndent();
		expr->body->Accept(this);
		DbgOutdent();

		DbgOutdent();

		DbgIndent();
		
		DbgPrintln("detailed type: %s", TokenTypeToString(expr->type));
		DbgOutdent();
	}
}
