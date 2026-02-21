#include <format>
#include <sstream>
#include "../Utils.h"

#include "TypeChecker.h"

#include <iomanip>

#include "../AST/AST.h"

#include "../../ScriptTokens.h"
#include "../../GameForms.h"
#include "../../Commands_Scripting.h"
#include "../../GameData.h"
#include "../../Hooks_Script.h"
#include "../../ScriptUtils.h"
#include "../../PluginAPI.h"

#define WRAP_ERROR(expr)             \
    try {                            \
        expr;                        \
}									 \
    catch (TypeCheckerError& er) { \
        ErrPrintln("%s", er.what());  \
    }

std::string getTypeErrorMsg(Token_Type lhs, Token_Type rhs) {
	return std::format("Cannot convert from {} to {}", TokenTypeToString(lhs), TokenTypeToString(rhs));
}

namespace Compiler::Passes {
	class TypeCheckerError : public std::runtime_error {
	public:
		TypeCheckerError() : runtime_error("") {}
	};

	void TypeChecker::error(const Expr* expr, const std::string& msg) {
		had_error = true;

		const auto err = HighlightSourceSpan(script->lines, msg, expr->sourceInfo, ESCAPE_RED);
		ErrPrint(err.c_str());

		throw TypeCheckerError{};
	}

	void TypeChecker::error(const Statement* stmt, const std::string& msg) {
		had_error = true;

		const auto err = HighlightSourceSpan(script->lines, msg, stmt->sourceInfo, ESCAPE_RED);
		ErrPrint(err.c_str());

		throw TypeCheckerError{};
	}

	void TypeChecker::Visit(AST* script) {
		DbgPrintln("[Type Checker]");
		DbgIndent();

		// Add nvse as requirement
		script->m_mpPluginRequirements["nvse"] = PACKED_NVSE_VERSION;

		// Add all #version statements to script requirements
		for (const auto& [plugin, version] : g_currentCompilerPluginVersions.top()) {
			auto nameLowered = plugin;
			std::ranges::transform(nameLowered.begin(), nameLowered.end(), nameLowered.begin(), [](unsigned char c) { return std::tolower(c); });
			script->m_mpPluginRequirements[nameLowered] = version;
		}

		for (const auto& global : script->globalVars) {
			global->Accept(this);

			// Dont allow initializers in global scope
			for (auto& [name, value, _] : dynamic_cast<Statements::VarDecl*>(global.get())->declarations) {
				if (value) {
					WRAP_ERROR(error(global.get(), "Variable initializers are not allowed in global scope."))
				}
			}
		}

		// Pre-process blocks - check for duplicate blocks
		std::unordered_map<std::string, std::unordered_set<std::string>> mpTypeToModes{};
		std::vector<StmtPtr> functions{};
		bool foundFn = false, foundEvent = false;
		for (const auto& block : script->blocks) {
			if (const auto b = dynamic_cast<Statements::Begin*>(block.get())) {
				if (foundFn) {
					WRAP_ERROR(error(b, "Cannot have a function block and an event block in the same script."))
				}

				foundEvent = true;

				std::string name = b->name;
				std::string param{};
				if (b->param.has_value()) {
					param = b->param->lexeme;
				}

				if (mpTypeToModes.contains(name) && mpTypeToModes[name].contains(param)) {
					WRAP_ERROR(error(b, "Duplicate block declaration."))
					continue;
				}

				if (!mpTypeToModes.contains(name)) {
					mpTypeToModes[name] = std::unordered_set<std::string>{};
				}
				mpTypeToModes[name].insert(param);
			}

			if (const auto b = dynamic_cast<Statements::UDFDecl*>(block.get())) {
				if (foundEvent) {
					WRAP_ERROR(error(b, "Cannot have a function block and an event block in the same script."))
				}

				if (foundFn) {
					WRAP_ERROR(error(b, "Duplicate block declaration."))
				}

				foundFn = true;
			}
		}

		for (const auto& block : script->blocks) {
			WRAP_ERROR(block->Accept(this));
		}

		DbgOutdent();
	}

