#pragma once

#include <nvse/Compiler/Lexer/Lexer.h>
#include <nvse/Compiler/AST/AST.h>

#include <memory>
#include <optional>

namespace Compiler {
    class NVSEParseError : public std::runtime_error {
    public:
	    explicit NVSEParseError(const std::string& message) : std::runtime_error(message) {}
    };

    class Parser {
    public:
        explicit Parser(const std::string& text);
        std::optional<AST> Parse();

	    [[nodiscard]] 
    	bool HadError() const {
            return hadError;
        }

    private:
        Lexer lexer;
        Token currentToken;
        Token previousToken;
        bool panicMode = false;
        bool hadError = false;

        std::shared_ptr<Statements::VarDecl> VarDecl(bool allowValue = true, bool allowOnlyOneVarDecl = false);

        StmtPtr Begin();
        StmtPtr Statement();
        StmtPtr ExpressionStatement();
        StmtPtr ForStatement();
	    void AdvanceToClosingCharOf(TokenType leftType);
	    ExprPtr ParenthesizedExpression();
	    StmtPtr IfStatement();
		std::shared_ptr<Statements::Match> MatchStatement();
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
		std::shared_ptr<Expressions::IdentExpr> IdentExpr();
		std::shared_ptr<Expressions::NumberExpr> NumericLiteral();

        void Advance();
        bool Match(TokenType type);
        bool MatchesType();
	    
    	[[nodiscard]] 
    	bool Peek(TokenType type) const;
        
    	[[nodiscard]]
    	bool PeekType() const;

		[[nodiscard]]
        bool PeekBlockType() const;

        Token ExpectBlockType(const std::string&& message);

        void Error(const std::string &message);
        void Error(Token token, std::string message);
        Token Expect(TokenType type, std::string message);
		Token Expect(TokenType type);
		void Synchronize();
    };

}
