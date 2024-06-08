#include "NVSETypeChecker.h"
#include "NVSEParser.h"
#include "ScriptTokens.h"

#include <format>
#include <sstream>

#include "GameForms.h"
#include "Commands_Scripting.h"
#include "GameData.h"
#include "NVSECompilerUtils.h"
#include "ScriptUtils.h"

#define WRAP_ERROR(expr)             \
    try {                            \
        expr;                        \
}									 \
    catch (std::runtime_error& er) { \
        CompErr("%s\n", er.what());  \
    }

Token_Type GetDetailedTypeFromNVSEToken(NVSETokenType type) {
	switch (type) {
	case NVSETokenType::StringType:
		return kTokenType_StringVar;
	case NVSETokenType::ArrayType:
		return kTokenType_ArrayVar;
	case NVSETokenType::RefType:
		return kTokenType_RefVar;
	// Short
	case NVSETokenType::DoubleType:
	case NVSETokenType::IntType:
		return kTokenType_NumericVar;
	default:
		return kTokenType_Ambiguous;
	}
}

Token_Type GetDetailedTypeFromVarType(Script::VariableType type) {
	switch (type) {
	case Script::eVarType_Float:
	case Script::eVarType_Integer:
		return kTokenType_Number;
	case Script::eVarType_String:
		return kTokenType_String;
	case Script::eVarType_Array:
		return kTokenType_Array;
	case Script::eVarType_Ref:
		return kTokenType_Ref;
	case Script::eVarType_Invalid:
	default:
		return kTokenType_Invalid;
	}
}

Token_Type GetVariableTypeFromNonVarType(Token_Type type) {
	switch (type) {
	case kTokenType_Number:
		return kTokenType_NumericVar;
	case kTokenType_String:
		return kTokenType_StringVar;
	case kTokenType_Ref:
		return kTokenType_RefVar;
	case kTokenType_Array:
		return kTokenType_ArrayVar;
	default:
		return kTokenType_Invalid;
	}
}

std::string getTypeErrorMsg(Token_Type lhs, Token_Type rhs) {
	return std::format("Cannot convert from {} to {}", TokenTypeToString(lhs), TokenTypeToString(rhs));
}

std::shared_ptr<NVSEScope> NVSETypeChecker::EnterScope(bool lambdaScope) {
	if (!scopes.empty()) {
		if (lambdaScope) {
			scopes.emplace(std::make_shared<NVSEScope>(scopeIndex++, scopes.top(), true));
		} else {
			scopes.emplace(std::make_shared<NVSEScope>(scopeIndex++, scopes.top()));
		}
	} else {
		scopes.emplace(std::make_shared<NVSEScope>(scopeIndex++, nullptr));
	}

	return scopes.top();
}

void NVSETypeChecker::LeaveScope() {
	scopes.pop();
}

void NVSETypeChecker::error(size_t line, std::string msg) {
	hadError = true;
	throw std::runtime_error(std::format("[line {}] {}", line, msg));
}

void NVSETypeChecker::error(size_t line, size_t column, std::string msg) {
	hadError = true;
	throw std::runtime_error(std::format("[line {}:{}] {}", line, column, msg));
}

bool NVSETypeChecker::check() {
	WRAP_ERROR(script->Accept(this))

	return !hadError;
}

void NVSETypeChecker::VisitNVSEScript(NVSEScript* script) {
	// Push global scope, gets discarded
	globalScope = EnterScope();
	
	for (auto global : script->globalVars) {
		global->Accept(this);

		// Dont allow initializers in global scope
		for (auto [name, value] : dynamic_cast<VarDeclStmt*>(global.get())->values) {
			if (value) {
				WRAP_ERROR(error(name.line, name.column, "Variable initializers are not allowed in global scope."))
			}
		}
	}

	// Pre-process blocks - check for duplicate blocks
	std::unordered_map<std::string, std::unordered_set<std::string>> mpTypeToModes{};
	std::vector<StmtPtr> functions {};
	bool foundFn = false, foundEvent = false;
	for (auto block : script->blocks) {
		if (auto b = dynamic_cast<BeginStmt*>(block.get())) {
			if (foundFn) {
				WRAP_ERROR(error(b->name.line, b->name.column, "Cannot have a function block and an event block in the same script."))
			}

			foundEvent = true;
			
			std::string name = b->name.lexeme;
			std::string param{};
			if (b->param.has_value()) {
				param = b->param->lexeme;
			}

			if (mpTypeToModes.contains(name) && mpTypeToModes[name].contains(param)) {
				WRAP_ERROR(error(b->name.line, b->name.column, "Duplicate block declaration."))
				continue;
			}

			if (!mpTypeToModes.contains(name)) {
				mpTypeToModes[name] = std::unordered_set<std::string>{};
			}
			mpTypeToModes[name].insert(param);
		}

		if (auto b = dynamic_cast<FnDeclStmt*>(block.get())) {
			if (foundEvent) {
				WRAP_ERROR(error(b->token.line, b->token.column, "Cannot have a function block and an event block in the same script."))
			}
			
			if (foundFn) {
				WRAP_ERROR(error(b->token.line, b->token.column, "Duplicate block declaration."))
			}

			foundFn = true;
		}
	}

	for (auto block : script->blocks) {
		block->Accept(this);
	}

	LeaveScope();
}

