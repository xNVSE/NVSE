#pragma once
#include <stack>

#include <nvse/GameScript.h>
#include <nvse/ScriptTokens.h>
#include <nvse/ScriptUtils.h>
#include <nvse/Compiler/Visitor.h>
#include <nvse/Compiler/AST/AST.h>

#include "nvse/Compiler/NVSECompilerUtils.h"

namespace Compiler::Passes {
	struct Scope {
		bool bIsRoot = false;
		Scope* parent = nullptr;
		std::map<std::string, std::shared_ptr<VarInfo>> variables{};

		std::shared_ptr<VarInfo> AddVariable(
			const std::string& name, 
			const Script::VariableType type,
			std::vector<std::shared_ptr<VarInfo>> &globalVars,
			std::vector<std::shared_ptr<VarInfo>> &tempVars
		) {
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

		std::shared_ptr<VarInfo> Lookup(const std::string& name) {
			const auto existing = variables.find(name);
			if (existing != variables.end()) {
				return existing->second;
			}

			if (parent) {
				return parent->Lookup(name);
			}

			return nullptr;
		}
	};

	class VariableResolution : Visitor {
		std::vector<std::shared_ptr<VarInfo>> globalVars{};
		std::vector<std::shared_ptr<VarInfo>> tempVars{};

		Script* pScript = nullptr;

		std::stack<std::shared_ptr<Scope>> scopes{};
		std::shared_ptr<Scope> currentScope{};
		std::shared_ptr<Scope> EnterScope();
		std::shared_ptr<Scope> LeaveScope();

	public:
		static bool Resolve(Script *pScript, AST* pAST) {
			auto resolver = VariableResolution{ pScript };
			resolver.Visit(pAST);
			return !resolver.HadError();
		}

		VariableResolution(Script* pScript) : pScript{ pScript } {
			scopes.emplace(std::make_shared<Scope>(true, nullptr));
			currentScope = scopes.top();
		}

		void Visit(AST* script) override;
		void VisitBeginStmt(Statements::Begin* stmt) override;
		void VisitFnStmt(Statements::UDFDecl* stmt) override;
		void VisitVarDeclStmt(Statements::VarDecl* stmt) override;
		void VisitForStmt(Statements::For* stmt) override;
		void VisitForEachStmt(Statements::ForEach* stmt) override;
		void VisitIfStmt(Statements::If* stmt) override;
		void VisitWhileStmt(Statements::While* stmt) override;
		void VisitBlockStmt(Statements::Block* stmt) override;
		void VisitIdentExpr(Expressions::IdentExpr* expr) override;
		void VisitLambdaExpr(Expressions::LambdaExpr* expr) override;
	};
}
