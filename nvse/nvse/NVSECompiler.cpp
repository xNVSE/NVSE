#include "NVSECompiler.h"
#include "Commands_MiscRef.h"
#include "Commands_Scripting.h"
#include "NVSECompilerUtils.h"
#include "NVSEParser.h"
#include "PluginAPI.h"

enum OPCodes {
    OP_LET = 0x1539,
    OP_EVAL = 0x153A,
    OP_WHILE = 0x153B,
    OP_LOOP = 0x153C,
    OP_FOREACH = 0x153D,
    OP_TERNARY = 0x166E,
    OP_CALL = 0x1545,
    OP_SET_MOD_LOCAL_DATA = 0x1549,
    OP_GET_MOD_LOCAL_DATA = 0x1548,
    OP_SET_FUNCTION_VALUE = 0x1546,
    OP_MATCHES_ANY = 0x166F,
    OP_AR_LIST = 0x1567,
    OP_AR_MAP = 0x1568,
    OP_AR_FIND = 0x1557,
    OP_AR_BAD_NUMERIC_INDEX = 0x155F,
};

bool NVSECompiler::Compile() {
    CompDbg("\n==== COMPILER ====\n\n");
    
    insideNvseExpr.push(false);
    loopIncrements.push(nullptr);
    statementCounter.push(0);
    
    ast.Accept(this);

    if (!partial) {
        engineScript->SetEditorID(scriptName.c_str());
    }

    engineScript->info.compiled = true;
    engineScript->info.dataLength = data.size();
    engineScript->info.numRefs = engineScript->refList.Count();
    engineScript->info.varCount = engineScript->varList.Count();
    engineScript->info.unusedVariableCount = engineScript->info.varCount - usedVars.size();
    engineScript->data = static_cast<uint8_t*>(FormHeap_Allocate(data.size()));
    memcpy(engineScript->data, data.data(), data.size());

    // Debug print local info
    CompDbg("\n[Locals]\n\n");
    for (int i = 0; i < engineScript->varList.Count(); i++) {
        auto item = engineScript->varList.GetNthItem(i);
        CompDbg("%i: %s %s\n", item->idx, item->name.CStr(), usedVars.contains(item->name.CStr()) ? "" : "(unused)");
    }
    
    CompDbg("\n");

    // Refs
    CompDbg("[Refs]\n\n");
    for (int i = 0; i < engineScript->refList.Count(); i++) {
        const auto ref = engineScript->refList.GetNthItem(i);
        if (ref->varIdx) {
            CompDbg("%d: (Var %d)\n", i, ref->varIdx);
        }
        else {
            CompDbg("%d, %s\n", i, ref->form->GetEditorID());
        }
    }
    
    CompDbg("\n");

    // Script data
    CompDbg("[Data]\n");
    for (int i = 0; i < engineScript->info.dataLength; i++) {
        CompDbg("%02X ", engineScript->data[i]);
    }
    
    CompDbg("\n\n");
    CompDbg("[Requirements]\n");
    for (std::string requirement : requirements) {
        CompDbg("%s\n", requirement.c_str());
    }

    CompDbg("\n");
    CompDbg("\nNum compiled bytes: %d\n", engineScript->info.dataLength);

    return true;
}

void NVSECompiler::VisitNVSEScript(NVSEScript* nvScript) {
    // Compile the script name
    scriptName = nvScript->name.lexeme;

    // Dont allow naming script the same as another form, unless that form is the script itself
    auto comp = strcmp(scriptName.c_str(), originalScriptName);
    if (ResolveObjReference(scriptName, false) && comp && !partial) {
        throw std::runtime_error(std::format("Error: Form name '{}' is already in use.\n", scriptName));
    }

    // SCN
    AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

    for (auto& global_var : nvScript->globalVars) {
        global_var->Accept(this);
    }

    for (auto& block : nvScript->blocks) {
        block->Accept(this);
    }
}

