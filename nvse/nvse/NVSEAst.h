#pragma once
#include "NVSELexer.h"
#include "NVSEScope.h"
#include "ScriptTokens.h"
#include "NVSEVisitor.h"

struct VarDeclStmt;
class NVSEVisitor;
struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct NVSEScript {
    NVSEToken name;
    std::vector<StmtPtr> globalVars;
    std::vector<StmtPtr> blocks;
    std::unordered_map<std::string, std::shared_ptr<LambdaExpr>> m_mpFunctions{};

    // Required NVSE plugins
    std::unordered_map<std::string, uint32_t> m_mpPluginRequirements{};

    NVSEScript(NVSEToken name, std::vector<StmtPtr> globalVars, std::vector<StmtPtr> blocks)
	: name(std::move(name)), globalVars(std::move(globalVars)), blocks(std::move(blocks)) {}

    void Accept(NVSEVisitor* visitor) {
        visitor->VisitNVSEScript(this);
    }
};

struct Stmt {
    size_t line = 0;

    // Some statements store type such as return and block statement
    Token_Type detailedType = kTokenType_Invalid;

    // Set later in type checker, used in compiler for local generation
    std::shared_ptr<NVSEScope> scope {};

    virtual ~Stmt() = default;

    virtual void Accept(NVSEVisitor* t) = 0;

    template <typename T>
    bool IsType() {
        return dynamic_cast<T*>(this);
    }
};

struct BeginStmt : Stmt {
    NVSEToken name;
    std::optional<NVSEToken> param;
    StmtPtr block;
    CommandInfo* beginInfo;

    BeginStmt(const NVSEToken& name, std::optional<NVSEToken> param, StmtPtr block, CommandInfo* beginInfo) : name(name),
        param(std::move(param)), block(std::move(block)), beginInfo(beginInfo) {
        line = name.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBeginStmt(this);
    }
};

struct FnDeclStmt : Stmt {
    NVSEToken token;
    std::optional<NVSEToken> name;
    std::vector<std::shared_ptr<VarDeclStmt>> args;
    StmtPtr body;

    FnDeclStmt(const NVSEToken& token, std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : token(token),
        args(std::move(args)), body(std::move(body)) {
        line = token.line;
    }

    FnDeclStmt(const NVSEToken& token, const NVSEToken &name, std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : token(token),
        name(name), args(std::move(args)), body(std::move(body)) {
        line = token.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitFnStmt(this);
    }
};

struct VarDeclStmt : Stmt {
    NVSEToken type;
    std::vector<std::tuple<NVSEToken, ExprPtr>> values{};

    // Set during type checker so that compiler can look up stack index
    std::vector<std::shared_ptr<NVSEScope::ScopeVar>> scopeVars{};

    VarDeclStmt(NVSEToken type, std::vector<std::tuple<NVSEToken, ExprPtr>> values)
        : type(std::move(type)), values(std::move(values)) {}

    VarDeclStmt(NVSEToken type, NVSEToken name, ExprPtr value) : type(std::move(type)) {
        values.emplace_back(name, std::move(value));
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitVarDeclStmt(this);
    }
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitExprStmt(this);
    }
};

struct ForStmt : Stmt {
    StmtPtr init;
    ExprPtr cond;
    ExprPtr post;
    std::shared_ptr<BlockStmt> block;

    ForStmt(StmtPtr init, ExprPtr cond, ExprPtr post, std::shared_ptr<BlockStmt> block) : init(std::move(init)),
        cond(std::move(cond)), post(std::move(post)), block(std::move(block)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitForStmt(this);
    }
};

struct ForEachStmt : Stmt {
    std::vector<std::shared_ptr<VarDeclStmt>> declarations;
    ExprPtr rhs;
    std::shared_ptr<BlockStmt> block;
    bool decompose = false;

