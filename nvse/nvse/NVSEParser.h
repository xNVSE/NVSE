#pragma once

#include "NVSELexer.h"
#include "NVSEVisitor.h"

#include <memory>
#include <optional>

#include "ScriptTokens.h"

struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct NVSEScript {
    NVSEToken name;
    std::vector<StmtPtr> globalVars;
    std::vector<StmtPtr> blocks;

    NVSEScript(NVSEToken name, std::vector<StmtPtr> globalVars, std::vector<StmtPtr> blocks) : name(name), globalVars(std::move(globalVars)), blocks(std::move(blocks)) {}

    void Accept(NVSEVisitor* visitor) {
        visitor->VisitNVSEScript(this);
    }
};

struct Stmt {
    int line = 0;

    // Some statements store type such as return and block statement
    Token_Type detailedType = kTokenType_Invalid;
    
    virtual ~Stmt() {}

    virtual void Accept(NVSEVisitor* t) = 0;

    template <typename T>
    bool IsType() {
        return dynamic_cast<T*>(this);
    }
};

struct BeginStmt : Stmt {
    NVSEToken name;
    std::optional<NVSEToken> param;
    StmtPtr block;

    BeginStmt(NVSEToken name, std::optional<NVSEToken> param, StmtPtr block) : name(name), param(param), block(std::move(block)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBeginStmt(this);
    }
};

struct FnDeclStmt : Stmt {
    std::vector<std::shared_ptr<VarDeclStmt>> args;
    StmtPtr body;

    FnDeclStmt(std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : args(std::move(args)), body(std::move(body)) {}

    void Accept(NVSEVisitor *visitor) override {
        visitor->VisitFnStmt(this);
    }
};

struct VarDeclStmt : Stmt {
    NVSEToken type;
    NVSEToken name;
    ExprPtr value{ nullptr };

    VarDeclStmt(NVSEToken type, NVSEToken name) : type(type), name(name) {}
    VarDeclStmt(NVSEToken type, NVSEToken name, ExprPtr value) : type(type), name(name), value(std::move(value)) {}

    void Accept(NVSEVisitor* visitor) override {
	    visitor->VisitVarDeclStmt(this);
    }
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void Accept(NVSEVisitor *visitor) override {
        visitor->VisitExprStmt(this);
    }
};

struct ForStmt : Stmt {
	StmtPtr init;
    ExprPtr cond;
    ExprPtr post;
    std::shared_ptr<BlockStmt> block;

    ForStmt(StmtPtr init, ExprPtr cond, ExprPtr post, std::shared_ptr<BlockStmt> block) : init(std::move(init)), cond(std::move(cond)), post(std::move(post)), block(std::move(block)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitForStmt(this);
    }
};

struct ForEachStmt : Stmt {
    StmtPtr lhs;
    ExprPtr rhs;
    std::shared_ptr<BlockStmt> block;

    ForEachStmt(StmtPtr lhs, ExprPtr rhs, std::shared_ptr<BlockStmt> block) : lhs(std::move(lhs)), rhs(std::move(rhs)), block(std::move(block)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitForEachStmt(this);
    }
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr block;
    StmtPtr elseBlock;

    IfStmt(ExprPtr cond, StmtPtr block, StmtPtr elseBlock) : cond(std::move(cond)), block(std::move(block)), elseBlock(std::move(elseBlock)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitIfStmt(this);
    }
};

struct ReturnStmt : Stmt {
    ExprPtr expr;

    ReturnStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitReturnStmt(this);
    }
};

struct ContinueStmt : Stmt {
    ContinueStmt() {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitContinueStmt(this);
    }
};

struct BreakStmt : Stmt {
    BreakStmt() {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBreakStmt(this);
    }
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtPtr block;

    WhileStmt(ExprPtr cond, StmtPtr block) : cond(std::move(cond)), block(std::move(block)) {}

    void Accept(NVSEVisitor *visitor) override {
        visitor->VisitWhileStmt(this);
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    BlockStmt(std::vector<StmtPtr> statements) : statements(std::move(statements)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBlockStmt(this);
    }
};

struct Expr {
    // Generate RTTI
    virtual ~Expr() {}

    int line = 0;

    virtual void Accept(NVSEVisitor *t) = 0;
    Token_Type detailedType = kTokenType_Invalid;
    
    template <typename T>
    bool IsType() {
        return dynamic_cast<T*>(this);
    }
};

struct AssignmentExpr : Expr {
    ExprPtr left;
    ExprPtr expr;

    AssignmentExpr(ExprPtr left, ExprPtr expr) : left(std::move(left)), expr(std::move(expr)) {};

