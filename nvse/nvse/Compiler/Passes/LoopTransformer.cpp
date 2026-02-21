#include "LoopTransformer.h"

#include "TreePrinter.h"
#include "nvse/Compiler/Utils.h"
#include "nvse/Compiler/AST/AST.h"

namespace Compiler::Passes {
	void LoopTransformer::VisitBlockStmt(Statements::Block* stmt) {
		auto &statementList = stmt->statements;
		for (int i = 0; i < statementList.size(); i++) {
			auto it = (statementList.begin() + i);

			if (const auto forStmt = std::dynamic_pointer_cast<Statements::For>(*it)) {
				// Move the for loop's <init> statement to be right before the resulting while loop
				// 
				// for (int i = 0; ...)
				//
				// Turns into:
				//
				// int i = 0;
				// while (...)
				// 
				if (const auto &init = forStmt->init) {
					statementList.insert(it, init);
					forStmt->init = nullptr;
					it = statementList.begin() + i + 1;
				}

				// Create standalone expression statement that will be inserted at 
				// the end of the while block and before any `continue` statements
				auto exprStmt = std::make_shared<Statements::ExpressionStatement>(
					forStmt->post, 
					forStmt->post->sourceInfo
				);

				auto& block = forStmt->block;
				auto& blockStatements = block->statements;

				// Remove any continue statement that is at the end of a loop block
				// Not likely to happen but is an easy optimization to make
				if (!blockStatements.empty() && blockStatements.back()->IsType<Statements::Continue>()) {
					const auto end = blockStatements.end() - 1;
					const auto msg = "Removed redundant 'continue' from end of loop block";
					const auto highlight = HighlightSourceSpan(pAst->lines, msg, (*end)->sourceInfo, ESCAPE_CYAN);
					DbgPrintln(highlight.c_str());
					blockStatements.erase(end);
				}

				// Scope management
				loopIncrements.emplace(exprStmt);
				block->Accept(this);
				loopIncrements.pop();

				const auto highlight = HighlightSourceSpan(
					pAst->lines, 
					"Inserted <incr> before end of block", 
					block->sourceInfo, 
					ESCAPE_CYAN
				);
				DbgPrint(highlight.c_str());

				block->statements.push_back(exprStmt);

				// Replace for loop with while
				*it = std::make_shared<Statements::While>(
					forStmt->cond,
					block,
					forStmt->sourceInfo
				);
			} 
			
			// Insert increment condition before continue statements
			else if (const auto continueStmt = std::dynamic_pointer_cast<Statements::Continue>(*it)) {
				if (!loopIncrements.empty() && loopIncrements.top()) {
					const auto msg = "Inserted <incr> before continue stmt";
					const auto highlight = HighlightSourceSpan(pAst->lines, msg, continueStmt->sourceInfo, ESCAPE_CYAN);
					DbgPrint(highlight.c_str());

					statementList.insert(it, loopIncrements.top());
					
					// Skip the continue statement
					i++;
				}
			}
			
			else {
				(*it)->Accept(this);
			}
		}
	}

	void LoopTransformer::VisitForStmt(Statements::For* stmt) {
		loopIncrements.emplace(nullptr);
		Visitor::VisitForStmt(stmt);
		loopIncrements.pop();
	}

	void LoopTransformer::VisitForEachStmt(Statements::ForEach* stmt) {
		loopIncrements.emplace(nullptr);
		Visitor::VisitForEachStmt(stmt);
		loopIncrements.pop();
	}

	void LoopTransformer::VisitWhileStmt(Statements::While* stmt) {
		loopIncrements.emplace(nullptr);
		Visitor::VisitWhileStmt(stmt);
		loopIncrements.pop();
	}

	void LoopTransformer::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		loopIncrements.emplace(nullptr);
		Visitor::VisitLambdaExpr(expr);
		loopIncrements.pop();
	}
}
