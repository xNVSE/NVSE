#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
#include <variant>
#include <unordered_map>

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
public:
    NVSELexer(const std::string& input) : input(input), pos(0) {}

    NVSEToken getNextToken() {
        while (pos < input.size() && std::isspace(input[pos])) {
            pos++;
            if (input[pos] == '\n') {
                line++;
                linePos = 1;
            }
        }

        if (pos == input.size()) return makeToken(TokenType::End);

        char current = input[pos];
        if (std::isdigit(current)) {
            size_t len;
            double value = std::stod(&input[pos], &len);
            if (input[pos + len - 1] == '.') {
                len--;
            }
            pos += len;
            linePos += len;
            return makeToken(TokenType::Number, value);
        }

        if (std::isalpha(current) || current == '_') {
            // Extract identifier
            size_t start = pos;
            while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) ++pos;
            std::string identifier = input.substr(start, pos - start);

            // Keywords
            if (identifier == "if") return makeToken(TokenType::If, identifier);
            if (identifier == "else") return makeToken(TokenType::Else, identifier);
            if (identifier == "while") return makeToken(TokenType::While, identifier);
            if (identifier == "fn") return makeToken(TokenType::Fn, identifier);
            if (identifier == "return") return makeToken(TokenType::Return, identifier);

            // Types
            if (identifier == "int") return makeToken(TokenType::IntType, identifier);
            if (identifier == "double") return makeToken(TokenType::DoubleType, identifier);
            if (identifier == "ref") return makeToken(TokenType::RefType, identifier);
            if (identifier == "string") return makeToken(TokenType::StringType, identifier);
            if (identifier == "array") return makeToken(TokenType::ArrayType, identifier);

            // Boolean literals
            if (identifier == "true") return makeToken(TokenType::Bool, 1);
            if (identifier == "false") return makeToken(TokenType::Bool, 0);

            return makeToken(TokenType::Identifier, identifier);
        }

        if (current == '"') {
            size_t start = pos++;
            while (pos < input.size() && (input[pos] != '"' || input[pos - 1] == '\\')) pos++;
            if (input[pos] != '\"') {
                throw std::runtime_error("Unexpected EOF");
            }
            pos++;
            const auto len = pos - start;
            linePos += len;
            std::string text = input.substr(start + 1, len - 2);
            return makeToken(TokenType::String, text);
        }

        pos++;
        switch (current) {
        // Operators
        case '+': {
            if (match('=')) {
                return makeToken(TokenType::PlusEq, "+=");
            }
            else if (match('+')) {
                return makeToken(TokenType::PlusPlus, "++");
            }
            return makeToken(TokenType::Plus, "+");
        }
        case '-': {
            if (match('=')) {
                return makeToken(TokenType::MinusEq, "-=");
            }
            else if (match('-')) {
                return makeToken(TokenType::MinusMinus, "--");
            }
            return makeToken(TokenType::Minus, "-");
        }
        case '*': 
            if (match('=')) {
                return makeToken(TokenType::StarEq, "*=");
            }
            return makeToken(TokenType::Star, "*");
        case '/':
            if (match('=')) {
                return makeToken(TokenType::SlashEq, "/=");
            }
            return makeToken(TokenType::Slash, "/");
        case '=':
            if (match('=')) {
                return makeToken(TokenType::EqEq, "==");
            }
            return makeToken(TokenType::Eq, "=");
        case '!':
            if (match('=')) {
                return makeToken(TokenType::BangEq, "!=");
            }
            return makeToken(TokenType::Bang, "!");
        case '<':
            if (match('=')) {
                return makeToken(TokenType::LessEq, "<=");
            }
            return makeToken(TokenType::Less, "<");
        case '>':
            if (match('=')) {
                return makeToken(TokenType::GreaterEq, ">=");
            }
            return makeToken(TokenType::Greater, ">");
        case '|':
            if (match('|')) {
                return makeToken(TokenType::LogicOr, "||");
            }
            return makeToken(TokenType::BitwiseOr, "|");
        case '~': return makeToken(TokenType::Tilde, "~");
        case '&':
            if (match('&')) {
                return makeToken(TokenType::LogicAnd, "&&");
            }
            return makeToken(TokenType::BitwiseAnd, "&");
        case '$': return makeToken(TokenType::Dollar, "$");

        // Braces
        case '{': return makeToken(TokenType::LeftBrace, "{");
        case '}': return makeToken(TokenType::RightBrace, "}");
        case '[': return makeToken(TokenType::LeftBracket, "[");
        case ']': return makeToken(TokenType::RightBracket, "]");
        case '(': return makeToken(TokenType::LeftParen, "(");
        case ')': return makeToken(TokenType::RightParen, ")");

        // Misc
        case ',': return makeToken(TokenType::Comma, ",");
        case ';': return makeToken(TokenType::Semicolon, ";");
        case '?': return makeToken(TokenType::Ternary, "?");
        case ':': return makeToken(TokenType::Colon, ":");
        case '.': return makeToken(TokenType::Dot, ".");

        default: throw std::runtime_error("Unexpected character");
        }
    }

    bool match(char c) {
        if (pos >= input.size()) {
            return false;
        }

        if (input[pos] == c) {
            pos++;
            return true;
        }
        
        return false;
    }

    NVSEToken makeToken(TokenType type) {
        NVSEToken t(type);
        t.line = line;
        t.linePos = linePos;
        return t;
    }

    NVSEToken makeToken(TokenType type, std::string lexeme) {
        NVSEToken t(type, lexeme);
        t.line = line;
        t.linePos = linePos;
        linePos += lexeme.length();
        return t;
    }

    NVSEToken makeToken(TokenType type, double value) {
        NVSEToken t(type, value);
        t.line = line;
        t.linePos = linePos;
        return t;
    }

private:
    std::string input;
    size_t pos;
    
    size_t linePos = 0;
    size_t line = 1;
};