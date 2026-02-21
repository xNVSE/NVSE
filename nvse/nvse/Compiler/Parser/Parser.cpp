#include "Parser.h"

#include <format>
#include <regex>
#include <utility>
#include <nvse/Compiler/AST/AST.h>
#include "nvse/Hooks_Script.h"
#include "nvse/PluginManager.h"
#include "nvse/Compiler/Utils.h"

using std::make_shared;
using std::vector;
using std::shared_ptr;

namespace Compiler {
	Parser::Parser(const std::string& text) : lexer(Lexer{ text }) {
		this->lexer = lexer;
		Advance();
	}

	std::optional<AST> Parser::Parse() {
		DbgPrintln("[Parser]");
		DbgIndent();
		try {
			Expect(TokenType::Name, "Expected 'name' as first statement of script.");
			auto expr = IdentExpr();
			Expect(TokenType::Semicolon, "Expected ';'.");

			vector<StmtPtr> globals;
			vector<StmtPtr> blocks{};
			auto& pluginVersions = g_currentCompilerPluginVersions.top();
			const auto& name = expr->As<Expressions::IdentExpr>()->str;

			while (currentToken.type != TokenType::Eof) {
				try {
					// Version statements
					// TODO: Extract this to a pre-processor
					if (Match(TokenType::Pound)) {
						if (!Match(TokenType::Identifier)) {
							Error(currentToken, "Expected 'version'.");
						}

						if (_stricmp(previousToken.lexeme.c_str(), "version") != 0) {
							Error(currentToken, "Expected 'version'.");
						}

						Expect(TokenType::LeftParen, "Expected '('.");
						auto plugin = Expect(TokenType::String, "Expected plugin name.");
						Expect(TokenType::Comma, "Expected ','.");
						auto pluginName = std::get<std::string>(plugin.value);
						std::ranges::transform(pluginName.begin(), pluginName.end(), pluginName.begin(), [](const unsigned char c) { return std::tolower(c); });
						if (StrEqual(pluginName.c_str(), "nvse")) {
							auto major = static_cast<int>(std::get<double>(Expect(TokenType::Number, "Expected major version").value));
							int minor = 255;
							int beta = 255;

							if (Match(TokenType::Comma)) {
								minor = static_cast<int>(std::get<double>(Expect(TokenType::Number, "Expected minor version").value));
							}
							if (Match(TokenType::Comma)) {
								beta = static_cast<int>(std::get<double>(Expect(TokenType::Number, "Expected beta version").value));
							}
							pluginVersions[pluginName] = MAKE_NEW_VEGAS_VERSION(major, minor, beta);
						}
						else { // handle versions for plugins
							auto pluginVersion = static_cast<int>(std::get<double>(Expect(TokenType::Number, "Expected plugin version").value));
							auto* pluginInfo = g_pluginManager.GetInfoByName(pluginName.c_str());
							if (!pluginInfo) [[unlikely]] {
								Error(std::format("No plugin with name {} could be found.\n", pluginName));
							}
							pluginVersions[pluginName] = pluginVersion;
						}
						Expect(TokenType::RightParen, "Expected ')'.");
					}
					// Get event or udf block
					else if (PeekBlockType()) {
						blocks.emplace_back(Begin());
					}
					else if (Match(TokenType::Fn)) {
						auto fnToken = previousToken;
						//auto ident = Expect(TokenType::Identifier, "Expected identifier.");
						auto args = ParseArgs();
						auto fnDecl = make_shared<Statements::UDFDecl>(fnToken, std::move(args), BlockStatement());
						fnDecl->is_udf_decl = true;
						fnDecl->sourceInfo = fnToken.sourceSpan + previousToken.sourceSpan;
						blocks.emplace_back(fnDecl);
					}
					else if (Peek(TokenType::IntType) || Peek(TokenType::DoubleType) || Peek(TokenType::RefType) ||
						Peek(TokenType::ArrayType) || Peek(TokenType::StringType))
					{
						globals.emplace_back(VarDecl());
						Expect(TokenType::Semicolon, "Expected ';' at end of statement.");
					}
					else {
						Error(currentToken, "Expected variable declaration, block type (GameMode, MenuMode, ...), or 'fn'.");
					}
				}
				catch (NVSEParseError&) {
					hadError = true;
					Synchronize();
				}
			}

			DbgOutdent();
			auto res = AST(name, std::move(globals), std::move(blocks));
			res.lines = lexer.lines;
			return res;
		}
		catch (NVSEParseError&) {
			hadError = true;
			DbgOutdent();
			return std::optional<AST>{};
		}
	}