void NVSECompiler::VisitBeginStmt(const BeginStmt* stmt) {
    auto name = stmt->name.lexeme;

    // Shouldn't be null
    CommandInfo* beginInfo = nullptr;
    for (auto& info : g_eventBlockCommandInfos) {
        if (!strcmp(info.longName, name.c_str())) {
            beginInfo = &info;
        }
    }

    // OP_BEGIN
    AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

    auto beginPatch = AddU16(0x0);
    auto beginStart = data.size();

    AddU16(beginInfo->opcode);
    
    auto blockPatch = AddU32(0x0);

    // Add param shit here?
    if (stmt->param.has_value()) {
        auto param = stmt->param.value();

        // Num args
        AddU16(0x1);

        if (beginInfo->params[0].typeID == kParamType_Integer) {
            AddU8('n');
            AddU32(static_cast<uint32_t>(std::get<double>(param.value)));
        }

        // All other instances use ref?
        else {
            // Try to resolve the global ref
            if (auto ref = ResolveObjReference(param.lexeme)) {
                AddU8('r');
                AddU16(ref);
            }
            else {
                throw std::runtime_error(std::format("Unable to resolve form '{}'.", param.lexeme));
            }
        }
    }

    SetU16(beginPatch, data.size() - beginStart);
    auto blockStart = data.size();
    CompileBlock(stmt->block, true);

    // OP_END
    AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::End));

    SetU32(blockPatch, data.size() - blockStart);
}

void NVSECompiler::VisitFnStmt(FnDeclStmt* stmt) {
    // OP_BEGIN
    AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

    // OP_MODE_LEN
    auto opModeLenPatch = AddU16(0x0);
    auto opModeStart = data.size();

    // OP_MODE
    AddU16(0x0D);

    // SCRIPT_LEN
    auto scriptLenPatch = AddU32(0x0);

    // BYTECODE VER
    AddU8(0x1);

    // arg count
    AddU8(stmt->args.size());

    // add args
    for (auto i = 0; i < stmt->args.size(); i++) {
        auto& arg = stmt->args[i];
        auto type = GetScriptTypeFromToken(arg->type);
        auto localIdx = AddLocal(std::get<0>(arg->values[0]).lexeme, type);

        AddU16(localIdx);
        AddU8(type);
    }

    // NUM_ARRAY_ARGS, always 0
    AddU8(0x0);

    auto scriptLenStart = data.size();

    // Patch mode len
    SetU16(opModeLenPatch, data.size() - opModeStart);

    // Compile script
    CompileBlock(stmt->body, false);

    // OP_END
    AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::End));

    SetU32(scriptLenPatch, data.size() - scriptLenStart);
}

void NVSECompiler::VisitVarDeclStmt(const VarDeclStmt* stmt) {
    uint8_t varType = GetScriptTypeFromToken(stmt->type);
    
    // Since we are doing multiple declarations at once, manually handle count here
    statementCounter.top()--;

    for (auto [token, value] : stmt->values) {
        auto name = token.lexeme;
        
        // Compile lambdas differently
        // Does not affect params as they cannot have value specified
        if (value->IsType<LambdaExpr>()) {
            // To do a similar thing on access
            lambdaVars.insert(name);

            StartCall(OP_SET_MOD_LOCAL_DATA);
            StartManualArg();
            AddString("__" + scriptName + "__" + name);
            FinishManualArg();
            AddCallArg(value);
            FinishCall();
            continue;
        }

        const auto idx = AddLocal(name, varType);
        if (!value) {
            continue;
        }

        statementCounter.top()++;

        StartCall(OP_LET);
        StartManualArg();

        // Add arg to stack
        AddU8('V');
        AddU8(varType);
        AddU16(0); // SCRV
        AddU16(idx); // SCDA

        // Build expression
        value->Accept(this);

        // OP_ASSIGN
        AddU8(0);

        FinishManualArg();
        FinishCall();
    }
}

