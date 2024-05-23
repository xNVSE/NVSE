#pragma once

#include "NVSELexer.h"
#include "NVSEVisitor.h"

#include <memory>
#include <optional>
#include <unordered_map>

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Stmt {
    virtual ~Stmt() {}

    virtual void accept(NVSEVisitor* t) = 0;
};

struct FnDeclStmt : Stmt {
    std::optional<NVSEToken> name{};
    std::optional<NVSEToken> mode{};
	std::vector<StmtPtr> args;
    StmtPtr body;

    FnDeclStmt(std::vector<StmtPtr> args, StmtPtr body) : args(std::move(args)), body(std::move(body)) {}
    FnDeclStmt(NVSEToken name, std::vector<StmtPtr> args, StmtPtr body) : name(name), args(std::move(args)), body(std::move(body)) {}
    FnDeclStmt(NVSEToken name, NVSEToken mode, std::vector<StmtPtr> args, StmtPtr body) : name(name), mode(mode), args(std::move(args)), body(std::move(body)) {}

    void accept(NVSEVisitor *visitor) override {
        visitor->visitFnDeclStmt(this);
    }
};

struct VarDeclStmt : Stmt {
    NVSEToken type;
    NVSEToken name;
    ExprPtr value{ nullptr };

    VarDeclStmt(NVSEToken type, NVSEToken name) : type(type), name(name) {}
    VarDeclStmt(NVSEToken type, NVSEToken name, ExprPtr value) : type(type), name(name), value(std::move(value)) {}

    void accept(NVSEVisitor* visitor) override {
	    visitor->visitVarDeclStmt(this);
    }
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void accept(NVSEVisitor *visitor) override {
        visitor->visitExprStmt(this);
    }
};

struct ForStmt : Stmt {
	// Todo
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr block;
    StmtPtr elseBlock;

    IfStmt(ExprPtr cond, StmtPtr block, StmtPtr elseBlock) : cond(std::move(cond)), block(std::move(block)), elseBlock(std::move(elseBlock)) {}

    void accept(NVSEVisitor* visitor) override {
        visitor->visitIfStmt(this);
    }
};

struct ReturnStmt : Stmt {
    ExprPtr expr;

    ReturnStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void accept(NVSEVisitor* visitor) override {
        visitor->visitReturnStmt(this);
    }
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtPtr block;

    WhileStmt(ExprPtr cond, StmtPtr block) : cond(std::move(cond)), block(std::move(block)) {}

    void accept(NVSEVisitor *visitor) override {
        visitor->visitWhileStmt(this);
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    BlockStmt(std::vector<StmtPtr> statements) : statements(std::move(statements)) {}

    void accept(NVSEVisitor* visitor) override {
        visitor->visitBlockStmt(this);
    }
};

struct Expr {
    // Generate RTTI
    virtual ~Expr() {}

    virtual void accept(NVSEVisitor *t) = 0;
};

struct AssignmentExpr : Expr {
    NVSEToken name;
    ExprPtr expr;

    AssignmentExpr(NVSEToken name, ExprPtr expr) : name(name), expr(std::move(expr)) {};

    void accept(NVSEVisitor* t) {
        t->visitAssignmentExpr(this);
    }
};

struct TernaryExpr : Expr {
    ExprPtr cond;
    ExprPtr left;
    ExprPtr right;

    TernaryExpr(ExprPtr cond, ExprPtr left, ExprPtr right) : cond(std::move(cond)), left(std::move(left)), right(std::move(right)) {}

    void accept(NVSEVisitor* t) {
        t->visitTernaryExpr(this);
    }
};

struct LogicalExpr : Expr {
    ExprPtr left, right;
    NVSEToken op;

    LogicalExpr(ExprPtr left, ExprPtr right, NVSEToken op) : left(std::move(left)), right(std::move(right)), op(op) {}

    void accept(NVSEVisitor* t) {
        t->visitLogicalExpr(this);
    }
};

struct BinaryExpr : Expr {
    ExprPtr left, right;
    NVSEToken op;

