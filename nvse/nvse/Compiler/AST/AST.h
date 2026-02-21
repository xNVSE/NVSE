#pragma once
#include "nvse/ScriptTokens.h"

#include "ASTForward.h"

#include "nvse/Compiler/AST/Visitor.h"
#include "nvse/Compiler/Lexer/Lexer.h"

namespace Compiler {
	struct AST {
		std::string name;
		std::vector<StmtPtr> globalVars;
		std::vector<StmtPtr> blocks;
		std::unordered_map<std::string, std::shared_ptr<Expressions::LambdaExpr>> m_mpFunctions
		{};

		// Required NVSE plugins
		std::unordered_map<std::string, uint32_t> m_mpPluginRequirements{};

		std::vector<std::string> lines;

		AST(
			const std::string &name,
			std::vector<StmtPtr> globalVars,
			std::vector<StmtPtr> blocks
		) : name(name),
			globalVars(std::move(globalVars)),
			blocks(std::move(blocks)) {
		}

		void Accept(Visitor* visitor) {
			visitor->Visit(this);
		}
	};

	struct Statement {
		// Some statements store type such as return and block statement
		Token_Type detailedType = kTokenType_Invalid;

		SourceSpan sourceInfo;

		virtual ~Statement() = default;

		virtual void Accept(Visitor* t) = 0;

		template <typename T>
		bool IsType() {
			return dynamic_cast<T*>(this);
		}

		template <typename T>
		T* As() {
			return dynamic_cast<T*>(this);
		}
	};

	struct Expr {
		Token_Type type = kTokenType_Invalid;

		SourceSpan sourceInfo;

		virtual ~Expr() = default;
		virtual void Accept(Visitor* t) = 0;

		template <typename T>
		bool IsType() {
			return dynamic_cast<T*>(this);
		}

		template <typename T>
		T* As() {
			return dynamic_cast<T*>(this);
		}
	};

	namespace Statements {
		struct Begin : Statement {
			std::string name;
			std::optional<Token> param;
			std::shared_ptr<Block> block;
			CommandInfo* beginInfo;

			Begin(
				const std::string& name,
				std::optional<Token> param,
				std::shared_ptr<Block> block,
				CommandInfo* beginInfo,
				SourceSpan&& sourceInfo
			) : name(name),
				param(std::move(param)),
				block(std::move(block)),
				beginInfo(beginInfo) {
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitBeginStmt(this);
			}
		};

		struct UDFDecl : Statement {
			Token token;
			std::optional<Token> name;
			std::vector<std::shared_ptr<VarDecl>> args;
			StmtPtr body;
			bool is_udf_decl = false;

			UDFDecl(
				const Token& token,
				std::vector<std::shared_ptr<VarDecl>> args,
				StmtPtr body
			) : token(token),
				args(std::move(args)),
				body(std::move(body)) 
			{}

			UDFDecl(
				const Token& token,
				const Token& name,
				std::vector<std::shared_ptr<VarDecl>> args,
				StmtPtr body
			) : token(token),
				name(name),
				args(std::move(args)),
				body(std::move(body)) 
			{}

			void Accept(Visitor* visitor) override {
				visitor->VisitFnStmt(this);
			}
		};

		struct VarDecl : Statement {
			struct Declaration {
				std::shared_ptr<Expressions::IdentExpr> token;
				ExprPtr expr = nullptr;
				std::shared_ptr<VarInfo> info = nullptr;
			};

			TokenType type;
			std::vector<Declaration> declarations{};

			VarDecl(
				TokenType type,
				const std::vector<Declaration>& declarations,
				SourceSpan&& sourceInfo
			) : type(std::move(type)),
				declarations(declarations)
			{
				this->sourceInfo = sourceInfo;
			}

			VarDecl(
				TokenType type,
				std::shared_ptr<Expressions::IdentExpr> name,
				ExprPtr value,
				const SourceSpan& sourceInfo
			) : type(std::move(type))
			{
				declarations.emplace_back(name, std::move(value));
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitVarDeclStmt(this);
			}
		};

		struct ExpressionStatement : Statement {
			ExprPtr expr;

