#include "NVSELexer.h"

#include <sstream>
#include <stack>

#include "NVSECompilerUtils.h"


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

// Unused for now, working though
std::deque<NVSEToken> NVSELexer::lexString() {
    std::stringstream resultString;

    int start = pos - 1;
    int startLine = line;

    std::deque<NVSEToken> results{};

    while (pos < input.size()) {
        char c = input[pos++];

        if (c == '\\') {
            if (pos >= input.size()) throw std::runtime_error("Unexpected end of input after escape character.");
            char next = input[pos++];
            if (next == '\\' || next == 'n' || next == '"') {
                if (next == 'n') resultString << '\n';
                else resultString << next;
            }
            else {
                throw std::runtime_error("Invalid escape sequence.");
            }
        }
        else if (c == '\n') {
            throw std::runtime_error("Unexpected end of line.");
        }
        else if (c == '"') {
            if (line != startLine) {
                throw std::runtime_error("Multiline strings are not allowed.");
            }

            // Push final result as string
            results.push_back(MakeToken(NVSETokenType::String, '"' + resultString.str() + '"', resultString.str()));
            return results;
        }
        else if (c == '$' && pos < input.size() && input[pos] == '{' && (pos == start + 1 || input[pos - 2] != '\\')) {
            if (interpDepth > 0) {
                throw std::runtime_error("Cannot have nested string interpolation.");
            }

            // Push previous result as string
            results.push_back(MakeToken(NVSETokenType::String, '"' + resultString.str() + '"', resultString.str()));

            pos += 1;
            NVSEToken tok;
            results.push_back(MakeToken(NVSETokenType::Interp, "${"));
            while ((tok = GetNextToken(false)).type != NVSETokenType::RightBrace) {
                if (tok.type == NVSETokenType::Eof) {
                    throw std::runtime_error("Unexpected end of file.");
                }
                results.push_back(tok);
            }
            results.push_back(MakeToken(NVSETokenType::EndInterp, "}"));

            resultString = {};
        }
        else {
            resultString << c;
        }
    }

    throw std::runtime_error("Unexpected end of input in string.");
}

