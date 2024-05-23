#include "NVSEParser.h"

#include <format>
#include <iostream>

NVSEParser::NVSEParser(NVSELexer& tokenizer) : lexer(tokenizer) {
	this->lexer = tokenizer;
	advance();
}

StmtPtr NVSEParser::parse() {
	try {
		return statement();
	}
	catch (NVSEParseError e) {
		std::cout << e.what() << std::endl;
		synchronize();
	}
}

StmtPtr NVSEParser::statement() {
	if (peek(TokenType::For)) {
		return forStmt();
	}

	if (peek(TokenType::If)) {
		return ifStmt();
	}

	if (peek(TokenType::Return)) {
		return returnStmt();
	}

	if (peek(TokenType::While)) {
		return whileStmt();
	}

	if (peek(TokenType::LeftBrace)) {
		return blockStmt();
	}

	if (peek(TokenType::IntType) || peek(TokenType::DoubleType) || peek(TokenType::RefType) || peek(TokenType::ArrayType) || peek(TokenType::StringType)) {
		auto expr = varDecl();
		expect(TokenType::Semicolon, "Expected ';' at end of statement.");
		return expr;
	}

	return exprStmt();
}

std::unique_ptr<VarDeclStmt> NVSEParser::varDecl() {
	if (!(match(TokenType::IntType) || match(TokenType::DoubleType) || match(TokenType::RefType) || match(TokenType::ArrayType) || match(TokenType::StringType))) {
		error(currentToken, "Expected type.");
	}

	auto varType = previousToken;
	expect(TokenType::Identifier, "Expected identifier.");
	auto ident = previousToken;
	ExprPtr value = nullptr;
	if (match(TokenType::Eq)) {
		value = expression();
	}

	return std::make_unique<VarDeclStmt>(varType, ident, std::move(value));
}

StmtPtr NVSEParser::forStmt() {
	match(TokenType::For);
	expect(TokenType::LeftParen, "Expected '(' after 'for'.");

	return nullptr;
}

StmtPtr NVSEParser::ifStmt() {
	match(TokenType::If);
	expect(TokenType::LeftParen, "Expected '(' after 'if'.");
	auto cond = expression();
	expect(TokenType::RightParen, "Expected ')' after 'if' condition.");
	auto block = blockStmt();
	StmtPtr elseBlock = nullptr;
	if (match(TokenType::Else)) {
		elseBlock = blockStmt();
	}

	return std::make_unique<IfStmt>(std::move(cond), std::move(block), std::move(elseBlock));
}

StmtPtr NVSEParser::returnStmt() {
	match(TokenType::Return);
	if (match(TokenType::Semicolon)) {
		return std::make_unique<ReturnStmt>(nullptr);
	}

	return std::make_unique<ReturnStmt>(expression());
}

StmtPtr NVSEParser::whileStmt() {
	match(TokenType::While);
	expect(TokenType::LeftParen, "Expected '(' after 'while'.");
	auto cond = expression();
	expect(TokenType::RightParen, "Expected ')' after 'while' condition.");
	auto block = blockStmt();

	return std::make_unique<WhileStmt>(std::move(cond), std::move(block));
}

StmtPtr NVSEParser::blockStmt() {
	expect(TokenType::LeftBrace, "Expected '{'.");

	std::vector<StmtPtr> statements{};
	while (!match(TokenType::RightBrace)) {
		try {
			statements.emplace_back(statement());
		}
		catch (NVSEParseError e) {
			std::cout << e.what() << std::endl;
			synchronize();
		}
	}

	return std::make_unique<BlockStmt>(std::move(statements));
}

StmtPtr NVSEParser::exprStmt() {
	// Allow empty expression statements
	if (match(TokenType::Semicolon)) {
		return std::make_unique<ExprStmt>(nullptr);
	}

	auto expr = expression();
	if (!match(TokenType::Semicolon)) {
		error(previousToken, "Expected ';' at end of statement.");
	}
	return std::make_unique<ExprStmt>(std::move(expr));
}

ExprPtr NVSEParser::expression() {
	return assignment();
}

