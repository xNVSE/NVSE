#include "MatchTransformer.h"

#include <format>

#include "TreePrinter.h"
#include "nvse/Compiler/Utils.h"
#include "nvse/Compiler/AST/AST.h"

namespace Compiler::Passes {
	void MatchTransformer::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		for (auto& [name, expr, _] : stmt->declarations) {
			if (expr) {
				TransformExpr(&expr);
				expr->Accept(this);
			}

			// Unbind this variable name in case of match expression
			for (auto& i : matchBindingStack) {
				if (!strcmp(i.c_str(), name->str.c_str())) {
					i = "";
				}
			}
		}
	}

	void MatchTransformer::VisitBlockStmt(Statements::Block* stmt) {
		auto& statements = stmt->statements;
		for (size_t i = 0; i < statements.size(); i++) {
			const auto line = statements[i];
			if (const auto matchExpr = line->As<Statements::Match>()) {
				const auto matchValue = matchExpr->expression;

				const auto bindingName = std::format("_mr{}", matchBindingStack.size() + 1);

				// See if any arm is using a binding
				for (const auto &[binding, expr, block] : matchExpr->arms) {
					if (binding) {
						matchBindingStack.push_back(binding->str);
						expr->Accept(this);
						block->Accept(this);
						matchBindingStack.pop_back();

						binding->str = bindingName;
					}
				}

				auto it = statements.begin() + static_cast<int>(i);

				// If any arm was using a binding, create a var decl for it
				{
					// Use array for type abuse
					auto arrLit = std::make_shared<Expressions::ArrayLiteralExpr>(
						std::vector{ matchValue },
						matchValue->sourceInfo
					);

					auto ident = std::make_shared<Expressions::IdentExpr>(bindingName, matchValue->sourceInfo);

					auto varDecl = std::make_shared<Statements::VarDecl>(
						TokenType::ArrayType,
						ident,
						arrLit,
						matchValue->sourceInfo
					);

					// Insert this before the match statement
					statements.insert(it, varDecl);

					// Increment iterator so that it stays pointing at match
					it = statements.begin() + static_cast<int>(i + 1);

					// Change the match expression to be *_match_result
					matchExpr->expression = std::make_shared<Expressions::UnaryExpr>(
						Token{ TokenType::Unbox },
						ident,
						false
					);
				}

				// Now transpile it to if/elseif chain
				std::shared_ptr<Statements::If> curIfStmt = nullptr;
				std::shared_ptr<Statements::If> firstIfStmt = nullptr;

				for (const auto &arm : matchExpr->arms) {
					std::shared_ptr<Expr> boolExpr;
					
					// If arm has a binding, just use its expression
					if (arm.binding) {
						boolExpr = arm.expr;
					} 

					// No binding, build <match cond> == <arm expr>
					else {
						boolExpr = std::make_shared<Expressions::BinaryExpr>(
							Token{ TokenType::EqEq },
							matchExpr->expression,
							arm.expr,
							arm.expr->sourceInfo
						);
					}

					// if (arm condition)
					const auto ifStmt = std::make_shared<Statements::If>(
						boolExpr, 
						arm.block, 
						nullptr, 
						arm.block->sourceInfo
					);

					if (curIfStmt == nullptr) {
						firstIfStmt = ifStmt;
					} else {
						curIfStmt->elseBlock = std::make_shared<Statements::Block>(
							std::vector<StmtPtr>{ ifStmt }, 
							ifStmt->sourceInfo
						);
					}

					curIfStmt = ifStmt;
				}

				if (matchExpr->defaultCase && curIfStmt) {
					curIfStmt->elseBlock = matchExpr->defaultCase;
				}

				// Replace match stmt with if
				*it = firstIfStmt;

				// Log this
				const auto msg = "Transformed match statement";
				const auto highlight = HighlightSourceSpan(pAst->lines, msg, matchExpr->sourceInfo, ESCAPE_CYAN);
				DbgPrint(highlight.c_str());

				TreePrinter tp{};
				matchExpr->Accept(&tp);
			}
			else {
				line->Accept(this);
			}
		}
	}

	void MatchTransformer::TransformExpr(std::shared_ptr<Expr>* ppExpr) {
		if (matchBindingStack.empty()) {
			return;
		}

		if (const auto identExpr = std::dynamic_pointer_cast<Expressions::IdentExpr>(*ppExpr)) {
			for (int i = 0; i < matchBindingStack.size(); i++) {
				const auto& binding = matchBindingStack[i];
				if (!strcmp(binding.c_str(), identExpr->str.c_str())) {
					identExpr->str = std::format("_mr{}", i + 1);

					*ppExpr = std::make_shared<Expressions::UnaryExpr>(
						Token{ TokenType::Unbox },
						identExpr,
						false
					);
				}
			}
		}
	}
}
