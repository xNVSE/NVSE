#pragma once

#include "NVSELexer.h"
#include "NVSEAst.h"

#include <memory>
#include <optional>

class NVSEParseError : public std::runtime_error {
public:
    NVSEParseError(const std::string& message) : std::runtime_error(message) {};
};

class NVSEParser {
public:
    NVSEParser(NVSELexer& tokenizer);
    std::optional<NVSEScript> Parse();
    StmtPtr Begin();

private:
    NVSELexer& lexer;
    NVSEToken currentToken;
    NVSEToken previousToken;
    bool panicMode = false;
    bool hadError = false;

    std::shared_ptr<FnDeclStmt> FnDecl();
    std::shared_ptr<VarDeclStmt> VarDecl(bool allowValue = true, bool allowOnlyOneVarDecl = false);

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
    ExprPtr Pair();
    ExprPtr LogicalOr();
    ExprPtr LogicalAnd();
    ExprPtr Slice();
    ExprPtr Equality();
    ExprPtr Comparison();
    ExprPtr BitwiseOr();
    ExprPtr BitwiseAnd();
    ExprPtr Shift();
    ExprPtr In();
    ExprPtr Term();
    ExprPtr Factor();
    ExprPtr Unary();
    ExprPtr Postfix();
    ExprPtr Call();
    ExprPtr FnExpr();
    ExprPtr ArrayLiteral();
    ExprPtr MapLiteral();
    std::vector<std::shared_ptr<VarDeclStmt>> ParseArgs();
    ExprPtr Primary();

    void Advance();
    bool Match(NVSETokenType type);
    bool MatchesType();
    bool Peek(NVSETokenType type) const;
    bool PeekType() const;
    void Error(std::string message);
    void Error(NVSEToken token, std::string message);
    NVSEToken Expect(NVSETokenType type, std::string message);
    void Synchronize();
};