void NVSETypeChecker::VisitBeginStmt(BeginStmt* stmt) {
	stmt->scope = EnterScope(true);
	stmt->block->Accept(this);
	LeaveScope();
}

void NVSETypeChecker::VisitFnStmt(FnDeclStmt* stmt) {
	stmt->scope = EnterScope(true);

	//bScopedGlobal = true;
	for (auto decl : stmt->args) {
		WRAP_ERROR(decl->Accept(this))
	}
	//bScopedGlobal = false;

	stmt->body->Accept(this);

	LeaveScope();
}

void NVSETypeChecker::VisitVarDeclStmt(VarDeclStmt* stmt) {
	auto detailedType = GetDetailedTypeFromNVSEToken(stmt->type.type);
	for (auto [name, expr] : stmt->values) {
		// See if variable has already been declared
		bool cont = false;
		WRAP_ERROR(
			if (auto *var = scopes.top()->resolveVariable(name.lexeme, false)) {
				cont = true;
				error(name.line, name.column, std::format("Variable with name '{}' has already been defined in the current scope (at line {}:{})\n", name.lexeme, var->token.line, var->token.column));
			}

			auto* var2 = scopes.top()->resolveVariable(name.lexeme, true);

			// If we are seeing 'export' keyword and another global has already been defined with this name
			if (stmt->bExport && var2 && var2->global) {
				cont = true;
				error(name.line, name.column, std::format("Variable with name '{}' has already been defined in the global scope (at line {}:{})\n", name.lexeme, var2->token.line, var2->token.column));
			}
		)
		
		if (cont) {
			continue;
		}

		// Warn if name shadows a form
		if (auto form = GetFormByID(name.lexeme.c_str())) {
			auto modName = DataHandler::Get()->GetActiveModList()[form->GetModIndex()]->name;
#ifdef EDITOR
			CompInfo("[line %d:%d] Info: Variable with name '%s' shadows a form with the same name from mod '%s'\n",
				name.line, name.column, name.lexeme.c_str(), modName);
#else
			CompInfo("Info: Variable with name '%s' shadows a form with the same name from mod '%s'. This is NOT an error. Do not contact the mod author.", name.lexeme.c_str(), modName);
#endif
		}
		
		if (auto shadowed = scopes.top()->resolveVariable(name.lexeme, true)) {
#ifdef EDITOR
			CompInfo("[line %d:%d] Info: Variable with name '%s' shadows a variable with the same name in outer scope. (Defined at line %d:%d)\n",
				name.line, name.column, name.lexeme.c_str(), shadowed->token.line, shadowed->token.column);
#endif
		}

		if (expr) {
			expr->Accept(this);
			auto rhsType = expr->detailedType;
			if (s_operators[kOpType_Assignment].GetResult(detailedType, rhsType) == kTokenType_Invalid) {
				WRAP_ERROR(error(name.line, name.column, getTypeErrorMsg(rhsType, detailedType)))
				return;
			}
		}

		NVSEScope::ScopeVar var {};
		var.token = name;
		var.detailedType = detailedType;
		if (bScopedGlobal || stmt->bExport || scopes.top() == globalScope) {
			var.global = true;
		}
        var.variableType = GetScriptTypeFromToken(stmt->type);

		// Assign this new scope var to this statment for lookup in compiler
		stmt->scopeVars.push_back(scopes.top()->addVariable(name.lexeme, var, bScopedGlobal));
	}
}

