#include "NVSETypeChecker.h"
#include "NVSEParser.h"
#include "ScriptTokens.h"

#include <format>

#include "NVSECompilerUtils.h"
#include "ScriptUtils.h"

Token_Type GetDetailedTypeFromNVSEToken(NVSETokenType tt) {
    switch (tt) {
    case NVSETokenType::Number:
        return kTokenType_NumericVar;
    case NVSETokenType::String:
        return kTokenType_StringVar;
    case NVSETokenType::ArrayType:
        return kTokenType_ArrayVar;
    case NVSETokenType::RefType:
        return kTokenType_RefVar;
    case NVSETokenType::DoubleType:
        return kTokenType_NumericVar;
    case NVSETokenType::IntType:
        return kTokenType_NumericVar;
    default:
        return kTokenType_Ambiguous;
    }
}

std::string getTypeErrorMsg(Token_Type lhs, Token_Type rhs) {
    return std::format("Cannot convert from {} to {}", TokenTypeToString(lhs), TokenTypeToString(rhs));
}

void NVSETypeChecker::error(size_t line, std::string msg) {
    printFn(std::format("[line {}] {}\n", line, msg));
    hadError = true;
}

bool NVSETypeChecker::check() {
    script->Accept(this);

    return !hadError;
}

void NVSETypeChecker::VisitNVSEScript(const NVSEScript* script) {
    for (auto global : script->globalVars) {
        global->Accept(this);
    }

    for (auto block : script->blocks) {
        block->Accept(this);
    }
}

void NVSETypeChecker::VisitBeginStmt(const BeginStmt* stmt) {
    stmt->block->Accept(this);
}

void NVSETypeChecker::VisitFnStmt(FnDeclStmt* stmt) {
    stmt->body->Accept(this);
}

void NVSETypeChecker::VisitVarDeclStmt(const VarDeclStmt* stmt) {
    auto name = stmt->name.lexeme;
    auto type = GetDetailedTypeFromNVSEToken(stmt->type.type);

    // Check rhs
    if (stmt->value) {
        stmt->value->Accept(this);
        auto rhsType = stmt->value->detailedType;
        if (s_operators[operatorMap[":="]].GetResult(type, rhsType) == kTokenType_Invalid) {
            error(stmt->name.line, getTypeErrorMsg(rhsType, type));
            return;
        }
    }

    typeCache[name] = type;
}

void NVSETypeChecker::VisitExprStmt(const ExprStmt* stmt) {
    stmt->expr->Accept(this);
}

