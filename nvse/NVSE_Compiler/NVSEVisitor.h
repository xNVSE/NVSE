#pragma once

struct FnDeclStmt;
struct VarDeclStmt;

struct ExprStmt;
struct ForStmt;
struct IfStmt;
struct ReturnStmt;
struct WhileStmt;
struct BlockStmt;

struct AssignmentExpr;
struct TernaryExpr;
struct LogicalExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct GetExpr;
struct SetExpr;
struct BoolExpr;
struct NumberExpr;
struct StringExpr;
struct IdentExpr;
struct GroupingExpr;
struct LambdaExpr;

class NVSEVisitor {
public:
	virtual void visitFnDeclStmt(const FnDeclStmt* stmt) = 0;
	virtual void visitVarDeclStmt(const VarDeclStmt* stmt) = 0;

	virtual void visitExprStmt(const ExprStmt* stmt) = 0;
	virtual void visitForStmt(const ForStmt* stmt) = 0;
	virtual void visitIfStmt(IfStmt* stmt) = 0;
	virtual void visitReturnStmt(const ReturnStmt* stmt) = 0;
	virtual void visitWhileStmt(const WhileStmt* stmt) = 0;
	virtual void visitBlockStmt(const BlockStmt* stmt) = 0;

	virtual void visitAssignmentExpr(const AssignmentExpr* expr) = 0;
	virtual void visitTernaryExpr(const TernaryExpr* expr) = 0;
	virtual void visitLogicalExpr(const LogicalExpr* expr) = 0;
	virtual void visitBinaryExpr(const BinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(const UnaryExpr* expr) = 0;
	virtual void visitCallExpr(const CallExpr* expr) = 0;
	virtual void visitGetExpr(const GetExpr* expr) = 0;
	virtual void visitSetExpr(const SetExpr* expr) = 0;
	virtual void visitBoolExpr(const BoolExpr* expr) = 0;
	virtual void visitNumberExpr(const NumberExpr* expr) = 0;
	virtual void visitStringExpr(const StringExpr* expr) = 0;
	virtual void visitIdentExpr(const IdentExpr* expr) = 0;
	virtual void visitGroupingExpr(const GroupingExpr* expr) = 0;
	virtual void visitLambdaExpr(LambdaExpr* expr) = 0;
};
