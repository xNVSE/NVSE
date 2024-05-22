#include "NVSEParser.h"

#include <format>
#include <iostream>

NVSEParser::NVSEParser(NVSELexer& tokenizer) : lexer(tokenizer) {
    this->lexer = tokenizer;
    advance();
}

ExprPtr NVSEParser::parse() {
    try {
        return assignment();
    }
    catch (NVSEParseError e) {
        std::cout << e.what() << std::endl;
        synchronize();
    }
}

ExprPtr NVSEParser::expression() {
    return assignment();
}

ExprPtr NVSEParser::assignment() {
    ExprPtr left = ternary();

    if (match(TokenType::Eq)) {
        const auto prevTok = previousToken;
        ExprPtr value = assignment();

        const auto idExpr = dynamic_cast<IdentExpr*>(left.get());
        const auto getExpr = dynamic_cast<GetExpr*>(left.get());

        if (idExpr) {
            return std::make_unique<AssignmentExpr>(idExpr->name, std::move(value));
        }

        if (getExpr) {
            return std::make_unique<SetExpr>(std::move(getExpr->left), getExpr->token, std::move(value));
        }

        error(prevTok, "Invalid assignment target.");
    }

    return left;
}

ExprPtr NVSEParser::ternary() {
    ExprPtr cond = logicOr();

    while (match(TokenType::Ternary)) {
        auto left = logicOr();
        expect(TokenType::Colon, "Expected ':'.");
        auto right = logicOr();

        cond = std::make_unique<TernaryExpr>(std::move(cond), std::move(left), std::move(right));
    }

    return cond;
}

ExprPtr NVSEParser::logicOr() {
    ExprPtr left = logicAnd();

    while (match(TokenType::LogicOr)) {
        const auto op = previousToken;
        ExprPtr right = logicAnd();
        left = std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

ExprPtr NVSEParser::logicAnd() {
    ExprPtr left = equality();

    while (match(TokenType::LogicAnd)) {
        const auto op = previousToken;
        ExprPtr right = equality();
        left = std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

ExprPtr NVSEParser::equality() {
    ExprPtr left = comparison();

    while (match(TokenType::EqEq) || match(TokenType::BangEq)) {
        const auto op = previousToken;
        ExprPtr right = comparison();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

ExprPtr NVSEParser::comparison() {
    ExprPtr left = term();

    while (match(TokenType::Less) || match(TokenType::LessEq) || match(TokenType::Greater) || match(TokenType::GreaterEq)) {
        const auto op = previousToken;
        ExprPtr right = term();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

// term -> factor ((+ | -) factor)?;
ExprPtr NVSEParser::term() {
    ExprPtr left = factor();

    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        const auto op = previousToken;
        ExprPtr right = factor();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

ExprPtr NVSEParser::factor() {
    ExprPtr left = unary();

    while (match(TokenType::Star) || match(TokenType::Slash)) {
        const auto op = previousToken;
        ExprPtr right = unary();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
    }

    return left;
}

ExprPtr NVSEParser::unary() {
    if (match(TokenType::Bang) || match(TokenType::Minus)) {
        const auto op = previousToken;
        ExprPtr right = unary();
        return std::make_unique<UnaryExpr>(std::move(right), op);
    }

    return call();
}

ExprPtr NVSEParser::call() {
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

ExprPtr NVSEParser::primary() {
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

void NVSEParser::advance() {
    previousToken = currentToken;
    currentToken = lexer.getNextToken();
}

bool NVSEParser::match(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }

    return false;
}

void NVSEParser::error(NVSEToken token, std::string message) {
    panicMode = true;
    hadError = true;
    throw NVSEParseError(std::format("[line {}:{}] {}", token.line, token.linePos, message));
}

NVSEToken NVSEParser::expect(TokenType type, std::string message) {
    if (!match(type)) {
        error(currentToken, message);
    }

    return previousToken;
}

void NVSEParser::synchronize() {
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