void NVSECompiler::VisitExprStmt(const ExprStmt* stmt) {
    // These do not need to be wrapped in op_eval
    if (stmt->expr->IsType<CallExpr>() || stmt->expr->IsType<AssignmentExpr>()) {
        stmt->expr->Accept(this);
    }
    else if (stmt->expr) {
        StartCall(OP_EVAL);
        AddCallArg(stmt->expr);
        FinishCall();
    } else {
	    // Decrement counter as this is a NOP
        statementCounter.top()--;
    }
}

void NVSECompiler::VisitForStmt(ForStmt* stmt) {
    if (stmt->init) {
        stmt->init->Accept(this);
        statementCounter.top()++;
    }

    // Store loop increment to be generated before OP_LOOP/OP_CONTINUE
    if (stmt->post) {
        loopIncrements.push(std::make_shared<ExprStmt>(stmt->post));
    } else {
        loopIncrements.push(nullptr);
    }

    // This is pretty much a copy of the while loop code,
    // but it does something slightly different with the loopIncrements stack
    
    // OP_WHILE
    AddU16(OP_WHILE);

    // Placeholder OP_LEN
    auto exprPatch = AddU16(0x0);
    auto exprStart = data.size();

    auto jmpPatch = AddU32(0x0);

    // 1 param
    AddU8(0x1);

    // Compile / patch condition
    auto condStart = data.size();
    auto condPatch = AddU16(0x0);
    insideNvseExpr.push(true);
    stmt->cond->Accept(this);
    insideNvseExpr.pop();
    SetU16(condPatch, data.size() - condStart);

    // Patch OP_LEN
    SetU16(exprPatch, data.size() - exprStart);

    // Compile block
    CompileBlock(stmt->block, true);

    // If we have a loop increment (for loop)
    // Emit that before OP_LOOP
    if (loopIncrements.top()) {
        loopIncrements.top()->Accept(this);
        statementCounter.top()++;
    }
    
    // OP_LOOP
    AddU16(OP_LOOP);
    AddU16(0x0);

    // OP_LOOP is an extra statement
    statementCounter.top()++;

    // Patch jmp
    SetU32(jmpPatch, data.size());

    loopIncrements.pop();
}

void NVSECompiler::VisitForEachStmt(ForEachStmt* stmt) {
    // Compile initializer
    stmt->lhs->Accept(this);
    statementCounter.top()++;

    // Get variable info
    auto varDecl = dynamic_cast<VarDeclStmt*>(stmt->lhs.get());
    if (!varDecl) {
        throw std::runtime_error("Unexpected compiler error.");
    }

    auto varInfo = engineScript->GetVariableByName(std::get<0>(varDecl->values[0]).lexeme.c_str());
    if (!varInfo) {
        throw std::runtime_error("Unexpected compiler error.");
    }

    // OP_FOREACH
    AddU16(OP_FOREACH);

    // Placeholder OP_LEN
    auto exprPatch = AddU16(0x0);
    auto exprStart = data.size();

    auto jmpPatch = AddU32(0x0);

    // Num args
    AddU8(0x1);

    // Arg 1 len
    auto argStart = data.size();
    auto argPatch = AddU16(0x0);

    insideNvseExpr.push(true);
    AddU8('V');
    AddU8(varInfo->type);
    AddU16(0x0);
    AddU16(varInfo->idx);
    stmt->rhs->Accept(this);
    AddU8(kOpType_In);
    insideNvseExpr.pop();

    SetU16(argPatch, data.size() - argStart);
    SetU16(exprPatch, data.size() - exprStart);

    loopIncrements.push(nullptr);
    CompileBlock(stmt->block, true);
    loopIncrements.pop();

    // OP_LOOP
    AddU16(OP_LOOP);
    AddU16(0x0);
    statementCounter.top()++;

    // Patch jmp
    SetU32(jmpPatch, data.size());
}

