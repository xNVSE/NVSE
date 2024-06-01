#pragma once
#include <stack>

#include "NVSELexer.h"
#include "NVSEScope.h"
#include "NVSEVisitor.h"
#include "ScriptTokens.h"

class NVSETypeChecker : NVSEVisitor {
    std::unordered_map<std::string, TESForm*> formCache{};
    bool hadError = false;
    NVSEScript *script;
    
    std::stack<bool> insideLoop {};
    std::stack<std::shared_ptr<NVSEScope>> scopes {};
    uint32_t scopeIndex {1};

    // Temporarily pushed to stack for things like lambda
    //     args as these must be in global scope
    std::shared_ptr<NVSEScope> globalScope{};
    bool bRenameArgs = false;

    std::shared_ptr<NVSEScope> EnterScope(bool lambdaScope = false);
    void LeaveScope();
    void error(size_t line, std::string msg);
    void error(size_t line, size_t column, std::string msg);

public:
    NVSETypeChecker(NVSEScript* script) : script(script) {}
    bool check();
    
    void VisitNVSEScript(NVSEScript* script) override;
    void VisitBeginStmt(const BeginStmt* stmt) override;
    void VisitFnStmt(FnDeclStmt* stmt) override;
    void VisitVarDeclStmt(VarDeclStmt* stmt) override;
    void VisitExprStmt(const ExprStmt* stmt) override;
    void VisitForStmt(ForStmt* stmt) override;
	void VisitForEachStmt(ForEachStmt* stmt) override;
    void VisitIfStmt(IfStmt* stmt) override;
    void VisitReturnStmt(ReturnStmt* stmt) override;
    void VisitContinueStmt(ContinueStmt* stmt) override;
    void VisitBreakStmt(BreakStmt* stmt) override;
    void VisitWhileStmt(WhileStmt* stmt) override;
    void VisitBlockStmt(BlockStmt* stmt) override;
    void VisitAssignmentExpr(AssignmentExpr* expr) override;
    void VisitTernaryExpr(TernaryExpr* expr) override;
	void VisitInExpr(InExpr* expr) override;
    void VisitBinaryExpr(BinaryExpr* expr) override;
    void VisitUnaryExpr(UnaryExpr* expr) override;
    void VisitSubscriptExpr(SubscriptExpr* expr) override;
    void VisitCallExpr(CallExpr* expr) override;
    void VisitGetExpr(GetExpr* expr) override;
    void VisitBoolExpr(BoolExpr* expr) override;
    void VisitNumberExpr(NumberExpr* expr) override;
	void VisitMapLiteralExpr(MapLiteralExpr* expr) override;
    void VisitArrayLiteralExpr(ArrayLiteralExpr* expr) override;
    void VisitStringExpr(StringExpr* expr) override;
    void VisitIdentExpr(IdentExpr* expr) override;
    void VisitGroupingExpr(GroupingExpr* expr) override;
    void VisitLambdaExpr(LambdaExpr* expr) override;
};
