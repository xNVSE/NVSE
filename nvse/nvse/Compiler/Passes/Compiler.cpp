#include "Compiler.h"

#include <assert.h>

#include "nvse/Commands_Array.h"
#include "nvse/Commands_MiscRef.h"
#include "nvse/Commands_Scripting.h"
#include "nvse/Hooks_Script.h"
#include "nvse/PluginAPI.h"

#include <nvse/Compiler/Parser/Parser.h>

#include "nvse/Compiler/Utils.h"

namespace Compiler::Passes {
    enum OPCodes {
        OP_LET = 0x1539,
        OP_EVAL = 0x153A,
        OP_WHILE = 0x153B,
        OP_LOOP = 0x153C,
        OP_FOREACH = 0x153D,
        OP_FOREACH_ALT = 0x1670,
        OP_AR_EXISTS = 0x1671,
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
        OP_CONTINUE = 0x153E,
        OP_BREAK = 0x153F,
        OP_VERSION = 0x1676,
    };

    void Compiler::PrintScriptInfo() {
        // Debug print local info
        DbgPrintln("[Locals]");
        DbgIndent();
        for (int i = 0; i < engineScript->varList.Count(); i++) {
            auto item = engineScript->varList.GetNthItem(i);
            DbgPrintln("%d: %s %s", item->idx, item->name.CStr(), usedVars.contains(item->name.CStr()) ? "" : "(unused)");
        }
        DbgOutdent();

        // Refs
        DbgPrintln("[Refs]");
        DbgIndent();
        for (int i = 0; i < engineScript->refList.Count(); i++) {
            const auto ref = engineScript->refList.GetNthItem(i);
            if (ref->varIdx) {
                DbgPrintln("%d: (Var %d)", i, ref->varIdx);
            }
            else {
                DbgPrintln("%d: %s", i, ref->form->GetEditorID());
            }
        }
        DbgOutdent();

        // Script data  
        DbgPrintln("[Data]");
        DbgIndent();
        for (int i = 0; i < engineScript->info.dataLength; i++) {
            DbgPrint("%02X ", engineScript->data[i]);
        }
        DbgPrint("\n");
        DbgOutdent();

        InfoPrintln("[Requirements]");
        DbgIndent();
        for (const auto& [plugin, version] : ast.m_mpPluginRequirements) {
            if (!_stricmp(plugin.c_str(), "nvse")) {
                InfoPrintln("%s [%d.%d.%d]", plugin.c_str(), version >> 24 & 0xFF, version >> 16 & 0xFF, version >> 4 & 0xFF);
            }
            else {
                InfoPrintln("%s [%d]", plugin.c_str(), version);
            }
        }
        DbgOutdent();

        DbgPrintln("\nNum compiled bytes: %d", engineScript->info.dataLength);
    }

    void Compiler::Visit(AST* nvScript) {
        DbgPrintln("[Compilation]");
        DbgIndent();

        insideNvseExpr.push(false);
        loopIncrements.push(nullptr);
        statementCounter.push(0);
        scriptStart.push(0);

        // Compile the script name
        scriptName = nvScript->name;

        // Dont allow naming script the same as another form, unless that form is the script itself
        const auto comp = strcmp(scriptName.c_str(), originalScriptName);
        if (ResolveObjReference(scriptName, false) && comp && !partial) {
            //TODO throw std::runtime_error(std::format("Error: Form name '{}' is already in use.\n", scriptName));
        }

        // SCN
        AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

        // Add script requirement opcodes
        for (const auto& [plugin, version] : ast.m_mpPluginRequirements) {
            StartCall(OP_VERSION);
            StartManualArg();
            AddString(plugin);
            FinishManualArg();
            StartManualArg();
            AddU8('L');
            AddU32(version);
            FinishManualArg();
            FinishCall();
        }

        for (auto& global_var : nvScript->globalVars) {
            global_var->Accept(this);
        }

        for (auto& block : nvScript->blocks) {
            block->Accept(this);
        }

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

        PrintScriptInfo();

        DbgOutdent();
    }