ExprPtr NVSEParser::assignment() {
	ExprPtr left = ternary();

	if (match(TokenType::Eq)) {
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

	while (match(TokenType::Ternary)) {
		ExprPtr left = nullptr;
		if (!match(TokenType::Colon)) {
			left = logicOr();
			expect(TokenType::Colon, "Expected ':'.");
		}
		auto right = logicOr();
		cond = std::make_unique<TernaryExpr>(std::move(cond), std::move(left), std::move(right));
	}

	return cond;
}

ExprPtr NVSEParser::logicOr() {
	ExprPtr left = logicAnd();

	while (match(TokenType::LogicOr)) {
		const auto op = previousToken;
		ExprPtr right = logicAnd();
		left = std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::logicAnd() {
	ExprPtr left = equality();

	while (match(TokenType::LogicAnd)) {
		const auto op = previousToken;
		ExprPtr right = equality();
		left = std::make_unique<LogicalExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::equality() {
	ExprPtr left = comparison();

	while (match(TokenType::EqEq) || match(TokenType::BangEq)) {
		const auto op = previousToken;
		ExprPtr right = comparison();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::comparison() {
	ExprPtr left = term();

	while (match(TokenType::Less) || match(TokenType::LessEq) || match(TokenType::Greater) || match(
		TokenType::GreaterEq)) {
		const auto op = previousToken;
		ExprPtr right = term();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

// term -> factor ((+ | -) factor)?;
ExprPtr NVSEParser::term() {
	ExprPtr left = factor();

	while (match(TokenType::Plus) || match(TokenType::Minus)) {
		const auto op = previousToken;
		ExprPtr right = factor();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::factor() {
	ExprPtr left = unary();

	while (match(TokenType::Star) || match(TokenType::Slash)) {
		const auto op = previousToken;
		ExprPtr right = unary();
		left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::unary() {
	if (match(TokenType::Bang) || match(TokenType::Minus)) {
		const auto op = previousToken;
		ExprPtr right = unary();
		return std::make_unique<UnaryExpr>(std::move(right), op);
	}

	return call();
}

ExprPtr NVSEParser::call() {
	ExprPtr expr = primary();

	while (match(TokenType::Dot) || match(TokenType::LeftParen)) {
		if (previousToken.type == TokenType::Dot) {
			if (!(dynamic_cast<GroupingExpr*>(expr.get())
				|| dynamic_cast<IdentExpr*>(expr.get())
				|| dynamic_cast<CallExpr*>(expr.get())
				|| dynamic_cast<GetExpr*>(expr.get()))) {
				error(currentToken, "Invalid member access.");
			}

			const auto ident = expect(TokenType::Identifier, "Expected identifier.");
			expr = std::make_unique<GetExpr>(std::move(expr), ident);
		}
		else {
			if (!(dynamic_cast<GroupingExpr*>(expr.get())
				|| dynamic_cast<IdentExpr*>(expr.get())
				|| dynamic_cast<CallExpr*>(expr.get())
				|| dynamic_cast<GetExpr*>(expr.get()))) {
				error(currentToken, "Invalid callee.");
			}


			if (match(TokenType::RightParen)) {
				expr = std::make_unique<CallExpr>(std::move(expr), std::move(std::vector<ExprPtr>{}));
				continue;
			}

			std::vector<ExprPtr> args{};
			args.emplace_back(std::move(expression()));
			while (match(TokenType::Comma)) {
				args.emplace_back(std::move(expression()));
			}

			expect(TokenType::RightParen, "Expected ')' after args.");

			expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
		}
	}

	return expr;
}

ExprPtr NVSEParser::primary() {
	if (match(TokenType::Bool)) {
		return std::make_unique<BoolExpr>(std::get<double>(previousToken.value));
	}

	if (match(TokenType::Number)) {
		// Double literal
		if (std::ranges::find(previousToken.lexeme, '.') != previousToken.lexeme.end()) {
			return std::make_unique<NumberExpr>(std::get<double>(previousToken.value), true);
		}
		// Int literal
		else {
			return std::make_unique<NumberExpr>(std::get<double>(previousToken.value), false);
		}
	}

	if (match(TokenType::String)) {
		return std::make_unique<StringExpr>(previousToken);
	}

	if (match(TokenType::Identifier)) {
		return std::make_unique<IdentExpr>(previousToken);
	}

	if (match(TokenType::LeftParen)) {
		ExprPtr expr = expression();
		expect(TokenType::RightParen, "Expected ')' after expression.");
		return std::make_unique<GroupingExpr>(std::move(expr));
	}

	if (match(TokenType::Fn)) {
		return fnExpr();
	}

	error(currentToken, "Expected expression.");
}

// Only called when 'fn' token is matched
ExprPtr NVSEParser::fnExpr() {
	expect(TokenType::LeftParen, "Expected '(' after 'fn'.");

	std::vector<std::unique_ptr<VarDeclStmt>> args{};
	if (!match(TokenType::RightParen)) {
		do {
			auto decl = varDecl();
			if (decl->value != nullptr) {
				error(previousToken, "Lambdas cannot have default values specified.");
			}
			args.emplace_back(std::move(decl));
		} while (match(TokenType::Comma));

		expect(TokenType::RightParen, "Expected ')' after arguments.");
	}

	return std::make_unique<LambdaExpr>(std::move(args), blockStmt());
}

void NVSEParser::advance() {
	previousToken = currentToken;
	currentToken = lexer.getNextToken();
}

bool NVSEParser::match(TokenType type) {
	if (currentToken.type == type) {
		advance();
		return true;
	}

	return false;
}

bool NVSEParser::peek(TokenType type) {
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

NVSEToken NVSEParser::expect(TokenType type, std::string message) {
	if (!match(type)) {
		error(currentToken, message);
	}

	return previousToken;
}

void NVSEParser::synchronize() {
	while (currentToken.type != TokenType::End) {
		if (previousToken.type == TokenType::Semicolon) {
			panicMode = false;
			return;
		}

		switch (currentToken.type) {
		case TokenType::If:
		case TokenType::While:
		case TokenType::Return:
		case TokenType::LeftBrace:
		case TokenType::RightBrace:
			return;
		}

		advance();
	}
}
