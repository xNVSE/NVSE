#pragma once
#include "nvse/Compiler/AST/Visitor.h"

namespace Compiler::Passes {
	class LoopTransformer : public Visitor
	{
		AST* pAst;
		std::stack<StmtPtr> loopIncrements{};
	public:
		explicit LoopTransformer(AST* pAst) : pAst{ pAst } {}

		void VisitBlockStmt(Statements::Block* stmt) override;

		void VisitForStmt(Statements::For* stmt) override;
		void VisitForEachStmt(Statements::ForEach* stmt) override;
		void VisitWhileStmt(Statements::While* stmt) override;
		void VisitLambdaExpr(Expressions::LambdaExpr* expr) override;
	};
}