	void TypeChecker::VisitFnStmt(Statements::UDFDecl* stmt) {
		for (const auto& decl : stmt->args) {
			WRAP_ERROR(decl->Accept(this))
		}

		returnType.emplace();
		stmt->body->Accept(this);
		returnType.pop();
	}

	void TypeChecker::VisitVarDeclStmt(Statements::VarDecl* stmt) {
		const auto detailedType = NVSETokenType_To_TokenType(stmt->type);
		for (auto [name, expr, varInfo] : stmt->declarations) {
			if (g_scriptCommands.GetByName(name->str.c_str())) {
				WRAP_ERROR(error(stmt, std::format("Variable name '{}' conflicts with a command with the same name.", name->str)));
				continue;
			}

			// Warn if name shadows a form
			if (auto form = GetFormByID(name->str.c_str())) {
				auto modName = DataHandler::Get()->GetActiveModList()[form->GetModIndex()]->name;
#ifdef EDITOR
				const auto msg = std::format("Variable shadows a form with the same name from mod '{}'", modName);
				const auto highlightMsg = HighlightSourceSpan(script->lines, msg, name->sourceInfo, ESCAPE_CYAN);
				InfoPrint(highlightMsg.c_str());
#else
				InfoPrintln("Info: Variable with name '%s' shadows a form with the same name from mod '%s'. This is NOT an error. Do not contact the mod author.", name->str.c_str(), modName);
#endif
			}

			if (expr) {
				expr->Accept(this);
				const auto rhsType = expr->type;
				if (s_operators[kOpType_Assignment].GetResult(detailedType, rhsType) == kTokenType_Invalid) {
					WRAP_ERROR(error(stmt, getTypeErrorMsg(rhsType, detailedType)))
					return;
				}
			}

			if (!varInfo) {
				error(expr.get(), "Unexpected compiler failure. Please report this as a bug.");
			}

			varInfo->detailed_type = detailedType;

			// Set lambda info
			if (expr->IsType<Expressions::LambdaExpr>()) {
				const auto* lambda = dynamic_cast<Expressions::LambdaExpr*>(expr.get());
				varInfo->lambda_type_info.is_lambda = true;
				varInfo->lambda_type_info.return_type = lambda->typeinfo.returnType;
				varInfo->lambda_type_info.param_types = lambda->typeinfo.paramTypes;
			}

			stmt->detailedType = detailedType;
		}
	}

	void TypeChecker::VisitExprStmt(Statements::ExpressionStatement* stmt) {
		if (stmt->expr) {
			stmt->expr->Accept(this);
		}
	}

	void TypeChecker::VisitForStmt(Statements::For* stmt) {
		if (stmt->init) {
			WRAP_ERROR(stmt->init->Accept(this))
		}

		if (stmt->cond) {
			WRAP_ERROR(
				stmt->cond->Accept(this);

			// Check if condition can evaluate to bool
			const auto lType = stmt->cond->type;
			const auto oType = s_operators[kOpType_Equals].GetResult(lType, kTokenType_Boolean);
			if (oType != kTokenType_Boolean) {
				error(stmt, std::format("Invalid expression type ('{}') for loop condition.", TokenTypeToString(oType)));
			}

			stmt->cond->type = oType;
				)
		}

		if (stmt->post) {
			WRAP_ERROR(stmt->post->Accept(this))
		}

		insideLoop.push(true);
		stmt->block->Accept(this);
		insideLoop.pop();
	}

