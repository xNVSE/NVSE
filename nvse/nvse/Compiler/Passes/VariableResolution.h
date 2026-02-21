#pragma once
#include <stack>

#include <nvse/GameScript.h>
#include <nvse/ScriptTokens.h>
#include <nvse/Compiler/AST/ASTForward.h>
#include <nvse/Compiler/AST/Visitor.h>

namespace Compiler::Passes {
	struct Scope {
		bool bIsRoot = false;
		Scope* parent = nullptr;
		std::map<std::string, std::shared_ptr<VarInfo>> variables{};

		std::shared_ptr<VarInfo> AddVariable(
			const std::string& name, 
			Script::VariableType type,
			std::vector<std::shared_ptr<VarInfo>> &globalVars,
			std::vector<std::shared_ptr<VarInfo>> &tempVars
		);

		std::shared_ptr<VarInfo> Lookup(const std::string& name);
	};

	class VariableResolution : public Visitor {
		std::vector<std::shared_ptr<VarInfo>> globalVars{};
		std::vector<std::shared_ptr<VarInfo>> tempVars{};

		AST* pAST = nullptr;
		Script* pScript = nullptr;

		std::stack<std::shared_ptr<Scope>> scopes{};
		std::shared_ptr<Scope> currentScope{};
		std::shared_ptr<Scope> EnterScope();
		std::shared_ptr<Scope> LeaveScope();

	public:
		VariableResolution(AST* pAST, Script* pScript) : pAST{ pAST }, pScript{ pScript } {}

		void Visit(AST* pAST) override;
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
