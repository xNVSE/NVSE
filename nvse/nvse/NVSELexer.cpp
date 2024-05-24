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

    if (pos == input.size()) return makeToken(NVSETokenType::End, "");

    char current = input[pos];
    if (std::isdigit(current)) {
        size_t len;
        double value = std::stod(&input[pos], &len);
        if (input[pos + len - 1] == '.') {
            len--;
        }
        pos += len;
        linePos += len;
        return makeToken(NVSETokenType::Number, input.substr(pos - len, len), value);
    }

    if (std::isalpha(current) || current == '_') {
        // Extract identifier
        size_t start = pos;
        while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) ++pos;
        std::string identifier = input.substr(start, pos - start);

        // Keywords
        if (identifier == "if") return makeToken(NVSETokenType::If, identifier);
        if (identifier == "else") return makeToken(NVSETokenType::Else, identifier);
        if (identifier == "while") return makeToken(NVSETokenType::While, identifier);
        if (identifier == "fn") return makeToken(NVSETokenType::Fn, identifier);
        if (identifier == "return") return makeToken(NVSETokenType::Return, identifier);
        if (identifier == "for") return makeToken(NVSETokenType::For, identifier);
        if (identifier == "begin") return makeToken(NVSETokenType::Begin, identifier);
        if (identifier == "name") return makeToken(NVSETokenType::Name, identifier);

        // Types
        if (identifier == "int") return makeToken(NVSETokenType::IntType, identifier);
        if (identifier == "double") return makeToken(NVSETokenType::DoubleType, identifier);
        if (identifier == "ref") return makeToken(NVSETokenType::RefType, identifier);
        if (identifier == "string") return makeToken(NVSETokenType::StringType, identifier);
        if (identifier == "array") return makeToken(NVSETokenType::ArrayType, identifier);

        // Boolean literals
        if (identifier == "true") return makeToken(NVSETokenType::Bool, identifier, 1);
        if (identifier == "false") return makeToken(NVSETokenType::Bool, identifier, 0);

        return makeToken(NVSETokenType::Identifier, identifier);
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
        return makeToken(NVSETokenType::String, lexeme, text);
    }

    pos++;
    switch (current) {
        // Operators
    case '+': {
        if (match('=')) {
            return makeToken(NVSETokenType::PlusEq, "+=");
        }
        else if (match('+')) {
            return makeToken(NVSETokenType::PlusPlus, "++");
        }
        return makeToken(NVSETokenType::Plus, "+");
    }
    case '-': {
        if (match('=')) {
            return makeToken(NVSETokenType::MinusEq, "-=");
        }
        else if (match('-')) {
            return makeToken(NVSETokenType::MinusMinus, "--");
        }
        return makeToken(NVSETokenType::Minus, "-");
    }
    case '*':
        if (match('=')) {
            return makeToken(NVSETokenType::StarEq, "*=");
        }
        return makeToken(NVSETokenType::Star, "*");
    case '/':
        if (match('=')) {
            return makeToken(NVSETokenType::SlashEq, "/=");
        }
        return makeToken(NVSETokenType::Slash, "/");
    case '=':
        if (match('=')) {
            return makeToken(NVSETokenType::EqEq, "==");
        }
        return makeToken(NVSETokenType::Eq, "=");
    case '!':
        if (match('=')) {
            return makeToken(NVSETokenType::BangEq, "!=");
        }
        return makeToken(NVSETokenType::Bang, "!");
    case '<':
        if (match('=')) {
            return makeToken(NVSETokenType::LessEq, "<=");
        }
        return makeToken(NVSETokenType::Less, "<");
    case '>':
        if (match('=')) {
            return makeToken(NVSETokenType::GreaterEq, ">=");
        }
        return makeToken(NVSETokenType::Greater, ">");
    case '|':
        if (match('|')) {
            return makeToken(NVSETokenType::LogicOr, "||");
        }
        return makeToken(NVSETokenType::BitwiseOr, "|");
    case '~': return makeToken(NVSETokenType::Tilde, "~");
    case '&':
        if (match('&')) {
            return makeToken(NVSETokenType::LogicAnd, "&&");
        }
        return makeToken(NVSETokenType::BitwiseAnd, "&");
    case '$': return makeToken(NVSETokenType::Dollar, "$");

        // Braces
    case '{': return makeToken(NVSETokenType::LeftBrace, "{");
    case '}': return makeToken(NVSETokenType::RightBrace, "}");
    case '[': return makeToken(NVSETokenType::LeftBracket, "[");
    case ']': return makeToken(NVSETokenType::RightBracket, "]");
    case '(': return makeToken(NVSETokenType::LeftParen, "(");
    case ')': return makeToken(NVSETokenType::RightParen, ")");

        // Misc
    case ',': return makeToken(NVSETokenType::Comma, ",");
    case ';': return makeToken(NVSETokenType::Semicolon, ";");
    case '?': return makeToken(NVSETokenType::Ternary, "?");
    case ':': return makeToken(NVSETokenType::Colon, ":");
    case '.': return makeToken(NVSETokenType::Dot, ".");

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

NVSEToken NVSELexer::makeToken(NVSETokenType type, std::string lexeme) {
    NVSEToken t(type, lexeme);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::makeToken(NVSETokenType type, std::string lexeme, double value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::makeToken(NVSETokenType type, std::string lexeme, std::string value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}