	void TypeChecker::VisitForEachStmt(Statements::ForEach* stmt) {
		for (const auto& decl : stmt->declarations) {
			if (decl) {
				WRAP_ERROR(decl->Accept(this))
			}
		}
		stmt->rhs->Accept(this);

		WRAP_ERROR(
			if (stmt->decompose) {
				// TODO
				// What should I check here?
			}
			else {
				// Get type of lhs identifier -- this is way too verbose
				const auto lType = stmt->declarations[0]->detailedType;
				const auto rType = stmt->rhs->type;
				if (s_operators[kOpType_In].GetResult(lType, rType) == kTokenType_Invalid) {
					error(stmt, std::format("Invalid types '{}' and '{}' passed to for-in expression.",
					TokenTypeToString(lType), TokenTypeToString(rType)));
				}
			}
		)

		insideLoop.push(true);
		stmt->block->Accept(this);
		insideLoop.pop();
	}

	void TypeChecker::VisitIfStmt(Statements::If* stmt) {
		WRAP_ERROR(
			stmt->cond->Accept(this);

		// Check if condition can evaluate to bool
		const auto lType = stmt->cond->type;
		if (!CanConvertOperand(lType, kTokenType_Boolean)) {
			error(stmt, std::format("Invalid expression type '{}' for if statement.", TokenTypeToString(lType)));
			stmt->cond->type = kTokenType_Invalid;
		}
		else {
			stmt->cond->type = kTokenType_Boolean;
		}
			)
		stmt->block->Accept(this);

		if (stmt->elseBlock) {
			stmt->elseBlock->Accept(this);
		}
	}

	void TypeChecker::VisitReturnStmt(Statements::Return* stmt) {
		if (stmt->expr) {
			stmt->expr->Accept(this);

			const auto basicType = GetBasicTokenType(stmt->expr->type);
			auto& [type, line] = returnType.top();

			if (type != kTokenType_Invalid) {
				if (!CanConvertOperand(basicType, type)) {
					const auto msg = std::format(
						"Return type '{}' not compatible with return type defined previously. ('{}' at line {})",
						TokenTypeToString(basicType),
						TokenTypeToString(type),
						line);

					error(stmt->expr.get(), msg);
				}
			}
			else {
				type = basicType;
			}

			stmt->detailedType = stmt->expr->type;
		}
		else {
			stmt->detailedType = kTokenType_Empty;
		}
	}

	void TypeChecker::VisitContinueStmt(Statements::Continue* stmt) {
		if (insideLoop.empty() || !insideLoop.top()) {
			error(stmt, "Keyword 'continue' not valid outside of loops.");
		}
	}

	void TypeChecker::VisitBreakStmt(Statements::Break* stmt) {
		if (insideLoop.empty() || !insideLoop.top()) {
			error(stmt, "Keyword 'break' not valid outside of loops.");
		}
	}

	void TypeChecker::VisitWhileStmt(Statements::While* stmt) {
		WRAP_ERROR(
			stmt->cond->Accept(this);

			// Check if condition can evaluate to bool
			const auto lType = stmt->cond->type;
			const auto oType = s_operators[kOpType_Equals].GetResult(lType, kTokenType_Boolean);
			if (oType != kTokenType_Boolean) {
				error(stmt->cond.get(), "Invalid expression type for while loop.");
			}
			stmt->cond->type = oType;
		)

		insideLoop.push(true);
		stmt->block->Accept(this);
		insideLoop.pop();
	}

	void TypeChecker::VisitBlockStmt(Statements::Block* stmt) {
		for (const auto& statement : stmt->statements) {
			WRAP_ERROR(statement->Accept(this))
		}
	}

	void TypeChecker::VisitMatchStmt(Statements::Match* stmt) {
		
	}

	void TypeChecker::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
		expr->left->Accept(this);
		expr->expr->Accept(this);

		const auto lType = expr->left->type;
		const auto rType = expr->expr->type;
		const auto oType = s_operators[tokenOpToNVSEOpType[expr->token.type]].GetResult(lType, rType);
		if (oType == kTokenType_Invalid) {
			const auto msg = std::format("Invalid types '{}' and '{}' for operator {} ({}).",
				TokenTypeToString(lType), TokenTypeToString(rType), expr->token.lexeme,
				TokenTypeStr[static_cast<int>(expr->token.type)]);
			error(expr, msg);
			return;
		}

