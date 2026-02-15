#pragma once

#include <nvse/Compiler/AST/ASTForward.h>

namespace Compiler {
	class Visitor {
	protected:
		boolean had_error = false;
	public:
		virtual ~Visitor() = default;

		virtual void Visit(AST* script);

		virtual void VisitBeginStmt(Statements::Begin* stmt);
		virtual void VisitFnStmt(Statements::UDFDecl* stmt);
		virtual void VisitVarDeclStmt(Statements::VarDecl* stmt);
		virtual void VisitExprStmt(const Statements::ExpressionStatement* stmt);
		virtual void VisitForStmt(Statements::For* stmt);
		virtual void VisitForEachStmt(Statements::ForEach* stmt);
		virtual void VisitIfStmt(Statements::If* stmt);
		virtual void VisitReturnStmt(Statements::Return* stmt);
		virtual void VisitContinueStmt(Statements::Continue* stmt);
		virtual void VisitBreakStmt(Statements::Break* stmt);
		virtual void VisitWhileStmt(Statements::While* stmt);
		virtual void VisitBlockStmt(Statements::Block* stmt);
		virtual void VisitShowMessageStmt(Statements::ShowMessage* stmt);

		virtual void VisitAssignmentExpr(Expressions::AssignmentExpr* expr);
		virtual void VisitTernaryExpr(Expressions::TernaryExpr* expr);
		virtual void VisitInExpr(Expressions::InExpr* expr);
		virtual void VisitBinaryExpr(Expressions::BinaryExpr* expr);
		virtual void VisitUnaryExpr(Expressions::UnaryExpr* expr);
		virtual void VisitSubscriptExpr(Expressions::SubscriptExpr* expr);
		virtual void VisitCallExpr(Expressions::CallExpr* expr);
		virtual void VisitGetExpr(Expressions::GetExpr* expr);
		virtual void VisitBoolExpr(Expressions::BoolExpr* expr);
		virtual void VisitNumberExpr(Expressions::NumberExpr* expr);
		virtual void VisitStringExpr(Expressions::StringExpr* expr);
		virtual void VisitIdentExpr(Expressions::IdentExpr* expr);
		virtual void VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr);
		virtual void VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr);
		virtual void VisitGroupingExpr(Expressions::GroupingExpr* expr);
		virtual void VisitLambdaExpr(Expressions::LambdaExpr* expr);

		virtual bool HadError() {
			return had_error;
		}
	};
}