void NVSETypeChecker::VisitExprStmt(const ExprStmt* stmt) {
	if (stmt->expr) {
		stmt->expr->Accept(this);
	}
}

void NVSETypeChecker::VisitForStmt(ForStmt* stmt) {
	stmt->scope = EnterScope();
	
	if (stmt->init) {
		WRAP_ERROR(stmt->init->Accept(this))
	}

	if (stmt->cond) {
		WRAP_ERROR(
			stmt->cond->Accept(this);

			// Check if condition can evaluate to bool
			auto lType = stmt->cond->detailedType;
			auto rType = kTokenType_Boolean;
			auto oType = s_operators[kOpType_Equals].GetResult(lType, rType);
			if (oType != kTokenType_Boolean) {
				error(stmt->line, std::format("Invalid expression type ('{}') for loop condition.", TokenTypeToString(oType)));
			}

			stmt->cond->detailedType = oType;
		)
	}

	if (stmt->post) {
		WRAP_ERROR(stmt->post->Accept(this))
	}

	insideLoop.push(true);
	stmt->block->Accept(this);
	insideLoop.pop();
	
	LeaveScope();
}

void NVSETypeChecker::VisitForEachStmt(ForEachStmt* stmt) {
	stmt->scope = EnterScope();
	for (const auto &decl : stmt->declarations) {
		if (decl) {
			decl->Accept(this);
		}
	}
	stmt->rhs->Accept(this);

	if (stmt->decompose) {
		// TODO
	} else {
		// Get type of lhs identifier -- this is way too verbose
		auto decl = stmt->declarations[0];
		auto ident = std::get<0>(decl->values[0]).lexeme;
		auto lType = scopes.top()->resolveVariable(ident)->detailedType;
		auto rType = stmt->rhs->detailedType;
		if (s_operators[kOpType_In].GetResult(lType, rType) == kTokenType_Invalid) {
			error(stmt->line, std::format("Invalid types '{}' and '{}' passed to for-in expression.",
				TokenTypeToString(lType), TokenTypeToString(rType)));
		}
	}

	insideLoop.push(true);
	stmt->block->Accept(this);
	insideLoop.pop();
	LeaveScope();
}

void NVSETypeChecker::VisitIfStmt(IfStmt* stmt) {
	stmt->scope = EnterScope(false);
	WRAP_ERROR(
		stmt->cond->Accept(this);

		// Check if condition can evaluate to bool
		const auto lType = stmt->cond->detailedType;
		const auto oType = s_operators[kOpType_Equals].GetResult(lType, kTokenType_Boolean);
		if (oType == kTokenType_Invalid) {
			error(stmt->line, std::format("Invalid expression type '{}' for if statement.", TokenTypeToString(lType)));
			stmt->cond->detailedType = kTokenType_Invalid;
		}
		else {
			stmt->cond->detailedType = oType;
		}
	)
	stmt->block->Accept(this);
	LeaveScope();

	if (stmt->elseBlock) {
		stmt->elseBlock->scope = EnterScope();
		stmt->elseBlock->Accept(this);
		LeaveScope();
	}
}

void NVSETypeChecker::VisitReturnStmt(ReturnStmt* stmt) {
	if (stmt->expr) {
		stmt->expr->Accept(this);
		stmt->detailedType = stmt->expr->detailedType;
	}
	else {
		stmt->detailedType = kTokenType_Empty;
	}
}

void NVSETypeChecker::VisitContinueStmt(ContinueStmt* stmt) {
	if (insideLoop.empty() || !insideLoop.top()) {
		error(stmt->line, "Keyword 'continue' not valid outside of loops.");
	}
}

void NVSETypeChecker::VisitBreakStmt(BreakStmt* stmt) {
	if (insideLoop.empty() || !insideLoop.top()) {
		error(stmt->line, "Keyword 'break' not valid outside of loops.");
	}
}

