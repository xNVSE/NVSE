#pragma once

#include <stack>
#include <string>
#include <stdexcept>
#include <variant>
#include <vector>

#include "nvse/Compiler/SourceInfo.h"

namespace Compiler {
    enum class TokenType {
        INVALID,

        // Keywords
        If,
        Else,
        While,
        Fn,
        Return,
        For,
        Name,
        Continue,
        Break,
        Match,

        // Types
        IntType,
        DoubleType,
        RefType,
        StringType,
        ArrayType,

        // Operators
        Plus, 
    	PlusEq, 
    	PlusPlus,
        Minus, 
    	Negate, 
    	MinusEq, 
    	MinusMinus,
        Star, StarEq,
        Slash, SlashEq,
        Mod, ModEq,
        Pow, PowEq,
        Eq, EqEq,
        Less, Greater,
        LessEq,
        GreaterEq,
        Bang, BangEq,
        LogicOr,
        LogicAnd,
        BitwiseOr,
        BitwiseOrEquals,
        BitwiseAnd,
        BitwiseAndEquals,
        LeftShift,
        RightShift,
        BitwiseNot,
        Dollar, Pound,

        Box, Unbox,

        // Braces
        LeftBrace, RightBrace,
        LeftBracket, RightBracket,
        LeftParen, RightParen,

        // Literals
        String,
        Number,
        Identifier,
        Bool,

        // Misc
        Comma,
        Semicolon,
        Ternary,
        Slice,
        Not,
        In,
        Eof,
        Dot,
        Interp,
        EndInterp,
        MakePair,
        Arrow,
        Underscore,
    };

    static const char* TokenTypeStr[]{
        "INVALID",

        // Keywords
        "if",
        "else",
        "while",
        "fn",
        "return",
        "for",
        "name",
        "continue",
        "break",
        "match",

        // Types
        "int",
        "double",
        "ref",
        "string",
        "array",

        // Operators
        "+",
        "+=",
        "++",
        "-",
        "-",
        "-=",
        "--",
        "*",
        "*=",
        "/",
        "/=",
        "%",
        "%=",
        "^",
        "^=",
        "=",
        "==",
        "<",
        ">",
        "<=",
        ">=",
        "!",
        "!=",
        "||",
        "&&",
        "|",
        "|=",
        "&",
        "&=",
        "<<",
        ">>",
        "~",
        "$",
        "#",

        // These two get set by parser as they are context dependent
        "Box",
        "Unbox",

        // Braces
        "{",
        "}",
        "[",
        "]",
        "(",
        ")",

        // Literals
        "String",
        "Number",
        "Identifier",
        "Bool",

        // Misc
        ",",
        ";",
        "?",
        ":",
        "not",
        "in",
        "end",
        ".",
        "${",
        "}",
		"::",
        "->",
        "_",
    };

    struct Token {
        TokenType type;
        std::variant<std::monostate, double, std::string> value;
        std::string lexeme;

        SourceSpan sourceSpan;

        Token(Token&& other) noexcept {
            this->operator=(std::move(other));
        }

        Token(const Token& other) noexcept {
            this->type = other.type;
            this->value = other.value;
            this->lexeme = other.lexeme;
            this->sourceSpan = other.sourceSpan;
        }

        Token& operator=(const Token& other) noexcept = default;

		Token& operator=(Token&& other) noexcept {
            this->type = other.type;
            this->value = std::move(other.value);
            this->lexeme = std::move(other.lexeme);
            this->sourceSpan = other.sourceSpan;

            return *this;
        }

        Token() : type(TokenType::Eof), value(std::monostate{}) {}

        explicit Token(const TokenType t) : type(t), value(std::monostate{}) {}
        Token(const TokenType t, std::string&& lexeme) : type(t), value(std::monostate{}), lexeme(std::move(lexeme)) {}

        Token(
            const TokenType t, 
            std::string&& lexeme, 
            const SourcePos& startPos, 
            const SourcePos& endPos,
            std::variant<std::monostate, double, std::string> &&value = std::monostate{}
        ) : type(t), 
    		value(std::move(value)), 
    		lexeme(std::move(lexeme)) 
    	{
            this->sourceSpan = { .start = startPos, .end = endPos };
        }

        Token(
            const TokenType t, 
            std::string&& lexeme, 
            const SourcePos& startPos, 
            SourcePos&& endPos,
            std::variant<std::monostate, double, std::string> &&value = std::monostate{}
        ) : type(t),
            value(std::move(value)),
    		lexeme(std::move(lexeme)) 
    	{
            this->sourceSpan = { .start = startPos, .end = endPos };
        }

        Token(const TokenType t, std::string&& lexeme, double value) : type(t), value(value), lexeme(std::move(lexeme)) {}
        Token(const TokenType t, std::string&& lexeme, std::string value) : type(t), value(value), lexeme(std::move(lexeme)) {}
    };

    class Lexer {
        std::string input;
        size_t pos;

        // For string interpolation
        std::deque<Token> tokenStack{};

        size_t AdvanceChar() {
            column++;
            return pos++;
        }

    public:
        size_t column = 1;
        size_t line = 1;
        std::vector<std::string> lines{};

        explicit Lexer(const std::string& input);
        std::deque<Token> lexString();
        static bool CheckIdentifier(const std::string& identifier, const char* ident);
		
    	[[nodiscard]] 
    	std::string GetSourceText(const SourcePos& startPos, const SourcePos& endPos) const;
        
    	[[nodiscard]]
    	std::string GetSourceText(const SourcePos& startPos) const;

        Token GetNextToken(bool useStack);
        bool Match(char c);
        bool Match(std::string_view str);

		[[nodiscard]]
        SourcePos CurSourcePos() const {
            return SourcePos(pos, line, column);
        }

        [[nodiscard]]
        SourcePos PrevSourcePos() const {
            return SourcePos(pos, line, column - 1);
        }
    };
}
