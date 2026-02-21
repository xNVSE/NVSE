#pragma once
#include <format>
#include <set>
#include <stack>
#include <string>

#include <nvse/Compiler/AST/AST.h>
#include "nvse/Compiler/AST/Visitor.h"
#include "nvse/GameScript.h"
#include "nvse/GameAPI.h"
#include "nvse/GameObjects.h"

class Script;

namespace Compiler::Passes {
	struct CallBuffer {
		size_t startPos{};
		size_t startPatch{};
		size_t argPos{};
		size_t numArgs{};
		size_t argStart{};
		size_t argPatch{};
		CommandInfo* cmdInfo{};
		Cmd_Parse* parse{};
		ExprPtr stackRef;
	};

	class Compiler : public Visitor {
	public:
		Script* engineScript;
		bool partial;
		AST& ast;

		const char* originalScriptName;
		std::string scriptName{};

		std::stack<CallBuffer> callBuffers{};

		std::stack<bool> insideNvseExpr{};

		// When we enter a for loop, push the increment condition on the stack
		// Insert this before any continue / break
		std::stack<StmtPtr> loopIncrements{};

		// Count number of statements compiled
		// Certain blocks such as 'if' need to know how many ops to skip
		// We also inject for loop increment statement before continue in loops
		// This is to count that
		std::stack<uint32_t> statementCounter{};

		// To set unused var count at end of script
		std::set<std::string> usedVars{};

		// Keep track of lambda vars as these get inlined
		std::set<std::string> lambdaVars{};

		std::stack<uint32_t> scriptStart{};

		// Look up a local variable, or create it if not already defined
		uint16_t AddVar(const std::string& identifier, const uint8_t type) const {
			if (const auto info = engineScript->GetVariableByName(identifier.c_str())) {
				if (info->type != type) {
					// Remove from ref list if it was a ref prior
					if (info->type == Script::eVarType_Ref) {
						if (auto* v = engineScript->refList.FindFirst([&](const Script::RefVariable* cur) { return cur->varIdx == info->idx; })) {
							engineScript->refList.Remove(v);
						}
					}

					// Add to ref list if new ref
					if (type == Script::eVarType_Ref) {
						auto ref = New<Script::RefVariable>();
						ref->name = String();
						ref->name.Set(identifier.c_str());
						ref->varIdx = info->idx;
						engineScript->refList.Append(ref);
					}

					info->type = type;
				}

				return info->idx;
			}

			const auto varInfo = New<VariableInfo>();
			varInfo->name = String();
			varInfo->name.Set(identifier.c_str());
			varInfo->type = type;
			varInfo->idx = engineScript->varList.Count() + 1;
			engineScript->varList.Append(varInfo);

			if (type == Script::eVarType_Ref) {
				const auto ref = New<Script::RefVariable>();
				ref->name = String();
				ref->name.Set(identifier.c_str());
				ref->varIdx = varInfo->idx;
				engineScript->refList.Append(ref);
			}

			return static_cast<uint16_t>(varInfo->idx);
		}

		uint16_t ResolveObjReference(std::string identifier, const bool add = true) const {
			TESForm* form;
			if (_stricmp(identifier.c_str(), "player") == 0) {
				// PlayerRef (this is how the vanilla compiler handles it so I'm changing it for consistency and to fix issues)
				form = LookupFormByID(0x14);
			}
			else {
				form = GetFormByID(identifier.c_str());
			}

			if (form) {
				// only persistent refs can be used in scripts
				if (const auto ref = DYNAMIC_CAST(form, TESForm, TESObjectREFR); ref && !ref->IsPersistent()) {
					throw std::runtime_error(std::format("Object reference '{}' must be persistent.", identifier));
				}

				// See if form is already registered
				uint16_t i = 1;
				for (auto cur = engineScript->refList.Begin(); !cur.End(); cur.Next()) {
					if (cur->form == form) {
						return i;
					}

					i++;
				}

				if (add) {
					const auto ref = New<Script::RefVariable>();
					ref->name = String();
					ref->name.Set(identifier.c_str());
					ref->form = form;
					engineScript->refList.Append(ref);
					return static_cast<uint16_t>(engineScript->refList.Count());
				}

				return 1;
			}

			return 0;
		}

		// Compiled bytes
		std::vector<uint8_t> data{};

		size_t AddU8(const uint8_t byte) {
			data.emplace_back(byte);

			return data.size() - 1;
		}

