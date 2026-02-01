#pragma once

#include <nvse/ScriptUtils.h>

namespace Compiler {
	struct AST;

	struct Statement;
	struct Expr;

	using ExprPtr = std::shared_ptr<Expr>;
	using StmtPtr = std::shared_ptr<Statement>;

	namespace Statements {
		struct Begin;
		struct UDFDecl;
		struct VarDecl;

		struct ExpressionStatement;
		struct For;
		struct ForEach;
		struct If;
		struct Return;
		struct Continue;
		struct Break;
		struct While;
		struct Block;
		struct ShowMessage;
	}

	namespace Expressions {
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
	}

	struct VarInfo {
		size_t index;
		std::string original_name;
		std::string remapped_name;
		bool pre_existing = false;

		Script::VariableType type;
		Token_Type detailed_type;

		struct {
			bool is_lambda;
			std::vector<NVSEParamType> param_types{};
			Token_Type return_type = kTokenType_Invalid;
		} lambda_type_info;
	};
}