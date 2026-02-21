#pragma once
#include "../AST/Visitor.h"

namespace Compiler {
	class TreePrinter : public Visitor {
		int curTab = 0;
		void PrintTabs(const bool debugOnly = true);

	public:
		TreePrinter() = default;

		void Visit(AST* script) override;
		void VisitBeginStmt(Statements::Begin* stmt) override;
		void VisitFnStmt(Statements::UDFDecl* stmt) override;
		void VisitVarDeclStmt(Statements::VarDecl* stmt) override;

		void VisitExprStmt(Statements::ExpressionStatement* stmt) override;
		void VisitForStmt(Statements::For* stmt) override;
		void VisitForEachStmt(Statements::ForEach* stmt) override;
		void VisitIfStmt(Statements::If* stmt) override;
		void VisitReturnStmt(Statements::Return* stmt) override;
		void VisitContinueStmt(Statements::Continue* stmt) override;
		void VisitBreakStmt(Statements::Break* stmt) override;
		void VisitWhileStmt(Statements::While* stmt) override;
		void VisitBlockStmt(Statements::Block* stmt) override;

		void VisitAssignmentExpr(Expressions::AssignmentExpr* expr) override;
		void VisitTernaryExpr(Expressions::TernaryExpr* expr) override;
		void VisitInExpr(Expressions::InExpr* expr) override;
		void VisitBinaryExpr(Expressions::BinaryExpr* expr) override;
		void VisitUnaryExpr(Expressions::UnaryExpr* expr) override;
		void VisitSubscriptExpr(Expressions::SubscriptExpr* expr) override;
		void VisitCallExpr(Expressions::CallExpr* expr) override;
		void VisitGetExpr(Expressions::GetExpr* expr) override;
		void VisitBoolExpr(Expressions::BoolExpr* expr) override;
		void VisitNumberExpr(Expressions::NumberExpr* expr) override;
		void VisitStringExpr(Expressions::StringExpr* expr) override;
		void VisitIdentExpr(Expressions::IdentExpr* expr) override;
		void VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) override;
		void VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) override;
		void VisitGroupingExpr(Expressions::GroupingExpr* expr) override;
		void VisitLambdaExpr(Expressions::LambdaExpr* expr) override;
	};
}