void NVSETypeChecker::VisitWhileStmt(WhileStmt* stmt) {
	stmt->scope = EnterScope();
	WRAP_ERROR(
		stmt->cond->Accept(this);

		// Check if condition can evaluate to bool
		auto lType = stmt->cond->detailedType;
		auto rType = kTokenType_Boolean;
		auto oType = s_operators[kOpType_Equals].GetResult(lType, rType);
		if (oType != kTokenType_Boolean) {
			error(stmt->line, "Invalid expression type for while loop.");
		}
		stmt->cond->detailedType = oType;
	)

	insideLoop.push(true);
	stmt->block->Accept(this);
	insideLoop.pop();
	LeaveScope();
}

void NVSETypeChecker::VisitBlockStmt(BlockStmt* stmt) {
	for (auto statement : stmt->statements) {
		WRAP_ERROR(statement->Accept(this))

		// TODO - Rework this
		// if (statement->IsType<ReturnStmt>()) {
		//     auto curType = stmt->detailedType;
		//     auto newType = statement->detailedType;
		//
		//     if (curType != kTokenType_Invalid && !CanConvertOperand(curType, newType)) {
		//         error(statement->line, std::format(
		//                   "Return type '{}' conflicts with previously specified return type '{}'.",
		//                   TokenTypeToString(newType), TokenTypeToString(curType)));
		//         stmt->detailedType = kTokenType_Invalid;
		//     }
		//     else {
		//         stmt->detailedType = statement->detailedType;
		//     }
		// }
	}
}

void NVSETypeChecker::VisitAssignmentExpr(AssignmentExpr* expr) {
	expr->left->Accept(this);
	expr->expr->Accept(this);

	auto lType = expr->left->detailedType;
	auto rType = expr->expr->detailedType;
	auto oType = s_operators[tokenOpToNVSEOpType[expr->token.type]].GetResult(lType, rType);
	if (oType == kTokenType_Invalid) {
		const auto msg = std::format("Invalid types '{}' and '{}' for operator {} ({}).",
		                             TokenTypeToString(lType), TokenTypeToString(rType), expr->token.lexeme,
		                             TokenTypeStr[static_cast<int>(expr->token.type)]);
		error(expr->line, msg);
		return;
	}

	expr->detailedType = oType;
	expr->left->detailedType = oType;
}

void NVSETypeChecker::VisitTernaryExpr(TernaryExpr* expr) {
	WRAP_ERROR(
		expr->cond->Accept(this);

		// Check if condition can evaluate to bool
		const auto lType = expr->cond->detailedType;
		const auto oType = s_operators[kOpType_Equals].GetResult(lType, kTokenType_Boolean);
		if (oType == kTokenType_Invalid) {
			error(expr->line, std::format("Invalid expression type '{}' for if statement.", TokenTypeToString(lType)));
			expr->cond->detailedType = kTokenType_Invalid;
		}
		else {
			expr->cond->detailedType = oType;
		}
	)

	WRAP_ERROR(expr->left->Accept(this))
	WRAP_ERROR(expr->right->Accept(this))
	if (!CanConvertOperand(expr->right->detailedType, expr->left->detailedType)) {
		error(expr->line, std::format("Incompatible value types ('{}' and '{}') specified for ternary expression.", 
			TokenTypeToString(expr->left->detailedType), TokenTypeToString(expr->right->detailedType)));
	}

	expr->detailedType = expr->left->detailedType;
}

void NVSETypeChecker::VisitInExpr(InExpr* expr) {
	expr->lhs->Accept(this);

	if (!expr->values.empty()) {
		int idx = 0;
		for (const auto& val : expr->values) {
			idx++;
			val->Accept(this);

			const auto lhsType = expr->lhs->detailedType;
			const auto rhsType = val->detailedType;
			const auto outputType = s_operators[tokenOpToNVSEOpType[NVSETokenType::EqEq]].GetResult(lhsType, rhsType);
			if (outputType == kTokenType_Invalid) {
				WRAP_ERROR(
					const auto msg = std::format("Value {} (type '{}') cannot compare against the {} specified on lhs of 'in' expression.", idx,
						TokenTypeToString(rhsType), TokenTypeToString(lhsType));
					error(expr->tok.line, expr->tok.column, msg);
				)
			}
		}
	}
	// Any other expression, compiles to ar_find
	else {
		expr->expression->Accept(this);
		if (expr->expression->detailedType != kTokenType_Ambiguous) {
			if (expr->expression->detailedType != kTokenType_Array && expr->expression->detailedType != kTokenType_ArrayVar) {
				WRAP_ERROR(
					const auto msg = std::format("Expected array for 'in' expression (Got '{}').", TokenTypeToString(expr->expression->detailedType));
					error(expr->tok.line, expr->tok.column, msg);
				)
			}
		}
	}

	expr->detailedType = kTokenType_Boolean;
}

