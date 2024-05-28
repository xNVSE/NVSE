#pragma once
#include <format>
#include <set>
#include <stack>
#include <string>

#include "NVSEParser.h"
#include "NVSEVisitor.h"

#include "nvse/GameScript.h"
#include "nvse/GameAPI.h"
#include "nvse/GameObjects.h"

class Script;

class NVSECompiler : NVSEVisitor {
public:
	Script* script;
	bool partial;
	NVSEScript &ast;

	const char* originalScriptName;
	std::string scriptName{};

	std::stack<bool> insideNvseExpr{};

	// When we enter a for loop, push the increment condition on the stack
	// Insert this before any continue / break
	std::stack<StmtPtr> loopIncrements{};

	// Count number of statements compiled
	// Certain blocks such as 'if' need to know how many ops to skip
	// We also inject for loop increment statement before continue in loops
	// This is to count that
	std::stack<uint32_t> statementCounter {};

	// To set unused var count at end of script
	std::set<std::string> usedVars{};

	// Keep track of lambda vars as these get inlined
	std::set<std::string> lambdaVars{};

	// Look up a local variable, or create it if not already defined
	uint16_t AddLocal(std::string identifier, uint8_t type) {
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

	uint16_t ResolveObjReference(std::string identifier, bool add = true) {
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

	size_t AddU8(uint8_t byte) {
		data.emplace_back(byte);

		return data.size() - 1;
	}

	size_t AddU16(uint16_t bytes) {
		data.emplace_back(bytes & 0xFF);
		data.emplace_back(bytes >> 8 & 0xFF);

		return data.size() - 2;
	}

	size_t AddU32(uint32_t bytes) {
		data.emplace_back(bytes & 0xFF);
		data.emplace_back(bytes >> 8 & 0xFF);
		data.emplace_back(bytes >> 16 & 0xFF);
		data.emplace_back(bytes >> 24 & 0xFF);

		return data.size() - 4;
	}

	size_t AddF64(double value) {
		uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);

		AddU8('Z');
		for (int i = 0; i < 8; i++) {
			AddU8(bytes[i]);
		}

		return data.size() - 8;
	}

	size_t AddString(std::string str) {
		AddU8('S');
		AddU16(str.size());
		for (char c : str) {
			AddU8(c);
		}

		return data.size() - str.size();
	}

	void SetU8(size_t index, uint8_t byte) {
		data[index] = byte;
	}

	void SetU16(size_t index, uint16_t byte) {
		data[index] = byte & 0xFF;
		data[index + 1] = byte >> 8 & 0xFF;
	}

	void SetU32(size_t index, uint32_t byte) {
		data[index] = byte & 0xFF;
		data[index + 1] = byte >> 8 & 0xFF;
		data[index + 2] = byte >> 16 & 0xFF;
		data[index + 3] = byte >> 24 & 0xFF;
	}

	NVSECompiler(Script *script, bool partial, NVSEScript& ast)
	: script(script), partial(partial), ast(ast), originalScriptName(script->GetEditorID()) {}
	bool Compile();

	void VisitNVSEScript(const NVSEScript* nvScript) override;

	void VisitBeginStmt(const BeginStmt* stmt) override;
	void VisitFnStmt(FnDeclStmt* stmt) override;
	void VisitVarDeclStmt(const VarDeclStmt* stmt) override;

	void VisitExprStmt(const ExprStmt* stmt) override;
	void VisitForStmt(ForStmt* stmt) override;
	void VisitForEachStmt(ForEachStmt* stmt) override;
	void VisitIfStmt(IfStmt* stmt) override;
	void VisitReturnStmt(ReturnStmt* stmt) override;
	void VisitContinueStmt(ContinueStmt* stmt) override;
	void VisitBreakStmt(BreakStmt* stmt) override;
	void VisitWhileStmt(const WhileStmt* stmt) override;
	uint32_t CompileBlock(StmtPtr stmt, bool incrementCurrent);
	void VisitBlockStmt(BlockStmt* stmt) override;

	// Inherited via NVSEVisitor
	void VisitAssignmentExpr(AssignmentExpr* expr) override;
	void VisitTernaryExpr(TernaryExpr* expr) override;
	void VisitBinaryExpr(BinaryExpr* expr) override;
	void VisitUnaryExpr(UnaryExpr* expr) override;
	void VisitSubscriptExpr(SubscriptExpr* expr) override;
	void VisitCallExpr(CallExpr* expr) override;
	void VisitGetExpr(GetExpr* expr) override;
	void VisitBoolExpr(BoolExpr* expr) override;
	void VisitNumberExpr(NumberExpr* expr) override;
	void VisitStringExpr(StringExpr* expr) override;
	void VisitIdentExpr(IdentExpr* expr) override;
	void VisitGroupingExpr(GroupingExpr* expr) override;
	void VisitLambdaExpr(LambdaExpr* expr) override;
};