    void Accept(NVSEVisitor* t) override {
        t->VisitAssignmentExpr(this);
    }
};

struct TernaryExpr : Expr {
    ExprPtr cond;
    ExprPtr left;
    ExprPtr right;

    TernaryExpr(ExprPtr cond, ExprPtr left, ExprPtr right) : cond(std::move(cond)), left(std::move(left)), right(std::move(right)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitTernaryExpr(this);
    }
};

struct BinaryExpr : Expr {
    ExprPtr left, right;
    NVSEToken op;

    BinaryExpr(ExprPtr left, ExprPtr right, NVSEToken op) : left(std::move(left)), right(std::move(right)), op(op) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitBinaryExpr(this);
    }
};

struct UnaryExpr : Expr {
    ExprPtr expr;
    NVSEToken op;
    bool postfix;

    UnaryExpr(ExprPtr expr, NVSEToken op, bool postfix) : expr(std::move(expr)), op(op), postfix(postfix) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitUnaryExpr(this);
    }
};

struct SubscriptExpr : Expr {
    ExprPtr left;
    ExprPtr index;

    SubscriptExpr(ExprPtr left, ExprPtr index) : left(std::move(left)), index(std::move(index)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitSubscriptExpr(this);
    }
};

struct CallExpr : Expr {
    ExprPtr left;
    std::vector<ExprPtr> args;

    CallExpr(ExprPtr left, std::vector<ExprPtr> args) : left(std::move(left)), args(std::move(args)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitCallExpr(this);
    }
};

struct GetExpr : Expr {
    ExprPtr left;
    NVSEToken token;

    GetExpr(ExprPtr left, NVSEToken token) : left(std::move(left)), token(token) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitGetExpr(this);
    }
};

struct BoolExpr : Expr {
    bool value;

    BoolExpr(bool value) : value(value) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitBoolExpr(this);
    }
};

struct NumberExpr : Expr {
    double value;
    bool isFp;

    NumberExpr(double value, bool isFp) : value(value), isFp(isFp) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitNumberExpr(this);
    }
};

struct StringExpr : Expr {
    NVSEToken value;

    StringExpr(NVSEToken value) : value(value) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitStringExpr(this);
    }
};

struct IdentExpr : Expr {
    NVSEToken name;

    IdentExpr(NVSEToken name) : name(name) {};

    void Accept(NVSEVisitor* t) override {
        t->VisitIdentExpr(this);
    }
};

struct GroupingExpr : Expr {
    ExprPtr expr;

    GroupingExpr(ExprPtr expr) : expr(std::move(expr)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitGroupingExpr(this);
    }
};

struct LambdaExpr : Expr {
    std::vector<std::shared_ptr<VarDeclStmt>> args;
    StmtPtr body;

    LambdaExpr(std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : args(std::move(args)), body(std::move(body)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitLambdaExpr(this);
    }
};

class NVSEParseError : public std::runtime_error {
public:
    NVSEParseError(std::string message) : std::runtime_error(message) {};
};

class NVSEParser {
    std::function<void(std::string)> printFn;
    
public:
    NVSEParser(NVSELexer& tokenizer, std::function<void(std::string)> outputfn);
    std::optional<NVSEScript> Parse();
    StmtPtr Begin();

private:
    NVSELexer& lexer;
    NVSEToken currentToken;
    NVSEToken previousToken;
    bool panicMode = false;
    bool hadError = false;

    StmtPtr FnDecl();
    std::shared_ptr<VarDeclStmt> VarDecl();

    StmtPtr Statement();
    StmtPtr ExpressionStatement();
    StmtPtr ForStatement();
    StmtPtr IfStatement();
    StmtPtr ReturnStatement();
    StmtPtr WhileStatement();
    std::shared_ptr<BlockStmt> BlockStatement();

    ExprPtr Expression();
    ExprPtr Assignment();
    ExprPtr Ternary();
    ExprPtr LogicalOr();
    ExprPtr LogicalAnd();
    ExprPtr Equality();
    ExprPtr Comparison();
    ExprPtr Term();
    ExprPtr Factor();
    ExprPtr Unary();
    ExprPtr Postfix();
    ExprPtr Call();
    ExprPtr FnExpr();
    std::vector<std::shared_ptr<VarDeclStmt>> ParseArgs();
    ExprPtr Primary();

    void Advance();
    bool Match(NVSETokenType type);
    bool Peek(NVSETokenType type) const;
    void Error(NVSEToken token, std::string message);
    NVSEToken Expect(NVSETokenType type, std::string message);
    void Synchronize();
};