void NVSECompiler::VisitIfStmt(IfStmt* stmt) {
    // OP_IF
    AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::If));

    // Placeholder OP_LEN
    auto exprPatch = AddU16(0x0);
    auto exprStart = data.size();

    // Placeholder JMP_OPS
    auto jmpPatch = AddU16(0x0);

    // Placeholder EXP_LEN
    auto compPatch = AddU16(0x0);
    auto compStart = data.size();

    // OP_PUSH
    AddU8(0x20);

    // Enter NVSE eval
    AddU8(0x58);
    StartCall(OP_EVAL);
    AddCallArg(stmt->cond);
    FinishCall();

    // Patch lengths
    SetU16(compPatch, data.size() - compStart);
    SetU16(exprPatch, data.size() - exprStart);

    // Patch JMP_OPS
    SetU16(jmpPatch, CompileBlock(stmt->block, true));

    // Build OP_ELSE
    if (stmt->elseBlock) {
        statementCounter.top()++;
        
        // OP_ELSE
        AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Else));

        // OP_LEN
        AddU16(0x02);

        // Placeholder JMP_OPS
        auto elsePatch = AddU16(0x0);

        // Compile else block
        SetU16(elsePatch, CompileBlock(stmt->elseBlock, true));
    }

    // OP_ENDIF
    statementCounter.top()++;
    AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::EndIf));
    AddU16(0x0);
}

void NVSECompiler::VisitReturnStmt(ReturnStmt* stmt) {
    // Compile SetFunctionValue if we have a return value
    if (stmt->expr) {
        StartCall(OP_SET_FUNCTION_VALUE);
        AddCallArg(stmt->expr);
        FinishCall();
    }

    // Emit op_return
    AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::Return));
}

void NVSECompiler::VisitContinueStmt(ContinueStmt* stmt) {
    if (loopIncrements.top()) {
        loopIncrements.top()->Accept(this);
        statementCounter.top()++;
    }

    AddU32(0x153E);
}

void NVSECompiler::VisitBreakStmt(BreakStmt* stmt) {
    AddU32(0x153F);
}

void NVSECompiler::VisitWhileStmt(const WhileStmt* stmt) {
    // OP_WHILE
    AddU16(OP_WHILE);

    // Placeholder OP_LEN
    auto exprPatch = AddU16(0x0);
    auto exprStart = data.size();

    auto jmpPatch = AddU32(0x0);

    // 1 param
    AddU8(0x1);

    // Compile / patch condition
    auto condStart = data.size();
    auto condPatch = AddU16(0x0);
    insideNvseExpr.push(true);
    stmt->cond->Accept(this);
    insideNvseExpr.pop();
    SetU16(condPatch, data.size() - condStart);

    // Patch OP_LEN
    SetU16(exprPatch, data.size() - exprStart);

    // Compile block
    loopIncrements.push(nullptr);
    CompileBlock(stmt->block, true);
    loopIncrements.pop();
    
    // OP_LOOP
    AddU16(OP_LOOP);
    AddU16(0x0);
    statementCounter.top()++;

    // Patch jmp
    SetU32(jmpPatch, data.size());
}

void NVSECompiler::VisitBlockStmt(BlockStmt* stmt) {
    for (auto& statement : stmt->statements) {
        statement->Accept(this);
        statementCounter.top()++;
    }
}

void NVSECompiler::VisitAssignmentExpr(AssignmentExpr* expr) {
    // Assignment as standalone statement
    if (!insideNvseExpr.top()) {
        StartCall(OP_LET);
        StartManualArg();
        expr->left->Accept(this);
        expr->expr->Accept(this);
        AddU8(tokenOpToNVSEOpType[expr->token.type]);
        FinishManualArg();
        FinishCall();
    }

    // Assignment as an expression
    else {
        // Build expression
        expr->left->Accept(this);
        expr->expr->Accept(this);

        // Assignment opcode
        AddU8(tokenOpToNVSEOpType[expr->token.type]);
    }
}

void NVSECompiler::VisitTernaryExpr(TernaryExpr* expr) {
    StartCall(OP_TERNARY);
    AddCallArg(expr->cond);
    AddCallArg(expr->left);
    AddCallArg(expr->right);
    FinishCall();
}

