#include "VariableResolution.h"

#include "nvse/Compiler/AST/AST.h"
#include "nvse/Compiler/Utils.h"

namespace Compiler::Passes {
	std::shared_ptr<VarInfo> Scope::AddVariable(const std::string& name,
		const Script::VariableType type,
		std::vector<std::shared_ptr<VarInfo>>& globalVars,
		std::vector<std::shared_ptr<VarInfo>>& tempVars) {
		if (variables.contains(name)) {
			const auto& existing = variables[name];

			if (existing->pre_existing) {
				existing->type = type;
				existing->pre_existing = false;
				return existing;
			}

			return nullptr;
		}

		const auto var = std::make_shared<VarInfo>(globalVars.size() + 1, name);
		var->pre_existing = false;
		var->type = type;
		var->detailed_type = VariableType_To_TokenType(type);

		if (bIsRoot) {
			var->remapped_name = var->original_name;
			globalVars.push_back(var);
		}
		else {
			var->remapped_name = "__temp_" + var->original_name + "_" + std::to_string(tempVars.size() + 1);
			tempVars.push_back(var);
		}

		variables[name] = var;

		return var;
	}

	std::shared_ptr<VarInfo> Scope::Lookup(const std::string& name) {
		const auto existing = variables.find(name);
		if (existing != variables.end()) {
			return existing->second;
		}

		if (parent) {
			return parent->Lookup(name);
		}

		return nullptr;
	}

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
			const auto varType = GetScriptTypeFromTokenType(stmt->type);
			if (const auto var = currentScope->AddVariable(token->str, varType, globalVars, tempVars)) {
				info = var;

				if (expr) {
					expr->Accept(this);
				}
			}
			// Existing variable within the same scope, produce an error
			else {
				const auto msg = HighlightSourceSpan(pAST->lines, "Variable with this name already exists in this scope", token->sourceInfo, ESCAPE_RED);
				ErrPrint(msg.c_str());
				this->had_error = true;
			}
		}
	}

	void VariableResolution::Visit(AST* pAST) {
		scopes.emplace(std::make_shared<Scope>(true, nullptr));
		currentScope = scopes.top();

		DbgPrintln("[Variable Resolution]");
		DbgIndent();

		// Need to associate / start indexing after existing non-temp script vars
		if (pScript && pScript->varList.Count() > 0) {
			for (const auto var : pScript->varList) {
				if (strncmp(var->name.CStr(), "__temp", strlen("__temp")) != 0) {
					const std::string varName{ var->name.CStr() };
					const auto added = currentScope->AddVariable(varName, static_cast<Script::VariableType>(var->type), globalVars, tempVars);
					added->pre_existing = true;
				}
			}
		}

		for (const auto& global : pAST->globalVars) {
			global->Accept(this);
		}

		for (const auto& block : pAST->blocks) {
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
			DbgPrintln("Created variable [Original Name: %s, New Name: %s, Index: %ul]", var->original_name.c_str(), var->remapped_name.c_str(), var->index);
		}

		// Assign temp var indices
		auto idx = globalVars.size() + 1;
		for (const auto &var : tempVars) {
			var->index = idx++;

			DbgPrintln("Created variable [Original Name: %s, New Name: %s, Index: %ul]", var->original_name.c_str(), var->remapped_name.c_str(), var->index);
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

		DbgOutdent();
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
		expr->varInfo = currentScope->Lookup(expr->str);
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
