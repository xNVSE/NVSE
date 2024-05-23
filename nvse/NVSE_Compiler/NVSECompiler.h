#pragma once
#include <string>

#include "NVSEParser.h"
#include "NVSEVisitor.h"

class Script;
constexpr auto MAX_LOCALS = (UINT16_MAX - 1);

class NVSECompiler : NVSEVisitor {
	// Used to hold result of visits
	// Like if one visit invokes a child visit and needs data from it, such as compiled size
	int result = 0;
	bool inNvseExpr = false;

	// For building SLSD/SCVR/SCRV/SCRO?
	std::vector<std::string> locals {};
	std::unordered_map <std::string, uint32_t> SCRV{};
	std::unordered_map <std::string, uint32_t> SCRO{};

	// Look up a local variable, or create it if not already defined
	uint16_t addLocal(std::string identifier) {
		auto index = std::ranges::find(locals, identifier);
		if (index != locals.end()) {
			return static_cast<uint16_t>(index - locals.begin() + 1);
		}

		locals.emplace_back(identifier);
		if (locals.size() > MAX_LOCALS) {
			throw std::runtime_error("Maximum number of locals defined.");
		}

		return static_cast<uint16_t>(locals.size());
	}

	// Compiled bytes
	std::vector<uint8_t> data{};

	size_t add_u8(uint8_t byte) {
		data.emplace_back(byte);

		return data.size() - 1;
	}

	size_t add_u16(uint16_t bytes) {
		data.emplace_back(bytes & 0xFF);
		data.emplace_back(bytes >> 8 & 0xFF);

		return data.size() - 2;
	}

	size_t add_u32(uint32_t bytes) {
		data.emplace_back(bytes & 0xFF);
		data.emplace_back(bytes >> 8 & 0xFF);
		data.emplace_back(bytes >> 16 & 0xFF);
		data.emplace_back(bytes >> 24 & 0xFF);

		return data.size() - 4;
	}

	void set_u8(size_t index, uint8_t byte) {
		data[index] = byte;
	}

	void set_u16(size_t index, uint16_t byte) {
		data[index] = byte & 0xFF;
		data[index + 1] = byte >> 8 & 0xFF;
	}

	void set_u32(size_t index, uint32_t byte) {
		data[index] = byte & 0xFF;
		data[index + 1] = byte >> 8 & 0xFF;
		data[index + 2] = byte >> 16 & 0xFF;
		data[index + 3] = byte >> 24 & 0xFF;
	}

public:
	// For initializing SLSD/SCVR/SCRV/SCRO with existing script data?
	Script compile(Script* existingScript, std::vector<StmtPtr> statements);
	Script compile(std::vector<StmtPtr> statements);
	std::vector<uint8_t> compile(StmtPtr &stmt);

	void visitFnDeclStmt(const FnDeclStmt* stmt) override;
	void visitVarDeclStmt(const VarDeclStmt* stmt) override;

	void visitExprStmt(const ExprStmt* stmt) override;
	void visitForStmt(const ForStmt* stmt) override;
	void visitIfStmt(const IfStmt* stmt) override;
	void visitReturnStmt(const ReturnStmt* stmt) override;
	void visitWhileStmt(const WhileStmt* stmt) override;
	void visitBlockStmt(const BlockStmt* stmt) override;

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
	void visitLambdaExpr(const LambdaExpr* expr) override;
};