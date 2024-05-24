#pragma once
#include <format>
#include <string>

#include "NVSEParser.h"
#include "NVSEVisitor.h"

#include "nvse/prefix.h"
#include "nvse/GameScript.h"
#include "nvse/GameAPI.h"
#include "nvse/GameObjects.h"

class Script;
constexpr auto MAX_LOCALS = (UINT16_MAX - 1);

class NVSECompiler : NVSEVisitor {
public:
	struct SCRO {
		std::string identifier;
		uint32_t refId;
	};

	std::string scriptName{};

	// Used to hold result of visits
	// Like if one visit invokes a child visit and needs data from it, such as compiled size
	uint32_t result = 0;
	bool inNvseExpr = false;

	// For building SLSD/SCVR/SCRV/SCRO?
	std::vector<std::string> locals{};
	std::vector<uint8_t> localTypes{};

	std::vector<uint32_t> localRefs{};
	std::vector<SCRO> objectRefs{};

	Script::RefList refList{};

	// Look up a local variable, or create it if not already defined
	uint16_t addLocal(std::string identifier, uint8_t type) {
		auto index = std::ranges::find(locals, identifier);
		if (index != locals.end()) {
			return static_cast<uint16_t>(index - locals.begin() + 1);
		}

		locals.emplace_back(identifier);
		localTypes.emplace_back(type);
		if (locals.size() > MAX_LOCALS) {
			throw std::runtime_error("Maximum number of locals defined.");
		}

		if (type == kParamType_ObjectRef) {
			localRefs.push_back(locals.size());
		}

		return static_cast<uint16_t>(locals.size());
	}

	bool hasLocal(std::string identifier) {
		return std::ranges::find(locals, identifier) != locals.end();
	}

	uint8_t getLocalType(std::string identifier) {
		if (!hasLocal(identifier)) {
			return 0;
		}

		auto index = std::ranges::find(locals, identifier);
		if (index != locals.end()) {
			return localTypes[index - locals.begin() + 1];
		}

		return 0;
	}

	uint16_t resolveObjectReference(std::string identifier) {
		for (int i = 0; i < objectRefs.size(); i++) {
			if (objectRefs[i].identifier == identifier) {
				return i;
			}
		}

		TESForm* form;
		if (_stricmp(identifier.c_str(), "player") == 0) {
			// PlayerRef (this is how the vanilla compiler handles it so I'm changing it for consistency and to fix issues)
			form = LookupFormByID(0x14);
		}
		else {
			form = GetFormByID(identifier.c_str());
		}

		if (form) {
			TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);

			// only persistent refs can be used in scripts
			if (refr && !refr->IsPersistent()) {
				throw std::runtime_error(std::format("Object reference '{}' must be persistent.", identifier));
			}

			if (!refr) {
				return 0;
			}

			objectRefs.emplace_back(SCRO{
				.identifier = identifier,
				.refId = refr->refID
			});
			return static_cast<uint16_t>(objectRefs.size());
		}

		return 0;
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

	// For initializing SLSD/SCVR/SCRV/SCRO with existing script data?
	std::vector<uint8_t> compile(NVSEScript& script);

	void visitNVSEScript(const NVSEScript* script) override;

	void visitBeginStatement(const BeginStmt* stmt) override;
	void visitFnDeclStmt(const FnDeclStmt* stmt) override;
	void visitVarDeclStmt(const VarDeclStmt* stmt) override;

	void visitExprStmt(const ExprStmt* stmt) override;
	void visitForStmt(const ForStmt* stmt) override;
	void visitIfStmt(IfStmt* stmt) override;
	void visitReturnStmt(const ReturnStmt* stmt) override;
	void visitWhileStmt(const WhileStmt* stmt) override;
	uint32_t compileBlock(StmtPtr& stmt, bool incrementCurrent);
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
	void visitLambdaExpr(LambdaExpr* expr) override;
};
