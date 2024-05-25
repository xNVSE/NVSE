#include "NVSEVisitor.h"

class NVSETreePrinter : public NVSEVisitor {
	int curTab = 0;
	void printTabs();
	std::function<void(std::string)> &printFn;

public:
	NVSETreePrinter(std::function<void(std::string)> &printFn) : printFn(printFn) {}

	void visitNVSEScript(const NVSEScript* script) override;

	void visitBeginStatement(const BeginStmt* stmt) override;
	void visitFnDeclStmt(FnDeclStmt* stmt) override;
	void visitVarDeclStmt(const VarDeclStmt* stmt) override;

	void visitExprStmt(const ExprStmt* stmt) override;
	void visitForStmt(const ForStmt* stmt) override;
	void visitIfStmt(IfStmt* stmt) override;
	void visitReturnStmt(const ReturnStmt* stmt) override;
	void visitWhileStmt(const WhileStmt* stmt) override;
	void visitBlockStmt(const BlockStmt* stmt) override;

	// Inherited via NVSEVisitor
	void visitAssignmentExpr(const AssignmentExpr* expr) override;
	void visitTernaryExpr(const TernaryExpr* expr) override;
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
	void visitLambdaExpr(LambdaExpr* expr) override;
};