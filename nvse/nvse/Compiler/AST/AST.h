#pragma once
#include "nvse/ScriptTokens.h"

#include "ASTForward.h"

#include "nvse/Compiler/Visitor.h"
#include "nvse/Compiler/Lexer/Lexer.h"

namespace Compiler {
	struct AST {
		NVSEToken name;
		std::vector<StmtPtr> globalVars;
		std::vector<StmtPtr> blocks;
		std::unordered_map<std::string, std::shared_ptr<Expressions::LambdaExpr>> m_mpFunctions
		{};

		// Required NVSE plugins
		std::unordered_map<std::string, uint32_t> m_mpPluginRequirements{};

		AST(
			NVSEToken name,
			std::vector<StmtPtr> globalVars,
			std::vector<StmtPtr> blocks
		) : name(std::move(name)),
			globalVars(std::move(globalVars)),
			blocks(std::move(blocks)) {
		}

		void Accept(Visitor* visitor) {
			visitor->Visit(this);
		}
	};

	struct Statement {
		size_t line = 0;

		// Some statements store type such as return and block statement
		Token_Type detailedType = kTokenType_Invalid;

		virtual ~Statement() = default;

		virtual void Accept(Visitor* t) = 0;

		template <typename T>
		bool IsType() {
			return dynamic_cast<T*>(this);
		}
	};

	namespace Statements {
		struct Begin : Statement {
			NVSEToken name;
			std::optional<NVSEToken> param;
			std::shared_ptr<Block> block;
			CommandInfo* beginInfo;

			Begin(
				const NVSEToken& name,
				std::optional<NVSEToken> param,
				std::shared_ptr<Block> block,
				CommandInfo* beginInfo
			) : name(name),
				param(std::move(param)),
				block(std::move(block)),
				beginInfo(beginInfo) {
				line = name.line;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitBeginStmt(this);
			}
		};

		struct UDFDecl : Statement {
			NVSEToken token;
			std::optional<NVSEToken> name;
			std::vector<std::shared_ptr<VarDecl>> args;
			StmtPtr body;
			bool is_udf_decl = false;

			UDFDecl(
				const NVSEToken& token,
				std::vector<std::shared_ptr<VarDecl>> args,
				StmtPtr body
			) : token(token),
				args(std::move(args)),
				body(std::move(body)) {
				line = token.line;
			}

			UDFDecl(
				const NVSEToken& token,
				const NVSEToken& name,
				std::vector<std::shared_ptr<VarDecl>> args,
				StmtPtr body
			) : token(token),
				name(name),
				args(std::move(args)),
				body(std::move(body)) {
				line = token.line;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitFnStmt(this);
			}
		};

		struct VarDecl : Statement {
			struct Declaration {
				NVSEToken token;
				ExprPtr expr = nullptr;
				std::shared_ptr<Compiler::VarInfo> info = nullptr;
			};

			NVSEToken type;
			std::vector<Declaration> declarations{};

			VarDecl(
				NVSEToken type,
				const std::vector<Declaration>& declarations
			) : type(std::move(type)),
				declarations(declarations)
			{
			}

			VarDecl(
				NVSEToken type,
				const NVSEToken& name,
				ExprPtr value
			) : type(std::move(type))
			{
				declarations.emplace_back(name, std::move(value));
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitVarDeclStmt(this);
			}
		};

		struct ExpressionStatement : Statement {
			ExprPtr expr;

			explicit ExpressionStatement(ExprPtr expr) : expr(std::move(expr)) {}

			void Accept(Visitor* visitor) override {
				visitor->VisitExprStmt(this);
			}
		};

		struct For : Statement {
			StmtPtr init;
			ExprPtr cond;
			ExprPtr post;
			std::shared_ptr<Block> block;

			For(
				StmtPtr init,
				ExprPtr cond,
				ExprPtr post,
				std::shared_ptr<Block> block
			) : init(std::move(init)),
				cond(std::move(cond)),
				post(std::move(post)),
				block(std::move(block))
			{
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitForStmt(this);
			}
		};

		struct ForEach : Statement {
			std::vector<std::shared_ptr<VarDecl>> declarations;
			ExprPtr rhs;
			std::shared_ptr<Block> block;
			bool decompose = false;

			ForEach(
				std::vector<std::shared_ptr<VarDecl>> declarations,
				ExprPtr rhs,
				std::shared_ptr<Block> block,
				bool decompose
			) : declarations(std::move(declarations)),
				rhs(std::move(rhs)),
				block(std::move(block)),
				decompose(decompose)
			{
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitForEachStmt(this);
			}
		};

