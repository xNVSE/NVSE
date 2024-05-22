#include "../nvse/CommandTable.h"
#include "NVSECompiler.h"

void NVSECompiler::visitFnDeclStmt(const FnDeclStmt* stmt) {}
void NVSECompiler::visitVarDeclStmt(const VarDeclStmt* stmt) {}

void NVSECompiler::visitExprStmt(const ExprStmt* stmt) {}
void NVSECompiler::visitForStmt(const ForStmt* stmt) {}
void NVSECompiler::visitIfStmt(const IfStmt* stmt) {}
void NVSECompiler::visitReturnStmt(const ReturnStmt* stmt) {}
void NVSECompiler::visitWhileStmt(const WhileStmt* stmt) {}
void NVSECompiler::visitBlockStmt(const BlockStmt* stmt) {}

void NVSECompiler::visitAssignmentExpr(const AssignmentExpr* expr) {}
void NVSECompiler::visitTernaryExpr(const TernaryExpr* expr) {}
void NVSECompiler::visitLogicalExpr(const LogicalExpr* expr) {}
void NVSECompiler::visitBinaryExpr(const BinaryExpr* expr) {}
void NVSECompiler::visitUnaryExpr(const UnaryExpr* expr) {}
void NVSECompiler::visitCallExpr(const CallExpr* expr) {}
void NVSECompiler::visitGetExpr(const GetExpr* expr) {}
void NVSECompiler::visitSetExpr(const SetExpr* expr) {}
void NVSECompiler::visitBoolExpr(const BoolExpr* expr) {}
void NVSECompiler::visitNumberExpr(const NumberExpr* expr) {}
void NVSECompiler::visitStringExpr(const StringExpr* expr) {}
void NVSECompiler::visitIdentExpr(const IdentExpr* expr) {}
void NVSECompiler::visitGroupingExpr(const GroupingExpr* expr) {}
void NVSECompiler::visitLambdaExpr(const LambdaExpr* expr) {}
