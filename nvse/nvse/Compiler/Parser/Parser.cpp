#include "Parser.h"

#include <format>
#include <regex>
#include <utility>
#include <nvse/Compiler/AST/AST.h>
#include "nvse/Hooks_Script.h"
#include "nvse/PluginManager.h"
#include "nvse/Compiler/NVSECompilerUtils.h"

namespace Compiler {
	Parser::Parser(Lexer& tokenizer) : lexer(tokenizer) {
		Advance();
	}

	std::optional<AST> Parser::Parse() {

		CompDbg("==== PARSER ====\n\n");

		try {
			Expect(NVSETokenType::Name, "Expected 'name' as first statement of script.");
			auto expr = Primary();
			if (!dynamic_cast<Expressions::IdentExpr*>(expr.get())) {
				Error(previousToken, "Expected identifier.");
			}
			Expect(NVSETokenType::Semicolon, "Expected ';'.");

			std::vector<StmtPtr> globals;
			std::vector<StmtPtr> blocks{};
			auto& pluginVersions = g_currentCompilerPluginVersions.top();
			const auto &name = dynamic_cast<Expressions::IdentExpr*>(expr.get())->token;

			while (currentToken.type != NVSETokenType::Eof) {
				try {
					// Version statements
					// TODO: Extract this to a pre-processor
					if (Match(NVSETokenType::Pound)) {
						if (!Match(NVSETokenType::Identifier)) {
							Error(currentToken, "Expected 'version'.");
						}

						if (_stricmp(previousToken.lexeme.c_str(), "version")) {
							Error(currentToken, "Expected 'version'.");
						}

						Expect(NVSETokenType::LeftParen, "Expected '('.");
						auto plugin = Expect(NVSETokenType::String, "Expected plugin name.");
						Expect(NVSETokenType::Comma, "Expected ','.");
						auto pluginName = std::get<std::string>(plugin.value);
						std::ranges::transform(pluginName.begin(), pluginName.end(), pluginName.begin(), [](unsigned char c) { return std::tolower(c); });
						if (StrEqual(pluginName.c_str(), "nvse")) {
							auto major = static_cast<int>(std::get<double>(Expect(NVSETokenType::Number, "Expected major version").value));
							int minor = 255;
							int beta = 255;

							if (Match(NVSETokenType::Comma)) {
								minor = static_cast<int>(std::get<double>(Expect(NVSETokenType::Number, "Expected minor version").value));
							}
							if (Match(NVSETokenType::Comma)) {
								beta = static_cast<int>(std::get<double>(Expect(NVSETokenType::Number, "Expected beta version").value));
							}
							pluginVersions[pluginName] = MAKE_NEW_VEGAS_VERSION(major, minor, beta);
						}
						else { // handle versions for plugins
							auto pluginVersion = static_cast<int>(std::get<double>(Expect(NVSETokenType::Number, "Expected plugin version").value));
							auto* pluginInfo = g_pluginManager.GetInfoByName(pluginName.c_str());
							if (!pluginInfo) [[unlikely]] {
								Error(std::format("No plugin with name {} could be found.\n", pluginName));
							}
							pluginVersions[pluginName] = pluginVersion;
						}
						Expect(NVSETokenType::RightParen, "Expected ')'.");
					}
					// Get event or udf block
					else if (PeekBlockType()) {
						blocks.emplace_back(Begin());
					}
					else if (Match(NVSETokenType::Fn)) {
						auto fnToken = previousToken;
						//auto ident = Expect(NVSETokenType::Identifier, "Expected identifier.");
						auto args = ParseArgs();
						auto fnDecl = std::make_shared<Statements::UDFDecl>(fnToken, std::move(args), BlockStatement());
						fnDecl->is_udf_decl = true;
						blocks.emplace_back(fnDecl);
					}
					else if (Peek(NVSETokenType::IntType) || Peek(NVSETokenType::DoubleType) || Peek(NVSETokenType::RefType) ||
						Peek(NVSETokenType::ArrayType) || Peek(NVSETokenType::StringType))
					{
						globals.emplace_back(VarDecl());
						Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
					}
					else {
						Error(currentToken, "Expected variable declaration, block type (GameMode, MenuMode, ...), or 'fn'.");
					}
				}
				catch (NVSEParseError& e) {
					hadError = true;
					CompErr("%s\n", e.what());
					Synchronize();
				}
			}

			return AST(name, std::move(globals), std::move(blocks));
		}
		catch (NVSEParseError& e) {
			hadError = true;
			CompErr("%s\n", e.what());
			return std::optional<AST>{};
		}
	}