    BinaryExpr(ExprPtr left, ExprPtr right, NVSEToken op) : left(std::move(left)), right(std::move(right)), op(op) {}

    void accept(NVSEVisitor* t) {
        t->visitBinaryExpr(this);
    }
};

struct UnaryExpr : Expr {
    ExprPtr expr;
    NVSEToken op;

    UnaryExpr(ExprPtr expr, NVSEToken op) : expr(std::move(expr)), op(op) {}

    void accept(NVSEVisitor* t) {
        t->visitUnaryExpr(this);
    }
};

struct CallExpr : Expr {
    ExprPtr left;
    std::vector<ExprPtr> args;

    CallExpr(ExprPtr left, std::vector<ExprPtr> args) : left(std::move(left)), args(std::move(args)) {}

    void accept(NVSEVisitor* t) {
        t->visitCallExpr(this);
    }
};

struct GetExpr : Expr {
    ExprPtr left;
    NVSEToken token;

    GetExpr(ExprPtr left, NVSEToken token) : left(std::move(left)), token(token) {}

    void accept(NVSEVisitor* t) {
        t->visitGetExpr(this);
    }
};

struct SetExpr : Expr {
    ExprPtr left;
    NVSEToken token;
    ExprPtr right;

    SetExpr(ExprPtr left, NVSEToken token, ExprPtr right) : left(std::move(left)), token(token), right(std::move(right)) {}

    void accept(NVSEVisitor* t) {
        t->visitSetExpr(this);
    }
};

struct BoolExpr : Expr {
    bool value;

    BoolExpr(bool value) : value(value) {}

    void accept(NVSEVisitor* t) {
        t->visitBoolExpr(this);
    }
};

struct NumberExpr : Expr {
    float value;
    bool isFp;

    NumberExpr(float value, bool isFp) : value(value), isFp(isFp) {}

    void accept(NVSEVisitor* t) {
        t->visitNumberExpr(this);
    }
};

struct StringExpr : Expr {
    NVSEToken value;

    StringExpr(NVSEToken value) : value(value) {}

    void accept(NVSEVisitor* t) {
        t->visitStringExpr(this);
    }
};

struct IdentExpr : Expr {
    NVSEToken name;

    IdentExpr(NVSEToken name) : name(name) {};

    void accept(NVSEVisitor* t) {
        t->visitIdentExpr(this);
    }
};

struct GroupingExpr : Expr {
    ExprPtr expr;

    GroupingExpr(ExprPtr expr) : expr(std::move(expr)) {}

    void accept(NVSEVisitor* t) {
        t->visitGroupingExpr(this);
    }
};

struct LambdaExpr : Expr {
    std::vector<std::unique_ptr<VarDeclStmt>> args;
    StmtPtr body;

    LambdaExpr(std::vector<std::unique_ptr<VarDeclStmt>> args, StmtPtr body) : args(std::move(args)), body(std::move(body)) {}

    void accept(NVSEVisitor* t) {
        t->visitLambdaExpr(this);
    }
};

class NVSEParseError : public std::runtime_error {
public:
    NVSEParseError(std::string message) : std::runtime_error(message) {};
};

class NVSEParser {
public:
    NVSEParser(NVSELexer& tokenizer);

    StmtPtr parse();

private:
    NVSELexer& lexer;
    NVSEToken currentToken;
    NVSEToken previousToken;
    bool panicMode = false;
    bool hadError = false;

    StmtPtr fnDecl();
    std::unique_ptr<VarDeclStmt> varDecl();

    StmtPtr statement();
    StmtPtr exprStmt();
    StmtPtr forStmt();
    StmtPtr ifStmt();
    StmtPtr returnStmt();
    StmtPtr whileStmt();
    StmtPtr blockStmt();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr ternary();
    ExprPtr logicOr();
    ExprPtr logicAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr fnExpr();
    ExprPtr primary();

    void advance();
    bool match(TokenType type);
    bool peek(TokenType type);
    void error(NVSEToken token, std::string message);
    NVSEToken expect(TokenType type, std::string message);
    void synchronize();
};