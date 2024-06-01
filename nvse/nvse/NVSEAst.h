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
    std::unordered_map<std::string, std::shared_ptr<LambdaExpr>> mpFunctions{};

    NVSEScript(NVSEToken name, std::vector<StmtPtr> globalVars, std::vector<StmtPtr> blocks) : name(std::move(name)),
        globalVars(std::move(globalVars)), blocks(std::move(blocks)) {}

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

    BeginStmt(const NVSEToken& name, std::optional<NVSEToken> param, StmtPtr block) : name(name),
        param(std::move(param)), block(std::move(block)) {
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
    bool bExport{false};

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
    StmtPtr lhs;
    ExprPtr rhs;
    std::shared_ptr<BlockStmt> block;

    ForEachStmt(StmtPtr lhs, ExprPtr rhs, std::shared_ptr<BlockStmt> block) : lhs(std::move(lhs)), rhs(std::move(rhs)),
        block(std::move(block)) {}

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

    ReturnStmt(NVSEToken token, ExprPtr expr) : token(std::move(token)), expr(std::move(expr)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitReturnStmt(this);
    }
};

struct ContinueStmt : Stmt {
    NVSEToken token;
    ContinueStmt(NVSEToken token) : token(std::move(token)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitContinueStmt(this);
    }
};

struct BreakStmt : Stmt {
    NVSEToken token;
    BreakStmt(NVSEToken token) : token(std::move(token)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitBreakStmt(this);
    }
};

struct WhileStmt : Stmt {
    NVSEToken token;
    ExprPtr cond;
    StmtPtr block;

    WhileStmt(NVSEToken token, ExprPtr cond, StmtPtr block) : token(std::move(token)), cond(std::move(cond)),
        block(std::move(block)) {}

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
    int line = 0;
    Token_Type detailedType = kTokenType_Invalid;

    virtual ~Expr() = default;
    virtual void Accept(NVSEVisitor* t) = 0;

    virtual NVSEToken* getToken() {
        return nullptr;
    }

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
        expr(std::move(expr)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitAssignmentExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
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
        right(std::move(right)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitTernaryExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};

struct InExpr : Expr {
    ExprPtr lhs;
    NVSEToken tok;
    std::vector<ExprPtr> values{};
    ExprPtr expression{};

    InExpr(ExprPtr lhs, NVSEToken tok, std::vector<ExprPtr> exprs) : lhs(lhs), tok(tok), values(exprs) {}
    InExpr(ExprPtr lhs, NVSEToken tok, ExprPtr expr) : lhs(lhs), tok(tok), expression(expr) {}

    void Accept(NVSEVisitor *visitor) override {
        visitor->VisitInExpr(this);
    }
};

struct BinaryExpr : Expr {
    NVSEToken op;
    ExprPtr left, right;

    BinaryExpr(NVSEToken token, ExprPtr left, ExprPtr right) : op(std::move(token)), left(std::move(left)),
        right(std::move(right)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitBinaryExpr(this);
    }

    NVSEToken* getToken() override {
        return &op;
    }
};

struct UnaryExpr : Expr {
    NVSEToken op;
    ExprPtr expr;
    bool postfix;

    UnaryExpr(NVSEToken token, ExprPtr expr, bool postfix) : op(std::move(token)), expr(std::move(expr)),
        postfix(postfix) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitUnaryExpr(this);
    }

    NVSEToken* getToken() override {
        return &op;
    }
};

struct SubscriptExpr : Expr {
    NVSEToken op;

    ExprPtr left;
    ExprPtr index;

    SubscriptExpr(NVSEToken token, ExprPtr left, ExprPtr index) : op(std::move(token)), left(std::move(left)),
        index(std::move(index)) {}

    void Accept(NVSEVisitor* visitor) override {
        visitor->VisitSubscriptExpr(this);
    }

    NVSEToken* getToken() override {
        return &op;
    }
};

struct CallExpr : Expr {
    ExprPtr left;
    NVSEToken token;
    std::vector<ExprPtr> args;

    // Set by typechecker
    CommandInfo* cmdInfo = nullptr;

    CallExpr(ExprPtr left, NVSEToken token, std::vector<ExprPtr> args) : left(std::move(left)), token(token), args(std::move(args)) {}

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
        identifier(std::move(identifier)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitGetExpr(this);
    }

    NVSEToken* getToken() override {
        return &identifier;
    }
};

struct BoolExpr : Expr {
    NVSEToken token;
    bool value;

    BoolExpr(NVSEToken token, bool value) : token(std::move(token)), value(value) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitBoolExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};

struct NumberExpr : Expr {
    NVSEToken token;
    double value;
    bool isFp;

    NumberExpr(NVSEToken token, double value, bool isFp) : token(std::move(token)), value(value), isFp(isFp) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitNumberExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};

struct StringExpr : Expr {
    NVSEToken token;

    StringExpr(NVSEToken token) : token(std::move(token)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitStringExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};

struct IdentExpr : Expr {
    NVSEToken token;

    IdentExpr(NVSEToken token) : token(std::move(token)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitIdentExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};

struct ArrayLiteralExpr : Expr {
    NVSEToken token;
    std::vector<ExprPtr> values;

    ArrayLiteralExpr(NVSEToken token, std::vector<ExprPtr> values) : token(token), values(values) {}

    void Accept(NVSEVisitor* t) override {
        return t->VisitArrayLiteralExpr(this);
    }
};

struct MapLiteralExpr : Expr {
    NVSEToken token;
    std::vector<ExprPtr> values;

    MapLiteralExpr(NVSEToken token, std::vector<ExprPtr> values) : token(token), values(values) {}

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

    // Set in type checker to init a fresh scope
    std::shared_ptr<NVSEScope> scope {nullptr};

    LambdaExpr(NVSEToken token, std::vector<std::shared_ptr<VarDeclStmt>> args, StmtPtr body) : token(std::move(token)),
        args(std::move(args)), body(std::move(body)) {}

    void Accept(NVSEVisitor* t) override {
        t->VisitLambdaExpr(this);
    }

    NVSEToken* getToken() override {
        return &token;
    }
};