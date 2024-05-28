#include "NVSETypeChecker.h"
#include "NVSEParser.h"
#include "ScriptTokens.h"

#include <format>
#include <sstream>

#include "GameForms.h"
#include "Commands_Scripting.h"
#include "NVSECompilerUtils.h"
#include "ScriptUtils.h"

Token_Type GetDetailedTypeFromNVSEToken(NVSETokenType type) {
	switch (type) {
	case NVSETokenType::String:
		return kTokenType_StringVar;
	case NVSETokenType::ArrayType:
		return kTokenType_ArrayVar;
	case NVSETokenType::RefType:
		return kTokenType_RefVar;
	// Short
	case NVSETokenType::Number:
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

void NVSETypeChecker::error(size_t line, std::string msg) {
	CompErr("[line %d] %s\n", line, msg.c_str());
	hadError = true;
}

bool NVSETypeChecker::check() {
	script->Accept(this);

	return !hadError;
}

void NVSETypeChecker::VisitNVSEScript(const NVSEScript* script) {
	for (auto global : script->globalVars) {
		global->Accept(this);

		// Dont allow initializers in global scope
		for (auto [name, value] : dynamic_cast<VarDeclStmt*>(global.get())->values) {
			if (value) {
				error(name.line, "Variable initializers are not allowed in global scope.");
			}
		}
	}

	for (auto block : script->blocks) {
		block->Accept(this);
	}
}

void NVSETypeChecker::VisitBeginStmt(const BeginStmt* stmt) {
	stmt->block->Accept(this);
}

void NVSETypeChecker::VisitFnStmt(FnDeclStmt* stmt) {
	for (auto decl : stmt->args) {
		decl->Accept(this);
	}

	stmt->body->Accept(this);
}

void NVSETypeChecker::VisitVarDeclStmt(const VarDeclStmt* stmt) {
	auto detailedType = GetDetailedTypeFromNVSEToken(stmt->type.type);
	for (auto [name, expr] : stmt->values) {
		if (expr) {
			expr->Accept(this);
			auto rhsType = expr->detailedType;
			if (s_operators[kOpType_Assignment].GetResult(detailedType, rhsType) == kTokenType_Invalid) {
				error(name.line, getTypeErrorMsg(rhsType, detailedType));
				return;
			}
		}

		typeCache[name.lexeme] = detailedType;
	}
}

void NVSETypeChecker::VisitExprStmt(const ExprStmt* stmt) {
	if (stmt->expr) {
		stmt->expr->Accept(this);
	}
}

void NVSETypeChecker::VisitForStmt(ForStmt* stmt) {
	if (stmt->init) {
		stmt->init->Accept(this);
	}

	if (stmt->cond) {
		stmt->cond->Accept(this);

		// Check if condition can evaluate to bool
		auto lType = stmt->cond->detailedType;
		auto rType = kTokenType_Boolean;
		auto oType = s_operators[kOpType_Equals].GetResult(lType, rType);
		if (oType != kTokenType_Boolean) {
			error(stmt->line, "Invalid expression type for loop condition.");
		}

		stmt->cond->detailedType = oType;
	}

	if (stmt->post) {
		stmt->post->Accept(this);
	}

	insideLoop.push(true);
	stmt->block->Accept(this);
	insideLoop.pop();
}

void NVSETypeChecker::VisitForEachStmt(ForEachStmt* stmt) {
	stmt->lhs->Accept(this);
	stmt->rhs->Accept(this);

	// Get type of lhs identifier -- this is way too verbose
	auto decl = dynamic_cast<VarDeclStmt*>(stmt->lhs.get());
	auto ident = std::get<0>(decl->values[0]).lexeme;
	auto lType = typeCache[ident];
	auto rType = stmt->rhs->detailedType;
	if (s_operators[kOpType_In].GetResult(lType, rType) == kTokenType_Invalid) {
		error(stmt->line, std::format("Invalid types '{}' and '{}' passed to for-in expression.",
		                              TokenTypeToString(lType), TokenTypeToString(rType)));
	}

	stmt->block->Accept(this);
}

void NVSETypeChecker::VisitIfStmt(IfStmt* stmt) {
	stmt->cond->Accept(this);

	// Check if condition can evaluate to bool
	auto lType = stmt->cond->detailedType;
	auto rType = kTokenType_Boolean;
	if (!CanConvertOperand(lType, rType)) {
		error(stmt->line, std::format("Invalid expression type {} for if statement.", TokenTypeToString(lType)));
		stmt->cond->detailedType = kTokenType_Invalid;
	}
	else {
		stmt->cond->detailedType = rType;
	}

	stmt->block->Accept(this);
	if (stmt->elseBlock) {
		stmt->elseBlock->Accept(this);
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

void NVSETypeChecker::VisitWhileStmt(const WhileStmt* stmt) {
	stmt->cond->Accept(this);

	// Check if condition can evaluate to bool
	auto lType = stmt->cond->detailedType;
	auto rType = kTokenType_Boolean;
	auto oType = s_operators[kOpType_Equals].GetResult(lType, rType);
	if (oType != kTokenType_Boolean) {
		error(stmt->line, "Invalid expression type for while loop.");
	}
	stmt->cond->detailedType = oType;

	insideLoop.push(true);
	stmt->block->Accept(this);
	insideLoop.pop();
}

void NVSETypeChecker::VisitBlockStmt(BlockStmt* stmt) {
	for (auto statement : stmt->statements) {
		statement->Accept(this);

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
		const auto msg = std::format("Invalid types {} and {} for operator {} ({}).",
		                             TokenTypeToString(lType), TokenTypeToString(rType), expr->token.lexeme,
		                             TokenTypeStr[static_cast<int>(expr->token.type)]);
		error(expr->line, msg);
		return;
	}

	expr->detailedType = oType;
	expr->left->detailedType = oType;
}

void NVSETypeChecker::VisitTernaryExpr(const TernaryExpr* expr) {}

void NVSETypeChecker::VisitBinaryExpr(BinaryExpr* expr) {
	expr->left->Accept(this);
	expr->right->Accept(this);

	auto lhsType = expr->left->detailedType;
	auto rhsType = expr->right->detailedType;
	auto outputType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lhsType, rhsType);
	if (outputType == kTokenType_Invalid) {
		const auto msg = std::format("Invalid types {} and {} for operator {} ({}).",
		                             TokenTypeToString(lhsType), TokenTypeToString(rhsType), expr->op.lexeme,
		                             TokenTypeStr[static_cast<int>(expr->op.type)]);
		error(expr->op.line, msg);
		return;
	}

	expr->detailedType = lhsType;
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
	if (lhsType != kTokenType_ArrayVar && lhsType != kTokenType_Array) {
		error(expr->op.line, std::format("Invalid type '{}' for operator [].", TokenTypeToString(lhsType)));
		return;
	}

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
	auto stackRefExpr = dynamic_cast<GetExpr*>(expr->left.get());
	auto identExpr = dynamic_cast<IdentExpr*>(expr->left.get());
	for (auto arg : expr->args) {
		arg->Accept(this);
	}

	std::string name{};
	if (stackRefExpr) {
		name = stackRefExpr->identifier.lexeme;
	}
	else if (identExpr) {
		name = identExpr->token.lexeme;
	}
	else {
		// Shouldn't happen I don't think
		error(expr->line, "Unable to compile call expression, expected stack reference or identifier.");
	}

	// Try to get the script command by lexeme
	auto cmd = g_scriptCommands.GetByName(name.c_str());
	if (!cmd) {
		error(expr->line, std::format("Invalid command '{}'.", name));
		return;
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
		error(expr->left->getToken()->line, err.str());
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

	// Basic type checks
	// TODO: Make this

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
			const auto detailedType = GetDetailedTypeFromVarType(scriptable->script->GetVariableType(varInfo));
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
	expr->detailedType = kTokenType_Number;
}

void NVSETypeChecker::VisitStringExpr(StringExpr* expr) {
	expr->detailedType = kTokenType_String;
}

void NVSETypeChecker::VisitIdentExpr(IdentExpr* expr) {
	const auto name = expr->token.lexeme;

	// See if we already have a local var with this type
	if (typeCache.contains(name)) {
		expr->detailedType = typeCache[name];
		return;
	}

	// Try to find form
	if (GetFormByID(name.c_str())) {
		expr->detailedType = kTokenType_Form;
		typeCache[name] = kTokenType_Form;
		return;
	}

	expr->detailedType = kTokenType_Invalid;
	error(expr->token.line, std::format("Unable to resolve identifier '{}'.", name));
}

void NVSETypeChecker::VisitGroupingExpr(GroupingExpr* expr) {
	expr->expr->Accept(this);
	expr->detailedType = expr->expr->detailedType;
}

void NVSETypeChecker::VisitLambdaExpr(LambdaExpr* expr) {
	for (auto decl : expr->args) {
		decl->Accept(this);
	}

	insideLoop.push(false);
	expr->body->Accept(this);
	insideLoop.pop();

	expr->detailedType = kTokenType_Lambda;
}
