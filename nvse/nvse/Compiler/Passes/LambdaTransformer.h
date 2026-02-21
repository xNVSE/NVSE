#pragma once
#include "nvse/Compiler/AST/Visitor.h"

namespace Compiler::Passes {
	class LambdaTransformer : public Visitor
	{
		AST* pAst;
	public:
		explicit LambdaTransformer(AST* pAst) : pAst{ pAst } {}
		void VisitBlockStmt(Statements::Block* stmt) override;
		void TransformExpr(std::shared_ptr<Expr>* ppExpr) override;
	};
}
