#pragma once

struct NVSEScript;

struct BeginStmt;
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
struct BinaryExpr;
struct UnaryExpr;
struct SubscriptExpr;
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
	virtual ~NVSEVisitor() = default;
	virtual void VisitNVSEScript(const NVSEScript* script) = 0;

	virtual void VisitBeginStmt(const BeginStmt* stmt) = 0;
	virtual void VisitFnStmt(FnDeclStmt* stmt) = 0;
	virtual void VisitVarDeclStmt(const VarDeclStmt* stmt) = 0;

	virtual void VisitExprStmt(const ExprStmt* stmt) = 0;
	virtual void VisitForStmt(const ForStmt* stmt) = 0;
	virtual void VisitIfStmt(IfStmt* stmt) = 0;
	virtual void VisitReturnStmt(const ReturnStmt* stmt) = 0;
	virtual void VisitWhileStmt(const WhileStmt* stmt) = 0;
	virtual void VisitBlockStmt(const BlockStmt* stmt) = 0;

	virtual void VisitAssignmentExpr(const AssignmentExpr* expr) = 0;
	virtual void VisitTernaryExpr(const TernaryExpr* expr) = 0;
	virtual void VisitBinaryExpr(const BinaryExpr* expr) = 0;
	virtual void VisitUnaryExpr(const UnaryExpr* expr) = 0;
	virtual void VisitSubscriptExpr(SubscriptExpr* expr) = 0;
	virtual void VisitCallExpr(const CallExpr* expr) = 0;
	virtual void VisitGetExpr(const GetExpr* expr) = 0;
	virtual void VisitSetExpr(const SetExpr* expr) = 0;
	virtual void VisitBoolExpr(const BoolExpr* expr) = 0;
	virtual void VisitNumberExpr(const NumberExpr* expr) = 0;
	virtual void VisitStringExpr(const StringExpr* expr) = 0;
	virtual void VisitIdentExpr(const IdentExpr* expr) = 0;
	virtual void VisitGroupingExpr(const GroupingExpr* expr) = 0;
	virtual void VisitLambdaExpr(LambdaExpr* expr) = 0;
};
