#pragma once

#include <string>
#include <cctype>
#include <stdexcept>
#include <variant>
#include <vector>

enum class TokenType {
    // Keywords
    If, 
    Else,
    While,
    Fn,
    Return,
    For,

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
    "For",

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
    std::variant<std::monostate, double, std::string> value;
    size_t line = 1;
    size_t linePos = 0;
    std::string lexeme;

    NVSEToken() : type(TokenType::End), lexeme(""), value(std::monostate{}) {}
    NVSEToken(TokenType t, std::string lexeme) : type(t), lexeme(lexeme), value(std::monostate{}) {}
    NVSEToken(TokenType t, std::string lexeme, double value) : type(t), lexeme(lexeme), value(value) {}
    NVSEToken(TokenType t, std::string lexeme, std::string value) : type(t), lexeme(lexeme), value(value) {}
};

class NVSELexer {
    std::string input;
    size_t pos;

    size_t linePos = 1;
    size_t line = 1;

public:
    std::vector<std::string> lines{};

    NVSELexer(const std::string& input);

    NVSEToken getNextToken();
    bool match(char c);
    NVSEToken makeToken(TokenType type, std::string lexeme);
    NVSEToken makeToken(TokenType type, std::string lexeme, double value);
    NVSEToken makeToken(TokenType type, std::string lexeme, std::string value);
};