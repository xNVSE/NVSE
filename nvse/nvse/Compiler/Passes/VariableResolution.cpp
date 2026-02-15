#include "VariableResolution.h"

#include "nvse/Compiler/AST/AST.h"
#include "nvse/Compiler/NVSECompilerUtils.h"

namespace Compiler::Passes {
	std::shared_ptr<Scope> VariableResolution::EnterScope() {
		scopes.emplace(std::make_shared<Scope>(false, currentScope.get()));
		currentScope = scopes.top();
		return currentScope;
	}

	std::shared_ptr<Scope> VariableResolution::LeaveScope() {
		scopes.pop();
		currentScope = scopes.top();
		return currentScope;
	}

	void VariableResolution::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		for (auto & [token, expr, info] : stmt->declarations) {
			const auto varType = GetScriptTypeFromTokenType(stmt->type.type);
			if (const auto var = currentScope->AddVariable(token.lexeme, varType, globalVars, tempVars)) {
				info = var;

				if (expr) {
					expr->Accept(this);
				}
			}
			// Existing variable within the same scope, produce an error
			else {
				CompErr("[line %d:%d] Error - Variable name '%s' already exists in this scope.\n", token.line, token.column, token.lexeme.c_str());
				this->had_error = true;
			}
		}
	}

	void VariableResolution::Visit(AST* script) {
		// Need to associate / start indexing after existing non-temp script vars
		if (pScript && pScript->varList.Count() > 0) {
			for (const auto var : pScript->varList) {
				if (strncmp(var->name.CStr(), "__temp", strlen("__temp")) != 0) {
					const std::string varName{ var->name.CStr() };
					auto added = currentScope->AddVariable(varName, static_cast<Script::VariableType>(var->type), globalVars, tempVars);
					added->pre_existing = true;
				}
			}
		}

		for (const auto& global : script->globalVars) {
			global->Accept(this);
		}

		for (const auto& block : script->blocks) {
			if (const auto fnDecl = dynamic_cast<const Statements::UDFDecl*>(block.get())) {
				for (const auto& arg : fnDecl->args) {
					arg->Accept(this);
				}

				EnterScope();
				fnDecl->body->Accept(this);
				LeaveScope();
			} else {
				block->Accept(this);
			}
		}

		for (const auto& var : globalVars) {
			CompDbg("Created variable [Original Name: %s, New Name: %s, Index: %ul]\n", var->original_name.c_str(), var->remapped_name.c_str(), var->index);
		}

		// Assign temp var indices
		auto idx = globalVars.size() + 1;
		for (const auto &var : tempVars) {
			var->index = idx++;

			CompDbg("Created variable [Original Name: %s, New Name: %s, Index: %ul]\n", var->original_name.c_str(), var->remapped_name.c_str(), var->index);
		}

		// Create engine var list
		if (pScript) {
			pScript->varList.DeleteAll();
			pScript->refList.DeleteAll();

			for (const auto &var : globalVars) {
				const auto pVarInfo = New<VariableInfo>();
				pVarInfo->type = var->type;
				pVarInfo->name = String();
				pVarInfo->name.Set(var->remapped_name.c_str());
				pVarInfo->idx = var->index;
				pScript->varList.Append(pVarInfo);

				// Rebuilding ref list
				if (var->type == Script::eVarType_Ref) {
					const auto ref = New<Script::RefVariable>();
					ref->name = String();
					ref->name.Set(var->remapped_name.c_str());
					ref->varIdx = var->index;
					pScript->refList.Append(ref);
				}
			}

			for (const auto& var : tempVars) {
				const auto pVarInfo = New<VariableInfo>();
				pVarInfo->type = var->type;
				pVarInfo->name = String();
				pVarInfo->name.Set(var->remapped_name.c_str());
				pVarInfo->idx = var->index;
				pScript->varList.Append(pVarInfo);

				// Rebuilding ref list
				if (var->type == Script::eVarType_Ref) {
					const auto ref = New<Script::RefVariable>();
					ref->name = String();
					ref->name.Set(var->remapped_name.c_str());
					ref->varIdx = var->index;
					pScript->refList.Append(ref);
				}
			}

			pScript->info.varCount = pScript->varList.Count();
			pScript->info.numRefs = pScript->refList.Count();
		}
	}

	void VariableResolution::VisitBeginStmt(Statements::Begin* stmt) {
		EnterScope();
		stmt->block->Accept(this);
		LeaveScope();
	}

	void VariableResolution::VisitFnStmt(Statements::UDFDecl* stmt) {
		EnterScope();

		for (const auto &arg : stmt->args) {
			arg->Accept(this);
		}
		stmt->body->Accept(this);

		LeaveScope();
	}

	void VariableResolution::VisitForStmt(Statements::For* stmt) {
		EnterScope();
		Visitor::VisitForStmt(stmt);
		LeaveScope();
	}

	void VariableResolution::VisitForEachStmt(Statements::ForEach* stmt) {
		EnterScope();
		Visitor::VisitForEachStmt(stmt);
		LeaveScope();
	}

	void VariableResolution::VisitIfStmt(Statements::If* stmt) {
		EnterScope();
		stmt->cond->Accept(this);
		stmt->block->Accept(this);
		LeaveScope();

		if (stmt->elseBlock) {
			EnterScope();
			stmt->elseBlock->Accept(this);
			LeaveScope();
		}
	}

	void VariableResolution::VisitWhileStmt(Statements::While* stmt) {
		EnterScope();
		stmt->cond->Accept(this);
		stmt->block->Accept(this);
		LeaveScope();
	}

	void VariableResolution::VisitBlockStmt(Statements::Block* stmt) {
		EnterScope();
		for (const auto &line : stmt->statements) {
			line->Accept(this);
		}
		LeaveScope();
	}

	void VariableResolution::VisitIdentExpr(Expressions::IdentExpr* expr) {
		expr->varInfo = currentScope->Lookup(expr->token.lexeme);
	}

	void VariableResolution::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		EnterScope();
		for (const auto &arg : expr->args) {
			arg->Accept(this);
		}
		expr->body->Accept(this);
		LeaveScope();
	}
}
