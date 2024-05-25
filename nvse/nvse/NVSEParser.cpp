#include "NVSEParser.h"
#include "NVSECompilerUtils.h"

#include <format>
#include <iostream>

NVSEParser::NVSEParser(NVSELexer& tokenizer, std::function<void(std::string)> printFn) : lexer(tokenizer), printFn(printFn) {
	advance();
}

std::optional<NVSEScript> NVSEParser::parse() {
	NVSEToken name;
	std::vector<StmtPtr> globals;
	std::vector<StmtPtr> blocks;

	try {
		expect(NVSETokenType::Name, "Expected 'name' as first statement of script.");
		auto expr = primary();
		if (!dynamic_cast<IdentExpr*>(expr.get())) {
			error(previousToken, "Expected identifier.");
		}
		expect(NVSETokenType::Semicolon, "Expected ';'.");

		name = dynamic_cast<IdentExpr*>(expr.get())->name;

		while (currentToken.type != NVSETokenType::Eof) {
			try {
				if (peek(NVSETokenType::BlockType) || peek(NVSETokenType::Fn)) {
					break;
				}

				auto stmt = statement();
				if (!stmt->isType<VarDeclStmt>()) {
					error(previousToken, "Expected variable declaration.");
				}

				globals.emplace_back(std::move(stmt));
			}
			catch (NVSEParseError e) {
				printFn(e.what());
				printFn("\n");
				
				synchronize();
			}
		}

		// Get event or udf block
		if (peek(NVSETokenType::BlockType)) {
			blocks.emplace_back(begin());
		}
		else if (match(NVSETokenType::Fn)) {
			blocks.emplace_back(fnDecl());
		}
		else {
			error(currentToken, "Expected block type (GameMode, MenuMode, ...) or 'fn'.");
		}

		// Only allow one block for now
		if (match(NVSETokenType::BlockType) || match(NVSETokenType::Fn)) {
			error(currentToken, "Cannot have multiple blocks in one script.");
		}
	}
	catch (NVSEParseError e) {
		printFn(e.what());
		printFn("\n");
	}

	if (hadError) {
		return std::optional<NVSEScript>{};
	}

	return NVSEScript(name, std::move(globals), std::move(blocks));
}

StmtPtr NVSEParser::begin() {
	match(NVSETokenType::BlockType);
	auto blockName = previousToken;
	
	std::optional<NVSEToken> mode{};
	auto [opcode, arg, argRequired, argType] = mpBeginInfo[blockName.lexeme];

	if (match(NVSETokenType::Colon)) {
		if (arg == false) {
			error(currentToken, "Cannot specify argument for this block type.");
		}

		if (match(NVSETokenType::Colon)) {
			if (match(NVSETokenType::Number)) {
				if (argType != ArgType::Int) {
					error(currentToken, "Expected integer.");
				}

				auto modeToken = previousToken;
				if (modeToken.lexeme.contains('.')) {
					error(currentToken, "Expected integer.");
				}

				mode = modeToken;
			} else if (match(NVSETokenType::Identifier)) {
				if (argType != ArgType::Ref) {
					error(currentToken, "Expected identifier.");
				}

				mode = previousToken;
			} else {
				error(currentToken, "Expected identifier or number.");
			}
		} else {
			error(currentToken, "Expected ':'.");
		}
	} else {
		if (argRequired) {
			error(currentToken, "Missing required parameter for block type '" + blockName.lexeme + "'");
		}
	}

	if (!peek(NVSETokenType::LeftBrace)) {
		error(currentToken, "Expected '{'.");
	}
	return std::make_unique<BeginStmt>(blockName, mode, blockStmt());
}

StmtPtr NVSEParser::fnDecl() {
	return std::make_unique<FnDeclStmt>(std::move(parseArgs()), blockStmt());
}

StmtPtr NVSEParser::statement() {
	if (peek(NVSETokenType::For)) {
		return forStmt();
	}

	if (peek(NVSETokenType::If)) {
		return ifStmt();
	}

	if (peek(NVSETokenType::Return)) {
		return returnStmt();
	}

	if (peek(NVSETokenType::While)) {
		return whileStmt();
	}

	if (peek(NVSETokenType::LeftBrace)) {
		return blockStmt();
	}

	if (peek(NVSETokenType::IntType) || peek(NVSETokenType::DoubleType) || peek(NVSETokenType::RefType) || peek(NVSETokenType::ArrayType) || peek(NVSETokenType::StringType)) {
		auto expr = varDecl();
		expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
		return expr;
	}

	return exprStmt();
}

std::unique_ptr<VarDeclStmt> NVSEParser::varDecl() {
	if (!(match(NVSETokenType::IntType) || match(NVSETokenType::DoubleType) || match(NVSETokenType::RefType) || match(NVSETokenType::ArrayType) || match(NVSETokenType::StringType))) {
		error(currentToken, "Expected type.");
	}

	auto varType = previousToken;
	expect(NVSETokenType::Identifier, "Expected identifier.");
	auto ident = previousToken;
	ExprPtr value = nullptr;
	if (match(NVSETokenType::Eq)) {
		value = expression();
	}

	return std::make_unique<VarDeclStmt>(varType, ident, std::move(value));
}