		size_t AddU16(const uint16_t bytes) {
			data.emplace_back(bytes & 0xFF);
			data.emplace_back(bytes >> 8 & 0xFF);

			return data.size() - 2;
		}

		size_t AddU32(const uint32_t bytes) {
			data.emplace_back(bytes & 0xFF);
			data.emplace_back(bytes >> 8 & 0xFF);
			data.emplace_back(bytes >> 16 & 0xFF);
			data.emplace_back(bytes >> 24 & 0xFF);

			return data.size() - 4;
		}

		size_t AddF64(double value) {
			const auto bytes = reinterpret_cast<uint8_t*>(&value);

			AddU8('Z');
			for (int i = 0; i < 8; i++) {
				AddU8(bytes[i]);
			}

			return data.size() - 8;
		}

		size_t AddString(const std::string& str) {
			AddU8('S');
			AddU16(str.size());
			for (const char c : str) {
				AddU8(c);
			}

			return data.size() - str.size();
		}

		void SetU8(const size_t index, const uint8_t byte) {
			data[index] = byte;
		}

		void SetU16(const size_t index, const uint16_t byte) {
			data[index] = byte & 0xFF;
			data[index + 1] = byte >> 8 & 0xFF;
		}

		void SetU32(const size_t index, const uint32_t byte) {
			data[index] = byte & 0xFF;
			data[index + 1] = byte >> 8 & 0xFF;
			data[index + 2] = byte >> 16 & 0xFF;
			data[index + 3] = byte >> 24 & 0xFF;
		}

		Compiler(AST& ast, Script* script, bool partial)
			: engineScript(script), partial(partial), ast(ast), originalScriptName(script->GetEditorID()) {
		}

		void PrintScriptInfo();

		void Visit(AST* nvScript) override;
		void VisitBeginStmt(Statements::Begin* stmt) override;
		void VisitFnStmt(Statements::UDFDecl* stmt) override;
		void VisitVarDeclStmt(Statements::VarDecl* stmt) override;
		void VisitExprStmt(Statements::ExpressionStatement* stmt) override;
		void VisitForStmt(Statements::For* stmt) override;
		void VisitForEachStmt(Statements::ForEach* stmt) override;
		void VisitIfStmt(Statements::If* stmt) override;
		void VisitReturnStmt(Statements::Return* stmt) override;
		void VisitContinueStmt(Statements::Continue* stmt) override;
		void VisitBreakStmt(Statements::Break* stmt) override;
		void VisitWhileStmt(Statements::While* stmt) override;
		void VisitBlockStmt(Statements::Block* stmt) override;
		void VisitShowMessageStmt(Statements::ShowMessage* stmt) override;

		void VisitAssignmentExpr(Expressions::AssignmentExpr* expr) override;
		void VisitTernaryExpr(Expressions::TernaryExpr* expr) override;
		void VisitInExpr(Expressions::InExpr* expr) override;
		void VisitBinaryExpr(Expressions::BinaryExpr* expr) override;
		void VisitUnaryExpr(Expressions::UnaryExpr* expr) override;
		void VisitSubscriptExpr(Expressions::SubscriptExpr* expr) override;
		void VisitCallExpr(Expressions::CallExpr* expr) override;
		void VisitGetExpr(Expressions::GetExpr* expr) override;
		void VisitBoolExpr(Expressions::BoolExpr* expr) override;
		void VisitNumberExpr(Expressions::NumberExpr* expr) override;
		void VisitStringExpr(Expressions::StringExpr* expr) override;
		void VisitIdentExpr(Expressions::IdentExpr* expr) override;
		void VisitMapLiteralExpr(Expressions::MapLiteralExpr* expr) override;
		void VisitArrayLiteralExpr(Expressions::ArrayLiteralExpr* expr) override;
		void VisitGroupingExpr(Expressions::GroupingExpr* expr) override;
		void VisitLambdaExpr(Expressions::LambdaExpr* expr) override;

		uint32_t CompileBlock(StmtPtr stmt, bool incrementCurrent);
		void FinishCall();
		void StartCall(CommandInfo* cmd, ExprPtr stackRef = nullptr);
		void StartCall(uint16_t opcode, ExprPtr stackRef = nullptr);
		void PerformCall(uint16_t opcode);
		void AddCallArg(ExprPtr arg);
		void StartManualArg();
		void FinishManualArg();
	};

}