void NVSETypeChecker::VisitForStmt(const ForStmt* stmt) {
    if (stmt->init) {
        stmt->init->Accept(this);
    }

    if (stmt->cond) {
        stmt->cond->Accept(this);

        // Check if condition can evaluate to bool
        auto lType = stmt->cond->detailedType;
        auto rType = kTokenType_Boolean;
        auto oType = s_operators[operatorMap["=="]].GetResult(lType, rType);
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

void NVSETypeChecker::VisitIfStmt(IfStmt* stmt) {
    stmt->cond->Accept(this);

    // Check if condition can evaluate to bool
    auto lType = stmt->cond->detailedType;
    auto rType = kTokenType_Boolean;
    if (!CanConvertOperand(lType, rType)) {
        error(stmt->line, "Invalid expression type for if statement.");
        stmt->cond->detailedType = kTokenType_Invalid;
    } else {
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
    } else {
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
    auto oType = s_operators[operatorMap["=="]].GetResult(lType, rType);
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

void NVSETypeChecker::VisitAssignmentExpr(const AssignmentExpr* expr) {
    expr->left->Accept(this);
    expr->expr->Accept(this);

    auto lType = expr->left->detailedType;
    auto rType = expr->expr->detailedType;
    auto oType = s_operators[operatorMap[":="]].GetResult(lType, rType);
    if (s_operators[operatorMap[":="]].GetResult(lType, rType) == kTokenType_Invalid) {
        const auto msg = std::format("Invalid types {} and {} for operator {}.",
                                     TokenTypeToString(lType), TokenTypeToString(rType), "=");
        error(expr->line, msg);
        return;
    }

    expr->left->detailedType = oType;
}

void NVSETypeChecker::VisitTernaryExpr(const TernaryExpr* expr) {}

void NVSETypeChecker::VisitBinaryExpr(BinaryExpr* expr) {
    expr->left->Accept(this);
    expr->right->Accept(this);

    auto lhsType = expr->left->detailedType;
    auto rhsType = expr->right->detailedType;
    auto outputType = s_operators[operatorMap[expr->op.lexeme]].GetResult(lhsType, rhsType);
    if (outputType == kTokenType_Invalid) {
        const auto msg = std::format("Invalid types {} and {} for operator {}.",
                                     TokenTypeToString(lhsType), TokenTypeToString(rhsType), expr->op.lexeme);
        error(expr->line, msg);
        return;
    }

    expr->detailedType = lhsType;
}

void NVSETypeChecker::VisitUnaryExpr(UnaryExpr* expr) {
    expr->expr->Accept(this);

    if (expr->postfix) {
        auto lType = expr->expr->detailedType;
        auto rType = kTokenType_Number;
        auto oType = s_operators[operatorMap[expr->op.lexeme]].GetResult(lType, rType);
        if (oType == kTokenType_Invalid) {
            error(expr->line, std::format("Invalid operand type '{}' for operator {}.", TokenTypeToString(lType), expr->op.lexeme));
        }
        
        expr->detailedType = oType;
    }
    // -/!/$
    else {
        auto lType = expr->expr->detailedType;
        auto rType = kTokenType_Invalid;
        auto oType = s_operators[operatorMap[expr->op.lexeme]].GetResult(lType, rType);
        if (oType == kTokenType_Invalid) {
            error(expr->line, std::format("Invalid operand type '{}' for operator {}.", TokenTypeToString(lType), expr->op.lexeme));
        }
        expr->detailedType = oType;
    }
}

void NVSETypeChecker::VisitSubscriptExpr(SubscriptExpr* expr) {
    expr->left->Accept(this);
    expr->index->Accept(this);

    auto lhsType = expr->left->detailedType;
    if (lhsType != kTokenType_ArrayVar && lhsType != kTokenType_Array) {
        error(expr->line, std::format("Invalid type '{}' for operator [].", TokenTypeToString(lhsType)));
        return;
    }

    auto indexType = expr->index->detailedType;
    auto outputType = s_operators[operatorMap["["]].GetResult(lhsType, indexType);
    if (outputType == kTokenType_Invalid) {
        error(expr->line, std::format("Expression type '{}' not valid for operator [].", TokenTypeToString(indexType)));
        return;
    }

    expr->detailedType = outputType;
}

void NVSETypeChecker::VisitCallExpr(CallExpr* expr) {
    auto stackRefExpr = dynamic_cast<GetExpr*>(expr->left.get());
    auto identExpr = dynamic_cast<IdentExpr*>(expr->left.get());

    std::string name{};
    if (stackRefExpr) {
        name = stackRefExpr->token.lexeme;
    }
    else if (identExpr) {
        name = identExpr->name.lexeme;
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

    const auto lhsName = ident->name.lexeme;
    const auto rhsName = expr->token.lexeme;

    // Ensure it is of type quest
    TESForm* form = GetFormByID(lhsName.c_str());
    TESScriptableForm* scriptable = DYNAMIC_CAST(form, TESForm, TESScriptableForm);
    if (!scriptable) {
        error(expr->line, std::format("Form '{}' is not referenceable from scripts.", lhsName));
        return;
    }

    // Try to find variable
    Script* questScript = scriptable->script;
    if (!questScript) {
        error(expr->line, std::format("Form '{}' does not have a script.", lhsName));
        return;
    }

    for (int i = 0; i < questScript->varList.Count(); i++) {
        const auto curVar = questScript->varList.GetNthItem(i);
        if (!strcmp(curVar->name.CStr(), rhsName.c_str())) {
            expr->detailedType = GetDetailedTypeFromNVSEToken(static_cast<NVSETokenType>(curVar->type));
            return;
        }
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
    const auto name = expr->name.lexeme;

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
    error(expr->name.line, std::format("Unable to resolve identifier '{}'.", name));
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
