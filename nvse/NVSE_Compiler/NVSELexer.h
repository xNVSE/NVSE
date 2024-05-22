#pragma once

#include <string>
#include <cctype>
#include <stdexcept>
#include <variant>

enum class TokenType {
    // Keywords
    If, 
    Else,
    While,
    Fn,
    Return,

    // Types
    IntType,
    DoubleType,
    RefType,
    StringType,
    ArrayType,

    // Operators
    Plus, PlusEq, PlusPlus,
    Minus, MinusEq, MinusMinus,
    Star, StarEq,
    Slash, SlashEq,
    Eq, EqEq,
    Less, Greater,
    LessEq,
    GreaterEq,
    NotEqual,
    Bang, BangEq,
    LogicOr, LogicAnd,
    BitwiseOr, BitwiseAnd,
    Tilde, 
    Dollar,
    
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
    Colon,
    End,
    Dot,
};

static const char* TokenTypeStr[]{
    // Keywords
    "If",
    "Else",
    "While",
    "Fn",
    "Return",

    // Types
    "IntType",
    "DoubleType",
    "RefType",
    "StringType",
    "ArrayType",

    // Operators
    "Plus",
    "PlusEq",
    "PlusPlus",
    "Minus",
    "MinusEq",
    "MinusMinus",
    "Star",
    "StarEq",
    "Slash",
    "SlashEq",
    "Eq",
    "EqEq",
    "Less",
    "Greater",
    "LessEq",
    "GreaterEq",
    "NotEqual",
    "Bang",
    "BangEq",
    "LogicOr",
    "LogicAnd",
    "BitwiseOr",
    "BitwiseAnd",
    "Tilde",
    "Dollar",

    // Braces
    "LeftBrace",
    "RightBrace",
    "LeftBracket",
    "RightBracket",
    "LeftParen",
    "RightParen",

    // Literals
    "String",
    "Number",
    "Identifier",
    "Bool",

    // Misc
    "Comma",
    "Semicolon",
    "Ternary",
    "Colon",
    "End",
    "Dot",
};

struct NVSEToken {
    TokenType type;
    std::variant<double, std::string> value;
    size_t line = 1;
    size_t linePos = 0;

    NVSEToken() : type(TokenType::End) {}
    NVSEToken(TokenType t) : type(t), value(0.0) {}
    NVSEToken(TokenType t, double v) : type(t), value(v) {}
    NVSEToken(TokenType t, std::string v) : type(t), value(v) {}
};

class NVSELexer {
    std::string input;
    size_t pos;

    size_t linePos = 1;
    size_t line = 1;

public:
    NVSELexer(const std::string& input);

    NVSEToken getNextToken();

    bool match(char c);

    NVSEToken makeToken(TokenType type);

    NVSEToken makeToken(TokenType type, std::string lexeme);

    NVSEToken makeToken(TokenType type, double value);
};