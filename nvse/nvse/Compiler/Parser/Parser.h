#pragma once

#include <nvse/Compiler/Lexer/Lexer.h>
#include <nvse/Compiler/AST/AST.h>

#include <memory>
#include <optional>

namespace Compiler {
    class NVSEParseError : public std::runtime_error {
    public:
        NVSEParseError(const std::string& message) : std::runtime_error(message) {};
    };

    class Parser {
    public:
        Parser(Lexer& tokenizer);
        std::optional<AST> Parse();

    private:
        Lexer& lexer;
        NVSEToken currentToken;
        NVSEToken previousToken;
        bool panicMode = false;
        bool hadError = false;

        std::shared_ptr<Statements::UDFDecl> FnDecl();
        std::shared_ptr<Statements::VarDecl> VarDecl(bool allowValue = true, bool allowOnlyOneVarDecl = false);

        StmtPtr Begin();
        StmtPtr Statement();
        StmtPtr ExpressionStatement();
        StmtPtr ForStatement();
        StmtPtr IfStatement();
        StmtPtr MatchStatement();
        StmtPtr ReturnStatement();
        StmtPtr WhileStatement();
        std::shared_ptr<Statements::Block> BlockStatement();

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
        std::vector<std::shared_ptr<Statements::VarDecl>> ParseArgs();
        ExprPtr Primary();
        std::shared_ptr<Expressions::NumberExpr> NumericLiteral();

        void Advance();
        bool Match(NVSETokenType type);
        bool MatchesType();
        bool Peek(NVSETokenType type) const;
        bool PeekType() const;

        bool PeekBlockType() const;
        NVSEToken ExpectBlockType(const std::string&& message);

        void Error(std::string message);
        void Error(NVSEToken token, std::string message);
        NVSEToken Expect(NVSETokenType type, std::string message);
        void Synchronize();
    };

}
