#include "NVSELexer.h"

#include <sstream>
#include <stack>

#include "NVSECompilerUtils.h"


NVSELexer::NVSELexer(std::string& input) : pos(0) {
	// Replace tabs with 4 spaces
	size_t it;
	while ((it = input.find('\t')) != std::string::npos) {
		input.replace(it, 1, "    ");
	}
	
	// Messy way of just getting string lines to start for later error reporting
	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = input.find('\n', prev)) != std::string::npos) {
		lines.push_back(input.substr(prev, pos - prev - 1));
		prev = pos + 1;
	}
	lines.push_back(input.substr(prev));
	
	this->input = input;
}

// Unused for now, working though
std::deque<NVSEToken> NVSELexer::lexString() {
	std::stringstream resultString;

	auto start = pos - 1;
	auto startLine = line;
	auto startCol = column;

	std::deque<NVSEToken> results{};

	while (pos < input.size()) {
		char c = input[pos++];

		if (c == '\\') {
			if (pos >= input.size()) throw std::runtime_error("Unexpected end of input after escape character.");
			if (char next = input[pos++]; next == '\\' || next == 'n' || next == '"') {
				if (next == 'n') {
					resultString << '\n';
				}
				else {
					resultString << next;
				}
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
			NVSEToken token{};
			token.type = NVSETokenType::String;
			token.lexeme = '"' + resultString.str() + '"';
			token.value = resultString.str();
			token.line = line;
			token.column = startCol;
			results.push_back(token);

			// Update column
			column = pos;

			return results;
		}
		else if (c == '$' && pos < input.size() && input[pos] == '{' && (pos == start + 1 || input[pos - 2] != '\\')) {
			// Push previous result as string
			results.push_back(MakeToken(NVSETokenType::String, '"' + resultString.str() + '"', resultString.str()));

			NVSEToken startInterpolate{};
			startInterpolate.type = NVSETokenType::Interp;
			startInterpolate.lexeme = "${";
			startInterpolate.column = startCol + (pos - start);
			startInterpolate.line = line;
			results.push_back(startInterpolate);

			pos += 1;
			NVSEToken tok;
			while ((tok = GetNextToken(false)).type != NVSETokenType::RightBrace) {
				if (tok.type == NVSETokenType::Eof) {
					throw std::runtime_error("Unexpected end of file.");
				}
				results.push_back(tok);
			}

			NVSEToken endInterpolate{};
			endInterpolate.type = NVSETokenType::EndInterp;
			endInterpolate.lexeme = "}";
			endInterpolate.column = column - 1;
			endInterpolate.line = line;
			results.push_back(endInterpolate);

			resultString = {};
			startCol = pos;
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

	// Skip over comments and whitespace in this block.
	{
		// Using an enum instead of two bools, since it's impossible to be in a singleline comment and a multiline comment at once.
		enum class InWhatComment
		{
			None = 0,
			SingleLine,
			MultiLine
		} inWhatComment = InWhatComment::None;

		while (pos < input.size()) {
			if (!std::isspace(input[pos]) && inWhatComment == InWhatComment::None) {
				if (pos < input.size() - 2 && input[pos] == '/') {
					if (input[pos + 1] == '/') {
						inWhatComment = InWhatComment::SingleLine;
						pos += 2;
					}
					else if (input[pos + 1] == '*')
					{
						inWhatComment = InWhatComment::MultiLine;
						pos += 2;
						column += 2; // multiline comment could end on the same line it's declared on and have valid code after itself.
					}
					else {
						break; // found start of next token
					}
				}
				else {
					break; // found start of next token
				}
			}

			if (inWhatComment == InWhatComment::MultiLine &&
				(pos < input.size() - 2) && input[pos] == '*' && input[pos + 1] == '/')
			{
				inWhatComment = InWhatComment::None;
				pos += 2;
				column += 2; // multiline comment could end on the same line it's declared on and have valid code after itself.
				continue; // could be entering another comment right after this one; 
				// Don't want to reach the end of the loop and increment `pos` before that.
			}

			if (input[pos] == '\n') {
				line++;
				column = 1;
				if (inWhatComment == InWhatComment::SingleLine) {
					inWhatComment = InWhatComment::None;
				}
			}
			else if (input[pos] == '\r' && (pos < input.size() - 1) && input[pos + 1] == '\n') {
				line++;
				pos++;
				column = 1;
				if (inWhatComment == InWhatComment::SingleLine) {
					inWhatComment = InWhatComment::None;
				}
			}
			else {
				column++;
			}

			pos++;
		}
	}

	if (pos >= input.size()) {
		return MakeToken(NVSETokenType::Eof, "");
	}

	char current = input[pos];
	if (std::isdigit(current)) {
		int base = 10;
		if (current == '0' && pos + 2 < input.size()) {
			if (std::tolower(input[pos + 1]) == 'b') {
				base = 2;
				pos += 2;
				current = input[pos];
			} else if (std::tolower(input[pos + 1]) == 'x') {
				base = 16;
				pos += 2;
				current = input[pos];
			}
		}
		
		size_t len;
		double value;
		try {
			if (base == 16 || base == 2) {
				value = std::stoll(&input[pos], &len, base);
			} else {
				value = std::stod(&input[pos], &len);
			}
	    } catch (const std::invalid_argument &ex) {
	    	throw std::runtime_error("Invalid numeric literal.");
	    } catch (const std::out_of_range &ex) {
	    	throw std::runtime_error("Numeric literal value out of range.");
	    }
		
		if (input[pos + len - 1] == '.') {
			len--;
		}
		pos += len;
		column += len;

		// Handle 0b/0x
		if (base == 2 || base == 16) {
			len += 2;
		}
		
		return MakeToken(NVSETokenType::Number, input.substr(pos - len, len), value);
	}

	{
		// Check if potential identifier has at least 1 alphabetical character.
		// Must either start with underscores, or an alphabetical character.
		bool isValidIdentifier = std::isalnum(current);
		if (!isValidIdentifier && current == '_') {
			size_t lookaheadPos = pos + 1;
			while (lookaheadPos < input.size()) {
				if (std::isalpha(input[lookaheadPos])) {
					isValidIdentifier = true;
					break;
				}
				else if (input[lookaheadPos] != '_') {
					break;
				}
				++lookaheadPos;
			}
		}

		if (isValidIdentifier) {
			// Extract identifier
			size_t start = pos;
			while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) {
				++pos;
			}
			std::string identifier = input.substr(start, pos - start);

			// Keywords
			if (_stricmp(identifier.c_str(), "if") == 0) return MakeToken(NVSETokenType::If, identifier);
			if (_stricmp(identifier.c_str(), "else") == 0) return MakeToken(NVSETokenType::Else, identifier);
			if (_stricmp(identifier.c_str(), "while") == 0) return MakeToken(NVSETokenType::While, identifier);
			if (_stricmp(identifier.c_str(), "fn") == 0) return MakeToken(NVSETokenType::Fn, identifier);
			if (_stricmp(identifier.c_str(), "return") == 0) return MakeToken(NVSETokenType::Return, identifier);
			if (_stricmp(identifier.c_str(), "for") == 0) return MakeToken(NVSETokenType::For, identifier);
			if (_stricmp(identifier.c_str(), "name") == 0) return MakeToken(NVSETokenType::Name, identifier);
			if (_stricmp(identifier.c_str(), "continue") == 0) return MakeToken(NVSETokenType::Continue, identifier);
			if (_stricmp(identifier.c_str(), "break") == 0) return MakeToken(NVSETokenType::Break, identifier);
			if (_stricmp(identifier.c_str(), "in") == 0) return MakeToken(NVSETokenType::In, identifier);
			if (_stricmp(identifier.c_str(), "not") == 0) return MakeToken(NVSETokenType::Not, identifier);

			if (_stricmp(identifier.c_str(), "int") == 0) return MakeToken(NVSETokenType::IntType, identifier);
			if (_stricmp(identifier.c_str(), "double") == 0) return MakeToken(NVSETokenType::DoubleType, identifier);
			if (_stricmp(identifier.c_str(), "float") == 0) return MakeToken(NVSETokenType::DoubleType, identifier);
			if (_stricmp(identifier.c_str(), "ref") == 0) return MakeToken(NVSETokenType::RefType, identifier);
			if (_stricmp(identifier.c_str(), "string") == 0) return MakeToken(NVSETokenType::StringType, identifier);
			if (_stricmp(identifier.c_str(), "array") == 0) return MakeToken(NVSETokenType::ArrayType, identifier);

			if (_stricmp(identifier.c_str(), "true") == 0) return MakeToken(NVSETokenType::Bool, identifier, 1);
			if (_stricmp(identifier.c_str(), "false") == 0) return MakeToken(NVSETokenType::Bool, identifier, 0);
			if (_stricmp(identifier.c_str(), "null") == 0) return MakeToken(NVSETokenType::Number, identifier, 0);

			// See if it is a begin block type
			for (auto& g_eventBlockCommandInfo : g_eventBlockCommandInfos) {
				if (!_stricmp(g_eventBlockCommandInfo.longName, identifier.c_str())) {
					return MakeToken(NVSETokenType::BlockType, identifier);
				}
			}

			return MakeToken(NVSETokenType::Identifier, identifier);
		}
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
		if (Match('+')) {
			return MakeToken(NVSETokenType::PlusPlus, "++");
		}
		return MakeToken(NVSETokenType::Plus, "+");
	}
	case '-': {
		if (Match('=')) {
			return MakeToken(NVSETokenType::MinusEq, "-=");
		}
		if (Match('-')) {
			return MakeToken(NVSETokenType::MinusMinus, "--");
		}
		if (Match('>')) {
			return MakeToken(NVSETokenType::Arrow, "->");
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
		if (Match('<')) {
			return MakeToken(NVSETokenType::LeftShift, "<<");
		}
		return MakeToken(NVSETokenType::Less, "<");
	case '>':
		if (Match('=')) {
			return MakeToken(NVSETokenType::GreaterEq, ">=");
		}
		if (Match('>')) {
			return MakeToken(NVSETokenType::RightShift, ">>");
		}
		return MakeToken(NVSETokenType::Greater, ">");
	case '|':
		if (Match('|')) {
			return MakeToken(NVSETokenType::LogicOr, "||");
		}
		if (Match('=')) {
			return MakeToken(NVSETokenType::BitwiseOrEquals, "|=");
		}
		return MakeToken(NVSETokenType::BitwiseOr, "|");
	case '~':
		return MakeToken(NVSETokenType::BitwiseNot, "~");
	case '&':
		if (Match('&')) {
			return MakeToken(NVSETokenType::LogicAnd, "&&");
		}
		if (Match('=')) {
			return MakeToken(NVSETokenType::BitwiseAndEquals, "&=");
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
	case ':':
		if (Match(':')) {
			return MakeToken(NVSETokenType::MakePair, "::");
		}
		return MakeToken(NVSETokenType::Slice, ":");
	case '.': 
		return MakeToken(NVSETokenType::Dot, ".");
	case '_':
		return MakeToken(NVSETokenType::Underscore, "_");
	default: throw std::runtime_error("Unexpected character");
	}

	throw std::runtime_error("Unexpected character");
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
	t.column = column;
	column += lexeme.length();
	return t;
}

NVSEToken NVSELexer::MakeToken(NVSETokenType type, std::string lexeme, double value) {
	NVSEToken t(type, lexeme, value);
	t.line = line;
	t.column = column;
	column += lexeme.length();
	return t;
}

NVSEToken NVSELexer::MakeToken(NVSETokenType type, std::string lexeme, std::string value) {
	NVSEToken t(type, lexeme, value);
	t.line = line;
	t.column = column;
	column += lexeme.length();
	return t;
}