			explicit ExpressionStatement(
				ExprPtr expr,
				const SourceSpan &sourceInfo
			) : expr(std::move(expr)) {
				this->sourceInfo = sourceInfo;
			}

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
				std::shared_ptr<Block> block,
				SourceSpan &&sourceInfo
			) : init(std::move(init)),
				cond(std::move(cond)),
				post(std::move(post)),
				block(std::move(block))
			{
				this->sourceInfo = sourceInfo;
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
				const std::shared_ptr<Block>& block,
				const std::shared_ptr<Block>& elseBlock,
				const SourceSpan &sourceInfo
			) : cond(std::move(cond)),
				block(std::move(block)),
				elseBlock(std::move(elseBlock)) {
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitIfStmt(this);
			}
		};

		struct Return : Statement {
			ExprPtr expr{};

			explicit Return(const SourceSpan& sourceInfo) {
				this->sourceInfo = sourceInfo;
			}

			explicit Return(ExprPtr expr, const SourceSpan& sourceInfo) : expr(std::move(expr)) {
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitReturnStmt(this);
			}
		};

		struct Continue : Statement {
			explicit Continue(const SourceSpan& sourceInfo) {
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitContinueStmt(this);
			}
		};

		struct Break : Statement {
			explicit Break(const SourceSpan& sourceInfo) {
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitBreakStmt(this);
			}
		};

		struct While : Statement {
			ExprPtr cond;
			StmtPtr block;

			While(ExprPtr cond, StmtPtr block, const SourceSpan& souceInfo)
				: cond(std::move(cond)), block(std::move(block)) {
				this->sourceInfo = souceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitWhileStmt(this);
			}
		};

		struct Block : Statement {
			std::vector<StmtPtr> statements;

			explicit Block(std::vector<StmtPtr> statements, const SourceSpan& sourceInfo)
				: statements(std::move(statements)) {
				this->sourceInfo = sourceInfo;
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

		struct Match : Statement {
			std::shared_ptr<Expr> expression;

			struct MatchArm {
				std::shared_ptr<Expressions::IdentExpr> binding = nullptr;
				std::shared_ptr<Expr> expr = nullptr;
				std::shared_ptr<Block> block = nullptr;
			};

			std::vector<MatchArm> arms{};
			std::shared_ptr<Block> defaultCase = nullptr;

			Match(
				const std::shared_ptr<Expr>& expression,
				std::vector<MatchArm>&& cases,
				const std::shared_ptr<Block>& defaultCase,
				const SourceSpan& sourceInfo
			) : expression(expression), arms(std::move(cases)), defaultCase(defaultCase)
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitMatchStmt(this);
			}
		};
	}

	namespace Expressions {
		struct AssignmentExpr : Expr {
			Token token;
			ExprPtr left;
			ExprPtr expr;

			AssignmentExpr(Token token, ExprPtr left, ExprPtr expr) : token(
				std::move(token)
			),
				left(std::move(left)),
				expr(std::move(expr)) 
			{}

			void Accept(Visitor* t) override {
				t->VisitAssignmentExpr(this);
			}
		};

		struct TernaryExpr : Expr {
			Token token;
			ExprPtr cond;
			ExprPtr left;
			ExprPtr right;

			TernaryExpr(
				Token token,
				ExprPtr cond,
				ExprPtr left,
				ExprPtr right
			) : token(std::move(token)),
				cond(std::move(cond)),
				left(std::move(left)),
				right(std::move(right)) 
			{}

			void Accept(Visitor* t) override {
				t->VisitTernaryExpr(this);
			}
		};

		struct InExpr : Expr {
			ExprPtr lhs;
			Token token;
			std::vector<ExprPtr> values{};
			ExprPtr expression{};
			bool isNot;

			InExpr(
				ExprPtr lhs,
				Token token,
				std::vector<ExprPtr> exprs,
				bool isNot
			) : lhs(std::move(lhs)),
				token(std::move(token)),
				values(std::move(exprs)),
				isNot(isNot) 
			{}

			InExpr(ExprPtr lhs, Token token, ExprPtr expr, bool isNot) : lhs(
				std::move(lhs)
			),
				token(std::move(token)),
				expression(std::move(expr)),
				isNot(isNot) 
			{}

			void Accept(Visitor* visitor) override {
				visitor->VisitInExpr(this);
			}
		};

		struct BinaryExpr : Expr {
			Token op;
			ExprPtr left, right;

			BinaryExpr(
				Token op, 
				ExprPtr left, 
				ExprPtr right,
				const SourceSpan& sourceInfo
			) : op(std::move(op)),
				left(std::move(left)),
				right(std::move(right)) 
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitBinaryExpr(this);
			}
		};

