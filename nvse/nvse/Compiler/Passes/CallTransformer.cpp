#include "CallTransformer.h"

#include <format>

#include "TreePrinter.h"
#include "nvse/Compiler/Utils.h"
#include "nvse/Compiler/AST/AST.h"

namespace Compiler::Passes {
	void CallTransformer::TransformCall(Expressions::CallExpr* expr) const {
		expr->args.insert(expr->args.begin(), expr->identifier);
		expr->identifier = std::make_shared<Expressions::IdentExpr>(
			"Call",
			expr->identifier->sourceInfo
		);

		// Log this
		const auto msg = "Transformed call expression";
		const auto highlight = HighlightSourceSpan(pAst->lines, msg, expr->sourceInfo, ESCAPE_CYAN);
		DbgPrint(highlight.c_str());

		TreePrinter tp{};
		expr->Accept(&tp);
	}

	void CallTransformer::VisitCallExpr(Expressions::CallExpr* expr) {
		if (expr->left || !expr->identifier) {
			return Visitor::VisitCallExpr(expr);
		}

		// Transform lambda(args) into call(lambda, args)
		const auto varInfo = expr->identifier->varInfo;
		if (varInfo && varInfo->type == Script::VariableType::eVarType_Ref) {
			TransformCall(expr);
			return Visitor::VisitCallExpr(expr);
		}

		// Transform UDF(args) into call(UDF, args)
		const auto &name = expr->identifier->str;
		const auto form = GetFormByID(name.c_str());
		if (form && DYNAMIC_CAST(form, TESForm, Script)) {
			TransformCall(expr);
		}

		return Visitor::VisitCallExpr(expr);
	}
}
