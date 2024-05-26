#include "NVSEParser.h"
#include "NVSECompilerUtils.h"

#include <format>
#include <iostream>

NVSEParser::NVSEParser(NVSELexer& tokenizer, std::function<void(std::string)> printFn) : lexer(tokenizer), printFn(printFn) {
	Advance();
}

std::optional<NVSEScript> NVSEParser::Parse() {
	NVSEToken name;
	std::vector<StmtPtr> globals;
	std::vector<StmtPtr> blocks;

	try {
		Expect(NVSETokenType::Name, "Expected 'name' as first statement of script.");
		auto expr = Primary();
		if (!dynamic_cast<IdentExpr*>(expr.get())) {
			Error(previousToken, "Expected identifier.");
		}
		Expect(NVSETokenType::Semicolon, "Expected ';'.");

		name = dynamic_cast<IdentExpr*>(expr.get())->name;

		while (currentToken.type != NVSETokenType::Eof) {
			try {
				if (Peek(NVSETokenType::BlockType) || Peek(NVSETokenType::Fn)) {
					break;
				}

				auto stmt = Statement();
				if (!stmt->IsType<VarDeclStmt>()) {
					Error(previousToken, "Expected variable declaration.");
				}

				globals.emplace_back(std::move(stmt));
			}
			catch (NVSEParseError e) {
				printFn(e.what());
				printFn("\n");
				
				Synchronize();
			}
		}

		// Get event or udf block
		if (Peek(NVSETokenType::BlockType)) {
			blocks.emplace_back(Begin());
		}
		else if (Match(NVSETokenType::Fn)) {
			blocks.emplace_back(FnDecl());
		}
		else {
			Error(currentToken, "Expected block type (GameMode, MenuMode, ...) or 'fn'.");
		}

		// Only allow one block for now
		if (Match(NVSETokenType::BlockType) || Match(NVSETokenType::Fn)) {
			Error(currentToken, "Cannot have multiple blocks in one script.");
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

StmtPtr NVSEParser::Begin() {
	Match(NVSETokenType::BlockType);
	auto blockName = previousToken;
	auto blockNameStr = blockName.lexeme;
	
	std::optional<NVSEToken> mode{};
	CommandInfo *blockInfo = nullptr;
	for (auto &info : g_eventBlockCommandInfos) {
		if (!strcmp(info.longName, blockNameStr.c_str())) {
			blockInfo = &info;
		}
	}

	if (Match(NVSETokenType::Colon)) {
		if (blockInfo->numParams == 0) {
			Error(currentToken, "Cannot specify argument for this block type.");
		}

		if (Match(NVSETokenType::Colon)) {
			if (Match(NVSETokenType::Number)) {
				if (blockInfo->params[0].typeID != kParamType_Integer) {
					Error(currentToken, "Expected identifier.");
				}

				auto modeToken = previousToken;
				if (modeToken.lexeme.contains('.')) {
					Error(currentToken, "Expected integer.");
				}

				mode = modeToken;
			} else if (Match(NVSETokenType::Identifier)) {
				if (blockInfo->params[0].typeID == kParamType_Integer) {
					Error(currentToken, "Expected number.");
				}

				mode = previousToken;
			} else {
				Error(currentToken, "Expected identifier or number.");
			}
		} else {
			Error(currentToken, "Expected ':'.");
		}
	} else {
		if (blockInfo->numParams > 0 && !blockInfo->params[0].isOptional) {
			Error(currentToken, "Missing required parameter for block type '" + blockName.lexeme + "'");
		}
	}

	if (!Peek(NVSETokenType::LeftBrace)) {
		Error(currentToken, "Expected '{'.");
	}
	return std::make_shared<BeginStmt>(blockName, mode, BlockStatement());
}

StmtPtr NVSEParser::FnDecl() {
	auto args = ParseArgs();
	return std::make_shared<FnDeclStmt>(std::move(args), BlockStatement());
}

StmtPtr NVSEParser::Statement() {
	if (Peek(NVSETokenType::For)) {
		return ForStatement();
	}

	if (Peek(NVSETokenType::If)) {
		return IfStatement();
	}

	if (Peek(NVSETokenType::Return)) {
		return ReturnStatement();
	}

	if (Peek(NVSETokenType::While)) {
		return WhileStatement();
	}

	if (Peek(NVSETokenType::LeftBrace)) {
		return BlockStatement();
	}

	if (Peek(NVSETokenType::For)) {
		return ForStatement();
	}

	if (Peek(NVSETokenType::IntType) || Peek(NVSETokenType::DoubleType) || Peek(NVSETokenType::RefType) || Peek(NVSETokenType::ArrayType) || Peek(NVSETokenType::StringType)) {
		auto expr = VarDecl();
		Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
		return expr;
	}

	return ExpressionStatement();
}

std::shared_ptr<VarDeclStmt> NVSEParser::VarDecl() {
	if (!(Match(NVSETokenType::IntType) || Match(NVSETokenType::DoubleType) || Match(NVSETokenType::RefType) || Match(NVSETokenType::ArrayType) || Match(NVSETokenType::StringType)) || Match(NVSETokenType::Fn)) {
		Error(currentToken, "Expected type.");
	}

	auto varType = previousToken;
	Expect(NVSETokenType::Identifier, "Expected identifier.");
	auto ident = previousToken;
	ExprPtr value = nullptr;
	if (Match(NVSETokenType::Eq)) {
		value = Expression();
	}

	return std::make_shared<VarDeclStmt>(varType, ident, std::move(value));
}

StmtPtr NVSEParser::ForStatement() {
	Match(NVSETokenType::For);
	Expect(NVSETokenType::LeftParen, "Expected '(' after 'for'.");

	// Optional expression
	StmtPtr init = {nullptr};
	if (!Peek(NVSETokenType::Semicolon)) {
		init = Statement();
		if (!init->IsType<VarDeclStmt>() && !init->IsType<AssignmentExpr>()) {
			Error(currentToken, "Expected variable declaration or assignment.");
		}

		// Don't need to Expect() a semicolon here because Statement() does.
	}

	// Default to true condition
	ExprPtr cond = std::make_shared<BoolExpr>(true);
	if (!Peek(NVSETokenType::Semicolon)) {
		cond = Expression();
		Expect(NVSETokenType::Semicolon, "Expected ';' after loop condition.");
	}

	ExprPtr incr = {nullptr};
	if (!Peek(NVSETokenType::RightParen)) {
		incr = Expression();
	}
	Expect(NVSETokenType::RightParen, "Expected ')'.");

	std::shared_ptr<BlockStmt> block = BlockStatement();
	return std::make_shared<ForStmt>(std::move(init), std::move(cond), std::move(incr), std::move(block));
}

StmtPtr NVSEParser::IfStatement() {
	Match(NVSETokenType::If);
	Expect(NVSETokenType::LeftParen, "Expected '(' after 'if'.");
	auto cond = Expression();
	Expect(NVSETokenType::RightParen, "Expected ')' after 'if' condition.");
	auto block = BlockStatement();
	StmtPtr elseBlock = nullptr;
	if (Match(NVSETokenType::Else)) {
		elseBlock = BlockStatement();
	}

	return std::make_shared<IfStmt>(std::move(cond), std::move(block), std::move(elseBlock));
}

StmtPtr NVSEParser::ReturnStatement() {
	Match(NVSETokenType::Return);
	if (Match(NVSETokenType::Semicolon)) {
		return std::make_shared<ReturnStmt>(nullptr);
	}

	auto expr = std::make_shared<ReturnStmt>(Expression());
	Expect(NVSETokenType::Semicolon, "Expected ';' at end of statement.");
	return expr;
}

StmtPtr NVSEParser::WhileStatement() {
	Match(NVSETokenType::While);
	Expect(NVSETokenType::LeftParen, "Expected '(' after 'while'.");
	auto cond = Expression();
	Expect(NVSETokenType::RightParen, "Expected ')' after 'while' condition.");
	auto block = BlockStatement();

	return std::make_shared<WhileStmt>(std::move(cond), std::move(block));
}

std::shared_ptr<BlockStmt> NVSEParser::BlockStatement() {
	Expect(NVSETokenType::LeftBrace, "Expected '{'.");

	std::vector<StmtPtr> statements{};
	while (!Match(NVSETokenType::RightBrace)) {
		try {
			statements.emplace_back(Statement());
		}
		catch (NVSEParseError e) {
			printFn(e.what());
			printFn("\n");
			Synchronize();
		}
	}

	return std::make_shared<BlockStmt>(std::move(statements));
}

StmtPtr NVSEParser::ExpressionStatement() {
	// Allow empty expression statements
	if (Match(NVSETokenType::Semicolon)) {
		return std::make_shared<ExprStmt>(nullptr);
	}

	auto expr = Expression();
	if (!Match(NVSETokenType::Semicolon)) {
		Error(previousToken, "Expected ';' at end of statement.");
	}
	return std::make_shared<ExprStmt>(std::move(expr));
}

ExprPtr NVSEParser::Expression() {
	return Assignment();
}

ExprPtr NVSEParser::Assignment() {
	ExprPtr left = Ternary();

	if (Match(NVSETokenType::Eq)) {
		const auto prevTok = previousToken;
		ExprPtr value = Assignment();

		if (left->IsType<IdentExpr>() || left->IsType<GetExpr>() || left->IsType<SubscriptExpr>()) {
			return std::make_shared<AssignmentExpr>(std::move(left), std::move(value));
		}

		Error(prevTok, "Invalid assignment target.");
	}

	return left;
}

ExprPtr NVSEParser::Ternary() {
	ExprPtr cond = LogicalOr();

	while (Match(NVSETokenType::Ternary)) {
		ExprPtr left = nullptr;
		if (!Match(NVSETokenType::Colon)) {
			left = LogicalOr();
			Expect(NVSETokenType::Colon, "Expected ':'.");
		}
		auto right = LogicalOr();
		cond = std::make_shared<TernaryExpr>(std::move(cond), std::move(left), std::move(right));
	}

	return cond;
}

ExprPtr NVSEParser::LogicalOr() {
	ExprPtr left = LogicalAnd();

	while (Match(NVSETokenType::LogicOr)) {
		const auto op = previousToken;
		ExprPtr right = LogicalAnd();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::LogicalAnd() {
	ExprPtr left = Equality();

	while (Match(NVSETokenType::LogicAnd)) {
		const auto op = previousToken;
		ExprPtr right = Equality();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::Equality() {
	ExprPtr left = Comparison();

	while (Match(NVSETokenType::EqEq) || Match(NVSETokenType::BangEq)) {
		const auto op = previousToken;
		ExprPtr right = Comparison();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::Comparison() {
	ExprPtr left = Term();

	while (Match(NVSETokenType::Less) || Match(NVSETokenType::LessEq) || Match(NVSETokenType::Greater) || Match(
		NVSETokenType::GreaterEq)) {
		const auto op = previousToken;
		ExprPtr right = Term();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

// term -> factor ((+ | -) factor)?;
ExprPtr NVSEParser::Term() {
	ExprPtr left = Factor();

	while (Match(NVSETokenType::Plus) || Match(NVSETokenType::Minus)) {
		const auto op = previousToken;
		ExprPtr right = Factor();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::Factor() {
	ExprPtr left = Unary();

	while (Match(NVSETokenType::Star) || Match(NVSETokenType::Slash)) {
		const auto op = previousToken;
		ExprPtr right = Unary();
		left = std::make_shared<BinaryExpr>(std::move(left), std::move(right), op);
	}

	return left;
}

ExprPtr NVSEParser::Unary() {
	if (Match(NVSETokenType::Bang) || Match(NVSETokenType::Minus) || Match(NVSETokenType::Dollar)) {
		const auto op = previousToken;
		ExprPtr right = Unary();
		return std::make_shared<UnaryExpr>(std::move(right), op, false);
	}

	return Postfix();
}

ExprPtr NVSEParser::Postfix() {
	ExprPtr expr = Call();

	if (Match(NVSETokenType::LeftBracket)) {
		auto inner = Expression();
		Expect(NVSETokenType::RightBracket, "Expected ']'.");

		return std::make_shared<SubscriptExpr>(std::move(expr), std::move(inner));
	}

	if (Match(NVSETokenType::PlusPlus) || Match(NVSETokenType::MinusMinus)) {
		if (!expr->IsType<IdentExpr>() && !expr->IsType<GetExpr>()) {
			Error(previousToken, "Invalid operand for '++', expected identifier.");
		}
		
		auto op = previousToken;
		return std::make_shared<UnaryExpr>(std::move(expr), op, true);
	}

	return expr;
}

ExprPtr NVSEParser::Call() {
	ExprPtr expr = Primary();

	while (Match(NVSETokenType::Dot) || Match(NVSETokenType::LeftParen)) {
		if (previousToken.type == NVSETokenType::Dot) {
			if (!expr->IsType<GroupingExpr>() &&
				!expr->IsType<IdentExpr>() &&
				!expr->IsType<CallExpr>() &&
				!expr->IsType<GetExpr>()) {
				Error(currentToken, "Invalid member access.");
			}

			const auto ident = Expect(NVSETokenType::Identifier, "Expected identifier.");

			if (!expr->IsType<IdentExpr>()) {
				if (!Peek(NVSETokenType::Dot) && !Peek(NVSETokenType::LeftParen)) {
					Error(currentToken, "Expected call or '.'.");
				}
			}
			
			expr = std::make_shared<GetExpr>(std::move(expr), ident);
		}
		else {
			// Can only call on ident or get expr
			if (!expr->IsType<IdentExpr>() &&
				!expr->IsType<GetExpr>()) {
				Error(currentToken, "Invalid callee.");
			}

			std::vector<ExprPtr> args{};
			while (!Match(NVSETokenType::RightParen)) {
				args.emplace_back(std::move(Expression()));

				if (!Peek(NVSETokenType::RightParen) && !Match(NVSETokenType::Comma)) {
					Error(currentToken, "Expected ',' or ')'.");
				}
			}
			expr = std::make_shared<CallExpr>(std::move(expr), std::move(args));
		}
	}

	return expr;
}

ExprPtr NVSEParser::Primary() {
	if (Match(NVSETokenType::Bool)) {
		return std::make_shared<BoolExpr>(std::get<double>(previousToken.value));
	}

	if (Match(NVSETokenType::Number)) {
		// Double literal
		if (std::ranges::find(previousToken.lexeme, '.') != previousToken.lexeme.end()) {
			return std::make_shared<NumberExpr>(std::get<double>(previousToken.value), true);
		}
		
		// Int literal
		return std::make_shared<NumberExpr>(std::get<double>(previousToken.value), false);
	}

	if (Match(NVSETokenType::String)) {
		return std::make_shared<StringExpr>(previousToken);
	}

	if (Match(NVSETokenType::Identifier)) {
		return std::make_shared<IdentExpr>(previousToken);
	}

	if (Match(NVSETokenType::LeftParen)) {
		ExprPtr expr = Expression();
		Expect(NVSETokenType::RightParen, "Expected ')' after expression.");
		return std::make_shared<GroupingExpr>(std::move(expr));
	}

	if (Match(NVSETokenType::Fn)) {
		return FnExpr();
	}

	Error(currentToken, "Expected expression.");
	return ExprPtr{nullptr};
}

// Only called when 'fn' token is matched
ExprPtr NVSEParser::FnExpr() {
	auto args = ParseArgs();
	return std::make_shared<LambdaExpr>(std::move(args), BlockStatement());
}

std::vector<std::shared_ptr<VarDeclStmt>> NVSEParser::ParseArgs() {
	Expect(NVSETokenType::LeftParen, "Expected '(' after 'fn'.");

	std::vector<std::shared_ptr<VarDeclStmt>> args{};
	while (!Match(NVSETokenType::RightParen)) {
		auto decl = VarDecl();
		if (decl->value) {
			Error(previousToken, "Cannot specify default values.");
		}
		args.emplace_back(std::move(decl));

		if (!Peek(NVSETokenType::RightParen) && !Match(NVSETokenType::Comma)) {
			Error(currentToken, "Expected ',' or ')'.");
		}
	}

	return std::move(args);
}

void NVSEParser::Advance() {
	previousToken = currentToken;
	currentToken = lexer.GetNextToken();
}

bool NVSEParser::Match(NVSETokenType type) {
	if (currentToken.type == type) {
		Advance();
		return true;
	}

	return false;
}

bool NVSEParser::Peek(NVSETokenType type) const {
	if (currentToken.type == type) {
		return true;
	}

	return false;
}

void NVSEParser::Error(NVSEToken token, std::string message) {
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

NVSEToken NVSEParser::Expect(NVSETokenType type, std::string message) {
	if (!Match(type)) {
		Error(currentToken, message);
	}

	return previousToken;
}

void NVSEParser::Synchronize() {
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
		default: ;
		}

		Advance();
	}
}
