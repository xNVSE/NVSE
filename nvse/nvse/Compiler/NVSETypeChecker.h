#pragma once
#include <stack>

#include "../ScriptTokens.h"

#include <nvse/Compiler/Lexer/Lexer.h>
#include "Visitor.h"

namespace Compiler {
    class NVSETypeChecker : Visitor {
        struct ReturnInfo {
            Token_Type returnType{ kTokenType_Invalid };
            size_t line{ 0 };
        };

        bool hadError = false;
        AST* script;
        Script* engineScript;

        std::stack<bool> insideLoop{};
        std::stack<ReturnInfo> returnType{};

        bool bScopedGlobal = false;

        void error(size_t line, std::string msg);
        void error(size_t line, size_t column, std::string msg);

    public:
        NVSETypeChecker(AST* script, Script* engineScript) : script(script), engineScript(engineScript) {}
        bool check();

        void Visit(AST* script) override;
        void VisitFnStmt(Statements::UDFDecl* stmt) override;
        void VisitVarDeclStmt(Statements::VarDecl* stmt) override;
        void VisitExprStmt(const Statements::ExpressionStatement* stmt) override;
        void VisitForStmt(Statements::For* stmt) override;
        void VisitForEachStmt(Statements::ForEach* stmt) override;
        void VisitIfStmt(Statements::If* stmt) override;
        void VisitReturnStmt(Statements::Return* stmt) override;
        void VisitContinueStmt(Statements::Continue* stmt) override;
        void VisitBreakStmt(Statements::Break* stmt) override;
        void VisitWhileStmt(Statements::While* stmt) override;
        void VisitBlockStmt(Statements::Block* stmt) override;
        void VisitAssignmentExpr(Expressions::AssignmentExpr* expr) override;
        void VisitTernaryExpr(Expressions::TernaryExpr* expr) override;
        void VisitInExpr(Expressions::InExpr* expr) override;
        void VisitBinaryExpr(Expressions::BinaryExpr* expr) override;
        void VisitUnaryExpr(Expressions::UnaryExpr* expr) override;
        void VisitSubscriptExpr(Expressions::SubscriptExpr* expr) override;
        void RegisterPluginDependency(CommandInfo* cmd);
        void VisitCallExpr(Expressions::CallExpr* expr) override;
        void VisitGetExpr(Expressions::GetExpr* expr) override;
        void VisitBoolExpr(Expressions::BoolExpr* expr) override;
        void VisitNumberExpr(Expressions::NumberExpr* expr) override;
        void VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) override;
        void VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) override;
        void VisitStringExpr(Expressions::StringExpr* expr) override;
        void VisitIdentExpr(Expressions::IdentExpr* expr) override;
        void VisitGroupingExpr(Expressions::GroupingExpr* expr) override;
        void VisitLambdaExpr(Expressions::LambdaExpr* expr) override;
    };

}