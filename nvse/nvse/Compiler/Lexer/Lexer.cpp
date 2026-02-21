#include "Lexer.h"

#include <regex>
#include <sstream>
#include <stack>

#include "nvse/Compiler/Utils.h"

namespace Compiler {
	Lexer::Lexer(const std::string& input) : pos(0) {
		auto inputStr = std::string{ input };

		// Replace tabs with 4 spaces
		size_t it;
		while ((it = inputStr.find('\t')) != std::string::npos) {
			inputStr.replace(it, 1, "    ");
		}

		while ((it = inputStr.find("\r\n")) != std::string::npos) {
			inputStr.replace(it, 2, "\n");
		}

		// Messy way of just getting string lines to start for later error reporting
		std::string::size_type pos = 0;
		std::string::size_type prev = 0;
		while ((pos = inputStr.find('\n', prev)) != std::string::npos) {
			lines.push_back(inputStr.substr(prev, pos - prev));
			prev = pos + 1;
		}
		lines.push_back(inputStr.substr(prev));

		this->input = inputStr;
	}

	// Unused for now, working though
	std::deque<Token> Lexer::lexString() {
		std::stringstream resultString;

		auto startPos = PrevSourcePos();

		auto start = pos - 1;
		auto startLine = line;

		std::deque<Token> results{};

		while (pos < input.size()) {
			const auto c = input[AdvanceChar()];

			if (c == '\\') {
				if (pos >= input.size()) throw std::runtime_error("Unexpected end of input after escape character.");
				if (char next = input[AdvanceChar()]; next == '\\' || next == 'n' || next == '"') {
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

				results.emplace_back(TokenType::String, '"' + resultString.str() + '"', startPos, PrevSourcePos(), resultString.str());

				return results;
			}
			else if (c == '$' && pos < input.size() && input[pos] == '{' && (pos == start + 1 || input[pos - 2] != '\\')) {
				// Push previous result as string
				results.emplace_back(TokenType::String, '"' + resultString.str() + '"', startPos, CurSourcePos() - 2, resultString.str());
				results.emplace_back(TokenType::Interp, "${", PrevSourcePos(), CurSourcePos());

				AdvanceChar();

				Token tok;
				while ((tok = GetNextToken(false)).type != TokenType::RightBrace) {
					if (tok.type == TokenType::Eof) {
						throw std::runtime_error("Unexpected end of file.");
					}
					results.push_back(tok);
				}

				results.emplace_back(TokenType::EndInterp, "}", PrevSourcePos(), PrevSourcePos());
				startPos = CurSourcePos();

				resultString = {};
			}
			else {
				resultString << c;
			}
		}

		throw std::runtime_error("Unexpected end of input in string.");
	}

	bool Lexer::CheckIdentifier(const std::string &identifier, const char* ident) {
		return _stricmp(identifier.c_str(), ident) == 0;
	}

	std::string Lexer::GetSourceText(const SourcePos& startPos, const SourcePos& endPos) const {
		return input.substr(startPos.absoluteIndex, endPos.absoluteIndex - startPos.absoluteIndex);
	}

	std::string Lexer::GetSourceText(const SourcePos& startPos) const {
		auto endPos = CurSourcePos();
		return input.substr(startPos.absoluteIndex, endPos.absoluteIndex - startPos.absoluteIndex);
	}

	Token Lexer::GetNextToken(bool useStack) {
		if (!tokenStack.empty() && useStack) {
			auto tok = tokenStack.front();
			tokenStack.pop_front();
			return tok;
		}

		// Skip over whitespace
		while (std::isspace(input[pos])) {
			if (input[pos] == '\n') {
				column = 1;
				line++;
			} else {
				column++;
			}

			pos++;

			if (pos >= input.size()) {
				break;
			}

			if (pos < input.size() - 1) {
				if (input[pos] == '/' && input[pos + 1] == '/') {
					pos += 2;
					column += 2; 
					while (pos < input.size()) {
						if (input[pos] == '\n') {
							column = 1;
							line++;
							pos++;
							break;
						}

						pos++;
						column++;
					}
				}
			}
		}

		// TODO - Maybe re-add multi-line
		// {
		// 	// Using an enum instead of two bools, since it's impossible to be in a singleline comment and a multiline comment at once.
		// 	enum class InWhatComment
		// 	{
		// 		None = 0,
		// 		SingleLine,
		// 		MultiLine
		// 	} inWhatComment = InWhatComment::None;
		//
		// 	while (pos < input.size()) {
		// 		if (!std::isspace(input[pos]) && inWhatComment == InWhatComment::None) {
		// 			if (pos < input.size() - 2 && input[pos] == '/') {
		// 				if (input[pos + 1] == '/') {
		// 					inWhatComment = InWhatComment::SingleLine;
		// 					pos += 2;
		// 				}
		// 				else if (input[pos + 1] == '*')
		// 				{
		// 					inWhatComment = InWhatComment::MultiLine;
		// 					pos += 2;
		// 					column += 2; // multiline comment could end on the same line it's declared on and have valid code after itself.
		// 				}
		// 				else {
		// 					break; // found start of next token
		// 				}
		// 			}
		// 			else {
		// 				break; // found start of next token
		// 			}
		// 		}
		//
		// 		if (inWhatComment == InWhatComment::MultiLine &&
		// 			(pos < input.size() - 2) && input[pos] == '*' && input[pos + 1] == '/')
		// 		{
		// 			inWhatComment = InWhatComment::None;
		// 			pos += 2;
		// 			column += 2; // multiline comment could end on the same line it's declared on and have valid code after itself.
		// 			continue; // could be entering another comment right after this one; 
		// 			// Don't want to reach the end of the loop and increment `pos` before that.
		// 		}
		//
		// 		if (input[pos] == '\n') {
		// 			line++;
		// 			column = 1;
		// 			if (inWhatComment == InWhatComment::SingleLine) {
		// 				inWhatComment = InWhatComment::None;
		// 			}
		// 		}
		// 		else if (input[pos] == '\r' && (pos < input.size() - 1) && input[pos + 1] == '\n') {
		// 			line++;
		// 			pos++;
		// 			column = 1;
		// 			if (inWhatComment == InWhatComment::SingleLine) {
		// 				inWhatComment = InWhatComment::None;
		// 			}
		// 		}
		// 		else {
		// 			column++;
		// 		}
		//
		// 		pos++;
		// 	}
		// }

		if (pos >= input.size()) {
			return Token{ TokenType::Eof };
		}

		char current = input[pos];

		if (std::isdigit(current)) {
			std::string_view numberView{};
			uint8_t numberBase = 10;

			auto start = CurSourcePos();

			// Advance until not a digit
			auto cur = pos + 1;

			// Handle numeric literals
			if (input[cur] == 'b') {
				numberBase = 2;
				cur++;
			} 
			else if (input[cur] == 'x') {
				numberBase = 16;
				cur++;
			}

			int numDecimal = 0;
			while (cur < input.size()) {
				const auto curChar = input[cur];
				const auto len = cur - start.absoluteIndex;

				if (std::isdigit(curChar)) {
					// If base-2, only allow processing of 0/1
					if (numberBase == 2) {
						if (curChar != '0' && curChar != '1') {
							break;
						}
					}

					cur++;
					continue;
				}

				if (curChar == '.') {
					numDecimal++;

					// Cannot use decimals with hex or binary numbers
					if (numberBase != 10) {
						break;
					}

					// Don't reach out of bounds
					if (cur + 1 >= input.size()) {
						break;
					}

					// <dot><number>
					if (std::isdigit(input[cur + 1])) {
						cur += 2;
						continue;
					}

					// Anything that is not <dot><number> is invalid
					break;
				}

				// Spaces terminate numbers
				if (curChar == ' ') {
					numberView = std::string_view(input).substr(start.absoluteIndex, len);
					pos += len;
					column += len;
					break;
				}

				// Alpha chars can not directly follow numbers
				if (std::isalpha(curChar)) {
					// Except if we are processing base 16
					if (std::tolower(curChar) >= 'a' && std::tolower(curChar) <= 'f') {
						cur++;
						continue;
					}

					break;
				}

				// Any punctuation chars will terminate the number as well
				numberView = std::string_view(input).substr(start.absoluteIndex, len);
				pos += len;
				column += len;
				break;
			}

			// Multiple decimals in a number does not make sense, so discard the entire thing
			if (numDecimal > 1) {
				numberView = {};
			}

			if (!numberView.empty()) {
				const auto end = SourcePos {
					.absoluteIndex = start.absoluteIndex + numberView.length() - 1,
					.line = start.line,
					.column = start.column + numberView.length() - 1
				};

				auto numStr = std::string(numberView);

				double value;
				if (numberBase == 16) {
					value = static_cast<double>(std::stoll(numStr, nullptr, numberBase));
				}
				
				else if (numberBase == 2) {
					value = static_cast<double>(std::stoll(numStr.substr(2), nullptr, numberBase));
				}
				
				else {
					value = std::stod(numStr);
				}

				DbgPrintln("Found number lexeme '%s' (Base %d) [%d:%d - %d:%d] - Value %.02lf", numStr.c_str(), numberBase, start.line, start.column, end.line, end.column, value);
				return { TokenType::Number, std::move(numStr), std::move(start), end, value };
			}
		}

		current = input[pos];
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
				auto startPos = CurSourcePos();

				// Extract identifier
				size_t start = pos;
				while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) {
					AdvanceChar();
				}
				std::string identifier = input.substr(start, pos - start);

				// Handle value types first
				if (CheckIdentifier(identifier, "true")) {
					return { TokenType::Bool, std::move(identifier), startPos, CurSourcePos(), 1.0 };
				}

				if (CheckIdentifier(identifier, "false")) {
					return { TokenType::Bool, std::move(identifier), startPos, CurSourcePos(), 0.0 };
				}

				if (CheckIdentifier(identifier, "null")) {
					return { TokenType::Number, std::move(identifier), startPos, CurSourcePos(), 0.0 };
				}

				// Identifier by default
				auto type = TokenType::Identifier;

				if (CheckIdentifier(identifier, "if")) type = TokenType::If;
				else if (CheckIdentifier(identifier, "else")) type = TokenType::Else;
				else if (CheckIdentifier(identifier, "while")) type = TokenType::While;
				else if (CheckIdentifier(identifier, "fn")) type = TokenType::Fn;
				else if (CheckIdentifier(identifier, "return")) type = TokenType::Return;
				else if (CheckIdentifier(identifier, "for")) type = TokenType::For;
				else if (CheckIdentifier(identifier, "name")) type = TokenType::Name;
				else if (CheckIdentifier(identifier, "continue")) type = TokenType::Continue;
				else if (CheckIdentifier(identifier, "break")) type = TokenType::Break;
				else if (CheckIdentifier(identifier, "in")) type = TokenType::In;
				else if (CheckIdentifier(identifier, "not")) type = TokenType::Not;
				else if (CheckIdentifier(identifier, "match")) type = TokenType::Match;
				else if (CheckIdentifier(identifier, "int")) type = TokenType::IntType;
				else if (CheckIdentifier(identifier, "bool")) type = TokenType::IntType;
				else if (CheckIdentifier(identifier, "double")) type = TokenType::DoubleType;
				else if (CheckIdentifier(identifier, "float")) type = TokenType::DoubleType;
				else if (CheckIdentifier(identifier, "ref")) type = TokenType::RefType;
				else if (CheckIdentifier(identifier, "string")) type = TokenType::StringType;
				else if (CheckIdentifier(identifier, "array")) type = TokenType::ArrayType;

				return { type, std::move(identifier), startPos, PrevSourcePos() };
			}
		}

		if (current == '"') {
			AdvanceChar();

			auto results = lexString();
			auto tok = results.front();
			results.pop_front();

			for (const auto& r : results) {
				tokenStack.push_back(r);
			}

			return tok;
		}

		auto startPos = CurSourcePos();
		auto type = TokenType::INVALID;

		if (Match("+=")) type = TokenType::PlusEq;
		else if (Match("++")) type = TokenType::PlusPlus;
		else if (Match("+")) type = TokenType::Plus;

		else if (Match("-=")) type = TokenType::MinusEq;
		else if (Match("--")) type = TokenType::MinusMinus;
		else if (Match("->")) type = TokenType::Arrow;
		else if (Match("-")) type = TokenType::Minus;

		else if (Match("*=")) type = TokenType::StarEq;
		else if (Match("*")) type = TokenType::Star;

		else if (Match("/=")) type = TokenType::SlashEq;
		else if (Match("/")) type = TokenType::Slash;

		else if (Match("%=")) type = TokenType::ModEq;
		else if (Match("%")) type = TokenType::Mod;

		else if (Match("^=")) type = TokenType::PowEq;
		else if (Match("^")) type = TokenType::Pow;

		else if (Match("==")) type = TokenType::EqEq;
		else if (Match("=")) type = TokenType::Eq;

		else if (Match("!=")) type = TokenType::BangEq;
		else if (Match("!")) type = TokenType::Bang;

		else if (Match("<=")) type = TokenType::LessEq;
		else if (Match("<<")) type = TokenType::LeftShift;
		else if (Match("<")) type = TokenType::Less;

		else if (Match(">=")) type = TokenType::GreaterEq;
		else if (Match(">>")) type = TokenType::RightShift;
		else if (Match(">")) type = TokenType::Greater;

		else if (Match("||")) type = TokenType::LogicOr;
		else if (Match("|=")) type = TokenType::BitwiseOrEquals;
		else if (Match("|")) type = TokenType::BitwiseOr;

		else if (Match("~")) type = TokenType::BitwiseNot;

		else if (Match("&&")) type = TokenType::LogicAnd;
		else if (Match("&=")) type = TokenType::BitwiseAndEquals;
		else if (Match("&")) type = TokenType::BitwiseAnd;

		else if (Match("::")) type = TokenType::MakePair;
		else if (Match(":")) type = TokenType::Slice;

		else if (Match("$")) type = TokenType::Dollar;
		else if (Match("#")) type = TokenType::Pound;

		else if (Match("{")) type = TokenType::LeftBrace;
		else if (Match("}")) type = TokenType::RightBrace;
		else if (Match("[")) type = TokenType::LeftBracket;
		else if (Match("]")) type = TokenType::RightBracket;
		else if (Match("(")) type = TokenType::LeftParen;
		else if (Match(")")) type = TokenType::RightParen;

		else if (Match(",")) type = TokenType::Comma;
		else if (Match(";")) type = TokenType::Semicolon;
		else if (Match("?")) type = TokenType::Ternary;
		else if (Match(".")) type = TokenType::Dot;
		else if (Match("_")) type = TokenType::Underscore;

		if (type == TokenType::INVALID) {
			pos++;
			throw std::runtime_error("Unexpected character");
		}

		return { type, GetSourceText(startPos), startPos, PrevSourcePos() };
	}

	bool Lexer::Match(const char c) {
		if (pos >= input.size()) {
			return false;
		}

		if (input[pos] == c) {
			pos++;
			return true;
		}

		return false;
	}

	bool Lexer::Match(const std::string_view str) {
		if (pos >= input.size()) {
			return false;
		}

		const auto len = str.length();

		if ((pos + len - 1) >= input.size()) {
			return false;
		}

		for (size_t i = 0; i < len; i++) {
			if (input[pos + i] != str[i]) {
				return false;
			}
		}

		column += len;
		pos += len;
		return true;
	}
}