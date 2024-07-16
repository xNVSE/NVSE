#pragma once
#include <stack>

#include "NVSELexer.h"
#include "NVSEScope.h"
#include "NVSEVisitor.h"
#include "ScriptTokens.h"

class NVSETypeChecker : NVSEVisitor {
    struct ReturnInfo {
        Token_Type returnType {kTokenType_Invalid};
        size_t line {0};
    };
    
    bool hadError = false;
    NVSEScript *script;
    Script* engineScript;
    
    std::stack<bool> insideLoop {};
    std::stack<std::shared_ptr<NVSEScope>> scopes {};
    std::stack<ReturnInfo> returnType {};
    
    uint32_t scopeIndex {1};
    
    bool bScopedGlobal = false;

    std::shared_ptr<NVSEScope> EnterScope();
    void LeaveScope();
    void error(size_t line, std::string msg);
    void error(size_t line, size_t column, std::string msg);

public:
    NVSETypeChecker(NVSEScript* script, Script* engineScript) : script(script), engineScript(engineScript) {}
    bool check();
    
    void VisitNVSEScript(NVSEScript* script) override;
    void VisitBeginStmt(BeginStmt* stmt) override;
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
