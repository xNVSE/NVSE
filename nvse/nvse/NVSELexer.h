#pragma once

#include <stack>
#include <string>
#include <stdexcept>
#include <variant>
#include <vector>

enum class NVSETokenType {
    // Keywords
    If, 
    Else,
    While,
    Fn,
    Return,
    For,
    BlockType,
    Name,
    Continue,
    Break,

    // Types
    IntType,
    DoubleType,
    RefType,
    StringType,
    ArrayType,

    // Operators
    Plus, PlusEq, PlusPlus,
    Minus, Negate, MinusEq, MinusMinus,
    Star, StarEq,
    Slash, SlashEq,
    Mod, ModEq,
    Pow, PowEq,
    Eq, EqEq,
    Less, Greater,
    LessEq,
    GreaterEq,
    NotEqual,
    Bang, BangEq,
    LogicOr, LogicAnd,
    BitwiseOr, BitwiseAnd,
    Tilde, 
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
    Colon,
    Eof,
    Dot,
    Interp,
    EndInterp,
};

static const char* TokenTypeStr[]{
    // Keywords
    "If",
    "Else",
    "While",
    "Fn",
    "Return",
    "For",
    "Begin",
    "Name",
    "Continue",
    "Break",

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
    "Negate",
    "MinusEq",
    "MinusMinus",
    "Star",
    "StarEq",
    "Slash",
    "SlashEq",
    "Mod",
    "ModEq",
    "Pow",
    "PowEq",
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
    "Pound",

    // These two get set by parser as they are context dependent
    "Box",
    "Unbox",

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
    "Interp",
    "EndInterp",
};

struct NVSEToken {
    NVSETokenType type;
    std::variant<std::monostate, double, std::string> value;
    size_t line = 1;
    size_t linePos = 0;
    std::string lexeme;

    NVSEToken() : type(NVSETokenType::Eof), lexeme(""), value(std::monostate{}) {}
    NVSEToken(NVSETokenType t, std::string lexeme) : type(t), lexeme(lexeme), value(std::monostate{}) {}
    NVSEToken(NVSETokenType t, std::string lexeme, double value) : type(t), lexeme(lexeme), value(value) {}
    NVSEToken(NVSETokenType t, std::string lexeme, std::string value) : type(t), lexeme(lexeme), value(value) {}
};

class NVSELexer {
    std::string input;
    size_t pos;

    // For string interpolation
    std::deque<NVSEToken> tokenStack{};
    int interpDepth = 0;

public:
    size_t linePos = 1;
    size_t line = 1;
    std::vector<std::string> lines{};

    NVSELexer(const std::string& input);
    std::deque<NVSEToken> lexString();

    NVSEToken GetNextToken(bool useStack);
    bool Match(char c);
    NVSEToken MakeToken(NVSETokenType type, std::string lexeme);
    NVSEToken MakeToken(NVSETokenType type, std::string lexeme, double value);
    NVSEToken MakeToken(NVSETokenType type, std::string lexeme, std::string value);
};