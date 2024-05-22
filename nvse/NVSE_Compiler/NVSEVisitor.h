#pragma once

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

class NVSEVisitor {
public:
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
};