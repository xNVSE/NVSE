#include "NVSEVisitor.h"

class NVSETreePrinter : public NVSEVisitor {
	int curTab = 0;

	void printTabs();

public:
	// Inherited via NVSEVisitor
	void visitAssignmentExpr(const AssignmentExpr* expr) override;
	void visitTernaryExpr(const TernaryExpr* expr) override;
	void visitLogicalExpr(const LogicalExpr* expr) override;
	void visitBinaryExpr(const BinaryExpr* expr) override;
	void visitUnaryExpr(const UnaryExpr* expr) override;
	void visitCallExpr(const CallExpr* expr) override;
	void visitGetExpr(const GetExpr* expr) override;
	void visitSetExpr(const SetExpr* expr) override;
	void visitBoolExpr(const BoolExpr* expr) override;
	void visitNumberExpr(const NumberExpr* expr) override;
	void visitStringExpr(const StringExpr* expr) override;
	void visitIdentExpr(const IdentExpr* expr) override;
	void visitGroupingExpr(const GroupingExpr* expr) override;
};