    ForEachStmt(std::vector<std::shared_ptr<VarDeclStmt>> declarations, ExprPtr rhs, std::shared_ptr<BlockStmt> block, bool decompose) : declarations(std::move(declarations)), rhs(std::move(rhs)),
        block(std::move(block)), decompose(decompose) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitForEachStmt(this);
    }
};

struct IfStmt : Stmt {
    NVSEToken token;
    ExprPtr cond;
    StmtPtr block;
    StmtPtr elseBlock;

    IfStmt(NVSEToken token, ExprPtr cond, StmtPtr block, StmtPtr elseBlock) : token(std::move(token)),
        cond(std::move(cond)),
        block(std::move(block)),
        elseBlock(std::move(elseBlock)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitIfStmt(this);
    }
};

struct ReturnStmt : Stmt {
    NVSEToken token;
    ExprPtr expr;

    ReturnStmt(NVSEToken token, ExprPtr expr) : token(std::move(token)), expr(std::move(expr)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitReturnStmt(this);
    }
};

struct ContinueStmt : Stmt {
    NVSEToken token;
    ContinueStmt(NVSEToken token) : token(std::move(token)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitContinueStmt(this);
    }
};

struct BreakStmt : Stmt {
    NVSEToken token;
    BreakStmt(NVSEToken token) : token(std::move(token)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBreakStmt(this);
    }
};

struct WhileStmt : Stmt {
    NVSEToken token;
    ExprPtr cond;
    StmtPtr block;

    WhileStmt(NVSEToken token, ExprPtr cond, StmtPtr block) : token(std::move(token)), cond(std::move(cond)),
        block(std::move(block)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitWhileStmt(this);
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    BlockStmt(std::vector<StmtPtr> statements) : statements(std::move(statements)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBlockStmt(this);
    }
};

struct Expr {
    size_t line = 0;
    Token_Type tokenType = kTokenType_Invalid;

    virtual ~Expr() = default;
    virtual void Accept(NVSEVisitor* t) = 0;

    template <typename T>
    bool IsType() {
        return dynamic_cast<T*>(this);
    }
};

struct AssignmentExpr : Expr {
    NVSEToken token;
    ExprPtr left;
    ExprPtr expr;

    AssignmentExpr(NVSEToken token, ExprPtr left, ExprPtr expr) : token(std::move(token)), left(std::move(left)),
        expr(std::move(expr)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitAssignmentExpr(this);
    }
};

struct TernaryExpr : Expr {
    NVSEToken token;
    ExprPtr cond;
    ExprPtr left;
    ExprPtr right;

    TernaryExpr(NVSEToken token, ExprPtr cond, ExprPtr left, ExprPtr right) : token(std::move(token)),
        cond(std::move(cond)),
        left(std::move(left)),
        right(std::move(right)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitTernaryExpr(this);
    }
};

struct InExpr : Expr {
    ExprPtr lhs;
    NVSEToken token;
    std::vector<ExprPtr> values{};
    ExprPtr expression{};
    bool isNot;

    InExpr(ExprPtr lhs, NVSEToken token, std::vector<ExprPtr> exprs, bool isNot) : lhs(std::move(lhs)), token(std::move(token)), values(std::move(exprs)), isNot(isNot) {
        line = this->token.line;
    }

    InExpr(ExprPtr lhs, NVSEToken token, ExprPtr expr, bool isNot) : lhs(std::move(lhs)), token(std::move(token)), expression(std::move(expr)), isNot(isNot) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor *visitor) override {
        visitor->VisitInExpr(this);
    }
};

struct BinaryExpr : Expr {
    NVSEToken op;
    ExprPtr left, right;

    BinaryExpr(NVSEToken token, ExprPtr left, ExprPtr right) : op(std::move(token)), left(std::move(left)),
        right(std::move(right)) {
        line = this->op.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitBinaryExpr(this);
    }
};

struct UnaryExpr : Expr {
    NVSEToken op;
    ExprPtr expr;
    bool postfix;