		struct If : Statement {
			ExprPtr cond;

			std::shared_ptr<Block> block;
			std::shared_ptr<Block> elseBlock;

			If(
				ExprPtr cond,
				std::shared_ptr<Block>&& block,
				std::shared_ptr<Block>&& elseBlock = nullptr
			) : cond(std::move(cond)),
				block(std::move(block)),
				elseBlock(std::move(elseBlock)) {
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitIfStmt(this);
			}
		};

		struct Return : Statement {
			ExprPtr expr{};

			Return() = default;
			explicit Return(ExprPtr expr) : expr(std::move(expr)) {}

			void Accept(Visitor* visitor) override {
				visitor->VisitReturnStmt(this);
			}
		};

		struct Continue : Statement {
			Continue() = default;

			void Accept(Visitor* visitor) override {
				visitor->VisitContinueStmt(this);
			}
		};

		struct Break : Statement {
			Break() = default;

			void Accept(Visitor* visitor) override {
				visitor->VisitBreakStmt(this);
			}
		};

		struct While : Statement {
			ExprPtr cond;
			StmtPtr block;

			While(ExprPtr cond, StmtPtr block)
				: cond(std::move(cond)), block(std::move(block)) {
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitWhileStmt(this);
			}
		};

		struct Block : Statement {
			std::vector<StmtPtr> statements;

			explicit Block(std::vector<StmtPtr> statements)
				: statements(std::move(statements)) {
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitBlockStmt(this);
			}
		};

		struct ShowMessage : Statement {
			std::shared_ptr<Expressions::IdentExpr> message;
			std::vector<std::shared_ptr<Expressions::IdentExpr>> args{};
			uint32_t message_time{};

			ShowMessage(
				const std::shared_ptr<Expressions::IdentExpr>& message,
				const std::vector<std::shared_ptr<Expressions::IdentExpr>>& args,
				const uint32_t messageTime
			) : message(message),
				args(args),
				message_time(messageTime) {
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitShowMessageStmt(this);
			}
		};
	}

	struct Expr {
		size_t line = 0;
		Token_Type type = kTokenType_Invalid;

		virtual ~Expr() = default;
		virtual void Accept(Visitor* t) = 0;

		template <typename T>
		bool IsType() {
			return dynamic_cast<T*>(this);
		}
	};

	namespace Expressions {
		struct AssignmentExpr : Expr {
			NVSEToken token;
			ExprPtr left;
			ExprPtr expr;

			AssignmentExpr(NVSEToken token, ExprPtr left, ExprPtr expr) : token(
				std::move(token)
			),
				left(std::move(left)),
				expr(std::move(expr)) {
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitAssignmentExpr(this);
			}
		};

		struct TernaryExpr : Expr {
			NVSEToken token;
			ExprPtr cond;
			ExprPtr left;
			ExprPtr right;

			TernaryExpr(
				NVSEToken token,
				ExprPtr cond,
				ExprPtr left,
				ExprPtr right
			) : token(std::move(token)),
				cond(std::move(cond)),
				left(std::move(left)),
				right(std::move(right)) {
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitTernaryExpr(this);
			}
		};

		struct InExpr : Expr {
			ExprPtr lhs;
			NVSEToken token;
			std::vector<ExprPtr> values{};
			ExprPtr expression{};
			bool isNot;

			InExpr(
				ExprPtr lhs,
				NVSEToken token,
				std::vector<ExprPtr> exprs,
				bool isNot
			) : lhs(std::move(lhs)),
				token(std::move(token)),
				values(std::move(exprs)),
				isNot(isNot) {
				line = this->token.line;
			}

			InExpr(ExprPtr lhs, NVSEToken token, ExprPtr expr, bool isNot) : lhs(
				std::move(lhs)
			),
				token(std::move(token)),
				expression(std::move(expr)),
				isNot(isNot) {
				line = this->token.line;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitInExpr(this);
			}
		};

		struct BinaryExpr : Expr {
			NVSEToken op;
			ExprPtr left, right;

			BinaryExpr(NVSEToken op, ExprPtr left, ExprPtr right) : op(std::move(op)),
				left(std::move(left)),
				right(std::move(right)) {
				line = this->op.line;
			}

			void Accept(Visitor* t) override {
				t->VisitBinaryExpr(this);
			}
		};

		struct UnaryExpr : Expr {
			NVSEToken op;
			ExprPtr expr;
			bool postfix;

