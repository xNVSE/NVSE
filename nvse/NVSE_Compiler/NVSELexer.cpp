#include "NVSELexer.h"

NVSELexer::NVSELexer(const std::string& input) : input(input), pos(0) {
	// Messy way of just getting string lines to start for later error reporting
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = input.find('\n', prev)) != std::string::npos)
    {
        lines.push_back(input.substr(prev, pos - prev - 1));
        prev = pos + 1;
    }
    lines.push_back(input.substr(prev));
}

NVSEToken NVSELexer::getNextToken() {
    while (pos < input.size() && std::isspace(input[pos])) {
        pos++;
        linePos++;
        if (input[pos] == '\n') {
            line++;
            pos++;
            linePos = 1;
        }
    }

    if (pos == input.size()) return makeToken(TokenType::End, "");

    char current = input[pos];
    if (std::isdigit(current)) {
        size_t len;
        double value = std::stod(&input[pos], &len);
        if (input[pos + len - 1] == '.') {
            len--;
        }
        pos += len;
        linePos += len;
        return makeToken(TokenType::Number, input.substr(pos - len, len), value);
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
        if (identifier == "for") return makeToken(TokenType::For, identifier);

        // Types
        if (identifier == "int") return makeToken(TokenType::IntType, identifier);
        if (identifier == "double") return makeToken(TokenType::DoubleType, identifier);
        if (identifier == "ref") return makeToken(TokenType::RefType, identifier);
        if (identifier == "string") return makeToken(TokenType::StringType, identifier);
        if (identifier == "array") return makeToken(TokenType::ArrayType, identifier);

        // Boolean literals
        if (identifier == "true") return makeToken(TokenType::Bool, identifier, 1);
        if (identifier == "false") return makeToken(TokenType::Bool, identifier, 0);

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
        std::string text = input.substr(start + 1, len - 2);
        std::string lexeme = '\"' + text + '\"';
        return makeToken(TokenType::String, lexeme, text);
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

bool NVSELexer::match(char c) {
    if (pos >= input.size()) {
        return false;
    }

    if (input[pos] == c) {
        pos++;
        return true;
    }

    return false;
}

NVSEToken NVSELexer::makeToken(TokenType type, std::string lexeme) {
    NVSEToken t(type, lexeme);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::makeToken(TokenType type, std::string lexeme, double value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::makeToken(TokenType type, std::string lexeme, std::string value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}