StmtPtr NVSEParser::forStmt() {
	match(NVSETokenType::For);
	expect(NVSETokenType::LeftParen, "Expected '(' after 'for'.");

	return nullptr;
}

StmtPtr NVSEParser::ifStmt() {
	match(NVSETokenType::If);
	expect(NVSETokenType::LeftParen, "Expected '(' after 'if'.");
	auto cond = expression();
	expect(NVSETokenType::RightParen, "Expected ')' after 'if' condition.");
	auto block = blockStmt();
	StmtPtr elseBlock = nullptr;
	if (match(NVSETokenType::Else)) {
		elseBlock = blockStmt();
	}

	return std::make_unique<IfStmt>(std::move(cond), std::move(block), std::move(elseBlock));
}

StmtPtr NVSEParser::returnStmt() {
	match(NVSETokenType::Return);
	if (match(NVSETokenType::Semicolon)) {
		return std::make_unique<ReturnStmt>(nullptr);
	}

	return std::make_unique<ReturnStmt>(expression());
}

StmtPtr NVSEParser::whileStmt() {
	match(NVSETokenType::While);
	expect(NVSETokenType::LeftParen, "Expected '(' after 'while'.");
	auto cond = expression();
	expect(NVSETokenType::RightParen, "Expected ')' after 'while' condition.");
	auto block = blockStmt();

	return std::make_unique<WhileStmt>(std::move(cond), std::move(block));
}

StmtPtr NVSEParser::blockStmt() {
	expect(NVSETokenType::LeftBrace, "Expected '{'.");

	std::vector<StmtPtr> statements{};
	while (!match(NVSETokenType::RightBrace)) {
		try {
			statements.emplace_back(statement());
		}
		catch (NVSEParseError e) {
			printFn(e.what());
			printFn("\n");
			synchronize();
		}
	}

	return std::make_unique<BlockStmt>(std::move(statements));
}

StmtPtr NVSEParser::exprStmt() {
	// Allow empty expression statements
	if (match(NVSETokenType::Semicolon)) {
		return std::make_unique<ExprStmt>(nullptr);
	}

	auto expr = expression();
	if (!match(NVSETokenType::Semicolon)) {
		error(previousToken, "Expected ';' at end of statement.");
	}
	return std::make_unique<ExprStmt>(std::move(expr));
}

ExprPtr NVSEParser::expression() {
	return assignment();
}

ExprPtr NVSEParser::assignment() {
	ExprPtr left = ternary();

	if (match(NVSETokenType::Eq)) {
		const auto prevTok = previousToken;
		ExprPtr value = assignment();

		const auto idExpr = dynamic_cast<IdentExpr*>(left.get());
		const auto getExpr = dynamic_cast<GetExpr*>(left.get());

		if (idExpr) {
			return std::make_unique<AssignmentExpr>(idExpr->name, std::move(value));
		}

		if (getExpr) {
			return std::make_unique<SetExpr>(std::move(getExpr->left), getExpr->token, std::move(value));
		}

		error(prevTok, "Invalid assignment target.");
	}

	return left;
}

ExprPtr NVSEParser::ternary() {
	ExprPtr cond = logicOr();

	while (match(NVSETokenType::Ternary)) {
		ExprPtr left = nullptr;
		if (!match(NVSETokenType::Colon)) {
			left = logicOr();
			expect(NVSETokenType::Colon, "Expected ':'.");
		}
		auto right = logicOr();
		cond = std::make_unique<TernaryExpr>(std::move(cond), std::move(left), std::move(right));
	}

	return cond;
}