    void Compiler::VisitBeginStmt(Statements::Begin* stmt) {
        auto name = stmt->name;

        // Shouldn't be null
        const CommandInfo* beginInfo = stmt->beginInfo;

        // OP_BEGIN
        AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

        auto beginPatch = AddU16(0x0);
        auto beginStart = data.size();

        AddU16(beginInfo->opcode);

        const auto blockPatch = AddU32(0x0);

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
        else {
            if (stmt->beginInfo->numParams > 0) {
                AddU16(0x0);
            }
        }

        SetU16(beginPatch, data.size() - beginStart);
        const auto blockStart = data.size();

        CompileBlock(stmt->block, true);

        // OP_END
        AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::End));

        SetU32(blockPatch, data.size() - blockStart);
    }

    void Compiler::VisitFnStmt(Statements::UDFDecl* stmt) {
        // OP_BEGIN
        AddU16(static_cast<uint16_t>(ScriptParsing::ScriptStatementCode::Begin));

        // OP_MODE_LEN
        const auto opModeLenPatch = AddU16(0x0);
        const auto opModeStart = data.size();

        // OP_MODE
        AddU16(0x0D);

        // SCRIPT_LEN
        auto scriptLenPatch = AddU32(0x0);

        // BYTECODE VER
        AddU8(0x1);

        // arg count
        AddU8(stmt->args.size());

        // add args
        for (const auto& arg : stmt->args) {
            for (auto& [_, __, var] : arg->declarations) {
                AddU16(var->index);
                AddU8(var->type);
            }
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

    void Compiler::VisitVarDeclStmt(Statements::VarDecl* stmt) {
        const uint8_t varType = GetScriptTypeFromTokenType(stmt->type);

        // Since we are doing multiple declarations at once, manually handle count here
        statementCounter.top()--;

        for (const auto& [token, value, info] : stmt->declarations) {
            // Empty var declarations don't need to be compiled
        	if (!value) {
                continue;
            }

            statementCounter.top()++;

            StartCall(OP_LET);
            StartManualArg();

            // Add arg to stack
            AddU8('V');
            AddU8(varType);
            AddU16(0);
            AddU16(static_cast<uint16_t>(info->index));

            // Build expression
            value->Accept(this);

            // OP_ASSIGN
            AddU8(0);

            FinishManualArg();
            FinishCall();
        }
    }

    void Compiler::VisitExprStmt(Statements::ExpressionStatement* stmt) {
        // These do not need to be wrapped in op_eval
        if (stmt->expr->IsType<Expressions::CallExpr>() || stmt->expr->IsType<Expressions::AssignmentExpr>()) {
            stmt->expr->Accept(this);
        }
        else if (stmt->expr) {
            StartCall(OP_EVAL);
            AddCallArg(stmt->expr);
            FinishCall();
        }
        else {
            throw std::runtime_error("Compiler::VisitExprStmt was called on an empty expression statement");
        }
    }

    void Compiler::VisitForStmt(Statements::For* stmt) {
        throw std::runtime_error("Compiler::VisitForStmt was called");
    }

    void Compiler::VisitForEachStmt(Statements::ForEach* stmt) {
        if (stmt->decompose) {
            // Try to resolve second var if there is one
            std::shared_ptr<VarInfo> var1 = nullptr;
            if (stmt->declarations[0]) {
                var1 = stmt->declarations[0]->declarations[0].info;
            }

            // Try to resolve second var if there is one
            std::shared_ptr<VarInfo> var2 = nullptr;
            if (stmt->declarations.size() == 2 && stmt->declarations[1]) {
                var2 = stmt->declarations[1]->declarations[0].info;
            }

            // OP_FOREACH
            AddU16(OP_FOREACH_ALT);

            // Placeholder OP_LEN
            const auto exprPatch = AddU16(0x0);
            const auto exprStart = data.size();
            const auto jmpPatch = AddU32(0x0);

            // Num args
            // Either 2 or 3, depending on:
            // "for ([int iKey, _] in { "1"::2 "3"::4 }" - 3 args, even though one can be an invalid variable
            // VS:
            // "for ([int iKey] in { "1"::2 "3"::4 }" - 2 args
            AddU8(1 + stmt->declarations.size());
            insideNvseExpr.push(true);

            // Arg 1 - the source array
            auto argStart = data.size();
            auto argPatch = AddU16(0x0);
            stmt->rhs->Accept(this);
            SetU16(argPatch, data.size() - argStart);
            SetU16(exprPatch, data.size() - exprStart);

            // Arg 2
            // If there's two vars (including "_"), the 2nd arg for ForEachAlt is the valueIter.
            // However, our syntax has key arg first: "for ([int iKey, int iValue] in { "1"::2 "3"::4 })"
            // Thus pass iValue arg as 2nd arg instead of 3rd, even though it's the 2nd var in the statement.
            if (stmt->declarations.size() == 2) {
                argStart = data.size();
                argPatch = AddU16(0x0);

                // Arg 2 - valueIterVar
                if (var2) {
                    AddU8('V');
                    AddU8(var2->type);
                    AddU16(0);
                    AddU16(var2->index);
                }
                else {
                    // OptionalEmpty token
                    AddU8('K');
                }
                SetU16(argPatch, data.size() - argStart);
                SetU16(exprPatch, data.size() - exprStart);

                // Arg 3 - pass keyIterVar last 
                argStart = data.size();
                argPatch = AddU16(0x0);

                if (var1) {
                    AddU8('V');
                    AddU8(var1->type);
                    AddU16(0);
                    AddU16(var1->index);
                }
                else {
                    // OptionalEmpty token
                    AddU8('K');
                }
                SetU16(argPatch, data.size() - argStart);
                SetU16(exprPatch, data.size() - exprStart);
            }
            else { // assume 1
                argStart = data.size();
                argPatch = AddU16(0x0);
                AddU8('V');
                AddU8(var1 != nullptr ? var1->type : 0);
                AddU16(0);
                if (var1) {
                    AddU16(var1->index);
                }
                else {
                    AddU16(0);
                }
                SetU16(argPatch, data.size() - argStart);
                SetU16(exprPatch, data.size() - exprStart);
            }

            insideNvseExpr.pop();

            loopIncrements.push(nullptr);
            CompileBlock(stmt->block, true);
            loopIncrements.pop();

            // OP_LOOP
            AddU16(OP_LOOP);
            AddU16(0x0);
            statementCounter.top()++;

            // Patch jmp
            SetU32(jmpPatch, data.size() - scriptStart.top());
        }
        else {
            // Get variable info
            const auto varDecl = stmt->declarations[0];
            if (!varDecl) {
                throw std::runtime_error("Unexpected compiler error.");
            }

            const auto var = varDecl->declarations[0].info;

            // OP_FOREACH
            AddU16(OP_FOREACH);

            // Placeholder OP_LEN
            const auto exprPatch = AddU16(0x0);
            const auto exprStart = data.size();

            const auto jmpPatch = AddU32(0x0);

            // Num args
            AddU8(0x1);

            // Arg 1 len
            const auto argStart = data.size();
            const auto argPatch = AddU16(0x0);

            insideNvseExpr.push(true);
            AddU8('V');
            AddU8(var->type);
            AddU16(0);
            AddU16(var->index);

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
            SetU32(jmpPatch, data.size() - scriptStart.top());
        }
    }

    void Compiler::VisitIfStmt(Statements::If* stmt) {
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

    void Compiler::VisitReturnStmt(Statements::Return* stmt) {
        // Compile SetFunctionValue if we have a return value
        if (stmt->expr) {
            StartCall(OP_SET_FUNCTION_VALUE);
            AddCallArg(stmt->expr);
            FinishCall();
        }

        // Emit op_return
        AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::Return));
    }

    void Compiler::VisitContinueStmt(Statements::Continue* stmt) {
        if (loopIncrements.top()) {
            loopIncrements.top()->Accept(this);
            statementCounter.top()++;
        }

        AddU32(OP_CONTINUE);
    }

    void Compiler::VisitBreakStmt(Statements::Break* stmt) {
        AddU32(OP_BREAK);
    }

    void Compiler::VisitWhileStmt(Statements::While* stmt) {
        // OP_WHILE
        AddU16(OP_WHILE);

        // Placeholder OP_LEN
        const auto exprPatch = AddU16(0x0);
        const auto exprStart = data.size();

        const auto jmpPatch = AddU32(0x0);

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
        SetU32(jmpPatch, data.size() - scriptStart.top());
    }

    void Compiler::VisitBlockStmt(Statements::Block* stmt) {
        for (auto& statement : stmt->statements) {
            statement->Accept(this);
            statementCounter.top()++;
        }
    }

    void Compiler::VisitAssignmentExpr(Expressions::AssignmentExpr* expr) {
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

    void Compiler::VisitTernaryExpr(Expressions::TernaryExpr* expr) {
        StartCall(OP_TERNARY);
        AddCallArg(expr->cond);
        AddCallArg(expr->left);
        AddCallArg(expr->right);
        FinishCall();
    }

    void Compiler::VisitInExpr(Expressions::InExpr* expr) {
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
            StartCall(OP_AR_EXISTS);
            AddCallArg(expr->expression);
            AddCallArg(expr->lhs);
            FinishCall();
        }

        if (expr->isNot) {
            AddU8(tokenOpToNVSEOpType[TokenType::Bang]);
        }
    }

    void Compiler::VisitBinaryExpr(Expressions::BinaryExpr* expr) {
        expr->left->Accept(this);
        expr->right->Accept(this);
        AddU8(tokenOpToNVSEOpType[expr->op.type]);
    }

    void Compiler::VisitUnaryExpr(Expressions::UnaryExpr* expr) {
        if (expr->postfix) {
            // Slight hack to get postfix operators working
            expr->expr->Accept(this);
            expr->expr->Accept(this);
            AddU8('B');
            AddU8(1);
            if (expr->op.type == TokenType::PlusPlus) {
                AddU8(tokenOpToNVSEOpType[TokenType::Plus]);
            }
            else {
                AddU8(tokenOpToNVSEOpType[TokenType::Minus]);
            }
            AddU8(tokenOpToNVSEOpType[TokenType::Eq]);

            AddU8('B');
            AddU8(1);
            if (expr->op.type == TokenType::PlusPlus) {
                AddU8(tokenOpToNVSEOpType[TokenType::Minus]);
            }
            else {
                AddU8(tokenOpToNVSEOpType[TokenType::Plus]);
            }
        }
        else {
            expr->expr->Accept(this);
            AddU8(tokenOpToNVSEOpType[expr->op.type]);
        }
    }

    void Compiler::VisitSubscriptExpr(Expressions::SubscriptExpr* expr) {
        expr->left->Accept(this);
        expr->index->Accept(this);
        AddU8(tokenOpToNVSEOpType[TokenType::LeftBracket]);
    }

    void Compiler::VisitCallExpr(Expressions::CallExpr* expr) {
        if (!expr->cmdInfo) {
            throw std::runtime_error("expr->cmdInfo was null. Please report this as a bug.");
        }

        // Handle call command separately, unique parse function
        if (expr->cmdInfo->parse == kCommandInfo_Call.parse) {
            if (insideNvseExpr.top()) {
                AddU8('X');
                AddU16(0x0); // SCRV
            }

            AddU16(OP_CALL);
            const auto callLengthStart = data.size() + (insideNvseExpr.top() ? 0 : 2);
            const auto callLengthPatch = AddU16(0x0);
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
            StartCall(OP_EVAL);
            StartManualArg();
            expr->Accept(this);
            FinishManualArg();
            FinishCall();
            return;
        }

        StartCall(expr->cmdInfo, expr->left);
        for (auto arg : expr->args) {
            auto numExpr = dynamic_cast<Expressions::NumberExpr*>(arg.get());
            if (numExpr && numExpr->enum_len > 0) {
                arg->Accept(this);
                callBuffers.top().numArgs++;
            }
            else {
                AddCallArg(arg);
            }
        }
        FinishCall();
    }

    void Compiler::VisitShowMessageStmt(Statements::ShowMessage* stmt) {
        static uint16_t opcode = g_scriptCommands.GetByName("ShowMessage")->opcode;

        AddU16(opcode);

        const auto callLengthPatch = AddU16(0x0);
        const auto callLengthStart = data.size();
        AddU16(0x1);
        AddU8('r');

        const auto messageRef = ResolveObjReference(stmt->message->str);
        AddU16(messageRef);

        AddU16(stmt->args.size());
        for (const auto& arg : stmt->args) {
            if (arg->varInfo->type == Script::eVarType_Float) {
                AddU8('f');
            }
            else {
                AddU8('s');
            }
            AddU16(arg->varInfo->index);
        }
        AddU32(stmt->message_time);

        SetU16(callLengthPatch, data.size() - callLengthStart);
    }

    void Compiler::VisitGetExpr(Expressions::GetExpr* expr) {
        const auto refIdx = ResolveObjReference(expr->reference_name);
        if (!refIdx) {
            throw std::runtime_error(std::format("Unable to resolve reference '{}'.", expr->reference_name));
        }

        // Put ref on stack
        AddU8('V');
        AddU8(expr->var_info->type);
        AddU16(refIdx);
        AddU16(expr->var_info->idx);
    }

    void Compiler::VisitBoolExpr(Expressions::BoolExpr* expr) {
        AddU8('B');
        AddU8(expr->value);
    }

    void Compiler::VisitNumberExpr(Expressions::NumberExpr* expr) {
        if (expr->is_fp) {
            AddF64(expr->value);
        }
        else if (expr->enum_len) {
            if (expr->enum_len == 1) {
                AddU8(static_cast<uint8_t>(expr->value));
            }
            else {
                AddU16(static_cast<uint16_t>(expr->value));
            }
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

    void Compiler::VisitStringExpr(Expressions::StringExpr* expr) {
        AddString(expr->value);
    }

	void Compiler::VisitIdentExpr(Expressions::IdentExpr* expr) {
        // Resolve in scope
        const auto var = expr->varInfo;
        std::string name;
        if (var) {
            name = var->remapped_name;
        }
        else {
            name = expr->str;
        }
        usedVars.emplace(name);

        // Local variable
        if (var) {
            DbgPrintln("Read from global variable %s", name.c_str());
            AddU8('V');
            AddU8(var->type);
            AddU16(0);
            AddU16(var->index);
        }

        else {
            // Try to look up as object reference
            if (auto refIdx = ResolveObjReference(name)) {
                auto form = engineScript->refList.GetNthItem(refIdx - 1)->form;
                if (form->typeID == kFormType_TESGlobal) {
                    AddU8('G');
                }
                else {
                    AddU8('R');
                }
                AddU16(refIdx);
            }
        }
    }

    void Compiler::VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) {
        StartCall(OP_AR_MAP);
        for (const auto& val : expr->values) {
            AddCallArg(val);
        }
        FinishCall();
    }

    void Compiler::VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) {
        StartCall(OP_AR_LIST);
        for (const auto& val : expr->values) {
            AddCallArg(val);
        }
        FinishCall();
    }

    void Compiler::VisitGroupingExpr(Expressions::GroupingExpr* expr) {
        expr->expr->Accept(this);
    }

    void Compiler::VisitLambdaExpr(Expressions::LambdaExpr* expr) {
        // PARAM_LAMBDA
        AddU8('F');

        // Arg size
        const auto argSizePatch = AddU32(0x0);
        const auto argStart = data.size();

        // Compile lambda
        {
            scriptStart.push(data.size());

            // SCN
            AddU32(static_cast<uint32_t>(ScriptParsing::ScriptStatementCode::ScriptName));

            // OP_BEGIN
            AddU16(0x10);

            // OP_MODE_LEN
            const auto opModeLenPatch = AddU16(0x0);
            const auto opModeStart = data.size();

            // OP_MODE
            AddU16(0x0D);

            // SCRIPT_LEN
            const auto scriptLenPatch = AddU32(0x0);

            // BYTECODE VER
            AddU8(0x1);

            // arg count
            AddU8(expr->args.size());

            // add args
            for (const auto& arg : expr->args) {
                for (const auto& [_, token, info] : arg->declarations) {
                    AddU16(info->index);
                    AddU8(info->type);
                }
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

            scriptStart.pop();
        }

        SetU32(argSizePatch, data.size() - argStart);
    }

    uint32_t Compiler::CompileBlock(StmtPtr stmt, bool incrementCurrent) {
        DbgIndent();
        DbgPrintln("Entering block");
        DbgIndent();

        // Get sub-block statement count
        statementCounter.push(0);
        stmt->Accept(this);
        const auto newStatementCount = statementCounter.top();
        statementCounter.pop();

        // Lambdas do not add to parent block stmt count
        // Others do
        if (incrementCurrent) {
            statementCounter.top() += newStatementCount;
        }

        DbgOutdent();
        DbgPrintln("Exiting Block. Size %d", newStatementCount);
        DbgOutdent();

        return newStatementCount;
    }

    void Compiler::StartCall(CommandInfo* cmd, ExprPtr stackRef) {
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

        if (IsDefaultParse(cmd->parse)) {
            buf.argPos = AddU16(0x0);
        }
        else {
            buf.argPos = AddU8(0x0);
        }

        callBuffers.push(buf);
    }

    void Compiler::FinishCall() {
        auto buf = callBuffers.top();
        callBuffers.pop();

        SetU16(buf.startPatch, data.size() - buf.startPos);

        if (IsDefaultParse(*buf.parse)) {
            SetU16(buf.argPos, buf.numArgs);
        }
        else {
            SetU8(buf.argPos, buf.numArgs);
        }

        // Handle stack refs
        if (buf.stackRef) {
            AddU8(tokenOpToNVSEOpType[TokenType::Dot]);
        }
    }

    void Compiler::StartCall(uint16_t opcode, ExprPtr stackRef) {
        StartCall(g_scriptCommands.GetByOpcode(opcode), stackRef);
    }

    void Compiler::PerformCall(uint16_t opcode) {
        StartCall(opcode);
        FinishCall();
    }

    void Compiler::AddCallArg(ExprPtr arg) {
        // Make NVSE aware
        if (IsDefaultParse(*callBuffers.top().parse)) {
            AddU16(0xFFFF);
        }

        const auto argStartInner = data.size();
        const auto argPatchInner = AddU16(0x0);

        insideNvseExpr.push(true);
        arg->Accept(this);
        insideNvseExpr.pop();

        SetU16(argPatchInner, data.size() - argStartInner);
        callBuffers.top().numArgs++;
    }

    void Compiler::StartManualArg() {
        // Make NVSE aware
        if (IsDefaultParse(*callBuffers.top().parse)) {
            AddU16(0xFFFF);
        }

        callBuffers.top().argStart = data.size();
        callBuffers.top().argPatch = AddU16(0x0);
        insideNvseExpr.push(true);
    }

    void Compiler::FinishManualArg() {
        insideNvseExpr.pop();
        SetU16(callBuffers.top().argPatch, data.size() - callBuffers.top().argStart);
        callBuffers.top().numArgs++;
    }
}