		struct UnaryExpr : Expr {
			Token op;
			ExprPtr expr;
			bool postfix;

			UnaryExpr(
				Token token, 
				ExprPtr expr, 
				const bool postfix
			) : op(
				std::move(token)
			),
				expr(std::move(expr)),
				postfix(postfix) 
			{}

			void Accept(Visitor* t) override {
				t->VisitUnaryExpr(this);
			}
		};

		struct SubscriptExpr : Expr {
			Token op;

			ExprPtr left;
			ExprPtr index;

			SubscriptExpr(
				Token token, 
				ExprPtr left, 
				ExprPtr index,
				const SourceSpan& span
			) : op(std::move(token)),
				left(std::move(left)),
				index(std::move(index)) 
			{
				this->sourceInfo = span;
			}

			void Accept(Visitor* visitor) override {
				visitor->VisitSubscriptExpr(this);
			}
		};

		struct CallExpr : Expr {
			ExprPtr left = {};
			std::shared_ptr<IdentExpr> identifier;
			std::vector<ExprPtr> args;

			// Set by typechecker
			CommandInfo* cmdInfo = nullptr;

			CallExpr(
				const std::shared_ptr<IdentExpr>& identifier,
				std::vector<ExprPtr> args,
				const SourceSpan& sourceInfo
			) : identifier(identifier),
				args(std::move(args))
			{
				this->sourceInfo = sourceInfo;
			}

			CallExpr(
				ExprPtr left,
				const std::shared_ptr<IdentExpr>& identifier,
				std::vector<ExprPtr> args,
				const SourceSpan& sourceInfo
			) : left(std::move(left)),
				identifier(identifier),
				args(std::move(args)) 
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitCallExpr(this);
			}
		};

		struct GetExpr : Expr {
			ExprPtr left;
			std::shared_ptr<IdentExpr> identifier;

			// Resolved in typechecker
			VariableInfo* var_info = nullptr;
			const char* reference_name = nullptr;

			GetExpr(
				ExprPtr left,
				const std::shared_ptr<IdentExpr>& identifier,
				const SourceSpan& sourceInfo
			) : left(std::move(left)),
				identifier(std::move(identifier)) 
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitGetExpr(this);
			}
		};

		struct BoolExpr : Expr {
			bool value;

			BoolExpr(
				const bool value,
				const SourceSpan& sourceInfo
			) : value(value) 
			{
				this->sourceInfo = sourceInfo;
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

			NumberExpr(
				const double value,
				const bool isFp,
				const int enumLen,
				const SourceSpan &sourceInfo
			) : value(value),
				enum_len(enumLen),
				is_fp(isFp) 
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitNumberExpr(this);
			}
		};

		struct StringExpr : Expr {
			std::string value;

			explicit StringExpr(
				const std::string &value, 
				const SourceSpan &sourceInfo
			) : value{ value } 
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitStringExpr(this);
			}
		};

		struct IdentExpr : Expr {
			std::string str;
			TESForm* form = nullptr;

			// Set during typechecker variable resolution so that compiler can reference
			std::shared_ptr<VarInfo> varInfo{ nullptr };

			explicit IdentExpr(
				const std::string& identifier,
				const SourceSpan& sourceInfo
			) : str{identifier}
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				t->VisitIdentExpr(this);
			}
		};

		struct ArrayLiteralExpr : Expr {
			std::vector<ExprPtr> values;

			ArrayLiteralExpr(
				std::vector<ExprPtr> values,
				const SourceSpan& sourceInfo
			) : values(std::move(values))
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				return t->VisitArrayLiteralExpr(this);
			}
		};

		struct MapLiteralExpr : Expr {
			std::vector<ExprPtr> values;

			MapLiteralExpr(
				std::vector<ExprPtr>&& values,
				const SourceSpan &sourceInfo
			) :
				values{ std::move(values) }
			{
				this->sourceInfo = sourceInfo;
			}

			void Accept(Visitor* t) override {
				return t->VisitMapLiteralExpr(this);
			}
		};

		struct GroupingExpr : Expr {
			ExprPtr expr;

			explicit GroupingExpr(
				ExprPtr expr,
				const SourceSpan& sourceInfo
			)
				: expr(std::move(expr)) 
			{
				this->sourceInfo = sourceInfo;
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
				StmtPtr body,
				const SourceSpan &sourceInfo
			)
				: args(std::move(args)), body(std::move(body)) 
			{
				this->sourceInfo = sourceInfo;
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