void NVSECompiler::VisitInExpr(InExpr* expr) {
    // Value list provided
    if (!expr->values.empty()) {
        StartCall(OP_MATCHES_ANY);
        AddCallArg(expr->lhs);
        for (auto arg : expr->values) {
            AddCallArg(arg);
        }
        FinishCall();
    }
    // Array
	else {
        StartCall(OP_AR_FIND);
        AddCallArg(expr->lhs);
        AddCallArg(expr->expression);
        FinishCall();
        StartCall(OP_AR_BAD_NUMERIC_INDEX);
        FinishCall();
        AddU8(tokenOpToNVSEOpType[NVSETokenType::BangEq]);
    }
}

void NVSECompiler::VisitBinaryExpr(BinaryExpr* expr) {
    expr->left->Accept(this);
    expr->right->Accept(this);
    AddU8(tokenOpToNVSEOpType[expr->op.type]);
}

void NVSECompiler::VisitUnaryExpr(UnaryExpr* expr) {
    if (expr->postfix) {
        // Slight hack to get postfix operators working
        expr->expr->Accept(this);
        expr->expr->Accept(this);
        AddU8('B');
        AddU8(1);
        if (expr->op.type == NVSETokenType::PlusPlus) {
            AddU8(tokenOpToNVSEOpType[NVSETokenType::Plus]);
        }
        else {
            AddU8(tokenOpToNVSEOpType[NVSETokenType::Minus]);
        }
        AddU8(tokenOpToNVSEOpType[NVSETokenType::Eq]);

        AddU8('B');
        AddU8(1);
        if (expr->op.type == NVSETokenType::PlusPlus) {
            AddU8(tokenOpToNVSEOpType[NVSETokenType::Minus]);
        }
        else {
            AddU8(tokenOpToNVSEOpType[NVSETokenType::Plus]);
        }
    }
    else {
        expr->expr->Accept(this);
        AddU8(tokenOpToNVSEOpType[expr->op.type]);
    }
}

void NVSECompiler::VisitSubscriptExpr(SubscriptExpr* expr) {
    expr->left->Accept(this);
    expr->index->Accept(this);
    AddU8(tokenOpToNVSEOpType[NVSETokenType::LeftBracket]);
}

void NVSECompiler::VisitCallExpr(CallExpr* expr) {
    // Handle call command separately, unique parse function
    if (expr->cmdInfo->parse == kCommandInfo_Call.parse) {
        if (insideNvseExpr.top()) {
            AddU8('X');
            AddU16(0x0); // SCRV
        }

        AddU16(OP_CALL);
        auto callLengthStart = data.size() + (insideNvseExpr.top() ? 0 : 2);
        auto callLengthPatch = AddU16(0x0);
        insideNvseExpr.push(true);

        // Bytecode ver
        AddU8(1);

        auto exprLengthStart = data.size();
        auto exprLengthPatch = AddU16(0x0);

        // Resolve identifier
        expr->args[0]->Accept(this);
        SetU16(exprLengthPatch, data.size() - exprLengthStart);

        // Add args
        AddU8(expr->args.size() - 1);
        for (int i = 1; i < expr->args.size(); i++) {
            auto argStartInner = data.size();
            auto argPatchInner = AddU16(0x0);
            expr->args[i]->Accept(this);
            SetU16(argPatchInner, data.size() - argStartInner);
        }

        SetU16(callLengthPatch, data.size() - callLengthStart);

        insideNvseExpr.pop();
        return;
    }

    // See if we should wrap inside let
    // Need to do this in the case of something like
    // player.AddItem(Caps001, 10) to pass player on stack and use 'x'
    // annoying but want to keep the way these were passed consistent, especially in case of
    // Quest.target.AddItem()
    if (expr->left && !insideNvseExpr.top()) {
        // OP_EVAL
        StartCall(OP_EVAL, expr->left);
        AddCallArg(static_cast<ExprPtr>(expr));
        FinishCall();
        return;
    }

    StartCall(expr->cmdInfo, expr->left);
    for (auto arg : expr->args) {
        AddCallArg(arg);
    }
    FinishCall();
}

