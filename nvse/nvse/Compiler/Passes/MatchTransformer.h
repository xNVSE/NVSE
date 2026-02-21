#pragma once
#include "nvse/Compiler/AST/Visitor.h"

namespace Compiler::Passes {
	class MatchTransformer : public Visitor {
		std::vector<std::string> matchBindingStack{};
		AST* pAst;
	public:
		explicit MatchTransformer(AST* pAst) : pAst{pAst}{}

		void VisitVarDeclStmt(Statements::VarDecl* stmt) override;
		void VisitBlockStmt(Statements::Block* stmt) override;
		void TransformExpr(std::shared_ptr<Expr>* ppExpr) override;
	};
}