	StmtPtr Parser::Begin() {
		auto startToken = previousToken;

		ExpectBlockType("Expected block type");

		auto blockName = previousToken;
		auto blockNameStr = blockName.lexeme;

		std::optional<Token> mode{};
		CommandInfo* beginInfo = nullptr;
		for (auto& info : g_eventBlockCommandInfos) {
			if (!_stricmp(info.longName, blockNameStr.c_str())) {
				beginInfo = &info;
				break;
			}
		}

		if (Match(TokenType::MakePair)) {
			if (beginInfo->numParams == 0) {
				Error(currentToken, "Cannot specify argument for this block type.");
			}

			if (Match(TokenType::Number)) {
				if (beginInfo->params[0].typeID != kParamType_Integer) {
					Error(currentToken, "Expected identifier.");
				}

				auto modeToken = previousToken;
				if (modeToken.lexeme.contains('.')) {
					Error(currentToken, "Expected integer.");
				}

				mode = modeToken;
			}
			else if (Match(TokenType::Identifier)) {
				if (beginInfo->params[0].typeID == kParamType_Integer) {
					Error(currentToken, "Expected number.");
				}

				mode = previousToken;
			}
			else {
				Error(currentToken, "Expected identifier or number.");
			}
		}
		else {
			if (beginInfo->numParams > 0 && !beginInfo->params[0].isOptional) {
				Error(currentToken, "Missing required parameter for block type '" + blockName.lexeme + "'");
			}
		}

		if (!Peek(TokenType::LeftBrace)) {
			Error(currentToken, "Expected '{'.");
		}

		auto block = BlockStatement();

		return make_shared<Statements::Begin>(
			blockName.lexeme, 
			mode, 
			std::move(block),
			beginInfo,
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	StmtPtr Parser::Statement() {
		if (
			Peek(TokenType::Identifier) &&
			!_stricmp(currentToken.lexeme.c_str(), "ShowMessage")
		) {
			const auto showMessageIdent = make_shared<Expressions::IdentExpr>("ShowMessage", currentToken.sourceSpan);

			Advance();
			Expect(TokenType::LeftParen, "Expected '('");
			const auto ident = Expect(TokenType::Identifier, "Expected message form");

			const auto messageIdent = make_shared<Expressions::IdentExpr>(
				ident.lexeme,
				previousToken.sourceSpan
			);

			vector<shared_ptr<Expressions::IdentExpr>> messageArgs{};

			uint32_t messageTime = 0;
			while (Match(TokenType::Comma)) {
				if (Peek(TokenType::Number)) {
					const auto numericLiteral = NumericLiteral();
					if (numericLiteral->is_fp) {
						Error(previousToken, "Duration must be an integer.");
					}

					messageTime = static_cast<uint32_t>(numericLiteral->value);
					break;
				}

				const auto identToken = Expect(TokenType::Identifier, "Expected identifier.");
				messageArgs.emplace_back(
					make_shared<Expressions::IdentExpr>(
						std::get<2>(identToken.value), 
						identToken.sourceSpan
					)
				);
			}

			Expect(TokenType::RightParen, "Expected ')'");
			Expect(TokenType::Semicolon, "Expected ';'");

			return make_shared<Statements::ShowMessage>(messageIdent, messageArgs, messageTime);
		}

		if (Peek(TokenType::For)) {
			return ForStatement();
		}

		if (Peek(TokenType::If)) {
			return IfStatement();
		}

		if (Peek(TokenType::Match)) {
			return MatchStatement();
		}

		if (Peek(TokenType::Return)) {
			return ReturnStatement();
		}

		if (Match(TokenType::Break)) {
			auto token = previousToken;
			Expect(TokenType::Semicolon);

			return make_shared<Statements::Break>(
				token.sourceSpan + previousToken.sourceSpan
			);
		}

		if (Match(TokenType::Continue)) {
			auto token = previousToken;
			Expect(TokenType::Semicolon);

			return make_shared<Statements::Continue>(
				token.sourceSpan + previousToken.sourceSpan
			);
		}

		if (Peek(TokenType::While)) {
			return WhileStatement();
		}

		if (Peek(TokenType::LeftBrace)) {
			return BlockStatement();
		}

		if (Peek(TokenType::Fn)) {
			auto startToken = Expect(TokenType::Fn);

			auto ident = IdentExpr();
			auto args = ParseArgs();
			auto block = BlockStatement();

			Expect(TokenType::Semicolon);

			auto fnDecl = make_shared<Expressions::LambdaExpr>(
				std::move(args), 
				block, 
				startToken.sourceSpan + previousToken.sourceSpan
			);

			return make_shared<Statements::VarDecl>(
				TokenType::RefType,
				ident,
				fnDecl,
				startToken.sourceSpan + previousToken.sourceSpan
			);
		}

		if (PeekType()) {
			auto expr = VarDecl();
			Expect(TokenType::Semicolon);
			return expr;
		}

		return ExpressionStatement();
	}

	shared_ptr<Statements::VarDecl> Parser::VarDecl(const bool allowValue, const bool allowOnlyOneVarDecl) {
		if (!MatchesType()) {
			Error(currentToken, "Expected type.");
		}

		auto varType = previousToken;

		vector<Statements::VarDecl::Declaration> declarations{};
		do {
			auto name = IdentExpr();
			ExprPtr value{ nullptr };

			if (Match(TokenType::Eq)) {
				if (!allowValue) {
					Error(previousToken, "Variable definition is not allowed here.");
				}
				value = Expression();
			}

			declarations.emplace_back(name, value);
			if (allowOnlyOneVarDecl) {
				break;
			}
		} while (Match(TokenType::Comma));

		return make_shared<Statements::VarDecl>(
			varType.type, 
			std::move(declarations), 
			varType.sourceSpan + previousToken.sourceSpan
		);
	}

	StmtPtr Parser::ForStatement() {
		const auto startToken = Expect(TokenType::For);
		const auto lParen = Expect(TokenType::LeftParen);

		StmtPtr init = { nullptr };
		if (!Peek(TokenType::Semicolon)) {

			// for (array aIter in [1::1, 2::2])..
			if (MatchesType()) {
				auto typeToken = previousToken;
				auto ident = IdentExpr();

				if (Match(TokenType::In)) {
					auto decl = make_shared<Statements::VarDecl>(typeToken.type, ident, nullptr, typeToken.sourceSpan + previousToken.sourceSpan);

					ExprPtr rhs = Expression();
					Expect(TokenType::RightParen, "Expected ')'.");

					shared_ptr<Statements::Block> block = BlockStatement();
					return make_shared<Statements::ForEach>(vector{ decl }, std::move(rhs), std::move(block), false);
				}

				ExprPtr value{ nullptr };
				if (Match(TokenType::Eq)) {
					value = Expression();
				}
				Expect(TokenType::Semicolon, "Expected ';' after loop initializer.");

				init = make_shared<Statements::VarDecl>(typeToken.type, ident, value, typeToken.sourceSpan + previousToken.sourceSpan);
			}

			// for ([int key, int value] in [1::1, 2::2]).
			// for ([_, int value] in [1::1, 2::2]).
			// for ([int key, _] in [1::1, 2::2]).
			// for ([int key] in [1::1, 2::2]).
			// for ([int value] in [1, 2]).
			else if (Match(TokenType::LeftBracket)) {
				vector<shared_ptr<Statements::VarDecl>> declarations{};

				// LHS
				if (Match(TokenType::Underscore)) {
					declarations.push_back(nullptr);
				}
				else {
					declarations.push_back(VarDecl(false, true));
				}

				// RHS
				if (Match(TokenType::Comma)) {
					if (Match(TokenType::Underscore)) {
						declarations.push_back(nullptr);
					}
					else {
						declarations.push_back(VarDecl(false, true));
					}
				}

				Expect(TokenType::RightBracket, "Expected ']'.");
				Expect(TokenType::In, "Expected 'in'.");

				ExprPtr rhs = Expression();
				Expect(TokenType::RightParen, "Expected ')'.");

				shared_ptr<Statements::Block> block = BlockStatement();
				return make_shared<Statements::ForEach>(declarations, std::move(rhs), std::move(block), true);
			}
			else {
				init = Statement();
				if (!init->IsType<Expressions::AssignmentExpr>()) {
					Error(currentToken, "Expected variable declaration or assignment.");
				}
			}
		}

		// Default to true condition
		ExprPtr cond = make_shared<Expressions::BoolExpr>(true, lParen.sourceSpan + previousToken.sourceSpan);
		if (!Peek(TokenType::Semicolon)) {
			cond = Expression();
			Expect(TokenType::Semicolon);
		}

		ExprPtr incrementExpr = { nullptr };
		if (!Peek(TokenType::RightParen)) {
			incrementExpr = Expression();
		}
		Expect(TokenType::RightParen, "Expected ')'.");

		shared_ptr<Statements::Block> block = BlockStatement();

		return make_shared<Statements::For>(
			std::move(init),
			std::move(cond),
			std::move(incrementExpr),
			std::move(block),
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	void Parser::AdvanceToClosingCharOf(const TokenType leftType) {
		TokenType rightType;

		if (leftType == TokenType::LeftParen) {
			rightType = TokenType::RightParen;
		}

		else if (leftType == TokenType::LeftBrace) {
			rightType = TokenType::RightBrace;
		}

		else if (leftType == TokenType::LeftBracket) {
			rightType = TokenType::RightBracket;
		}

		else {
			return;
		}

		auto depth = 0;
		while (true) {
			Advance();
			const auto curType = currentToken.type;

			if (curType == TokenType::Eof) {
				break;
			}

			if (curType == leftType) {
				depth++;
			} else if (curType == rightType) {
				if (depth == 0) {
					break;
				}
					
				depth--;
			}
		}
	}

	ExprPtr Parser::ParenthesizedExpression() {
		const auto startToken = Expect(TokenType::LeftParen);

		ExprPtr cond = nullptr;
		try {
			cond = Expression();
		} 

		// Try to recover from mis-parsed if condition
		catch (NVSEParseError &_) {
			hadError = true;
			AdvanceToClosingCharOf(TokenType::LeftParen);
		}

		Expect(TokenType::RightParen);

		return cond;
	}

	StmtPtr Parser::IfStatement() {
		const auto startToken = Expect(TokenType::If);

		auto condition = ParenthesizedExpression();
		auto block = BlockStatement();

		shared_ptr<Statements::Block> elseBlock = nullptr;
		if (Match(TokenType::Else)) {
			if (Peek(TokenType::If)) {
				vector<StmtPtr> statements{};
				auto ifStmt = IfStatement();
				auto span = ifStmt->sourceInfo;
				statements.emplace_back(std::move(ifStmt));
				elseBlock = make_shared<Statements::Block>(std::move(statements), span);
			}
			else {
				elseBlock = BlockStatement();
			}
		}

		return make_shared<Statements::If>(
			std::move(condition), 
			std::move(block), 
			std::move(elseBlock), 
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	shared_ptr<Statements::Match> Parser::MatchStatement() {
		const auto startToken = Expect(TokenType::Match);

		auto cond = ParenthesizedExpression();

		Expect(TokenType::LeftBrace);

		vector<Statements::Match::MatchArm> arms{};
		shared_ptr<Statements::Block> defaultCase{};

		do {
			if (Match(TokenType::Underscore)) {
				if (defaultCase) {
					Error(currentToken, "Only one default case can be specified.");
				}

				Expect(TokenType::Arrow);
				defaultCase = BlockStatement();
				continue;
			}

			Statements::Match::MatchArm arm {};

			if (Match(TokenType::Identifier)) {
				const auto ident = std::make_shared<Expressions::IdentExpr>(previousToken.lexeme, previousToken.sourceSpan);

				if (Match(TokenType::Slice)) {
					arm.binding = ident;
					arm.expr = Expression();
				} else {
					arm.expr = ident;
				}
			} else {
				arm.expr = Expression();
			}

			Expect(TokenType::Arrow);
			arm.block = BlockStatement();
			arms.emplace_back(std::move(arm));
		} while (!Peek(TokenType::RightBrace));

		Expect(TokenType::RightBrace);

		return make_shared<Statements::Match>(
			cond, 
			std::move(arms), 
			defaultCase, 
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	StmtPtr Parser::ReturnStatement() {
		const auto returnToken = Expect(TokenType::Return);

		if (Match(TokenType::Semicolon)) {
			return make_shared<Statements::Return>(returnToken.sourceSpan);
		}

		const auto expr = Expression();

		auto returnExpr = make_shared<Statements::Return>(
			expr, 
			returnToken.sourceSpan + expr->sourceInfo
		);

		Expect(TokenType::Semicolon);

		return returnExpr;
	}

	StmtPtr Parser::WhileStatement() {
		const auto startToken = Expect(TokenType::While);
		auto cond = ParenthesizedExpression();
		auto block = BlockStatement();
		
		return make_shared<Statements::While>(
			std::move(cond), 
			std::move(block),
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	shared_ptr<Statements::Block> Parser::BlockStatement() {
		const auto startToken = Expect(TokenType::LeftBrace);

		vector<StmtPtr> statements{};
		while (!Match(TokenType::RightBrace)) {
			try {
				// Skip empty expression statements
				if (Match(TokenType::Semicolon)) {
					continue;
				}

				statements.emplace_back(Statement());
			}
			catch (NVSEParseError&) {
				hadError = true;
				Synchronize();

				if (currentToken.type == TokenType::Eof) {
					break;
				}
			}
		}

		return make_shared<Statements::Block>(
			std::move(statements),
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	StmtPtr Parser::ExpressionStatement() {
		const auto startSpan = currentToken.sourceSpan;

		// Allow empty expression statements
		if (Match(TokenType::Semicolon)) {
			return make_shared<Statements::ExpressionStatement>(nullptr, previousToken.sourceSpan);
		}

		auto expr = Expression();
		Expect(TokenType::Semicolon);

		return make_shared<Statements::ExpressionStatement>(
			std::move(expr), 
			startSpan + previousToken.sourceSpan
		);
	}

	ExprPtr Parser::Expression() {
		return Assignment();
	}

	ExprPtr Parser::Assignment() {
		ExprPtr left = Slice();

		if (
			Match(TokenType::Eq) || 
			Match(TokenType::PlusEq) || 
			Match(TokenType::MinusEq) ||
			Match(TokenType::StarEq) || 
			Match(TokenType::SlashEq) || 
			Match(TokenType::ModEq) || 
			Match(TokenType::PowEq) || 
			Match(TokenType::BitwiseOrEquals) || 
			Match(TokenType::BitwiseAndEquals)
		) {
			auto token = previousToken;

			const auto prevTok = previousToken;
			ExprPtr value = Assignment();

			// TODO - Move this to a semantic analysis pass?
			if (
				left->IsType<Expressions::IdentExpr>() || 
				left->IsType<Expressions::GetExpr>() || 
				left->IsType<Expressions::SubscriptExpr>()
			) {
				return make_shared<Expressions::AssignmentExpr>(token, std::move(left), std::move(value));
			}

			Error(prevTok, "Invalid assignment target.");
		}

		return left;
	}

	ExprPtr Parser::Slice() {
		ExprPtr left = Ternary();

		while (Match(TokenType::Slice)) {
			const auto op = previousToken;
			ExprPtr right = Ternary();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Ternary() {
		ExprPtr cond = LogicalOr();

		while (Match(TokenType::Ternary)) {
			auto token = previousToken;

			ExprPtr left;
			if (Match(TokenType::Slice)) {
				left = cond;
			}
			else {
				left = LogicalOr();
				Expect(TokenType::Slice, "Expected ':'.");
			}

			auto right = LogicalOr();
			cond = make_shared<Expressions::TernaryExpr>(token, std::move(cond), std::move(left), std::move(right));
		}

		return cond;
	}

	ExprPtr Parser::LogicalOr() {
		ExprPtr left = LogicalAnd();

		while (Match(TokenType::LogicOr)) {
			auto op = previousToken;
			ExprPtr right = LogicalAnd();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::LogicalAnd() {
		ExprPtr left = Equality();

		while (Match(TokenType::LogicAnd)) {
			const auto op = previousToken;
			ExprPtr right = Equality();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Equality() {
		ExprPtr left = Comparison();

		while (Match(TokenType::EqEq) || Match(TokenType::BangEq)) {
			const auto op = previousToken;
			ExprPtr right = Comparison();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Comparison() {
		ExprPtr left = In();

		while (
			Match(TokenType::Less) || 
			Match(TokenType::LessEq) || 
			Match(TokenType::Greater) || 
			Match(TokenType::GreaterEq)
		) {
			const auto op = previousToken;
			ExprPtr right = In();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::In() {
		ExprPtr left = BitwiseOr();

		bool isNot = Match(TokenType::Not);
		if (Match(TokenType::In)) {
			const auto op = previousToken;

			if (Match(TokenType::LeftBracket)) {
				vector<ExprPtr> values{};
				while (!Match(TokenType::RightBracket)) {
					if (!values.empty()) {
						Expect(TokenType::Comma, "Expected ','.");
					}
					values.emplace_back(Expression());
				}

				return make_shared<Expressions::InExpr>(left, op, values, isNot);
			}

			auto expr = Expression();
			return make_shared<Expressions::InExpr>(left, op, expr, isNot);
		}

		if (isNot) {
			Error(currentToken, "Expected 'in'.");
		}

		return left;
	}

	ExprPtr Parser::BitwiseOr() {
		ExprPtr left = BitwiseAnd();

		while (Match(TokenType::BitwiseOr)) {
			const auto op = previousToken;
			ExprPtr right = BitwiseAnd();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::BitwiseAnd() {
		ExprPtr left = Shift();

		while (Match(TokenType::BitwiseAnd)) {
			const auto op = previousToken;
			ExprPtr right = Shift();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Shift() {
		ExprPtr left = Term();

		while (Match(TokenType::LeftShift) || Match(TokenType::RightShift)) {
			const auto op = previousToken;
			ExprPtr right = Term();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	// term -> factor ((+ | -) factor)?;
	ExprPtr Parser::Term() {
		ExprPtr left = Factor();

		while (Match(TokenType::Plus) || Match(TokenType::Minus)) {
			const auto op = previousToken;
			ExprPtr right = Factor();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Factor() {
		ExprPtr left = Pair();

		while (Match(TokenType::Star) || Match(TokenType::Slash) || Match(TokenType::Mod) || Match(
			TokenType::Pow)) {
			const auto op = previousToken;
			ExprPtr right = Pair();
			left = make_shared<Expressions::BinaryExpr>(
				op,
				std::move(left),
				std::move(right),
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Pair() {
		ExprPtr left = Unary();

		while (Match(TokenType::MakePair)) {
			auto op = previousToken;
			ExprPtr right = Unary();
			left = make_shared<Expressions::BinaryExpr>(
				op, 
				std::move(left), 
				std::move(right), 
				left->sourceInfo + right->sourceInfo
			);
		}

		return left;
	}

	ExprPtr Parser::Unary() {
		if (
			Match(TokenType::Bang)			|| 
			Match(TokenType::Minus)			|| 
			Match(TokenType::Dollar)		|| 
			Match(TokenType::Pound)			|| 
			Match(TokenType::BitwiseAnd)	|| 
			Match(TokenType::Star)			|| 
			Match(TokenType::BitwiseNot)
		) {
			auto opToken = previousToken;

			ExprPtr right = Unary();

			// Convert these two in case of box/unbox
			if (opToken.type == TokenType::BitwiseAnd) {
				opToken.type = TokenType::Box;
			}

			if (opToken.type == TokenType::Star) {
				opToken.type = TokenType::Unbox;
			}

			if (opToken.type == TokenType::Minus) {
				opToken.type = TokenType::Negate;
			}

			return make_shared<Expressions::UnaryExpr>(opToken, std::move(right), false);
		}

		return Postfix();
	}

	ExprPtr Parser::Postfix() {
		ExprPtr expr = Call();

		while (Match(TokenType::LeftBracket)) {
			auto token = previousToken;
			auto inner = Expression();
			auto closingBracket = Expect(TokenType::RightBracket);

			expr = make_shared<Expressions::SubscriptExpr>(
				token, 
				std::move(expr), 
				std::move(inner), 
				token.sourceSpan + closingBracket.sourceSpan
			);
		}

		if (Match(TokenType::PlusPlus) || Match(TokenType::MinusMinus)) {
			if (!expr->IsType<Expressions::IdentExpr>() && !expr->IsType<Expressions::GetExpr>()) {
				Error(previousToken, "Invalid operand for '++', expected identifier.");
			}

			auto op = previousToken;
			return make_shared<Expressions::UnaryExpr>(op, std::move(expr), true);
		}

		return expr;
	}

	ExprPtr Parser::Call() {
		auto startToken = currentToken;

		ExprPtr expr = Primary();

		while (Match(TokenType::Dot) || Match(TokenType::LeftParen)) {
			if (previousToken.type == TokenType::Dot) {
				// TODO: Move this to a semantic analysis pass?
				if (
					!expr->IsType<Expressions::GroupingExpr>()	&&
					!expr->IsType<Expressions::IdentExpr>()		&&
					!expr->IsType<Expressions::CallExpr>()		&&
					!expr->IsType<Expressions::GetExpr>()
				) {
					Error(currentToken, "Invalid member access.");
				}

				auto token = previousToken;
				const auto ident = IdentExpr();

				if (Match(TokenType::LeftParen)) {
					vector<ExprPtr> args{};
					while (!Match(TokenType::RightParen)) {
						try {
							args.emplace_back(Expression());

							if (!Peek(TokenType::RightParen) && !Match(TokenType::Comma)) {
								Error(currentToken, "Expected ',' or ')'.");
							}
						}

						catch (NVSEParseError &_) {
							AdvanceToClosingCharOf(TokenType::LeftParen);
						}
					}

					expr = make_shared<Expressions::CallExpr>(
						std::move(expr),
						ident, 
						std::move(args), 
						startToken.sourceSpan + previousToken.sourceSpan
					);
				}
				else {
					const auto sourceInfo = expr->sourceInfo;
					expr = make_shared<Expressions::GetExpr>(
						std::move(expr),
						ident,
						sourceInfo
					);
				}
			}
			else {
				// Can only call on ident or get expr
				if (!expr->IsType<Expressions::IdentExpr>()) {
					Error(currentToken, "Invalid callee.");
				}

				auto ident = std::dynamic_pointer_cast<Expressions::IdentExpr>(expr);

				vector<ExprPtr> args{};
				while (!Match(TokenType::RightParen)) {
					try {
						args.emplace_back(Expression());

						if (!Peek(TokenType::RightParen) && !Match(TokenType::Comma)) {
							Error(currentToken, "Expected ',' or ')'.");
						}
					}

					catch (NVSEParseError& _) {
						AdvanceToClosingCharOf(TokenType::LeftParen);
					}
				}
				expr = make_shared<Expressions::CallExpr>(
					ident, 
					std::move(args),
					startToken.sourceSpan + previousToken.sourceSpan
				);
			}
		}

		return expr;
	}

	ExprPtr Parser::Primary() {
		if (Match(TokenType::Bool)) {
			return make_shared<Expressions::BoolExpr>(
				std::get<double>(previousToken.value),
				previousToken.sourceSpan
			);
		}

		if (Peek(TokenType::Number)) {
			return NumericLiteral();
		}

		if (Match(TokenType::String)) {
			ExprPtr expr = make_shared<Expressions::StringExpr>(std::get<2>(previousToken.value), previousToken.sourceSpan);

			while (Match(TokenType::Interp)) {
				auto inner = make_shared<Expressions::UnaryExpr>(
					Token{ TokenType::Dollar, "$" }, 
					Expression(), 
					false
				);

				Expect(TokenType::EndInterp, "Expected '}'");

				expr = make_shared<Expressions::BinaryExpr>(
					Token{ TokenType::Plus}, 
					expr,
					inner,
					inner->sourceInfo
				);

				if (Match(TokenType::String) && previousToken.lexeme.length() > 2) {
					auto endStr = make_shared<Expressions::StringExpr>(
						std::get<2>(previousToken.value),
						previousToken.sourceSpan
					);

					expr = make_shared<Expressions::BinaryExpr>(
						Token{ TokenType::Plus, "+" }, 
						expr, 
						endStr,
						endStr->sourceInfo
					);
				}
			}
			return expr;
		}

		if (Peek(TokenType::Identifier)) {
			return IdentExpr();
		}

		if (Peek(TokenType::LeftParen)) {
			const auto startToken = currentToken;
			auto expr = ParenthesizedExpression();
			return make_shared<Expressions::GroupingExpr>(expr, startToken.sourceSpan + previousToken.sourceSpan);
		}

		if (Peek(TokenType::LeftBracket)) {
			return ArrayLiteral();
		}

		if (Peek(TokenType::LeftBrace)) {
			return MapLiteral();
		}

		if (Peek(TokenType::Fn)) {
			return FnExpr();
		}

		Error(currentToken, "Expected expression.");
		return ExprPtr{ nullptr };
	}

	shared_ptr<Expressions::IdentExpr> Parser::IdentExpr() {
		const auto identToken = Expect(TokenType::Identifier);
		return make_shared<Expressions::IdentExpr>(identToken.lexeme, identToken.sourceSpan);
	}

	shared_ptr<Expressions::NumberExpr> Parser::NumericLiteral() {
		Match(TokenType::Number);

		// Double literal
		if (std::ranges::find(previousToken.lexeme, '.') != previousToken.lexeme.end()) {
			return make_shared<Expressions::NumberExpr>(
				std::get<double>(previousToken.value), 
				true,
				0,
				previousToken.sourceSpan
			);
		}

		// Int literal
		return make_shared<Expressions::NumberExpr>(
			floor(std::get<double>(previousToken.value)), 
			false,
			0,
			previousToken.sourceSpan
		);
	}

	// Only called when 'fn' token is matched
	ExprPtr Parser::FnExpr() {
		const auto token = Expect(TokenType::Fn);

		auto args = ParseArgs();

		if (Match(TokenType::Arrow)) {
			// Build call stmt
			const auto expr = Expression();

			auto callExpr = make_shared<Expressions::CallExpr>(
				make_shared<Expressions::IdentExpr>("SetFunctionValue", expr->sourceInfo),
				vector{ expr }, 
				previousToken.sourceSpan + expr->sourceInfo
			);

			StmtPtr exprStmt = make_shared<Statements::ExpressionStatement>(callExpr, callExpr->sourceInfo);
			auto block = make_shared<Statements::Block>(vector{ exprStmt }, callExpr->sourceInfo);

			return make_shared<Expressions::LambdaExpr>(
				std::move(args),
				std::move(block),
				token.sourceSpan + previousToken.sourceSpan
			);
		}

		auto lambdaBlock = BlockStatement();
		return make_shared<Expressions::LambdaExpr>(
			std::move(args),
			std::move(lambdaBlock),
			token.sourceSpan + previousToken.sourceSpan
		);
	}

	ExprPtr Parser::ArrayLiteral() {
		const auto startToken = Expect(TokenType::LeftBracket);

		vector<ExprPtr> values{};
		while (!Match(TokenType::RightBracket)) {
			try {
				if (!values.empty()) {
					Expect(TokenType::Comma, "Expected ','.");
				}
				values.emplace_back(Expression());
			} catch (NVSEParseError &_) {
				hadError = true;
				AdvanceToClosingCharOf(TokenType::LeftBracket);
				break;
			}
		}

		return make_shared<Expressions::ArrayLiteralExpr>(
			values,
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	ExprPtr Parser::MapLiteral() {
		const auto startToken = Expect(TokenType::LeftBrace);

		vector<ExprPtr> values{};
		while (!Match(TokenType::RightBrace)) {
			try {
				if (!values.empty()) {
					Expect(TokenType::Comma, "Expected ','.");
				}
				values.emplace_back(Expression());
			}
			catch (NVSEParseError& _) {
				hadError = true;
				AdvanceToClosingCharOf(TokenType::LeftBrace);
				break;
			}
		}

		return make_shared<Expressions::MapLiteralExpr>(
			std::move(values), 
			startToken.sourceSpan + previousToken.sourceSpan
		);
	}

	vector<shared_ptr<Statements::VarDecl>> Parser::ParseArgs() {
		Expect(TokenType::LeftParen);

		vector<shared_ptr<Statements::VarDecl>> args{};
		while (!Match(TokenType::RightParen)) {
			if (!Match(TokenType::IntType) && 
				!Match(TokenType::DoubleType) && 
				!Match(TokenType::RefType) &&
				!Match(TokenType::ArrayType) && 
				!Match(TokenType::StringType)
			) {
				Error(currentToken, "Expected type.");
			}

			auto type = previousToken;
			auto ident = IdentExpr();
			auto decl = make_shared<Statements::VarDecl>(type.type, ident, nullptr, type.sourceSpan + previousToken.sourceSpan);

			if (!Peek(TokenType::RightParen) && !Match(TokenType::Comma)) {
				Error(currentToken, "Expected ',' or ')'.");
			}

			args.emplace_back(std::move(decl));
		}

		return args;
	}

	void Parser::Advance() {
		previousToken = std::move(currentToken);

		// Bubble up lexer errors
		try {
			currentToken = lexer.GetNextToken(true);

			const auto msg = std::format("[New Token] {}", TokenTypeStr[static_cast<int>(currentToken.type)]);
			const auto highlighted = HighlightSourceSpan(lexer.lines, msg, currentToken.sourceSpan, ESCAPE_CYAN);
			DbgPrint(highlighted.c_str());
		}
		catch (std::runtime_error& er) {
			ErrPrintln("[line %d] %s", lexer.line, er.what());
		}
	}

	bool Parser::Match(const TokenType type) {
		if (currentToken.type == type) {
			Advance();
			return true;
		}

		return false;
	}

	bool Parser::MatchesType() {
		return 
			Match(TokenType::IntType)    || 
			Match(TokenType::DoubleType) || 
			Match(TokenType::RefType)    ||
			Match(TokenType::ArrayType)  || 
			Match(TokenType::StringType);
	}

	bool Parser::Peek(const TokenType type) const {
		if (currentToken.type == type) {
			return true;
		}

		return false;
	}

	bool Parser::PeekType() const {
		return 
			Peek(TokenType::IntType)    || 
			Peek(TokenType::DoubleType) || 
			Peek(TokenType::RefType)    ||
			Peek(TokenType::ArrayType)  || 
			Peek(TokenType::StringType);
	}

	bool Parser::PeekBlockType() const {
		if (!Peek(TokenType::Identifier)) {
			return false;
		}

		const auto identifier = currentToken.lexeme;
		return std::ranges::any_of(g_eventBlockCommandInfos, [&](const CommandInfo& ci) {
			return !_stricmp(ci.longName, identifier.c_str());
		});
	}

	Token Parser::ExpectBlockType(const std::string&& message) {
		if (!PeekBlockType()) {
			Error(currentToken, message);
			return previousToken;
		}

		const auto identifier = currentToken.lexeme;
		for (const auto& g_eventBlockCommandInfo : g_eventBlockCommandInfos) {
			if (!_stricmp(g_eventBlockCommandInfo.longName, identifier.c_str())) {
				Advance();
				return previousToken;
			}
		}

		Error(currentToken, message);
		return previousToken;
	}

	void Parser::Error(const std::string &message) {
		Error(currentToken, message);
	}

	void Parser::Error(Token token, std::string message) {
		panicMode = true;
		hadError = true;

		const auto msg = std::format("[line {}:{}] {}", token.sourceSpan.start.line, token.sourceSpan.start.column, message);
		ErrPrintln("%s", msg.c_str());
		throw NVSEParseError(msg);
	}

	Token Parser::Expect(const TokenType type, std::string message) {
		if (!Match(type)) {
			Error(currentToken, std::move(message));
		}

		return previousToken;
	}

	Token Parser::Expect(TokenType type) {
		if (!Match(type)) {
			Error(currentToken, std::format("Expected '{}'", TokenTypeStr[static_cast<uint32_t>(type)]));
		}

		return previousToken;
	}

	void Parser::Synchronize() {
		Advance();

		while (currentToken.type != TokenType::Eof) {
			if (previousToken.type == TokenType::Semicolon) {
				panicMode = false;
				return;
			}

			switch (currentToken.type) {
				case TokenType::If:
				case TokenType::While:
				case TokenType::For:
				case TokenType::Return:
				case TokenType::RightBrace:
					panicMode = false;
					return;
				default: break;
			}

			Advance();
		}
	}
}