void NVSECompiler::VisitGetExpr(GetExpr* expr) {
    const auto refIdx = ResolveObjReference(expr->referenceName);
    if (!refIdx) {
        throw std::runtime_error(std::format("Unable to resolve reference '{}'.", expr->referenceName));
    }

    // Put ref on stack
    AddU8('V');
    AddU8(expr->varInfo->type);
    AddU16(refIdx);
    AddU16(expr->varInfo->idx);
}

void NVSECompiler::VisitBoolExpr(BoolExpr* expr) {
    AddU8('B');
    AddU8(expr->value);
}

void NVSECompiler::VisitNumberExpr(NumberExpr* expr) {
    if (expr->isFp) {
        AddF64(expr->value);
    }
    else {
        if (expr->value <= UINT8_MAX) {
            AddU8('B');
            AddU8(static_cast<uint8_t>(expr->value));
        }
        else if (expr->value <= UINT16_MAX) {
            AddU8('I');
            AddU16(static_cast<uint16_t>(expr->value));
        }
        else {
            AddU8('L');
            AddU32(static_cast<uint32_t>(expr->value));
        }
    }
}

void NVSECompiler::VisitStringExpr(StringExpr* expr) {
    AddString(std::get<std::string>(expr->token.value));
}

void NVSECompiler::VisitIdentExpr(IdentExpr* expr) {
    auto name = expr->token.lexeme;
    usedVars.emplace(name);

    // If this is a lambda var, inline it as a call to GetModLocalData
    if (lambdaVars.contains(name)) {
        StartCall(OP_GET_MOD_LOCAL_DATA);
        StartManualArg();
        AddString("__" + scriptName + "__" + name);
        FinishManualArg();
        FinishCall();
        return;
    }

    // Attempt to resolve as variable
    if (auto local = engineScript->varList.GetVariableByName(name.c_str())) {
        AddU8('V');
        AddU8(local->type);
        AddU16(0);
        AddU16(local->idx);
        return;
    }

    // Try to look up as global
    if (auto refIdx = ResolveObjReference(name)) {
        AddU8('R');
        AddU16(refIdx);
    }
}

void NVSECompiler::VisitMapLiteralExpr(MapLiteralExpr* expr) {
    StartCall(OP_AR_MAP);
    for (const auto& val : expr->values) {
        AddCallArg(val);
    }
    FinishCall();
}

void NVSECompiler::VisitArrayLiteralExpr(ArrayLiteralExpr* expr) {
    StartCall(OP_AR_MAP);
    for (const auto &val : expr->values) {
        AddCallArg(val);
    }
    FinishCall();
}

void NVSECompiler::VisitGroupingExpr(GroupingExpr* expr) {
    expr->expr->Accept(this);
}

void NVSECompiler::VisitLambdaExpr(LambdaExpr* expr) {
    // PARAM_LAMBDA
    AddU8('F');

    // Arg size
    auto argSizePatch = AddU32(0x0);
    auto argStart = data.size();

    // Compile lambda
    {
        // SCN
        AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

        // OP_BEGIN
        AddU16(0x10);

        // OP_MODE_LEN
        auto opModeLenPatch = AddU16(0x0);
        auto opModeStart = data.size();

        // OP_MODE
        AddU16(0x0D);

        // SCRIPT_LEN
        auto scriptLenPatch = AddU32(0x0);

        // BYTECODE VER
        AddU8(0x1);

        // arg count
        AddU8(expr->args.size());

        // add args
        for (auto i = 0; i < expr->args.size(); i++) {
            auto& arg = expr->args[i];
            auto type = GetScriptTypeFromToken(arg->type);
            auto localIdx = AddLocal(std::get<0>(arg->values[0]).lexeme, type);

            AddU16(localIdx);
            AddU8(type);
        }

        // NUM_ARRAY_ARGS, always 0
        AddU8(0x0);

        auto scriptLenStart = data.size();

        // Patch mode len
        SetU16(opModeLenPatch, data.size() - opModeStart);

        // Compile script
        insideNvseExpr.push(false);
        CompileBlock(expr->body, false);
        insideNvseExpr.pop();

        // OP_END
        AddU32(0x11);

        // Different inside of lambda for some reason?
        SetU32(scriptLenPatch, data.size() - scriptLenStart);
    }

    SetU32(argSizePatch, data.size() - argStart);
}