void NVSETypeChecker::VisitBinaryExpr(BinaryExpr* expr) {
	expr->left->Accept(this);
	expr->right->Accept(this);

	auto lhsType = expr->left->detailedType;
	auto rhsType = expr->right->detailedType;
	auto outputType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lhsType, rhsType);
	if (outputType == kTokenType_Invalid) {
		const auto msg = std::format("Invalid types '{}' and '{}' for operator {} ({}).",
		                             TokenTypeToString(lhsType), TokenTypeToString(rhsType), expr->op.lexeme,
		                             TokenTypeStr[static_cast<int>(expr->op.type)]);
		error(expr->op.line, msg);
		return;
	}

	expr->detailedType = outputType;
}

void NVSETypeChecker::VisitUnaryExpr(UnaryExpr* expr) {
	expr->expr->Accept(this);

	if (expr->postfix) {
		auto lType = expr->expr->detailedType;
		auto rType = kTokenType_Number;
		auto oType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lType, rType);
		if (oType == kTokenType_Invalid) {
			error(expr->op.line, std::format("Invalid operand type '{}' for operator {} ({}).",
			                                 TokenTypeToString(lType),
			                                 expr->op.lexeme, TokenTypeStr[static_cast<int>(expr->op.type)]));
		}

		expr->detailedType = oType;
	}
	// -/!/$
	else {
		auto lType = expr->expr->detailedType;
		auto rType = kTokenType_Invalid;
		auto oType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lType, rType);
		if (oType == kTokenType_Invalid) {
			error(expr->op.line, std::format("Invalid operand type '{}' for operator {} ({}).",
			                                 TokenTypeToString(lType),
			                                 expr->op.lexeme, TokenTypeStr[static_cast<int>(expr->op.type)]));
		}
		expr->detailedType = oType;
	}
}

void NVSETypeChecker::VisitSubscriptExpr(SubscriptExpr* expr) {
	expr->left->Accept(this);
	expr->index->Accept(this);

	auto lhsType = expr->left->detailedType;
	auto indexType = expr->index->detailedType;
	auto outputType = s_operators[kOpType_LeftBracket].GetResult(lhsType, indexType);
	if (outputType == kTokenType_Invalid) {
		error(expr->op.line,
		      std::format("Expression type '{}' not valid for operator [].", TokenTypeToString(indexType)));
		return;
	}

	expr->detailedType = outputType;
}

