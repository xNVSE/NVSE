#pragma once

#include "NVSEVisitor.h"

class NVSETreePrinter : public NVSEVisitor {
	int curTab = 0;
	void PrintTabs();
	const std::function<void(std::string)> &printFn;

public:
	NVSETreePrinter(const std::function<void(std::string)>& printFn) : printFn(printFn) {}

	void VisitNVSEScript(const NVSEScript* script) override;
	void VisitBeginStmt(const BeginStmt* stmt) override;
	void VisitFnStmt(FnDeclStmt* stmt) override;
	void VisitVarDeclStmt(const VarDeclStmt* stmt) override;

	void VisitExprStmt(const ExprStmt* stmt) override;
	void VisitForStmt(const ForStmt* stmt) override;
	void VisitIfStmt(IfStmt* stmt) override;
	void VisitReturnStmt(const ReturnStmt* stmt) override;
	void VisitWhileStmt(const WhileStmt* stmt) override;
	void VisitBlockStmt(const BlockStmt* stmt) override;

	void VisitAssignmentExpr(const AssignmentExpr* expr) override;
	void VisitTernaryExpr(const TernaryExpr* expr) override;
	void VisitBinaryExpr(const BinaryExpr* expr) override;
	void VisitUnaryExpr(const UnaryExpr* expr) override;
	void VisitSubscriptExpr(SubscriptExpr* expr) override;
	void VisitCallExpr(const CallExpr* expr) override;
	void VisitGetExpr(const GetExpr* expr) override;
	void VisitBoolExpr(const BoolExpr* expr) override;
	void VisitNumberExpr(const NumberExpr* expr) override;
	void VisitStringExpr(const StringExpr* expr) override;
	void VisitIdentExpr(const IdentExpr* expr) override;
	void VisitGroupingExpr(const GroupingExpr* expr) override;
	void VisitLambdaExpr(LambdaExpr* expr) override;
};