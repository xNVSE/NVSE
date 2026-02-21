#include "LambdaTransformer.h"

#include <format>

#include "TreePrinter.h"
#include "nvse/Compiler/Utils.h"
#include "nvse/Compiler/AST/AST.h"

namespace Compiler::Passes {
	void LambdaTransformer::VisitBlockStmt(Statements::Block* stmt) {
		auto& statementList = stmt->statements;
		for (int i = 0; i < statementList.size(); i++) {
			auto it = (statementList.begin() + i);

			if (const auto declStmt = std::dynamic_pointer_cast<Statements::VarDecl>(*it)) {
				auto& declarations = declStmt->declarations;
				for (int j = 0; j < declarations.size(); j++) {
					const auto& [token, expr, info] = declarations[j];
					if (info && info->lambda_type_info.is_lambda) {
						const auto ident = std::make_shared<Expressions::StringExpr>(
							std::format(
								"__lambda_{}_{}",
								info->index,
								token->str
							), token->sourceInfo
						);

						// Build SetModLocalData Command
						const auto setMLD = std::make_shared<Expressions::CallExpr>(
							std::make_shared<Expressions::IdentExpr>("SetModLocalData", token->sourceInfo),
							std::vector<ExprPtr> { ident, expr },
							token->sourceInfo
						);

						setMLD->cmdInfo = g_scriptCommands.GetByName("SetModLocalData");

						// Build ExprStmt
						const auto exprStmt = std::make_shared<Statements::ExpressionStatement>(setMLD, setMLD->sourceInfo);
						statementList.insert(statementList.begin() + i, exprStmt);

						const auto msg = "Built SetModLocalData call for ident";
						const auto highlight = HighlightSourceSpan(pAst->lines, msg, token->sourceInfo + expr->sourceInfo, ESCAPE_CYAN);
						DbgPrint(highlight.c_str());

						// Remove this variable declaration
						declarations.erase(declarations.begin() + j);
						j--;
						i++;
					} else {
						token->Accept(this);
						if (expr) {
							expr->Accept(this);
						}
					}
				}

				// Remove empty var decl statements
				if (declarations.empty()) {
					statementList.erase(statementList.begin() + i);
					i--;

					const auto msg = "Erased old variable declaration";
					const auto highlight = HighlightSourceSpan(pAst->lines, msg, declStmt->sourceInfo, ESCAPE_CYAN);
					DbgPrint(highlight.c_str());
				}
			}

			else {
				(*it)->Accept(this);
			}
		}
	}

	void LambdaTransformer::TransformExpr(std::shared_ptr<Expr>* ppExpr) {
		if (const auto identExpr = std::dynamic_pointer_cast<Expressions::IdentExpr>(*ppExpr)) {
			if (!identExpr->varInfo || !identExpr->varInfo->lambda_type_info.is_lambda) {
				return Visitor::TransformExpr(ppExpr);
			}

			const auto ident = std::make_shared<Expressions::StringExpr>(
				std::format(
					"__lambda_{}_{}",
					identExpr->varInfo->index,
					identExpr->str
				), identExpr->sourceInfo
			);

			// Build GetModLocalData
			const auto getMLD = std::make_shared<Expressions::CallExpr>(
				std::make_shared<Expressions::IdentExpr>("GetModLocalData", identExpr->sourceInfo),
				std::vector<ExprPtr> { ident },
				identExpr->sourceInfo
			);

			getMLD->cmdInfo = g_scriptCommands.GetByName("GetModLocalData");

			// Replace ident expr with call
			(*ppExpr) = getMLD;

			const auto msg = "Transformed lambda read into GetModLocalData";
			const auto highlight = HighlightSourceSpan(pAst->lines, msg, identExpr->sourceInfo, ESCAPE_CYAN);
			DbgPrint(highlight.c_str());

			return;
		}

		return Visitor::TransformExpr(ppExpr);
	}
}
