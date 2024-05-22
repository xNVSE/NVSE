#include "NVSELexer.h"
#include "NVSEVisitor.h"

#include <memory>
#include <unordered_map>
#include <format>

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
    NVSEParser(NVSELexer& tokenizer) : lexer(tokenizer) {
        this->lexer = tokenizer;
        currentToken = tokenizer.getNextToken();
    }

    ExprPtr parse() {
        try {
            return assignment();
        }
        catch (NVSEParseError e) {
            std::cout << e.what() << std::endl;
            synchronize();
        }
    }

private:
    NVSELexer& lexer;
    NVSEToken currentToken;
    NVSEToken previousToken;
    bool panicMode = false;
    bool hadError = false;

    ExprPtr expression() {
        return assignment();
    }

    ExprPtr assignment() {
        ExprPtr left = logicOr();

        // todo

        return left;
    }

    ExprPtr logicOr() {
        ExprPtr left = logicAnd();

        while (match(TokenType::LogicOr)) {
            const auto op = previousToken;
            ExprPtr right = logicAnd();
            return std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    ExprPtr logicAnd() {
        ExprPtr left = equality();

        while (match(TokenType::LogicAnd)) {
            const auto op = previousToken;
            ExprPtr right = equality();
            return std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    ExprPtr equality() {
        ExprPtr left = comparison();

        while (match(TokenType::EqEq) || match(TokenType::BangEq)) {
            const auto op = previousToken;
            ExprPtr right = comparison();
            return std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    ExprPtr comparison() {
        ExprPtr left = term();

        while (match(TokenType::Less) || match(TokenType::LessEq) || match(TokenType::Greater) || match(TokenType::GreaterEq)) {
            const auto op = previousToken;
            ExprPtr right = term();
            return std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    // term -> factor ((+ | -) factor)?;
    ExprPtr term() {
        ExprPtr left = factor();

        while (match(TokenType::Plus) || match(TokenType::Minus)) {
            const auto op = previousToken;
            ExprPtr right = factor();
            return std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    ExprPtr factor() {
        ExprPtr left = unary();

        while (match(TokenType::Star) || match(TokenType::Slash)) {
            const auto op = previousToken;
            ExprPtr right = unary();
            return std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
        }

        return left;
    }

    ExprPtr unary() {
        if (match(TokenType::Bang) || match(TokenType::Minus)) {
            const auto op = previousToken;
            ExprPtr right = unary();
            return std::make_unique<UnaryExpr>(std::move(right), op);
        }

        return call();
    }

    ExprPtr call() {
        ExprPtr expr = primary();

        while (match(TokenType::Dot) || match(TokenType::LeftParen)) {
            if (previousToken.type == TokenType::Dot) {
                if (!(dynamic_cast<GroupingExpr*>(expr.get()) 
                    || dynamic_cast<IdentExpr*>(expr.get()) 
                    || dynamic_cast<CallExpr*>(expr.get()) 
                    || dynamic_cast<GetExpr*>(expr.get()))) {
                    error(currentToken, "Invalid member access.");
                }

                const auto ident = expect(TokenType::Identifier, "Expected identifier.");
                expr = std::make_unique<GetExpr>(std::move(expr), ident);
            }
            else {
                if (!(dynamic_cast<GroupingExpr*>(expr.get())
                    || dynamic_cast<IdentExpr*>(expr.get())
                    || dynamic_cast<CallExpr*>(expr.get())
                    || dynamic_cast<GetExpr*>(expr.get()))) {
                    error(currentToken, "Invalid callee.");
                }

                if (match(TokenType::RightParen)) {
                    expr = std::make_unique<CallExpr>(std::move(expr), std::move(std::vector<ExprPtr>{}));
                    continue;
                }

                std::vector<ExprPtr> args{};
                args.emplace_back(std::move(expression()));
                while (match(TokenType::Comma)) {
                    args.emplace_back(std::move(expression()));
                }

                expect(TokenType::RightParen, "Expected ')' after args.");

                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
            }
        }

        return expr;
    }

    ExprPtr primary() {
        if (match(TokenType::Bool)) {
            return std::make_unique<BoolExpr>(std::get<double>(previousToken.value));
        }

        if (match(TokenType::Number)) {
            return std::make_unique<NumberExpr>(std::get<double>(previousToken.value));
        }

        if (match(TokenType::String)) {
            return std::make_unique<StringExpr>(previousToken);
        }

        if (match(TokenType::Identifier)) {
            return std::make_unique<IdentExpr>(previousToken);
        }

        if (match(TokenType::LeftParen)) {
            ExprPtr expr = expression();
            expect(TokenType::RightParen, "Expected ')' after expression.");
            return std::make_unique<GroupingExpr>(std::move(expr));
        }

        error(currentToken, "Expected expression.");
    }

    void advance() {
        previousToken = currentToken;
        currentToken = lexer.getNextToken();
    }

    bool match(TokenType type) {
        if (currentToken.type == type) {
            advance();
            return true;
        }

        return false;
    }

    void error(NVSEToken token, std::string message) {
        throw NVSEParseError(std::format("[line {}:{}] {}", token.line, token.linePos, message));
        panicMode = true;
        hadError = true;
    }

    NVSEToken expect(TokenType type, std::string message) {
        if (!match(type)) {
            error(currentToken, message);
        }

        return previousToken;
    }

    void synchronize() {
        advance();
        NVSEToken curToken;

        while (curToken.type != TokenType::End) {
            if (previousToken.type == TokenType::Semicolon) {
                return;
            }

            switch (currentToken.type) {
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
            case TokenType::IntType:
            case TokenType::DoubleType:
            case TokenType::RefType:
            case TokenType::ArrayType:
            case TokenType::StringType:
                return;
            }

            advance();
        }
    }
};