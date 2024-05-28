#include "NVSECompiler.h"
#include "Commands_MiscRef.h"
#include "Commands_Scripting.h"
#include "NVSECompilerUtils.h"
#include "NVSEParser.h"

bool NVSECompiler::Compile() {
    CompDbg("\n==== COMPILER ====\n\n");
    
    insideNvseExpr.push(false);
    loopIncrements.push(nullptr);
    statementCounter.push(0);
    
    ast.Accept(this);

    if (!partial) {
        script->SetEditorID(scriptName.c_str());
    }

    script->info.compiled = true;
    script->info.dataLength = data.size();
    script->info.numRefs = script->refList.Count();
    script->info.varCount = script->varList.Count();
    script->info.unusedVariableCount = script->info.varCount - usedVars.size();
    script->data = static_cast<uint8_t*>(FormHeap_Allocate(data.size()));
    memcpy(script->data, data.data(), data.size());

    // Debug print local info
    CompDbg("\n[Locals]\n\n");
    for (int i = 0; i < script->varList.Count(); i++) {
        auto item = script->varList.GetNthItem(i);
        CompDbg("%i: %s %s\n", item->idx, item->name.CStr(), usedVars.contains(item->name.CStr()) ? "" : "(unused)");
    }
    
    CompDbg("\n");

    // Refs
    CompDbg("[Refs]\n\n");
    for (int i = 0; i < script->refList.Count(); i++) {
        const auto ref = script->refList.GetNthItem(i);
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
    for (int i = 0; i < script->info.dataLength; i++) {
        CompDbg("%02X ", script->data[i]);
    }
    
    CompDbg("\n");
    CompDbg("\nNum compiled bytes: %d\n", script->info.dataLength);

    return true;
}

void NVSECompiler::VisitNVSEScript(const NVSEScript* nvScript) {
    // Compile the script name
    scriptName = nvScript->name.lexeme;

    // Dont allow naming script the same as another form, unless that form is the script itself
    auto comp = strcmp(scriptName.c_str(), originalScriptName);
    if (ResolveObjReference(scriptName, false) && comp && !partial) {
        throw std::runtime_error(std::format("Error: Form name '{}' is already in use.\n", scriptName.c_str()));
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

        AddU16(0xFFFF);
        auto varStart = data.size();
        auto varPatch = AddU16(0x0);

        if (beginInfo->params[0].typeID == kParamType_Integer) {
            AddU8('L');
            AddU32(static_cast<uint32_t>(std::get<double>(param.value)));
        }

        // All other instances use ref?
        else {
            // Try to resolve the global ref
            auto ref = ResolveObjReference(param.lexeme);
            if (ref) {
                auto refInfo = script->refList.GetNthItem(ref - 1);
                printf("Resolved ref %s: %08x\n", param.lexeme.c_str(), refInfo->form->refID);
                AddU8('R');
                AddU16(ref);
            }
            else {
                throw std::runtime_error(std::format("Unable to resolve form '{}'.", param.lexeme));
            }
        }

        SetU16(varPatch, data.size() - varStart);
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
        // Compile lambdas differently
        // Does not affect params as they cannot have value specified
        if (value->IsType<LambdaExpr>()) {
            // To do a similar thing on access
            lambdaVars.insert(token.lexeme);

            // <scriptName>__lambdaName
            const auto name = scriptName + "__" + token.lexeme;

            // Build a call expr
            NVSEToken nameToken{};
            nameToken.value = name;
            auto arg = std::make_shared<StringExpr>(nameToken);

            std::vector<ExprPtr> args{};
            args.emplace_back(arg);
            args.emplace_back(value);
            auto expr = std::make_shared<CallExpr>(nullptr, NVSEToken {NVSETokenType::Identifier, "SetModLocalData" }, std::move(args));

            expr->Accept(this);
            continue;
        }

        const auto idx = AddLocal(token.lexeme, varType);
        if (!value) {
            continue;
        }

        statementCounter.top()++;

        // OP_LET
        AddU16(0x1539);

        //  Placeholder [expr_len]
        auto exprPatch = AddU16(0x0);
        auto exprStart = data.size();

        // Num args
        AddU8(0x1);

        // Placeholder [param1_len]
        auto argStart = data.size();
        auto argPatch = AddU16(0x0);

        // Add arg to stack
        AddU8('V');
        AddU8(varType);
        AddU16(0); // SCRV
        AddU16(idx); // SCDA

        // Build expression
        insideNvseExpr.push(true);
        value->Accept(this);
        insideNvseExpr.pop();

        // OP_ASSIGN
        AddU8(0);

        // Patch lengths
        SetU16(argPatch, data.size() - argStart);
        SetU16(exprPatch, data.size() - exprStart);
    }
}

void NVSECompiler::VisitExprStmt(const ExprStmt* stmt) {
    // These do not need to be wrapped in op_eval
    if (stmt->expr->IsType<CallExpr>() || stmt->expr->IsType<AssignmentExpr>()) {
        stmt->expr->Accept(this);
    }
    else if (stmt->expr) {
        // OP_EVAL
        AddU16(0x153A);

        // Placeholder OP_LEN
        auto exprPatch = AddU16(0x0);
        auto exprStart = data.size();

        // Num args
        AddU8(0x1);

        // Placeholder [arg_len]
        auto argStart = data.size();
        auto argPatch = AddU16(0x0);

        //Build expression
        insideNvseExpr.push(true);
        stmt->expr->Accept(this);
        insideNvseExpr.pop();

        // Patch lengths
        SetU16(argPatch, data.size() - argStart);
        SetU16(exprPatch, data.size() - exprStart);
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
    AddU16(0x153B);

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
    AddU16(0x153c);
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

    auto varInfo = script->GetVariableByName(std::get<0>(varDecl->values[0]).lexeme.c_str());
    if (!varInfo) {
        throw std::runtime_error("Unexpected compiler error.");
    }

    // OP_FOREACH
    AddU16(0x153D);

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
    AddU16(0x153c);
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
    {
        // OP_EVAL
        AddU16(0x153A);

        // Placeholder OP_EVAL_LEN
        auto opEvalPatch = AddU16(0x0);
        auto opEvalStart = data.size();

        // OP_EVAL num params
        AddU8(0x1);

        // Placeholder OP_EVAL_ARG_LEN
        auto opEvalArgStart = data.size();
        auto opEvalArgPatch = AddU16(0x0);

        // Compile cond
        insideNvseExpr.push(true);
        stmt->cond->Accept(this);
        insideNvseExpr.pop();

        // Patch OP_EVAL lengths
        SetU16(opEvalArgPatch, data.size() - opEvalArgStart);
        SetU16(opEvalPatch, data.size() - opEvalStart);
    }

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
        AddU16(0x1546);

        auto opLenPatch = AddU16(0x0);
        auto opLenStart = data.size();

        AddU8(0x1);

        auto argLenStart = data.size();
        auto argLenPatch = AddU16(0x0);

        insideNvseExpr.push(true);
        stmt->expr->Accept(this);
        insideNvseExpr.pop();

        SetU16(argLenPatch, data.size() - argLenStart);
        SetU16(opLenPatch, data.size() - opLenStart);
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
    AddU16(0x153B);

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
    AddU16(0x153c);
    AddU16(0x0);
    statementCounter.top()++;

    // Patch jmp
    SetU32(jmpPatch, data.size());
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

void NVSECompiler::VisitBlockStmt(BlockStmt* stmt) {
    for (auto& statement : stmt->statements) {
        statement->Accept(this);
        statementCounter.top()++;
    }
}

void NVSECompiler::VisitAssignmentExpr(AssignmentExpr* expr) {
    // Assignment as standalone statement
    if (!insideNvseExpr.top()) {
        // OP_LET
        AddU16(0x1539);

        //  Placeholder [expr_len]
        auto exprPatch = AddU16(0x0);
        auto exprStart = data.size();

        // Num args
        AddU8(0x1);

        // Placeholder [param1_len]
        auto argStart = data.size();
        auto argPatch = AddU16(0x0);

        // Build expression
        insideNvseExpr.push(true);
        expr->left->Accept(this);
        expr->expr->Accept(this);
        insideNvseExpr.pop();

        // Assignment opcode
        AddU8(tokenOpToNVSEOpType[expr->token.type]);

        // Patch lengths
        SetU16(argPatch, data.size() - argStart);
        SetU16(exprPatch, data.size() - exprStart);
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

void NVSECompiler::VisitTernaryExpr(const TernaryExpr* expr) {
    // TODO
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
    const auto stackRefExpr = expr->left;
    const auto cmd = expr->cmdInfo;

    auto defaultParse = isDefaultParse(cmd->parse);

    // See if we should wrap inside let
    // Need to do this in the case of something like
    // player.AddItem(Caps001, 10) to pass player on stack and use 'x'
    // annoying but want to keep the way these were passed consistent, especially in case of
    // Quest.target.AddItem()
    if (stackRefExpr && !insideNvseExpr.top()) {
        // OP_EVAL
        AddU16(0x153A);

        //  Placeholder [expr_len]
        auto exprPatch = AddU16(0x0);
        auto exprStart = data.size();

        // Num args
        AddU8(0x1);

        // Placeholder [param1_len]
        auto argStart = data.size();
        auto argPatch = AddU16(0x0);

        insideNvseExpr.push(true);
        VisitCallExpr(expr);
        insideNvseExpr.pop();
        
        // Patch lengths
        SetU16(argPatch, data.size() - argStart);
        SetU16(exprPatch, data.size() - exprStart);
        return;
    }

    // If call command
    if (cmd->parse == kCommandInfo_Call.parse) {
        if (insideNvseExpr.top()) {
            AddU8('X');
            AddU16(0x0); // SCRV
        }

        AddU16(0x1545);
        auto callLengthStart = data.size();
        auto callLengthPatch = AddU16(0x0);

        // Compiled differently for some reason inside vs outside of calls
        if (!insideNvseExpr.top()) {
            callLengthStart = data.size();
        }
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

    // Handle stack refs
    if (stackRefExpr) {
        stackRefExpr->Accept(this);
    }

    // Put lhs on stack if its a dot expr
    if (insideNvseExpr.top()) {
        if (stackRefExpr) {
            AddU8('x');
        }
        else {  
            AddU8('X');
        }
        AddU16(0x0); // SCRV
    }
    AddU16(cmd->opcode);

    // Call size
    auto callStart = data.size() + (insideNvseExpr.top() ? 0 : 2);
    auto callPatch = AddU16(0x0);

    // Num args
    if (defaultParse) {
        AddU16(expr->args.size());
    } else {
        AddU8(expr->args.size());
    }

    // Args
    insideNvseExpr.push(true);
    for (int i = 0; i < expr->args.size(); i++) {
        // Make NVSE aware
        if (defaultParse) {
            AddU16(0xFFFF);
        }

        auto argStartInner = data.size();
        auto argPatchInner = AddU16(0x0);
        
        expr->args[i]->Accept(this);
        
        SetU16(argPatchInner, data.size() - argStartInner);
    }
    insideNvseExpr.pop();

    SetU16(callPatch, data.size() - callStart);

    // Handle stack refs
    if (stackRefExpr) {
        AddU8(tokenOpToNVSEOpType[NVSETokenType::Dot]);
    }
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
        // <scriptName>__lambdaName
        const auto lambdaName = scriptName + "__" + name;

        // Build a call expr
        NVSEToken nameToken{};
        nameToken.value = lambdaName;
        auto arg = std::make_shared<StringExpr>(nameToken);

        std::vector<ExprPtr> args{};
        args.emplace_back(arg);
        auto getMLDCall = std::make_shared<CallExpr>(nullptr, NVSEToken{ NVSETokenType::Identifier, "GetModLocalData" }, std::move(args));

        getMLDCall->Accept(this);
        return;
    }

    // Attempt to resolve as variable
    if (auto local = script->varList.GetVariableByName(name.c_str())) {
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