		// Probably always true
		if (expr->left->IsType<Expressions::IdentExpr>()) {
			const auto* ident = dynamic_cast<Expressions::IdentExpr*>(expr->left.get());
			if (ident->varInfo && ident->varInfo->lambda_type_info.is_lambda) {
				error(expr, "Cannot assign to a variable that is holding a lambda.");
				return;
			}

			if (expr->expr->type == kTokenType_Lambda) {
				error(expr, "Cannot assign a lambda to a variable after it has been declared.");
				return;
			}
		}

		expr->type = oType;
		expr->left->type = oType;
	}

	void TypeChecker::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
		WRAP_ERROR(
			expr->cond->Accept(this);

			// Check if condition can evaluate to bool
			const auto lType = expr->cond->type;
			const auto oType = s_operators[kOpType_Equals].GetResult(lType, kTokenType_Boolean);
			if (oType == kTokenType_Invalid) {
				error(expr, std::format("Invalid expression type '{}' for if statement.", TokenTypeToString(lType)));
				expr->cond->type = kTokenType_Invalid;
			}
			else {
				expr->cond->type = oType;
			}
		)

		WRAP_ERROR(expr->left->Accept(this))
		WRAP_ERROR(expr->right->Accept(this))
		if (!CanConvertOperand(expr->right->type, GetBasicTokenType(expr->left->type))) {
			error(
				expr, 
				std::format(
					"Incompatible value types ('{}' and '{}') specified for ternary expression.",
					TokenTypeToString(expr->left->type), 
					TokenTypeToString(expr->right->type)
				)
			);
		}

		expr->type = expr->left->type;
	}

	void TypeChecker::VisitInExpr(Expressions::InExpr* expr) {
		expr->lhs->Accept(this);

		if (!expr->values.empty()) {
			int idx = 0;
			for (const auto& val : expr->values) {
				idx++;
				val->Accept(this);

				const auto lhsType = expr->lhs->type;
				const auto rhsType = val->type;
				const auto outputType = s_operators[tokenOpToNVSEOpType[TokenType::EqEq]].GetResult(lhsType, rhsType);
				if (outputType == kTokenType_Invalid) {
					WRAP_ERROR(
						const auto msg = std::format(
							"Value {} (type '{}') cannot compare against the {} specified on lhs of 'in' expression.", 
							idx,
							TokenTypeToString(rhsType), 
							TokenTypeToString(lhsType)
						);

						error(expr, msg);
					)
				}
			}
		}
		// Any other expression, compiles to ar_find
		else {
			expr->expression->Accept(this);
			if (expr->expression->type != kTokenType_Ambiguous) {
				if (expr->expression->type != kTokenType_Array && expr->expression->type != kTokenType_ArrayVar) {
					WRAP_ERROR(
						const auto msg = std::format("Expected array for 'in' expression (Got '{}').", TokenTypeToString(expr->expression->type));
						error(expr, msg);
					)
				}
			}
		}

		expr->type = kTokenType_Boolean;
	}

	void TypeChecker::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
		expr->left->Accept(this);
		expr->right->Accept(this);

		const auto lhsType = expr->left->type;
		const auto rhsType = expr->right->type;
		const auto outputType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lhsType, rhsType);
		if (outputType == kTokenType_Invalid) {
			const auto msg = std::format("Invalid types '{}' and '{}' for operator {} ({}).",
				TokenTypeToString(lhsType), TokenTypeToString(rhsType), expr->op.lexeme,
				TokenTypeStr[static_cast<int>(expr->op.type)]);
			error(expr, msg);
			return;
		}

		expr->type = outputType;
	}

	void TypeChecker::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
		expr->expr->Accept(this);

		if (expr->postfix) {
			const auto lType = expr->expr->type;
			const auto rType = kTokenType_Number;
			const auto oType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lType, rType);
			if (oType == kTokenType_Invalid) {
				error(expr, std::format("Invalid operand type '{}' for operator {} ({}).",
					TokenTypeToString(lType),
					expr->op.lexeme, TokenTypeStr[static_cast<int>(expr->op.type)]));
			}

			expr->type = oType;
		}
		// -/!/$
		else {
			const auto lType = expr->expr->type;
			const auto rType = kTokenType_Invalid;
			const auto oType = s_operators[tokenOpToNVSEOpType[expr->op.type]].GetResult(lType, rType);
			if (oType == kTokenType_Invalid) {
				error(expr, std::format("Invalid operand type '{}' for operator {} ({}).",
					TokenTypeToString(lType),
					expr->op.lexeme, TokenTypeStr[static_cast<int>(expr->op.type)]));
			}
			expr->type = oType;
		}
	}

	void TypeChecker::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
		expr->left->Accept(this);
		expr->index->Accept(this);

		const auto lhsType = expr->left->type;
		const auto indexType = expr->index->type;
		const auto outputType = s_operators[kOpType_LeftBracket].GetResult(lhsType, indexType);
		if (outputType == kTokenType_Invalid) {
			error(expr,
				std::format("Expression type '{}' not valid for operator [].", TokenTypeToString(indexType)));
			return;
		}

		expr->type = outputType;
	}

	struct CallCommandInfo {
		int funcIndex{};
		int argStart{};
	};

	std::unordered_map<const char*, CallCommandInfo> callCmds = {
		{"Call", { .funcIndex = 0, .argStart = 1}},
		{"CallAfterFrames", { .funcIndex = 1, .argStart = 3}},
		{"CallAfterSeconds", { .funcIndex = 1, .argStart = 3}},
		{"CallForSeconds", { .funcIndex = 1, .argStart = 3}},
	};

	static CallCommandInfo* getCallCommandInfo(const char* name) {
		for (auto& [key, value] : callCmds) {
			if (!_stricmp(key, name)) {
				return &value;
			}
		}

		return nullptr;
	}

	void TypeChecker::VisitCallExpr(Expressions::CallExpr* expr) {
		auto checkLambdaArgs = [&](Expressions::IdentExpr* ident, int startArgs) {
			const auto& paramTypes = ident->varInfo->lambda_type_info.param_types;

			for (auto i = startArgs; i < expr->args.size(); i++) {
				const auto& arg = expr->args[i];

				const auto idx = i - startArgs + 1;

				// Too many args passed, handled below
				if (i - 1 >= paramTypes.size()) {
					break;
				}

				const auto expected = paramTypes[i - 1];
				if (!ExpressionParser::ValidateArgType(static_cast<ParamType>(expected), arg->type, true, nullptr)) {
					WRAP_ERROR(
						error(expr, std::format("Invalid expression for lambda parameter {}. (Expected {}, got {})", idx, GetBasicParamTypeString(expected), TokenTypeToString(arg->type)));
					)
				}
			}

			const auto numArgsPassed = max(0, (int)expr->args.size() - startArgs);
			if (numArgsPassed != paramTypes.size()) {
				WRAP_ERROR(
					error(expr, std::format("Invalid number of parameters specified for lambda '{}' (Expected {}, got {}).", ident->str, paramTypes.size(), numArgsPassed));
				)
			}
		};

		const std::string &name = expr->identifier->str;
		const auto cmd = g_scriptCommands.GetByName(name.c_str());

		// Try to get the script command by lexeme
		if (!cmd) {
			error(expr->identifier.get(), "Not a valid command");
			return;
		}
		expr->cmdInfo = cmd;

		// Add command to script requirements
		if (const auto plugin = g_scriptCommands.GetParentPlugin(cmd)) {
			auto nameLowered = std::string(plugin->name);
			std::ranges::transform(nameLowered.begin(), nameLowered.end(), nameLowered.begin(), [](unsigned char c) { return std::tolower(c); });

			if (!script->m_mpPluginRequirements.contains(nameLowered)) {
				script->m_mpPluginRequirements[std::string(nameLowered)] = plugin->version;
			}
			else {
				script->m_mpPluginRequirements[std::string(nameLowered)] = max(script->m_mpPluginRequirements[std::string(nameLowered)], plugin->version);
			}
		}

		if (expr->left) {
			expr->left->Accept(this);
		}

		if (expr->left && !CanUseDotOperator(expr->left->type)) {
			WRAP_ERROR(error(expr, "Left side of '.' must be a form or reference."))
		}

		// Check for calling reference if not an object script
		if (engineScript) {
			if (cmd->needsParent && !expr->left && engineScript->Type() != Script::eType_Object && engineScript->Type() != Script::eType_Magic) {
				WRAP_ERROR(error(expr, std::format("Command '{}' requires a calling reference.", expr->identifier->str)))
			}
		}

		// Normal (nvse + vanilla) calls
		int argIdx = 0;
		int paramIdx = 0;
		for (; paramIdx < cmd->numParams && argIdx < expr->args.size(); paramIdx++) {
			const auto param = &cmd->params[paramIdx];
			auto arg = expr->args[argIdx];
			bool convertedEnum = false;

			if (IsDefaultParse(cmd->parse) || !_stricmp(cmd->longName, "ShowMessage")) {
				// Try to resolve identifiers as vanilla enums
				const auto ident = arg->As<Expressions::IdentExpr>();

				uint32_t enumIndex = -1;
				uint32_t len = 0;
				if (ident) {
					ResolveVanillaEum(param, ident->str.c_str(), &enumIndex, &len);
				}

				if (enumIndex != -1) {
					DbgPrintln("[line %d] INFO: Converting identifier '%s' to enum index %d", arg->sourceInfo.start.line, ident->str.c_str(), enumIndex);
					arg = std::make_shared<Expressions::NumberExpr>(static_cast<double>(enumIndex), false, len, arg->sourceInfo);
					arg->type = kTokenType_Number;
					convertedEnum = true;
				}
				else {
					WRAP_ERROR(arg->Accept(this))
				}

				if (
					ident && 
					arg->type == kTokenType_Form && 
					param->typeID >= kParamType_ObjectRef && 
					param->typeID <= kParamType_Region
				) {
					// Extract form from param
					if (!DoesFormTypeMatchParamType(ident->form, static_cast<ParamType>(param->typeID))) {
						if (!param->isOptional) {
							WRAP_ERROR(
								error(arg.get(), std::format("Invalid expression for parameter {}. Expected {}.", argIdx + 1, param->typeStr));
							)
						}
					}
					else {
						argIdx++;
						continue;
					}
				}
			}
			else {
				WRAP_ERROR(arg->Accept(this))
				if (arg->type == kTokenType_Invalid) {
					argIdx++;
					continue;
				}
			}

			// Try to resolve as nvse param
			if (!convertedEnum && !ExpressionParser::ValidateArgType(static_cast<ParamType>(param->typeID), arg->type, !IsDefaultParse(cmd->parse), cmd)) {
				if (!param->isOptional) {
					WRAP_ERROR(
						error(arg.get(), std::format("Invalid expression for parameter {}. (Expected {}, got {}).", argIdx + 1, param->typeStr, TokenTypeToString(arg->type)));
					)
				}
			}
			else {
				expr->args[argIdx] = arg;
				argIdx++;
			}
		}

		int numRequiredArgs = 0;
		for (paramIdx = 0; std::cmp_less(paramIdx, cmd->numParams); paramIdx++) {
			if (!cmd->params[paramIdx].isOptional) {
				numRequiredArgs++;
			}
		}

		const bool enoughArgs = expr->args.size() >= numRequiredArgs;
		if (!enoughArgs) {
			WRAP_ERROR(
				error(expr, std::format("Invalid number of parameters specified to {} (Expected {}, got {}).", expr->identifier->str, numRequiredArgs, expr->args.size()));
			)
		}

		// Check lambda args differently
		if (const auto callInfo = getCallCommandInfo(cmd->longName)) {
			if (!enoughArgs) {
				return;
			}

			const auto& callee = expr->args[callInfo->funcIndex];
			const auto ident = dynamic_cast<Expressions::IdentExpr*>(callee.get());
			if (ident) {
				if (const auto form = GetFormByID(ident->str.c_str())) {
					if (const auto pScript = DYNAMIC_CAST(form, TESForm, Script)) {
						if (pScript->Type() != Script::eType_Object) {
							WRAP_ERROR(
								error(ident, std::format("Target script is not an object script (Invalid UDF)"));
							)
						}
					}
				}
			}

			const auto lambdaCallee = ident && ident->varInfo && ident->varInfo->lambda_type_info.is_lambda;

			// Handle each call command separately as args to check are in different positions
			// TODO: FIX THE CALL COMMAND
			// Manually visit remaining args for call commands, since they operate differently from all other commands
			while (argIdx < expr->args.size()) {
				expr->args[argIdx]->Accept(this);
				argIdx++;
			}

			if (lambdaCallee) {
				checkLambdaArgs(ident, callInfo->argStart);
				expr->type = ident->varInfo->lambda_type_info.return_type;
			}
			else {
				expr->type = kTokenType_Ambiguous;
			}

			return;
		}

		auto type = ToTokenType(g_scriptCommands.GetReturnType(cmd));
		if (type == kTokenType_Invalid) {
			type = kTokenType_Ambiguous;
		}
		expr->type = type;
	}

	void TypeChecker::VisitGetExpr(Expressions::GetExpr* expr) {
		expr->left->Accept(this);

		// Resolve variable type from form
		// Try to resolve lhs reference
		// Should be ident here
		const auto ident = dynamic_cast<Expressions::IdentExpr*>(expr->left.get());
		if (!ident || expr->left->type != kTokenType_Form) {
			error(expr, "Member access not valid here. Left side of '.' must be a form or persistent reference.");
		}

		const auto form = ident->form;
		const auto& lhsName = ident->str;
		const auto& rhsName = expr->identifier->str;

		const TESScriptableForm* scriptable = nullptr;
		switch (form->typeID) {
			case kFormType_TESObjectREFR: {
				const auto pRef = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
				scriptable = DYNAMIC_CAST(pRef->baseForm, TESForm, TESScriptableForm);
				break;
			}
			case kFormType_TESQuest: {
				scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
				break;
			}
			default: {
				error(expr, "Unexpected form type found.");
			}
		}

		if (scriptable && scriptable->script) {
			if (const auto varInfo = scriptable->script->GetVariableByName(rhsName.c_str())) {
				const auto varTokenType = VariableType_To_TokenType(static_cast<Script::VariableType>(varInfo->type));
				const auto variableType = TokenType_To_Variable_TokenType(varTokenType);
				if (variableType == kTokenType_Invalid) {
					expr->type = varTokenType;
				}
				else {
					expr->type = variableType;
				}
				expr->var_info = varInfo;
				expr->reference_name = form->GetEditorID();
				return;
			}

			error(expr, std::format("Variable {} not found on form {}.", rhsName, lhsName));
		}

		error(expr, std::format("Unable to resolve type for '{}.{}'.", lhsName, rhsName));
	}

	void TypeChecker::VisitBoolExpr(Expressions::BoolExpr* expr) {
		expr->type = kTokenType_Boolean;
	}

	void TypeChecker::VisitNumberExpr(Expressions::NumberExpr* expr) {
		if (!expr->is_fp) {
			if (expr->value > UINT32_MAX) {
				WRAP_ERROR(error(expr, "Maximum value for integer literal exceeded. (Max: " + std::to_string(UINT32_MAX) + ")"))
			}
		}

		expr->type = kTokenType_Number;
	}

	void TypeChecker::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {
		if (expr->values.empty()) {
			WRAP_ERROR(error(expr,
				"A map literal cannot be empty, as the key type cannot be deduced."))
				return;
		}

		// Check that all expressions are pairs
		for (int i = 0; i < expr->values.size(); i++) {
			const auto& val = expr->values[i];
			val->Accept(this);
			if (val->type != kTokenType_Pair && val->type != kTokenType_Ambiguous) {
				WRAP_ERROR(error(expr,
					std::format("Value {} is not a pair and is not valid for a map literal.", i + 1)))
					return;
			}
		}

		// Now check key types
		auto lhsType = kTokenType_Invalid;
		for (int i = 0; i < expr->values.size(); i++) {
			if (expr->values[i]->type != kTokenType_Pair) {
				continue;
			}

			// Try to check the key
			const auto pairPtr = dynamic_cast<Expressions::BinaryExpr*>(expr->values[i].get());
			if (!pairPtr) {
				continue;
			}

			if (pairPtr->op.type != TokenType::MakePair && pairPtr->left->type != kTokenType_Pair) {
				continue;
			}

			if (lhsType == kTokenType_Invalid) {
				lhsType = GetBasicTokenType(pairPtr->left->type);
			}
			else {
				if (lhsType != pairPtr->left->type) {
					auto msg = std::format(
						"Key for value {} (type '{}') specified for map literal conflicts with key type of previous value ('{}').",
						i,
						TokenTypeToString(pairPtr->left->type),
						TokenTypeToString(lhsType));

					WRAP_ERROR(error(expr, msg))
				}
			}
		}

		expr->type = kTokenType_Array;
	}

	void TypeChecker::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
		if (expr->values.empty()) {
			expr->type = kTokenType_Array;
			return;
		}

		for (const auto& val : expr->values) {
			val->Accept(this);

			if (val->type == kTokenType_Pair) {
				WRAP_ERROR(error(val.get(), "Invalid type inside of array literal. Expected array, string, ref, or number."))
			}
		}

		auto lhsType = expr->values[0]->type;
		for (int i = 1; i < expr->values.size(); i++) {
			if (!CanConvertOperand(expr->values[i]->type, lhsType)) {
				auto msg = std::format(
					"Value {} (type '{}') specified for array literal conflicts with the type already specified in first element ('{}').",
					i,
					TokenTypeToString(expr->values[i]->type),
					TokenTypeToString(lhsType));

				WRAP_ERROR(error(expr, msg))
			}
		}

		expr->type = kTokenType_Array;
	}

	void TypeChecker::VisitStringExpr(Expressions::StringExpr* expr) {
		expr->type = kTokenType_String;
	}

	void TypeChecker::VisitIdentExpr(Expressions::IdentExpr* expr) {
		const auto name = expr->str;

		if (expr->varInfo) {
			expr->type = expr->varInfo->detailed_type;
			return;
		}

		TESForm* form;
		if (!_stricmp(name.c_str(), "player")) {
			form = LookupFormByID(0x14);
		}
		else {
			form = GetFormByID(name.c_str());
		}

		if (!form) {
			expr->type = kTokenType_Invalid;
			error(expr, std::format("Unable to resolve identifier '{}'.", name));
		}

		if (form->typeID == kFormType_TESGlobal) {
			expr->type = kTokenType_Global;
		}
		else {
			expr->type = kTokenType_Form;
			expr->form = form;
		}
	}

	void TypeChecker::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
		WRAP_ERROR(
			expr->expr->Accept(this);
			expr->type = expr->expr->type;
		)
	}

	void TypeChecker::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
		returnType.emplace();

		for (const auto& decl : expr->args) {
			WRAP_ERROR(decl->Accept(this))
			expr->typeinfo.paramTypes.push_back(TokenType_To_ParamType(GetBasicTokenType(decl->detailedType)));
		}

		insideLoop.push(false);
		expr->body->Accept(this);
		insideLoop.pop();

		expr->type = kTokenType_Lambda;
		expr->typeinfo.returnType = returnType.top().returnType;

		returnType.pop();
	}
}