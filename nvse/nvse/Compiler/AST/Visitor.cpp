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
				this->TransformExpr(&expr);
				expr->Accept(this);
			}
		}
	}

	void Visitor::VisitExprStmt(Statements::ExpressionStatement* stmt) {
		if (stmt->expr) {
			this->TransformExpr(&stmt->expr);
			stmt->expr->Accept(this);
		}
	}

	void Visitor::VisitForStmt(Statements::For* stmt) {
		if (stmt->init) {
			stmt->init->Accept(this);
		}

		if (stmt->cond) {
			this->TransformExpr(&stmt->cond);
			stmt->cond->Accept(this);
		}

		this->TransformExpr(&stmt->post);
		stmt->post->Accept(this);

		stmt->block->Accept(this);
	}

	void Visitor::VisitForEachStmt(Statements::ForEach* stmt) {
		for (const auto& decl : stmt->declarations) {
			if (decl) {
				decl->Accept(this);
			}
		}

		this->TransformExpr(&stmt->rhs);
		stmt->rhs->Accept(this);

		stmt->block->Accept(this);
	}

	void Visitor::VisitIfStmt(Statements::If* stmt) {
		this->TransformExpr(&stmt->cond);
		stmt->cond->Accept(this);

		stmt->block->Accept(this);

		if (stmt->elseBlock) {
			stmt->elseBlock->Accept(this);
		}
	}

	void Visitor::VisitReturnStmt(Statements::Return* stmt) {
		if (stmt->expr) {
			this->TransformExpr(&stmt->expr);
			stmt->expr->Accept(this);
		}
	}

	void Visitor::VisitContinueStmt(Statements::Continue* stmt) {}

	void Visitor::VisitBreakStmt(Statements::Break* stmt) {}

	void Visitor::VisitWhileStmt(Statements::While* stmt) {
		this->TransformExpr(&stmt->cond);
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

	void Visitor::VisitMatchStmt(Statements::Match* stmt) {
		this->TransformExpr(&stmt->expression);
		stmt->expression->Accept(this);

		for (auto& [binding, expr, block] : stmt->arms) {
			if (binding) {
				binding->Accept(this);
			}

			this->TransformExpr(&expr);
			expr->Accept(this);

			block->Accept(this);
		}

		if (stmt->defaultCase) {
			stmt->defaultCase->Accept(this);
		}
	}

	void Visitor::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
		expr->left->Accept(this);

		this->TransformExpr(&expr->expr);
		expr->expr->Accept(this);
	}

	void Visitor::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
		this->TransformExpr(&expr->cond);
		expr->cond->Accept(this);

		this->TransformExpr(&expr->left);
		expr->left->Accept(this);

		this->TransformExpr(&expr->right);
		expr->right->Accept(this);
	}

	void Visitor::VisitInExpr(Expressions::InExpr* expr) {
		this->TransformExpr(&expr->lhs);
		expr->lhs->Accept(this);

		if (!expr->values.empty()) {
			for (auto& val : expr->values) {
				this->TransformExpr(&val);
				val->Accept(this);
			}
		}

		// Any other expression, compiles to ar_find
		else {
			this->TransformExpr(&expr->expression);
			expr->expression->Accept(this);
		}
	}

	void Visitor::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
		this->TransformExpr(&expr->left);
		expr->left->Accept(this);

		this->TransformExpr(&expr->right);
		expr->right->Accept(this);
	}

	void Visitor::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
		this->TransformExpr(&expr->expr);
		expr->expr->Accept(this);
	}

	void Visitor::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
		this->TransformExpr(&expr->left);
		expr->left->Accept(this);

		this->TransformExpr(&expr->index);
		expr->index->Accept(this);
	}

	void Visitor::VisitCallExpr(Expressions::CallExpr* expr) {
		if (expr->left) {
			this->TransformExpr(&expr->left);
			expr->left->Accept(this);
		}

		expr->identifier->Accept(this);

		for (auto& arg : expr->args) {
			this->TransformExpr(&arg);
			arg->Accept(this);
		}
	}

	void Visitor::VisitGetExpr(Expressions::GetExpr* expr) {
		this->TransformExpr(&expr->left);
		expr->left->Accept(this);
	}

	void Visitor::VisitBoolExpr(Expressions::BoolExpr* expr) {}

	void Visitor::VisitNumberExpr(Expressions::NumberExpr* expr) {}

	void Visitor::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {
		for (auto& val : expr->values) {
			this->TransformExpr(&val);
			val->Accept(this);
		}
	}

	void Visitor::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
		for (auto& val : expr->values) {
			this->TransformExpr(&val);
			val->Accept(this);
		}
	}

	void Visitor::VisitStringExpr(Expressions::StringExpr* expr) {}

	void Visitor::VisitIdentExpr(Expressions::IdentExpr* expr) {}

	void Visitor::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
		this->TransformExpr(&expr->expr);
		expr->expr->Accept(this);
	}

	void Visitor::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		for (const auto& decl : expr->args) {
			decl->Accept(this);
		}

		expr->body->Accept(this);
	}

	void Visitor::TransformExpr(std::shared_ptr<Expr>* ppExpr) {}
}