void NVSETypeChecker::VisitCallExpr(CallExpr* expr) {
	std::string name = expr->token.lexeme;
	auto cmd = g_scriptCommands.GetByName(name.c_str());

	int insertedIdx = -1;
	if (auto &left = expr->left) {
		left->Accept(this);

		// Try to restructure AST in the case of 'array.filter()' or 'string.find()' syntactic sugar
		// array.filter(filterFn) becomes Ar_Filter(array, filterFn)
		if (left->detailedType == kTokenType_Array || left->detailedType == kTokenType_ArrayVar || left->detailedType == kTokenType_String || left->detailedType == kTokenType_StringVar) {
			if (left->detailedType == kTokenType_Array || left->detailedType == kTokenType_ArrayVar) {
				expr->token.lexeme = "Ar_" + name;
			} else {
				expr->token.lexeme = "Sv_" + name;
			}

			cmd = g_scriptCommands.GetByName(expr->token.lexeme.c_str());
			if (!cmd) {
				error(expr->token.line, expr->token.column, std::format("Invalid command '{}'.", name));
			}

			// Perform basic typechecking on call params
			int requiredParams = 0;
			for (int i = 0; i < cmd->numParams; i++) {
				if (!cmd->params[i].isOptional) {
					requiredParams++;
				}
			}

			// Check arg count
			if (expr->args.size() + 1 < requiredParams) {
				error(expr->token.line, expr->token.column, std::format("Invalid number of parameters specified for command {} (Expected '{}', Got {}).", name, requiredParams - 1, expr->args.size() + 1));
			}

			// Scan for replacement index
			for (int i = 0; i < cmd->numParams; i++) {
				auto param = cmd->params[i];
				if (ExpressionParser::ValidateArgType(static_cast<ParamType>(param.typeID), left->detailedType, true, cmd)) {
					expr->args.insert(expr->args.begin() + i, expr->left);
					expr->left = nullptr;

					// InsertedIdx is used to ignore the argument in later type checking, as it has already been checked
					// This is to resolve variable redeclaration errors, as it was declared already when checking LHS
					// and it will get visited again when compiling the args
					insertedIdx = i;
					break;
				}
			}
		}
	}

	// Try to get the script command by lexeme
	if (!cmd) {
		cmd = g_scriptCommands.GetByName(name.c_str());
		if (!cmd) {
			error(expr->token.line, expr->token.column, std::format("Invalid command '{}'.", name));
			return;
		}
	}
	expr->cmdInfo = cmd;

	// Perform basic typechecking on call params
	int requiredParams = 0;
	for (int i = 0; i < cmd->numParams; i++) {
		if (!cmd->params[i].isOptional) {
			requiredParams++;
		}
	}

	// Make sure minimum was passed
	if (expr->args.size() < requiredParams) {
		std::stringstream err{};
		err << std::format(
			"Number of parameters passed to command {} does not match what was expected. (Expected {}, Got {})\n", name,
			requiredParams, expr->args.size());
		error(expr->token.line, expr->token.column, err.str());
	}

	// Make sure not too many were passed
	// TODO: This breaks with at least one command: 'call', only expects 1 although it can support multiple.
	if (expr->args.size() > cmd->numParams && cmd->parse != kCommandInfo_Call.parse) {
		std::stringstream err{};
		err << std::format(
			"Number of parameters passed to command {} does not match what was expected. (Expected up to {}, Got {})\n",
			name,
			requiredParams, expr->args.size());
		error(expr->left->getToken()->line, err.str());
	}

	int maxParams = (cmd->parse == kCommandInfo_Call.parse) ? 16 : cmd->numParams;

	// Basic type checks
	// We already validated number of args, just verify types
	if (!isDefaultParse(cmd->parse)) { // if expression parser
		for (int i = 0; i < expr->args.size() && i < maxParams; i++) {
			auto param = cmd->params[i];
			auto arg = expr->args[i];

			// If we injected another param for something like array.filter(fn () -> {}) into Ar_Filter(array, fn () -> {})
			// Revisiting will cause a variable re-declaration exception
			if (i != insertedIdx) {
				WRAP_ERROR(arg->Accept(this))
			} else {
				// TODO: TEMP DISABLE TYPE CHECKS ON INSERTED PARAMS
				continue;
			}

			// TODO
			if (cmd->parse == kCommandInfo_Call.parse) {
				continue;
			}

			// Try to resolve as NVSE param
			if (!ExpressionParser::ValidateArgType(static_cast<ParamType>(cmd->params[i].typeID), arg->detailedType, true, cmd)) {
				WRAP_ERROR(
					error(expr->token.line, expr->token.column, std::format("Invalid expression for parameter {}. Expected {} (got {}).", i + 1, param.typeStr, TokenTypeToString(arg->detailedType)));
				)
			}
		}
	} else { // default parser
		for (int i = 0; i < expr->args.size() && i < maxParams; i++) {
			auto param = cmd->params[i];
			auto arg = expr->args[i];

			// Try to resolve identifiers as vanilla enums
			auto ident = dynamic_cast<IdentExpr*>(arg.get());
			uint32_t idx = -1;
			if (ident) {
				idx = resolveVanillaEnum(&param, ident->token.lexeme.c_str());
			}
			if (idx != -1) {
				CompDbg("[line %d] INFO: Converting identifier '%s' to enum index %d\n", arg->line, ident->token.lexeme.c_str(), idx);
				expr->args[i] = std::make_shared<NumberExpr>(NVSEToken{}, static_cast<double>(idx), false);
				expr->args[i]->detailedType = kTokenType_Number;
				continue;
			}

			// If we injected another param for something like array.filter(fn () -> {}) into Ar_Filter(array, fn () -> {})
			// Revisiting will cause a variable re-declaration exception
			if (i != insertedIdx) {
				WRAP_ERROR(arg->Accept(this))
			} else {
				// TODO: TEMP DISABLE TYPE CHECKS ON INSERTED PARAMS
				continue;
			}

			// TODO
			if (cmd->parse == kCommandInfo_Call.parse) {
				continue;
			}

			if (ident && arg->detailedType == kTokenType_Form) {
				// Extract form from param
				if (!doesFormMatchParamType(formCache[ident->token.lexeme], static_cast<ParamType>(param.typeID))) {
					WRAP_ERROR(
						error(expr->token.line, expr->token.column, std::format("Invalid expression for parameter {}. Expected {}.", i + 1, param.typeStr));
					)
				}
			}

			// Try to resolve as vanilla param
			if (!ExpressionParser::ValidateArgType(static_cast<ParamType>(cmd->params[i].typeID), arg->detailedType, false, cmd)) {
				WRAP_ERROR(
					error(expr->token.line, expr->token.column, std::format("Invalid expression for parameter {}. Expected {} (got {}).", i + 1, param.typeStr, TokenTypeToString(arg->detailedType)));
				)
			}
		}
	}

	auto type = ToTokenType(g_scriptCommands.GetReturnType(cmd));
	if (type == kTokenType_Invalid) {
		type = kTokenType_Ambiguous;
	}
	expr->detailedType = type;
}

