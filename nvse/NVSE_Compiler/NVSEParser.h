#pragma once

#include "NVSELexer.h"
#include "NVSEVisitor.h"

#include <memory>
#include <unordered_map>

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr {
    // Generate RTTI
    virtual ~Expr() {};

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
    int value;

    NumberExpr(int value) : value(value) {}

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

class NVSEParseError : public std::runtime_error {
public:
    NVSEParseError(std::string message) : std::runtime_error(message) {};
};

class NVSEParser {
public:
    NVSEParser(NVSELexer& tokenizer);

    ExprPtr parse();

private:
    NVSELexer& lexer;
    NVSEToken currentToken;
    NVSEToken previousToken;
    bool panicMode = false;
    bool hadError = false;

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
    ExprPtr primary();

    void advance();
    bool match(TokenType type);
    void error(NVSEToken token, std::string message);
    NVSEToken expect(TokenType type, std::string message);
    void synchronize();
};