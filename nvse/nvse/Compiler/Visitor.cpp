#include "Visitor.h"
#include "Parser.h"
#include <nvse/Compiler/AST/AST.h>

#include <format>

namespace Compiler {

	void Visitor::Visit(AST* script) {
		for (const auto& global : script->globalVars) {
			global->Accept(this);
		}

		for (const auto& block : script->blocks) {
			block->Accept(this);
		}
	}

	void Visitor::VisitBeginStmt(Statements::Begin* stmt) {
		stmt->block->Accept(this);
	}

	void Visitor::VisitFnStmt(Statements::UDFDecl* stmt) {
		for (const auto& decl : stmt->args) {
			decl->Accept(this);
		}

		stmt->body->Accept(this);
	}

	void Visitor::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		for (auto& [name, expr, _] : stmt->declarations) {
			if (expr) {
				expr->Accept(this);
			}
		}
	}

	void Visitor::VisitExprStmt(const Statements::ExpressionStatement* stmt) {
		if (stmt->expr) {
			stmt->expr->Accept(this);
		}
	}

	void Visitor::VisitForStmt(Statements::For* stmt) {
		if (stmt->init) {
			stmt->init->Accept(this);
		}

		if (stmt->cond) {
			stmt->cond->Accept(this);
		}

		stmt->post->Accept(this);
		stmt->block->Accept(this);
	}

	void Visitor::VisitForEachStmt(Statements::ForEach* stmt) {
		for (const auto& decl : stmt->declarations) {
			if (decl) {
				decl->Accept(this);
			}
		}
		stmt->rhs->Accept(this);
		stmt->block->Accept(this);
	}

	void Visitor::VisitIfStmt(Statements::If* stmt) {
		stmt->cond->Accept(this);
		stmt->block->Accept(this);

		if (stmt->elseBlock) {
			stmt->elseBlock->Accept(this);
		}
	}

	void Visitor::VisitReturnStmt(Statements::Return* stmt) {
		if (stmt->expr) {
			stmt->expr->Accept(this);
		}
	}

	void Visitor::VisitContinueStmt(Statements::Continue* stmt) {}

	void Visitor::VisitBreakStmt(Statements::Break* stmt) {}

	void Visitor::VisitWhileStmt(Statements::While* stmt) {
		stmt->cond->Accept(this);
		stmt->block->Accept(this);
	}

	void Visitor::VisitBlockStmt(Statements::Block* stmt) {
		for (const auto& statement : stmt->statements) {
			statement->Accept(this);
		}
	}

	void Visitor::VisitShowMessageStmt(Statements::ShowMessage* stmt) {
		stmt->message->Accept(this);
		for (const auto& statement : stmt->args) {
			statement->Accept(this);
		}
	}

	void Visitor::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
		expr->left->Accept(this);
		expr->expr->Accept(this);
	}

	void Visitor::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
		expr->cond->Accept(this);
		expr->left->Accept(this);
		expr->right->Accept(this);
	}

	void Visitor::VisitInExpr(Expressions::InExpr* expr) {
		expr->lhs->Accept(this);

		if (!expr->values.empty()) {
			for (const auto& val : expr->values) {
				val->Accept(this);
			}
		}

		// Any other expression, compiles to ar_find
		else {
			expr->expression->Accept(this);
		}
	}

	void Visitor::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
		expr->left->Accept(this);
		expr->right->Accept(this);
	}

	void Visitor::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
		expr->expr->Accept(this);
	}

	void Visitor::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
		expr->left->Accept(this);
		expr->index->Accept(this);
	}

	void Visitor::VisitCallExpr(Expressions::CallExpr* expr) {
		if (expr->left) {
			expr->left->Accept(this);
		}
		for (const auto& arg : expr->args) {
			arg->Accept(this);
		}
	}

	void Visitor::VisitGetExpr(Expressions::GetExpr* expr) {
		expr->left->Accept(this);
	}

	void Visitor::VisitBoolExpr(Expressions::BoolExpr* expr) {}

	void Visitor::VisitNumberExpr(Expressions::NumberExpr* expr) {}

	void Visitor::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {
		for (const auto& val : expr->values) {
			val->Accept(this);
		}
	}

	void Visitor::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
		for (const auto& val : expr->values) {
			val->Accept(this);
		}
	}

	void Visitor::VisitStringExpr(Expressions::StringExpr* expr) {}

	void Visitor::VisitIdentExpr(Expressions::IdentExpr* expr) {}

	void Visitor::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
		expr->expr->Accept(this);
	}

	void Visitor::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		for (const auto& decl : expr->args) {
			decl->Accept(this);
		}

		expr->body->Accept(this);
	}
}