void NVSETypeChecker::VisitGetExpr(GetExpr* expr) {
	expr->left->Accept(this);

	if (expr->left->detailedType != kTokenType_Form) {
		error(expr->line, std::format("Type '{}' not valid for operator '.'. Expected form.",
		                              TokenTypeToString(expr->left->detailedType)));
	}

	// Resolve variable type from form// Try to resolve lhs reference
	// Should be ident here
	const auto ident = dynamic_cast<IdentExpr*>(expr->left.get());

	const auto& lhsName = ident->token.lexeme;
	const auto& rhsName = expr->identifier.lexeme;

	TESForm* form = GetFormByID(lhsName.c_str());
	if (!form) {
		error(expr->line, std::format("Unable to find form '{}'", lhsName));
		return;
	}

	TESScriptableForm* scriptable = nullptr;
	switch (form->typeID) {
		case kFormType_TESObjectREFR: {
			const auto refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
			scriptable = DYNAMIC_CAST(refr->baseForm, TESForm, TESScriptableForm);
			break;
		}
		case kFormType_TESQuest: {
			scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
			break;
		}
		default: {
			error(expr->line, "Unexpected form type found.");
		}
	}

	if (scriptable) {
		if (const auto varInfo = scriptable->script->GetVariableByName(rhsName.c_str())) {
			const auto detailedType = GetDetailedTypeFromVarType(static_cast<Script::VariableType>(varInfo->type));
			const auto detailedTypeConverted = GetVariableTypeFromNonVarType(detailedType);
			if (detailedTypeConverted == kTokenType_Invalid) {
				expr->detailedType = detailedType;
			}
			else {
				expr->detailedType = detailedTypeConverted;
			}
			expr->varInfo = varInfo;
			expr->referenceName = form->GetEditorID();
			return;
		}

		error(expr->line, std::format("Variable {} not found on form {}.", rhsName, lhsName));
	}

	error(expr->line, std::format("Unable to resolve type for '{}.{}'.", lhsName, rhsName));
}

void NVSETypeChecker::VisitBoolExpr(BoolExpr* expr) {
	expr->detailedType = kTokenType_Boolean;
}

void NVSETypeChecker::VisitNumberExpr(NumberExpr* expr) {
	if (!expr->isFp) {
		if (expr->value > INT32_MAX) {
			WRAP_ERROR(error(expr->token.line, expr->token.column, "Maximum value for integer literal exceeded. (Max: " + std::to_string(INT32_MAX) + ")"))
		} else if (expr->value < INT32_MIN) {
			WRAP_ERROR(error(expr->token.line, expr->token.column, "Minimum value for integer literal exceeded. (Min: " + std::to_string(INT32_MIN) + ")"))
		}
	}
	
	expr->detailedType = kTokenType_Number;
}