    UnaryExpr(NVSEToken token, ExprPtr expr, bool postfix) : op(std::move(token)), expr(std::move(expr)),
        postfix(postfix) {
        line = this->op.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitUnaryExpr(this);
    }
};

struct SubscriptExpr : Expr {
    NVSEToken op;

    ExprPtr left;
    ExprPtr index;

    SubscriptExpr(NVSEToken token, ExprPtr left, ExprPtr index) : op(std::move(token)), left(std::move(left)),
        index(std::move(index)) {
        line = this->op.line;
    }

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitSubscriptExpr(this);
    }
};

struct CallExpr : Expr {
    ExprPtr left;
    NVSEToken token;
    std::vector<ExprPtr> args;

    // Set by typechecker
    CommandInfo* cmdInfo = nullptr;

    CallExpr(ExprPtr left, NVSEToken token, std::vector<ExprPtr> args) : left(std::move(left)), token(std::move(token)), args(std::move(args)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitCallExpr(this);
    }
};

struct GetExpr : Expr {
    NVSEToken token;
    ExprPtr left;
    NVSEToken identifier;

    // Resolved in typechecker
    VariableInfo* varInfo = nullptr;
    const char* referenceName = nullptr;

    GetExpr(NVSEToken token, ExprPtr left, NVSEToken identifier) : token(std::move(token)), left(std::move(left)),
        identifier(std::move(identifier)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitGetExpr(this);
    }
};

struct BoolExpr : Expr {
    NVSEToken token;
    bool value;

    BoolExpr(NVSEToken token, bool value) : token(std::move(token)), value(value) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitBoolExpr(this);
    }
};

struct NumberExpr : Expr {
    NVSEToken token;
    double value;
    bool isFp;

    // For some reason axis enum is one byte and the rest are two?
    int enumLen;

    NumberExpr(NVSEToken token, double value, bool isFp, int enumLen = 0) : token(std::move(token)), value(value), isFp(isFp), enumLen(enumLen) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitNumberExpr(this);
    }
};

struct StringExpr : Expr {
    NVSEToken token;

    StringExpr(NVSEToken token) : token(std::move(token)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitStringExpr(this);
    }
};

struct IdentExpr : Expr {
    NVSEToken token;
    TESForm* form;

    // Set during typechecker variable resolution so that compiler can reference
    std::shared_ptr<NVSEScope::ScopeVar> varInfo {nullptr};

    IdentExpr(NVSEToken token) : token(std::move(token)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitIdentExpr(this);
    }
};

struct ArrayLiteralExpr : Expr {
    NVSEToken token;
    std::vector<ExprPtr> values;

    ArrayLiteralExpr(NVSEToken token, std::vector<ExprPtr> values) : token(token), values(values) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        return t->VisitArrayLiteralExpr(this);
    }
};

struct MapLiteralExpr : Expr {
    NVSEToken token;
    std::vector<ExprPtr> values;

    MapLiteralExpr(NVSEToken token, std::vector<ExprPtr> values) : token(token), values(values) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        return t->VisitMapLiteralExpr(this);
    }
};

struct GroupingExpr : Expr {
    ExprPtr expr;

    GroupingExpr(ExprPtr expr) : expr(std::move(expr)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitGroupingExpr(this);
    }
};

struct LambdaExpr : Expr {
    NVSEToken token;
    std::vector<std::shared_ptr<VarDeclStmt>> args;
    StmtPtr body;

    LambdaExpr(NVSEToken token, std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : token(std::move(token)),
        args(std::move(args)), body(std::move(body)) {
        line = this->token.line;
    }

    void Accept(NVSEVisitor* t) override {
        t->VisitLambdaExpr(this);
    }

    // Set via type checker
    struct {
        std::vector<NVSEParamType> paramTypes{};
        Token_Type returnType = kTokenType_Invalid;
    } typeinfo;
};