uint32_t NVSECompiler::CompileBlock(StmtPtr stmt, bool incrementCurrent) {
    for (int i = 0; i < statementCounter.size(); i++) {
        CompDbg("  ");
    }
    CompDbg("Entering block\n");
    
    // Get sub-block statement count
    statementCounter.push(0);
    stmt->Accept(this);
    auto newStatementCount = statementCounter.top();
    statementCounter.pop();

    // Lambdas do not add to parent block stmt count
    // Others do
    if (incrementCurrent) {
        statementCounter.top() += newStatementCount;
    }

    for (int i = 0; i < statementCounter.size(); i++) {
        CompDbg("  ");
    }
    CompDbg("Exiting Block. Size %d\n", newStatementCount);

    return newStatementCount;
}

void NVSECompiler::FinishCall() {
    auto buf = callBuffers.top();
    callBuffers.pop();

    SetU16(buf.startPatch, data.size() - buf.startPos);

    if (isDefaultParse(*buf.parse)) {
        SetU16(buf.argPos, buf.numArgs);
    }
    else {
        SetU8(buf.argPos, buf.numArgs);
    }

    // Handle stack refs
    if (buf.stackRef) {
        AddU8(tokenOpToNVSEOpType[NVSETokenType::Dot]);
    }
}

void NVSECompiler::StartCall(CommandInfo* cmd, ExprPtr stackRef) {
    if (const auto plugin = g_scriptCommands.GetParentPlugin(cmd)) {
        requirements.insert(std::string(plugin->name));
    }

    // Handle stack refs
    if (stackRef) {
        stackRef->Accept(this);
    }

    if (insideNvseExpr.top()) {
        if (stackRef) {
            AddU8('x');
        }
        else {
            AddU8('X');
        }
        AddU16(0x0); // SCRV
    }
    AddU16(cmd->opcode);

    // Call size
    CallBuffer buf{};
    buf.parse = &cmd->parse;
    buf.stackRef = stackRef;
    buf.startPos = data.size() + (insideNvseExpr.top() ? 0 : 2);
    buf.startPatch = AddU16(0x0);

    if (isDefaultParse(cmd->parse)) {
        buf.argPos = AddU16(0x0);
    } else {
        buf.argPos = AddU8(0x0);
    }

    callBuffers.push(buf);
}

void NVSECompiler::StartCall(const std::string &&command, ExprPtr stackRef) {
    StartCall(g_scriptCommands.GetByName(command.c_str()), stackRef);
}

void NVSECompiler::StartCall(uint16_t opcode, ExprPtr stackRef) {
    StartCall(g_scriptCommands.GetByOpcode(opcode), stackRef);
}

void NVSECompiler::AddCallArg(ExprPtr arg) {
    // Make NVSE aware
    if (isDefaultParse(*callBuffers.top().parse)) {
        AddU16(0xFFFF);
    }

    auto argStartInner = data.size();
    auto argPatchInner = AddU16(0x0);

    insideNvseExpr.push(true);
    arg->Accept(this);
    insideNvseExpr.pop();

    SetU16(argPatchInner, data.size() - argStartInner);
    callBuffers.top().numArgs++;
}

void NVSECompiler::StartManualArg() {
    // Make NVSE aware
    if (isDefaultParse(*callBuffers.top().parse)) {
        AddU16(0xFFFF);
    }

    callBuffers.top().argStart = data.size();
    callBuffers.top().argPatch = AddU16(0x0);
    insideNvseExpr.push(true);
}

void NVSECompiler::FinishManualArg() {
    insideNvseExpr.pop();
    SetU16(callBuffers.top().argPatch, data.size() - callBuffers.top().argStart);
    callBuffers.top().numArgs++;
}