	StmtPtr Parser::Begin() {
		ExpectBlockType("Expected block type");

		auto blockName = previousToken;
		auto blockNameStr = blockName.lexeme;

		std::optional<NVSEToken> mode{};
		CommandInfo* beginInfo = nullptr;
		for (auto& info : g_eventBlockCommandInfos) {
			if (!_stricmp(info.longName, blockNameStr.c_str())) {
				beginInfo = &info;
				break;
			}
		}

		if (Match(NVSETokenType::MakePair)) {
			if (beginInfo->numParams == 0) {
				Error(currentToken, "Cannot specify argument for this block type.");
			}

			if (Match(NVSETokenType::Number)) {
				if (beginInfo->params[0].typeID != kParamType_Integer) {
					Error(currentToken, "Expected identifier.");
				}

				auto modeToken = previousToken;
				if (modeToken.lexeme.contains('.')) {
					Error(currentToken, "Expected integer.");
				}

				mode = modeToken;
			}
			else if (Match(NVSETokenType::Identifier)) {
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

		if (!Peek(NVSETokenType::LeftBrace)) {
			Error(currentToken, "Expected '{'.");
		}
		return std::make_shared<Statements::Begin>(blockName, mode, BlockStatement(), beginInfo);
	}

	StmtPtr Parser::Statement() {
		if (
			Peek(NVSETokenType::Identifier) &&
			!_stricmp(currentToken.lexeme.c_str(), "ShowMessage")
			) {
			const auto ident = std::make_shared<Expressions::IdentExpr>(currentToken);

			Advance();
			Expect(NVSETokenType::LeftParen, "Expected '('");

			Expect(NVSETokenType::Identifier, "Expected message form");
			const auto messageIdent = std::make_shared<Expressions::IdentExpr>(previousToken);

			std::vector<std::shared_ptr<Expressions::IdentExpr>> messageArgs{};

			uint32_t messageTime = 0;
			while (Match(NVSETokenType::Comma)) {
				if (Peek(NVSETokenType::Number)) {
					const auto numericLiteral = NumericLiteral();
					if (numericLiteral->is_fp) {
						Error(previousToken, "Duration must be an integer.");
					}

					messageTime = static_cast<uint32_t>(numericLiteral->value);
					break;
				}

				const auto identToken = Expect(NVSETokenType::Identifier, "Expected identifier.");
				messageArgs.emplace_back(std::make_shared<Expressions::IdentExpr>(identToken));
			}

			Expect(NVSETokenType::RightParen, "Expected ')'");
			Expect(NVSETokenType::Semicolon, "Expected ';'");

			return std::make_shared<Statements::ShowMessage>(messageIdent, messageArgs, messageTime);
		}

		if (Peek(NVSETokenType::For)) {
			return ForStatement();
		}

		if (Peek(NVSETokenType::If)) {
			return IfStatement();
		}

		if (Peek(NVSETokenType::Match)) {
			return MatchStatement();
		}

		if (Peek(NVSETokenType::Return)) {
			return ReturnStatement();
		}

		if (Match(NVSETokenType::Break)) {
			auto token = previousToken;
			Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
			return std::make_shared<Statements::Break>();
		}

		if (Match(NVSETokenType::Continue)) {
			auto token = previousToken;
			Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
			return std::make_shared<Statements::Continue>();
		}

		if (Peek(NVSETokenType::While)) {
			return WhileStatement();
		}

		if (Peek(NVSETokenType::LeftBrace)) {
			return BlockStatement();
		}

		if (PeekType()) {
			auto expr = VarDecl();
			Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
			return expr;
		}

		return ExpressionStatement();
	}

	std::shared_ptr<Statements::VarDecl> Parser::VarDecl(bool allowValue, bool allowOnlyOneVarDecl) {
		if (!MatchesType()) {
			Error(currentToken, "Expected type.");
		}
		auto varType = previousToken;
		std::vector<Statements::VarDecl::Declaration> declarations{};
		do {
			auto name = Expect(NVSETokenType::Identifier, "Expected identifier.");
			ExprPtr value{ nullptr };

			if (Match(NVSETokenType::Eq)) {
				if (!allowValue) {
					Error(previousToken, "Variable definition is not allowed here.");
				}
				value = Expression();
			}

			declarations.emplace_back(name, value);
			if (allowOnlyOneVarDecl) {
				break;
			}
		} while (Match(NVSETokenType::Comma));

		return std::make_shared<Statements::VarDecl>(varType, std::move(declarations));
	}

	StmtPtr Parser::ForStatement() {
		Match(NVSETokenType::For);
		Expect(NVSETokenType::LeftParen, "Expected '(' after 'for'.");

		// Optional expression
		bool forEach = false;
		StmtPtr init = { nullptr };
		if (!Peek(NVSETokenType::Semicolon)) {
			// for (array aIter in [1::1, 2::2])..
			if (MatchesType()) {
				auto type = previousToken;
				auto ident = Expect(NVSETokenType::Identifier, "Expected identifier.");

				if (Match(NVSETokenType::In)) {
					auto decl = std::make_shared<Statements::VarDecl>(type, ident, nullptr);

					ExprPtr rhs = Expression();
					Expect(NVSETokenType::RightParen, "Expected ')'.");

					std::shared_ptr<Statements::Block> block = BlockStatement();
					return std::make_shared<Statements::ForEach>(std::vector{ decl }, std::move(rhs), std::move(block), false);
				}

				ExprPtr value{ nullptr };
				if (Match(NVSETokenType::Eq)) {
					value = Expression();
				}
				Expect(NVSETokenType::Semicolon, "Expected ';' after loop initializer.");
				init = std::make_shared<Statements::VarDecl>(type, ident, value);
			}
			// for ([int key, int value] in [1::1, 2::2]).
			// for ([_, int value] in [1::1, 2::2]).
			// for ([int key, _] in [1::1, 2::2]).
			// for ([int key] in [1::1, 2::2]).
			// for ([int value] in [1, 2]).
			else if (Match(NVSETokenType::LeftBracket)) {
				std::vector<std::shared_ptr<Statements::VarDecl>> decls{};

				// LHS
				if (Match(NVSETokenType::Underscore)) {
					decls.push_back(nullptr);
				}
				else {
					decls.push_back(VarDecl(false, true));
				}

				// RHS
				if (Match(NVSETokenType::Comma)) {
					if (Match(NVSETokenType::Underscore)) {
						decls.push_back(nullptr);
					}
					else {
						decls.push_back(VarDecl(false, true));
					}
				}

				Expect(NVSETokenType::RightBracket, "Expected ']'.");
				Expect(NVSETokenType::In, "Expected 'in'.");

				ExprPtr rhs = Expression();
				Expect(NVSETokenType::RightParen, "Expected ')'.");

				std::shared_ptr<Statements::Block> block = BlockStatement();
				return std::make_shared<Statements::ForEach>(decls, std::move(rhs), std::move(block), true);
			}
			else {
				init = Statement();
				if (!init->IsType<Expressions::AssignmentExpr>()) {
					Error(currentToken, "Expected variable declaration or assignment.");
				}
			}
		}

		// Default to true condition
		ExprPtr cond = std::make_shared<Expressions::BoolExpr>(NVSEToken{ NVSETokenType::Bool, "true", 1 }, true);
		if (!Peek(NVSETokenType::Semicolon)) {
			cond = Expression();
			Expect(NVSETokenType::Semicolon, "Expected ';' after loop condition.");
		}

		ExprPtr incr = { nullptr };
		if (!Peek(NVSETokenType::RightParen)) {
			incr = Expression();
		}
		Expect(NVSETokenType::RightParen, "Expected ')'.");

		std::shared_ptr<Statements::Block> block = BlockStatement();
		return std::make_shared<Statements::For>(std::move(init), std::move(cond), std::move(incr), std::move(block));
	}

	StmtPtr Parser::IfStatement() {
		Match(NVSETokenType::If);
		auto token = previousToken;

		Expect(NVSETokenType::LeftParen, "Expected '(' after 'if'.");
		auto cond = Expression();
		Expect(NVSETokenType::RightParen, "Expected ')' after 'if' condition.");
		auto block = BlockStatement();
		std::shared_ptr<Statements::Block> elseBlock = nullptr;
		if (Match(NVSETokenType::Else)) {
			if (Peek(NVSETokenType::If)) {
				std::vector<StmtPtr> statements{};
				statements.emplace_back(IfStatement());
				elseBlock = std::make_shared<Statements::Block>(std::move(statements));
			}
			else {
				elseBlock = BlockStatement();
			}
		}

		return std::make_shared<Statements::If>(std::move(cond), std::move(block), std::move(elseBlock));
	}

	StmtPtr Parser::MatchStatement() {
		Match(NVSETokenType::Match);

		Expect(NVSETokenType::LeftParen, "Expected '(' after 'match'.");
		auto cond = Expression();
		Expect(NVSETokenType::RightParen, "Expected ')' after 'match' expression.");

		Expect(NVSETokenType::LeftBrace, "Expected '{' after 'match' declaration");

		std::vector<std::shared_ptr<Statements::If>> matchCases{};
		std::shared_ptr<Statements::Block> defaultCase{};

		do {
			if (Match(NVSETokenType::Underscore)) {
				if (defaultCase) {
					Error(currentToken, "Only one default case can be specified.");
				}

				Expect(NVSETokenType::Arrow, "Expected '->' after match case value");
				defaultCase = BlockStatement();
				continue;
			}

			auto caseVal = Expression();
			auto eqExpr = std::make_shared<Expressions::BinaryExpr>(NVSEToken{ NVSETokenType::EqEq, "==" }, cond, caseVal);
			Expect(NVSETokenType::Arrow, "Expected '->' after match case value");

			const auto curIfStmt = std::make_shared<Statements::If>(std::move(eqExpr), BlockStatement());

			matchCases.push_back(curIfStmt);

			const auto caseCount = matchCases.size();
			if (caseCount > 1) {
				matchCases[caseCount - 1]->elseBlock = std::make_shared<Statements::Block>(std::vector<StmtPtr>{ curIfStmt });
			}
		} while (!Peek(NVSETokenType::RightBrace));

		if (defaultCase) {
			matchCases[matchCases.size() - 1]->elseBlock = defaultCase;
		}

		Expect(NVSETokenType::RightBrace, "Expected '}'");

		return matchCases[0];
	}

	StmtPtr Parser::ReturnStatement() {
		Match(NVSETokenType::Return);
		auto token = previousToken;
		if (Match(NVSETokenType::Semicolon)) {
			return std::make_shared<Statements::Return>();
		}

		auto expr = std::make_shared<Statements::Return>(Expression());
		Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
		return expr;
	}

	StmtPtr Parser::WhileStatement() {
		Match(NVSETokenType::While);
		auto token = previousToken;

		Expect(NVSETokenType::LeftParen, "Expected '(' after 'while'.");
		auto cond = Expression();
		Expect(NVSETokenType::RightParen, "Expected ')' after 'while' condition.");
		auto block = BlockStatement();

		return std::make_shared<Statements::While>(std::move(cond), std::move(block));
	}

	std::shared_ptr<Statements::Block> Parser::BlockStatement() {
		Expect(NVSETokenType::LeftBrace, "Expected '{'.");

		std::vector<StmtPtr> statements{};
		while (!Match(NVSETokenType::RightBrace)) {
			try {
				statements.emplace_back(Statement());
			}
			catch (NVSEParseError e) {
				hadError = true;
				CompErr("%s\n", e.what());
				Synchronize();

				if (currentToken.type == NVSETokenType::Eof) {
					break;
				}
			}
		}

		return std::make_shared<Statements::Block>(std::move(statements));
	}

	StmtPtr Parser::ExpressionStatement() {
		// Allow empty expression statements
		if (Match(NVSETokenType::Semicolon)) {
			return std::make_shared<Statements::ExpressionStatement>(nullptr);
		}

		auto expr = Expression();
		if (!Match(NVSETokenType::Semicolon)) {
			Error(previousToken, "Expected ';' at end of statement.");
		}
		return std::make_shared<Statements::ExpressionStatement>(std::move(expr));
	}

	ExprPtr Parser::Expression() {
		return Assignment();
	}

	ExprPtr Parser::Assignment() {
		ExprPtr left = Slice();

		if (Match(NVSETokenType::Eq) || Match(NVSETokenType::PlusEq) || Match(NVSETokenType::MinusEq) ||
			Match(NVSETokenType::StarEq) || Match(NVSETokenType::SlashEq) || Match(NVSETokenType::ModEq) || Match(
				NVSETokenType::PowEq) || Match(NVSETokenType::BitwiseOrEquals) || Match(NVSETokenType::BitwiseAndEquals)) {
			auto token = previousToken;

			const auto prevTok = previousToken;
			ExprPtr value = Assignment();

			if (left->IsType<Expressions::IdentExpr>() || left->IsType<Expressions::GetExpr>() || left->IsType<Expressions::SubscriptExpr>()) {
				return std::make_shared<Expressions::AssignmentExpr>(token, std::move(left), std::move(value));
			}

			Error(prevTok, "Invalid assignment target.");
		}

		return left;
	}

	ExprPtr Parser::Slice() {
		ExprPtr left = Ternary();

		while (Match(NVSETokenType::Slice)) {
			const auto op = previousToken;
			ExprPtr right = Ternary();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Ternary() {
		ExprPtr cond = LogicalOr();

		while (Match(NVSETokenType::Ternary)) {
			auto token = previousToken;

			ExprPtr left;
			if (Match(NVSETokenType::Slice)) {
				left = cond;
			}
			else {
				left = LogicalOr();
				Expect(NVSETokenType::Slice, "Expected ':'.");
			}

			auto right = LogicalOr();
			cond = std::make_shared<Expressions::TernaryExpr>(token, std::move(cond), std::move(left), std::move(right));
		}

		return cond;
	}

	ExprPtr Parser::LogicalOr() {
		ExprPtr left = LogicalAnd();

		while (Match(NVSETokenType::LogicOr)) {
			auto op = previousToken;
			ExprPtr right = LogicalAnd();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::LogicalAnd() {
		ExprPtr left = Equality();

		while (Match(NVSETokenType::LogicAnd)) {
			const auto op = previousToken;
			ExprPtr right = Equality();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Equality() {
		ExprPtr left = Comparison();

		while (Match(NVSETokenType::EqEq) || Match(NVSETokenType::BangEq)) {
			const auto op = previousToken;
			ExprPtr right = Comparison();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Comparison() {
		ExprPtr left = In();

		while (Match(NVSETokenType::Less) || Match(NVSETokenType::LessEq) || Match(NVSETokenType::Greater) || Match(
			NVSETokenType::GreaterEq)) {
			const auto op = previousToken;
			ExprPtr right = In();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::In() {
		ExprPtr left = BitwiseOr();

		bool isNot = Match(NVSETokenType::Not);
		if (Match(NVSETokenType::In)) {
			const auto op = previousToken;

			if (Match(NVSETokenType::LeftBracket)) {
				std::vector<ExprPtr> values{};
				while (!Match(NVSETokenType::RightBracket)) {
					if (!values.empty()) {
						Expect(NVSETokenType::Comma, "Expected ','.");
					}
					values.emplace_back(Expression());
				}

				return std::make_shared<Expressions::InExpr>(left, op, values, isNot);
			}

			auto expr = Expression();
			return std::make_shared<Expressions::InExpr>(left, op, expr, isNot);
		}

		if (isNot) {
			Error(currentToken, "Expected 'in'.");
		}

		return left;
	}

	ExprPtr Parser::BitwiseOr() {
		ExprPtr left = BitwiseAnd();

		while (Match(NVSETokenType::BitwiseOr)) {
			const auto op = previousToken;
			ExprPtr right = BitwiseAnd();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::BitwiseAnd() {
		ExprPtr left = Shift();

		while (Match(NVSETokenType::BitwiseAnd)) {
			const auto op = previousToken;
			ExprPtr right = Shift();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Shift() {
		ExprPtr left = Term();

		while (Match(NVSETokenType::LeftShift) || Match(NVSETokenType::RightShift)) {
			const auto op = previousToken;
			ExprPtr right = Term();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	// term -> factor ((+ | -) factor)?;
	ExprPtr Parser::Term() {
		ExprPtr left = Factor();

		while (Match(NVSETokenType::Plus) || Match(NVSETokenType::Minus)) {
			const auto op = previousToken;
			ExprPtr right = Factor();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Factor() {
		ExprPtr left = Pair();

		while (Match(NVSETokenType::Star) || Match(NVSETokenType::Slash) || Match(NVSETokenType::Mod) || Match(
			NVSETokenType::Pow)) {
			const auto op = previousToken;
			ExprPtr right = Pair();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Pair() {
		ExprPtr left = Unary();

		while (Match(NVSETokenType::MakePair)) {
			auto op = previousToken;
			ExprPtr right = Unary();
			left = std::make_shared<Expressions::BinaryExpr>(op, std::move(left), std::move(right));
		}

		return left;
	}

	ExprPtr Parser::Unary() {
		if (Match(NVSETokenType::Bang) || Match(NVSETokenType::Minus) || Match(NVSETokenType::Dollar) || Match(
			NVSETokenType::Pound) || Match(NVSETokenType::BitwiseAnd) || Match(NVSETokenType::Star) || Match(NVSETokenType::BitwiseNot)) {
			auto op = previousToken;
			ExprPtr right = Unary();

			// Convert these two in case of box/unbox
			if (op.type == NVSETokenType::BitwiseAnd) {
				op.type = NVSETokenType::Box;
			}

			if (op.type == NVSETokenType::Star) {
				op.type = NVSETokenType::Unbox;
			}

			if (op.type == NVSETokenType::Minus) {
				op.type = NVSETokenType::Negate;
			}

			return std::make_shared<Expressions::UnaryExpr>(op, std::move(right), false);
		}

		return Postfix();
	}

	ExprPtr Parser::Postfix() {
		ExprPtr expr = Call();

		while (Match(NVSETokenType::LeftBracket)) {
			auto token = previousToken;
			auto inner = Expression();
			Expect(NVSETokenType::RightBracket, "Expected ']'.");

			expr = std::make_shared<Expressions::SubscriptExpr>(token, std::move(expr), std::move(inner));
		}

		if (Match(NVSETokenType::PlusPlus) || Match(NVSETokenType::MinusMinus)) {
			if (!expr->IsType<Expressions::IdentExpr>() && !expr->IsType<Expressions::GetExpr>()) {
				Error(previousToken, "Invalid operand for '++', expected identifier.");
			}

			auto op = previousToken;
			return std::make_shared<Expressions::UnaryExpr>(op, std::move(expr), true);
		}

		return expr;
	}

	ExprPtr Parser::Call() {
		ExprPtr expr = Primary();

		while (Match(NVSETokenType::Dot) || Match(NVSETokenType::LeftParen)) {
			if (previousToken.type == NVSETokenType::Dot) {
				if (!expr->IsType<Expressions::GroupingExpr>() &&
					!expr->IsType<Expressions::IdentExpr>() &&
					!expr->IsType<Expressions::CallExpr>() &&
					!expr->IsType<Expressions::GetExpr>()) {
					Error(currentToken, "Invalid member access.");
				}

				auto token = previousToken;
				const auto ident = Expect(NVSETokenType::Identifier, "Expected identifier.");

				if (Match(NVSETokenType::LeftParen)) {
					std::vector<ExprPtr> args{};
					while (!Match(NVSETokenType::RightParen)) {
						args.emplace_back(std::move(Expression()));

						if (!Peek(NVSETokenType::RightParen) && !Match(NVSETokenType::Comma)) {
							Error(currentToken, "Expected ',' or ')'.");
						}
					}
					expr = std::make_shared<Expressions::CallExpr>(std::move(expr), ident, std::move(args));
				}
				else {
					expr = std::make_shared<Expressions::GetExpr>(token, std::move(expr), ident);
				}
			}
			else {
				// Can only call on ident or get expr
				if (!expr->IsType<Expressions::IdentExpr>()) {
					Error(currentToken, "Invalid callee.");
				}

				auto ident = dynamic_cast<Expressions::IdentExpr*>(expr.get())->token;

				std::vector<ExprPtr> args{};
				while (!Match(NVSETokenType::RightParen)) {
					args.emplace_back(std::move(Expression()));

					if (!Peek(NVSETokenType::RightParen) && !Match(NVSETokenType::Comma)) {
						Error(currentToken, "Expected ',' or ')'.");
					}
				}
				expr = std::make_shared<Expressions::CallExpr>(nullptr, ident, std::move(args));
			}
		}

		return expr;
	}

	ExprPtr Parser::Primary() {
		if (Match(NVSETokenType::Bool)) {
			return std::make_shared<Expressions::BoolExpr>(
				NVSEToken{ previousToken }, 
				std::get<double>(previousToken.value)
			);
		}

		if (Peek(NVSETokenType::Number)) {
			return NumericLiteral();
		}

		if (Match(NVSETokenType::String)) {
			ExprPtr expr = std::make_shared<Expressions::StringExpr>(previousToken);
			while (Match(NVSETokenType::Interp)) {
				auto inner = std::make_shared<Expressions::UnaryExpr>(NVSEToken{ NVSETokenType::Dollar, "$" }, Expression(), false);
				Expect(NVSETokenType::EndInterp, "Expected '}'");
				expr = std::make_shared<Expressions::BinaryExpr>(NVSEToken{ NVSETokenType::Plus, "+" }, expr, inner);
				if (Match(NVSETokenType::String) && previousToken.lexeme.length() > 2) {
					auto endStr = std::make_shared<Expressions::StringExpr>(previousToken);
					expr = std::make_shared<Expressions::BinaryExpr>(NVSEToken{ NVSETokenType::Plus, "+" }, expr, endStr);
				}
			}
			return expr;
		}

		if (Match(NVSETokenType::Identifier)) {
			return std::make_shared<Expressions::IdentExpr>(previousToken);
		}

		if (Match(NVSETokenType::LeftParen)) {
			ExprPtr expr = Expression();
			Expect(NVSETokenType::RightParen, "Expected ')' after expression.");
			return std::make_shared<Expressions::GroupingExpr>(std::move(expr));
		}

		if (Peek(NVSETokenType::LeftBracket)) {
			return ArrayLiteral();
		}

		if (Peek(NVSETokenType::LeftBrace)) {
			return MapLiteral();
		}

		if (Match(NVSETokenType::Fn)) {
			return FnExpr();
		}

		Error(currentToken, "Expected expression.");
		return ExprPtr{ nullptr };
	}

	std::shared_ptr<Expressions::NumberExpr> Parser::NumericLiteral() {
		Match(NVSETokenType::Number);

		// Double literal
		if (std::ranges::find(previousToken.lexeme, '.') != previousToken.lexeme.end()) {
			return std::make_shared<Expressions::NumberExpr>(previousToken, std::get<double>(previousToken.value), true);
		}

		// Int literal
		return std::make_shared<Expressions::NumberExpr>(previousToken, floor(std::get<double>(previousToken.value)), false);
	}

	// Only called when 'fn' token is matched
	ExprPtr Parser::FnExpr() {
		auto token = previousToken;
		auto args = ParseArgs();

		if (Match(NVSETokenType::Arrow)) {
			// Build call stmt
			auto callExpr = std::make_shared<Expressions::CallExpr>(nullptr, NVSEToken{ NVSETokenType::Identifier, "SetFunctionValue" },
			                                                        std::vector{ Expression() });
			StmtPtr exprStmt = std::make_shared<Statements::ExpressionStatement>(callExpr);
			auto block = std::make_shared<Statements::Block>(std::vector{ exprStmt });
			return std::make_shared<Expressions::LambdaExpr>(std::move(args), block);
		}

		return std::make_shared<Expressions::LambdaExpr>(std::move(args), BlockStatement());
	}

	ExprPtr Parser::ArrayLiteral() {
		Expect(NVSETokenType::LeftBracket, "Expected '['.");
		const auto tok = previousToken;

		std::vector<ExprPtr> values{};
		while (!Match(NVSETokenType::RightBracket)) {
			if (!values.empty()) {
				Expect(NVSETokenType::Comma, "Expected ','.");
			}
			values.emplace_back(Expression());
		}

		return std::make_shared<Expressions::ArrayLiteralExpr>(tok, values);
	}

	ExprPtr Parser::MapLiteral() {
		Expect(NVSETokenType::LeftBrace, "Expected '{'.");
		auto tok = previousToken;

		std::vector<ExprPtr> values{};
		while (!Match(NVSETokenType::RightBrace)) {
			if (!values.empty()) {
				Expect(NVSETokenType::Comma, "Expected ','.");
			}
			values.emplace_back(Expression());
		}

		return std::make_shared<Expressions::MapLiteralExpr>(std::move(tok), std::move(values));
	}

	std::vector<std::shared_ptr<Statements::VarDecl>> Parser::ParseArgs() {
		Expect(NVSETokenType::LeftParen, "Expected '(' after 'fn'.");

		std::vector<std::shared_ptr<Statements::VarDecl>> args{};
		while (!Match(NVSETokenType::RightParen)) {
			if (!Match(NVSETokenType::IntType) && !Match(NVSETokenType::DoubleType) && !Match(NVSETokenType::RefType) &&
				!Match(NVSETokenType::ArrayType) && !Match(NVSETokenType::StringType)) {
				Error(currentToken, "Expected type.");
			}
			auto type = previousToken;
			auto ident = Expect(NVSETokenType::Identifier, "Expected identifier.");
			auto decl = std::make_shared<Statements::VarDecl>(type, ident, nullptr);

			if (!Peek(NVSETokenType::RightParen) && !Match(NVSETokenType::Comma)) {
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
		}
		catch (std::runtime_error& er) {
			CompErr("[line %d] %s\n", lexer.line, er.what());
		}
	}

	bool Parser::Match(NVSETokenType type) {
		if (currentToken.type == type) {
			Advance();
			return true;
		}

		return false;
	}

	bool Parser::MatchesType() {
		return Match(NVSETokenType::IntType) || Match(NVSETokenType::DoubleType) || Match(NVSETokenType::RefType) ||
			Match(NVSETokenType::ArrayType) || Match(NVSETokenType::StringType);
	}

	bool Parser::Peek(NVSETokenType type) const {
		if (currentToken.type == type) {
			return true;
		}

		return false;
	}

	bool Parser::PeekType() const {
		return Peek(NVSETokenType::IntType) || Peek(NVSETokenType::DoubleType) || Peek(NVSETokenType::RefType) ||
			Peek(NVSETokenType::ArrayType) || Peek(NVSETokenType::StringType);
	}

	bool Parser::PeekBlockType() const {
		if (!Peek(NVSETokenType::Identifier)) {
			return false;
		}

		const auto identifier = currentToken.lexeme;
		return std::ranges::any_of(g_eventBlockCommandInfos, [&](const CommandInfo& ci) {
			return !_stricmp(ci.longName, identifier.c_str());
		});
	}

	NVSEToken Parser::ExpectBlockType(const std::string&& message) {
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

	void Parser::Error(std::string message) {
		panicMode = true;
		hadError = true;

		throw NVSEParseError(std::format("Error: {}", message));
	}

	void Parser::Error(NVSEToken token, std::string message) {
		panicMode = true;
		hadError = true;

		throw NVSEParseError(std::format("[line {}:{}] {}", token.line, token.column, message));
	}

	NVSEToken Parser::Expect(NVSETokenType type, std::string message) {
		if (!Match(type)) {
			Error(currentToken, std::move(message));
		}

		return previousToken;
	}

	void Parser::Synchronize() {
		Advance();

		while (currentToken.type != NVSETokenType::Eof) {
			if (previousToken.type == NVSETokenType::Semicolon) {
				panicMode = false;
				return;
			}

			switch (currentToken.type) {
			case NVSETokenType::If:
			case NVSETokenType::While:
			case NVSETokenType::For:
			case NVSETokenType::Return:
			case NVSETokenType::RightBrace:
				panicMode = false;
				return;
			default:;
			}

			Advance();
		}
	}
}