			UnaryExpr(NVSEToken token, ExprPtr expr, const bool postfix) : op(
				std::move(token)
			),
				expr(std::move(expr)),
				postfix(postfix) {
				line = this->op.line;
			}

			void Accept(Visitor* t) override {
				t->VisitUnaryExpr(this);
			}
		};

		struct SubscriptExpr : Expr {
			NVSEToken op;

			ExprPtr left;
			ExprPtr index;

			SubscriptExpr(NVSEToken token, ExprPtr left, ExprPtr index) : op(
				std::move(token)
			),
				left(std::move(left)),
				index(std::move(index)) {
				line = this->op.line;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitSubscriptExpr(this);
			}
		};

		struct CallExpr : Expr {
			ExprPtr left;
			NVSEToken token;
			std::vector<ExprPtr> args;

			// Set by typechecker
			CommandInfo* cmdInfo = nullptr;

			CallExpr(ExprPtr left, NVSEToken token, std::vector<ExprPtr> args) : left(
				std::move(left)
			),
				token(std::move(token)),
				args(std::move(args)) {
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitCallExpr(this);
			}
		};

		struct GetExpr : Expr {
			ExprPtr left;
			NVSEToken token;
			NVSEToken identifier;

			// Resolved in typechecker
			VariableInfo* var_info = nullptr;
			const char* reference_name = nullptr;


			GetExpr(
				NVSEToken token,
				ExprPtr left,
				NVSEToken identifier
			) : left(std::move(left)),
				token(std::move(token)),
				identifier(std::move(identifier))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitGetExpr(this);
			}
		};

		struct BoolExpr : Expr {
			bool value;
			NVSEToken token;

			BoolExpr(
				NVSEToken&& token,
				const bool value
			) : value(value),
				token(std::move(token))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitBoolExpr(this);
			}
		};

		struct NumberExpr : Expr {
			double value;
			// For some reason axis enum is one byte and the rest are two?
			int enum_len;
			bool is_fp;
			NVSEToken token;

			NumberExpr(
				NVSEToken token,
				const double value,
				const bool isFp,
				const int enumLen = 0
			) : value(value),
				enum_len(enumLen),
				is_fp(isFp),
				token(std::move(token))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitNumberExpr(this);
			}
		};

		struct StringExpr : Expr {
			NVSEToken token;

			explicit StringExpr(NVSEToken token) :
				token(std::move(token))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitStringExpr(this);
			}
		};

		struct IdentExpr : Expr {
			NVSEToken token;
			TESForm* form = nullptr;

			// Set during typechecker variable resolution so that compiler can reference
			std::shared_ptr<Compiler::VarInfo> varInfo{ nullptr };

			explicit IdentExpr(NVSEToken token) :
				token(std::move(token))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				t->VisitIdentExpr(this);
			}
		};

		struct ArrayLiteralExpr : Expr {
			NVSEToken token;
			std::vector<ExprPtr> values;

			ArrayLiteralExpr(
				NVSEToken token,
				std::vector<ExprPtr> values
			) : token(std::move(token)),
				values(std::move(values))
			{
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				return t->VisitArrayLiteralExpr(this);
			}
		};

		struct MapLiteralExpr : Expr {
			std::vector<ExprPtr> values;
			NVSEToken token;

			MapLiteralExpr(
				NVSEToken&& token,
				std::vector<ExprPtr>&& values
			) :
				values{ std::move(values) },
				token{ std::move(token) } {
				line = this->token.line;
			}

			void Accept(Visitor* t) override {
				return t->VisitMapLiteralExpr(this);
			}
		};

		struct GroupingExpr : Expr {
			ExprPtr expr;

			explicit GroupingExpr(ExprPtr expr)
				: expr(std::move(expr)) {
			}

			void Accept(Visitor* t) override {
				t->VisitGroupingExpr(this);
			}
		};

		struct LambdaExpr : Expr {
			std::vector<std::shared_ptr<Statements::VarDecl>> args;
			StmtPtr body;

			LambdaExpr(
				std::vector<std::shared_ptr<Statements::VarDecl>>&& args,
				StmtPtr body
			)
				: args(std::move(args)), body(std::move(body)) {
			}

			void Accept(Visitor* t) override {
				t->VisitLambdaExpr(this);
			}

			// Set via type checker
			struct {
				std::vector<NVSEParamType> paramTypes{};
				Token_Type returnType = kTokenType_Invalid;
			} typeinfo;
		};
	}
}
