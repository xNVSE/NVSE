#pragma once
#include "nvse/Compiler/AST/Visitor.h"

namespace Compiler::Passes {
	class CallTransformer : public Visitor {
		std::vector<std::string> matchBindingStack{};
		AST* pAst;

	public:
		explicit CallTransformer(AST* pAst) : pAst{ pAst } {}
		void TransformCall(Expressions::CallExpr* expr) const;
		void VisitCallExpr(Expressions::CallExpr* expr) override;
	};
}