ExprPtr NVSEParser::logicOr() {
	ExprPtr left = logicAnd();

	while (match(NVSETokenType::LogicOr)) {
		const auto op = previousToken;
		ExprPtr right = logicAnd();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::logicAnd() {
	ExprPtr left = equality();

	while (match(NVSETokenType::LogicAnd)) {
		const auto op = previousToken;
		ExprPtr right = equality();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::equality() {
	ExprPtr left = comparison();

	while (match(NVSETokenType::EqEq) || match(NVSETokenType::BangEq)) {
		const auto op = previousToken;
		ExprPtr right = comparison();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::comparison() {
	ExprPtr left = term();

	while (match(NVSETokenType::Less) || match(NVSETokenType::LessEq) || match(NVSETokenType::Greater) || match(
		NVSETokenType::GreaterEq)) {
		const auto op = previousToken;
		ExprPtr right = term();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

// term -> factor ((+ | -) factor)?;
ExprPtr NVSEParser::term() {
	ExprPtr left = factor();

	while (match(NVSETokenType::Plus) || match(NVSETokenType::Minus)) {
		const auto op = previousToken;
		ExprPtr right = factor();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::factor() {
	ExprPtr left = unary();

	while (match(NVSETokenType::Star) || match(NVSETokenType::Slash)) {
		const auto op = previousToken;
		ExprPtr right = unary();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::unary() {
	if (match(NVSETokenType::Bang) || match(NVSETokenType::Minus) || match(NVSETokenType::Dollar)) {
		const auto op = previousToken;
		ExprPtr right = unary();
		return std::make_unique<UnaryExpr>(std::move(right), op);
	}

	return call();
}

ExprPtr NVSEParser::call() {
	ExprPtr expr = primary();

	while (match(NVSETokenType::Dot) || match(NVSETokenType::LeftParen)) {
		if (previousToken.type == NVSETokenType::Dot) {
			if (!expr->isType<GroupingExpr>() &&
				!expr->isType<IdentExpr>() &&
				!expr->isType<CallExpr>() &&
				!expr->isType<GetExpr>()) {
				error(currentToken, "Invalid member access.");
			}

			const auto ident = expect(NVSETokenType::Identifier, "Expected identifier.");
			expr = std::make_unique<GetExpr>(std::move(expr), ident);
		}
		else {
			// Can only call on ident or get expr
			if (!expr->isType<IdentExpr>() &&
				!expr->isType<GetExpr>()) {
				error(currentToken, "Invalid callee.");
			}

			std::vector<ExprPtr> args{};
			while (!match(NVSETokenType::RightParen)) {
				args.emplace_back(std::move(expression()));

				if (!peek(NVSETokenType::RightParen) && !match(NVSETokenType::Comma)) {
					error(currentToken, "Expected ',' or ')'.");
				}
			}
			expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
		}
	}

	return expr;
}

ExprPtr NVSEParser::primary() {
	if (match(NVSETokenType::Bool)) {
		return std::make_unique<BoolExpr>(std::get<double>(previousToken.value));
	}

	if (match(NVSETokenType::Number)) {
		// Double literal
		if (std::ranges::find(previousToken.lexeme, '.') != previousToken.lexeme.end()) {
			return std::make_unique<NumberExpr>(std::get<double>(previousToken.value), true);
		}
		
		// Int literal
		return std::make_unique<NumberExpr>(std::get<double>(previousToken.value), false);
	}

	if (match(NVSETokenType::String)) {
		return std::make_unique<StringExpr>(previousToken);
	}

	if (match(NVSETokenType::Identifier)) {
		return std::make_unique<IdentExpr>(previousToken);
	}

	if (match(NVSETokenType::LeftParen)) {
		ExprPtr expr = expression();
		expect(NVSETokenType::RightParen, "Expected ')' after expression.");
		return std::make_unique<GroupingExpr>(std::move(expr));
	}

	if (match(NVSETokenType::Fn)) {
		return fnExpr();
	}

	error(currentToken, "Expected expression.");
	return ExprPtr{nullptr};
}

// Only called when 'fn' token is matched
ExprPtr NVSEParser::fnExpr() {
	return std::make_unique<LambdaExpr>(std::move(parseArgs()), blockStmt());
}

std::vector<std::unique_ptr<VarDeclStmt>> NVSEParser::parseArgs() {
	expect(NVSETokenType::LeftParen, "Expected '(' after 'fn'.");

	std::vector<std::unique_ptr<VarDeclStmt>> args{};
	while (!match(NVSETokenType::RightParen)) {
		auto decl = varDecl();
		if (decl->value) {
			error(previousToken, "Cannot specify default values.");
		}
		args.emplace_back(std::move(decl));

		if (!peek(NVSETokenType::RightParen) && !match(NVSETokenType::Comma)) {
			error(currentToken, "Expected ',' or ')'.");
		}
	}

	return std::move(args);
}

void NVSEParser::advance() {
	previousToken = currentToken;
	currentToken = lexer.getNextToken();
}

bool NVSEParser::match(NVSETokenType type) {
	if (currentToken.type == type) {
		advance();
		return true;
	}

	return false;
}

bool NVSEParser::peek(NVSETokenType type) const {
	if (currentToken.type == type) {
		return true;
	}

	return false;
}

void NVSEParser::error(NVSEToken token, std::string message) {
	panicMode = true;
	hadError = true;

	std::string msg{};
	std::string lineInfo = std::format("[line {}:{}] ", token.line, token.linePos);

	msg += lineInfo;
	msg += lexer.lines[token.line - 1] + '\n';
	msg += std::string(lineInfo.length() + token.linePos - 1, ' ');
	msg += std::string(token.lexeme.length(), '^');
	msg += " " + message;

	throw NVSEParseError(msg);
}

NVSEToken NVSEParser::expect(NVSETokenType type, std::string message) {
	if (!match(type)) {
		error(currentToken, message);
	}

	return previousToken;
}

void NVSEParser::synchronize() {
	while (currentToken.type != NVSETokenType::Eof) {
		if (previousToken.type == NVSETokenType::Semicolon) {
			panicMode = false;
			return;
		}

		switch (currentToken.type) {
		case NVSETokenType::If:
		case NVSETokenType::While:
		case NVSETokenType::Return:
		case NVSETokenType::LeftBrace:
		case NVSETokenType::RightBrace:
			return;
		}

		advance();
	}
}
