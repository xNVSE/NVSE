#pragma once
#include <format>
#include <set>
#include <stack>
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
	std::function<void(std::string)> printFn;
	
	Script* script;

	const char* originalScriptName;
	std::string scriptName{};

	std::stack<bool> insideNvseExpr{};

	// Used to hold result of visits
	// Like if one visit invokes a child visit and needs data from it, such as compiled size
	uint32_t result = 0;

	std::set<std::string> usedVars{};

	// Keep track of lambda vars as these get inlined
	std::set<std::string> lambdaVars{};

	// Look up a local variable, or create it if not already defined
	uint16_t addLocal(std::string identifier, uint8_t type) {
		usedVars.insert(identifier);

		if (auto info = script->GetVariableByName(identifier.c_str())) {
			return info->idx;
		}

		auto varInfo = New<VariableInfo>();
		varInfo->name = String();
		varInfo->name.Set(identifier.c_str());
		varInfo->type = type;
		varInfo->idx = script->varList.Count() + 1;
		script->varList.Append(varInfo);

		if (type == Script::eVarType_Ref) {
			auto ref = New<Script::RefVariable>();
			ref->name = String();
			ref->name.Set(identifier.c_str());
			ref->varIdx = varInfo->idx;
			script->refList.Append(ref);
		}

		return static_cast<uint16_t>(varInfo->idx);
	}

	uint16_t resolveObjectReference(std::string identifier, bool add = false) {
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

			// See if form is already registered
			uint16_t i = 1;
			for (auto cur = script->refList.Begin(); !cur.End(); cur.Next()) {
				if (cur->form == form) {
					return i;
				}

				i++;
			}

			if (add) {
				auto ref = New<Script::RefVariable>();
				ref->name = String();
				ref->name.Set(identifier.c_str());
				ref->form = form;
				script->refList.Append(ref);
				return static_cast<uint16_t>(script->refList.Count());
			}

			return 1;
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
	bool compile(Script* script, NVSEScript& ast, std::function<void(std::string)> printFn);

	void visitNVSEScript(const NVSEScript* nvScript) override;

	void visitBeginStatement(const BeginStmt* stmt) override;
	void visitFnDeclStmt(FnDeclStmt* stmt) override;
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
	void visitBinaryExpr(const BinaryExpr* expr) override;
	void visitUnaryExpr(const UnaryExpr* expr) override;
	void visitSubscriptExpr(SubscriptExpr* expr) override;
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
