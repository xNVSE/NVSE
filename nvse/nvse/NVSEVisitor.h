#pragma once

struct NVSEScript;

struct BeginStmt;
struct FnDeclStmt;
struct VarDeclStmt;

struct ExprStmt;
struct ForStmt;
struct ForEachStmt;
struct IfStmt;
struct ReturnStmt;
struct ContinueStmt;
struct BreakStmt;
struct WhileStmt;
struct BlockStmt;

struct AssignmentExpr;
struct TernaryExpr;
struct InExpr;
struct BinaryExpr;
struct UnaryExpr;
struct SubscriptExpr;
struct CallExpr;
struct GetExpr;
struct BoolExpr;
struct NumberExpr;
struct StringExpr;
struct IdentExpr;
struct MapLiteralExpr;
struct ArrayLiteralExpr;
struct GroupingExpr;
struct LambdaExpr;

class NVSEVisitor {
public:
	virtual ~NVSEVisitor() = default;
	
	virtual void VisitNVSEScript(NVSEScript* script) = 0;
	virtual void VisitBeginStmt(BeginStmt* stmt) = 0;
	virtual void VisitFnStmt(FnDeclStmt* stmt) = 0;
	virtual void VisitVarDeclStmt(VarDeclStmt* stmt) = 0;

	virtual void VisitExprStmt(const ExprStmt* stmt) = 0;
	virtual void VisitForStmt(ForStmt* stmt) = 0;
	virtual void VisitForEachStmt(ForEachStmt* stmt) = 0;
	virtual void VisitIfStmt(IfStmt* stmt) = 0;
	virtual void VisitReturnStmt(ReturnStmt* stmt) = 0;
	virtual void VisitContinueStmt(ContinueStmt* stmt) = 0;
	virtual void VisitBreakStmt(BreakStmt* stmt) = 0;
	virtual void VisitWhileStmt(WhileStmt* stmt) = 0;
	virtual void VisitBlockStmt(BlockStmt* stmt) = 0;

	virtual void VisitAssignmentExpr(AssignmentExpr* expr) = 0;
	virtual void VisitTernaryExpr(TernaryExpr* expr) = 0;
	virtual void VisitInExpr(InExpr* in_expr) = 0;
	virtual void VisitBinaryExpr(BinaryExpr* expr) = 0;
	virtual void VisitUnaryExpr(UnaryExpr* expr) = 0;
	virtual void VisitSubscriptExpr(SubscriptExpr* expr) = 0;
	virtual void VisitCallExpr(CallExpr* expr) = 0;
	virtual void VisitGetExpr(GetExpr* expr) = 0;
	virtual void VisitBoolExpr(BoolExpr* expr) = 0;
	virtual void VisitNumberExpr(NumberExpr* expr) = 0;
	virtual void VisitStringExpr(StringExpr* expr) = 0;
	virtual void VisitIdentExpr(IdentExpr* expr) = 0;
	virtual void VisitArrayLiteralExpr(ArrayLiteralExpr* expr) = 0;
	virtual void VisitMapLiteralExpr(MapLiteralExpr* expr) = 0;
	virtual void VisitGroupingExpr(GroupingExpr* expr) = 0;
	virtual void VisitLambdaExpr(LambdaExpr* expr) = 0;
};