void NVSETypeChecker::VisitMapLiteralExpr(MapLiteralExpr* expr) {
	if (expr->values.empty()) {
		WRAP_ERROR(error(expr->token.line, expr->token.column,
			"A map literal cannot be empty, as the key type cannot be deduced."))
		return;
	}

	// Check that all expressions are pairs
	for (int i = 0; i < expr->values.size(); i++) {
		const auto &val = expr->values[i];
		val->Accept(this);
		if (val->detailedType != kTokenType_Pair && val->detailedType != kTokenType_Ambiguous) {
			WRAP_ERROR(error(expr->token.line, expr->token.column,
				std::format("Value {} is not a pair and is not valid for a map literal.", i + 1)))
			return;
		}
	}

	// Now check key types
	auto lhsType = kTokenType_Invalid;
	for (int i = 0; i < expr->values.size(); i++) {
		if (expr->values[i]->detailedType != kTokenType_Pair) {
			continue;
		}

		// Try to check the key
		const auto pairPtr = dynamic_cast<BinaryExpr*>(expr->values[i].get());
		if (!pairPtr) {
			continue;
		}
		
		if (pairPtr->op.type != NVSETokenType::MakePair && pairPtr->left->detailedType != kTokenType_Pair) {
			continue;
		}
		
		if (lhsType == kTokenType_Invalid) {
			lhsType = pairPtr->left->detailedType;
		} else {
			if (lhsType != pairPtr->left->detailedType) {
				auto msg = std::format("Key for value {} (type '{}') specified for map literal conflicts with key type of previous value ('{}').",
					i, TokenTypeToString(pairPtr->left->detailedType), TokenTypeToString(lhsType));
				WRAP_ERROR(error(expr->token.line, expr->token.column, msg))
			}
		}
	}
	
	expr->detailedType = kTokenType_Array;
}

void NVSETypeChecker::VisitArrayLiteralExpr(ArrayLiteralExpr* expr) {
	if (expr->values.empty()) {
		expr->detailedType = kTokenType_Array;
		return;
	}

	for (const auto& val : expr->values) {
		val->Accept(this);

		if (val->detailedType == kTokenType_Pair) {
			WRAP_ERROR(error(val->getToken()->line, val->getToken()->column, "Invalid type inside of array literal. Expected array, string, ref, or number."))
		}
	}

	auto lhsType = expr->values[0]->detailedType;
	for (int i = 1; i < expr->values.size(); i++) {
		if (!CanConvertOperand(expr->values[i]->detailedType, lhsType)) {
			auto msg = std::format("Value {} (type '{}') specified for array literal conflicts with the type already specified in first element ('{}').",
				i, TokenTypeToString(expr->values[i]->detailedType), TokenTypeToString(lhsType));
			WRAP_ERROR(error(expr->token.line, expr->token.column, msg))
		}
	}

	expr->detailedType = kTokenType_Array;
}

void NVSETypeChecker::VisitStringExpr(StringExpr* expr) {
	expr->detailedType = kTokenType_String;
}

void NVSETypeChecker::VisitIdentExpr(IdentExpr* expr) {
	const auto name = expr->token.lexeme;

	const auto localVar = scopes.top()->resolveVariable(name);
	if (localVar) {
		//CompDbg("Resolved %s variable at scope %d:%d", localVar->global ? "global" : "local", localVar->scopeIndex, localVar->index);
		expr->detailedType = localVar->detailedType;
		expr->varInfo = localVar;
		return;
	}

	TESForm* form;
	if (formCache.contains(name)) {
		form = formCache[name];
	} else {
		form = GetFormByID(name.c_str());
	}

	if (!form) {
		expr->detailedType = kTokenType_Invalid;
		error(expr->token.line, expr->token.column, std::format("Unable to resolve identifier '{}'.", name));
	}

	if (form->typeID == kFormType_TESGlobal) {
		expr->detailedType = kTokenType_Global;
	} else {
		expr->detailedType = kTokenType_Form;
	}
	formCache[name] = form;
}

void NVSETypeChecker::VisitGroupingExpr(GroupingExpr* expr) {
	WRAP_ERROR(
		expr->expr->Accept(this);
		expr->detailedType = expr->expr->detailedType;
	)
}

void NVSETypeChecker::VisitLambdaExpr(LambdaExpr* expr) {
	expr->scope = EnterScope(true);

	//bScopedGlobal = true;
	for (auto &decl : expr->args) {
		WRAP_ERROR(decl->Accept(this))
	}
	//bScopedGlobal = false;

	insideLoop.push(false);
	expr->body->Accept(this);
	insideLoop.pop();

	expr->detailedType = kTokenType_Lambda;

	LeaveScope();
}