NVSEToken NVSELexer::GetNextToken(bool useStack) {
    if (!tokenStack.empty() && useStack) {
        auto tok = tokenStack.front();
        tokenStack.pop_front();
        return tok;
    }

    bool inComment = false;
    while (pos < input.size()) {
        if (!std::isspace(input[pos]) && !inComment) {
            if (pos < input.size() - 2 && input[pos] == '/' && input[pos+1] == '/') {
                inComment = true;
                pos += 2;
            } else {
                break;
            }
        }
        
        if (input[pos] == '\n') {
            line++;
            linePos = 1;
            inComment = false;
        }
        
        pos++;
        linePos++;
    }
    
    if (pos >= input.size()) return MakeToken(NVSETokenType::Eof, "");

    char current = input[pos];
    if (std::isdigit(current)) {
        size_t len;
        double value = std::stod(&input[pos], &len);
        if (input[pos + len - 1] == '.') {
            len--;
        }
        pos += len;
        linePos += len;
        return MakeToken(NVSETokenType::Number, input.substr(pos - len, len), value);
    }

    if (std::isalpha(current) || current == '_') {
        // Extract identifier
        size_t start = pos;
        while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) ++pos;
        std::string identifier = input.substr(start, pos - start);

        // Keywords
        if (identifier == "if") return MakeToken(NVSETokenType::If, identifier);
        if (identifier == "else") return MakeToken(NVSETokenType::Else, identifier);
        if (identifier == "while") return MakeToken(NVSETokenType::While, identifier);
        if (identifier == "fn") return MakeToken(NVSETokenType::Fn, identifier);
        if (identifier == "return") return MakeToken(NVSETokenType::Return, identifier);
        if (identifier == "for") return MakeToken(NVSETokenType::For, identifier);
        if (identifier == "name") return MakeToken(NVSETokenType::Name, identifier);
        if (identifier == "continue") return MakeToken(NVSETokenType::Continue, identifier);
        if (identifier == "break") return MakeToken(NVSETokenType::Break, identifier);

        // Types
        if (identifier == "int") return MakeToken(NVSETokenType::IntType, identifier);
        if (identifier == "double") return MakeToken(NVSETokenType::DoubleType, identifier);
        if (identifier == "ref") return MakeToken(NVSETokenType::RefType, identifier);
        if (identifier == "string") return MakeToken(NVSETokenType::StringType, identifier);
        if (identifier == "array") return MakeToken(NVSETokenType::ArrayType, identifier);

        // Boolean literals
        if (identifier == "true") return MakeToken(NVSETokenType::Bool, identifier, 1);
        if (identifier == "false") return MakeToken(NVSETokenType::Bool, identifier, 0);

        // See if it is a begin block type
        for (auto& g_eventBlockCommandInfo : g_eventBlockCommandInfos) {
            if (!strcmp(g_eventBlockCommandInfo.longName, identifier.c_str())) {
                return MakeToken(NVSETokenType::BlockType, identifier);
            }
        }

        return MakeToken(NVSETokenType::Identifier, identifier);
    }

    if (current == '"') {
        pos++;
        auto results = lexString();
        auto tok = results.front();
        results.pop_front();

        for (auto r : results) {
            tokenStack.push_back(r);
        }

        return tok;
    }

    pos++;
    switch (current) {
        // Operators
    case '+': {
        if (Match('=')) {
            return MakeToken(NVSETokenType::PlusEq, "+=");
        }
        else if (Match('+')) {
            return MakeToken(NVSETokenType::PlusPlus, "++");
        }
        return MakeToken(NVSETokenType::Plus, "+");
    }
    case '-': {
        if (Match('=')) {
            return MakeToken(NVSETokenType::MinusEq, "-=");
        }
        else if (Match('-')) {
            return MakeToken(NVSETokenType::MinusMinus, "--");
        }
        return MakeToken(NVSETokenType::Minus, "-");
    }
    case '*':
        if (Match('=')) {
            return MakeToken(NVSETokenType::StarEq, "*=");
        }
        return MakeToken(NVSETokenType::Star, "*");
    case '/':
        if (Match('=')) {
            return MakeToken(NVSETokenType::SlashEq, "/=");
        }
        return MakeToken(NVSETokenType::Slash, "/");
    case '%':
        if (Match('=')) {
            return MakeToken(NVSETokenType::ModEq, "%=");
        }
        return MakeToken(NVSETokenType::Mod, "%");
    case '^':
        if (Match('=')) {
            return MakeToken(NVSETokenType::PowEq, "^=");
        }
        return MakeToken(NVSETokenType::Pow, "^");
    case '=':
        if (Match('=')) {
            return MakeToken(NVSETokenType::EqEq, "==");
        }
        return MakeToken(NVSETokenType::Eq, "=");
    case '!':
        if (Match('=')) {
            return MakeToken(NVSETokenType::BangEq, "!=");
        }
        return MakeToken(NVSETokenType::Bang, "!");
    case '<':
        if (Match('=')) {
            return MakeToken(NVSETokenType::LessEq, "<=");
        }
        return MakeToken(NVSETokenType::Less, "<");
    case '>':
        if (Match('=')) {
            return MakeToken(NVSETokenType::GreaterEq, ">=");
        }
        return MakeToken(NVSETokenType::Greater, ">");
    case '|':
        if (Match('|')) {
            return MakeToken(NVSETokenType::LogicOr, "||");
        }
        return MakeToken(NVSETokenType::BitwiseOr, "|");
    case '~': return MakeToken(NVSETokenType::Tilde, "~");
    case '&':
        if (Match('&')) {
            return MakeToken(NVSETokenType::LogicAnd, "&&");
        }
        return MakeToken(NVSETokenType::BitwiseAnd, "&");
    case '$': return MakeToken(NVSETokenType::Dollar, "$");
    case '#': return MakeToken(NVSETokenType::Pound, "#");

        // Braces
    case '{': return MakeToken(NVSETokenType::LeftBrace, "{");
    case '}': return MakeToken(NVSETokenType::RightBrace, "}");
    case '[': return MakeToken(NVSETokenType::LeftBracket, "[");
    case ']': return MakeToken(NVSETokenType::RightBracket, "]");
    case '(': return MakeToken(NVSETokenType::LeftParen, "(");
    case ')': return MakeToken(NVSETokenType::RightParen, ")");

        // Misc
    case ',': return MakeToken(NVSETokenType::Comma, ",");
    case ';': return MakeToken(NVSETokenType::Semicolon, ";");
    case '?': return MakeToken(NVSETokenType::Ternary, "?");
    case ':': return MakeToken(NVSETokenType::Colon, ":");
    case '.': return MakeToken(NVSETokenType::Dot, ".");

    default: throw std::runtime_error("Unexpected character");
    }
}

bool NVSELexer::Match(char c) {
    if (pos >= input.size()) {
        return false;
    }

    if (input[pos] == c) {
        pos++;
        return true;
    }

    return false;
}

NVSEToken NVSELexer::MakeToken(NVSETokenType type, std::string lexeme) {
    NVSEToken t(type, lexeme);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::MakeToken(NVSETokenType type, std::string lexeme, double value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}

NVSEToken NVSELexer::MakeToken(NVSETokenType type, std::string lexeme, std::string value) {
    NVSEToken t(type, lexeme, value);
    t.line = line;
    t.linePos = linePos;
    linePos += lexeme.length();
    return t;
}