#include "Hooks_ExpressionEvalOptimized.h"



#include "Commands_Scripting.h"
#include "FastStack.h"
#include "GameObjects.h"
#include "SafeWrite.h"

namespace GameFunctions
{
	UInt32(__cdecl* IsStringInt)(char*) = reinterpret_cast<UInt32(*)(char* string)>(0x4B1230);
	UInt32(__cdecl* IsStringFloat)(char*) = reinterpret_cast<UInt32(*)(char* string)>(0x4B12D0);

	UInt32(__thiscall* Expression_Extract)(Expression::Expression*, char**, char*, Expression::ScriptOperator*, bool,
		UInt8**)
		= reinterpret_cast<UInt32(__thiscall*)(Expression::Expression*, char** a1, char* a2,
			Expression::ScriptOperator * operatorPtr, bool a4,
			UInt8 * *scriptData)>(0x593870);

	bool(__cdecl* EvaluateCommandRef)(double*, void*, UInt32*, TESObjectREFR*, TESObjectREFR*, Script*, ScriptEventList*, bool)
		= reinterpret_cast<bool(__cdecl*)(double* commandResult, void* scriptData, UInt32 * offsetPtr,
			TESObjectREFR * ref,
			TESObjectREFR * containingObj, Script * scriptObj,
			ScriptEventList * scriptEventList, bool aFalse)>(0x5AC7A0);
}


thread_local kEvaluator::TokenListMap g_tokenListMap;

kEvaluator::Token* GetOperatorParent(kEvaluator::Token* iter, kEvaluator::Token* iterEnd)
{
	iter++;
	int count = 0;
	while (iter < iterEnd)
	{
		if (iter->type == kEvaluator::Token::Type::kOperator)
		{
			if (count == 0)
			{
				return iter;
			}

			// e.g. `1 0 ! &&` prevent ! being parent of 1
			if (iter->value.operator_ != Expression::kOp_Tilde)
			{
				count -= 1;
			}
		}
		else
		{
			count++;
		}
		if (count == 0)
		{
			return iter;
		}
		iter++;
	}
	return iterEnd;
}

void ParseShortCircuit(kEvaluator::TokenList& cachedTokens)
{
	auto* end = cachedTokens.tokens.Data() + cachedTokens.tokens.Size();
	for (auto iter = cachedTokens.tokens.Begin(); !iter.End(); ++iter)
	{
		auto* token = &iter.Get();
		auto* grandparent = token;
		kEvaluator::Token* furthestParent;

		do
		{
			// Find last "parent" operator of same type. E.g `0 1 && 1 && 1 &&` should jump straight to end of expression.
			furthestParent = grandparent;
			grandparent = GetOperatorParent(grandparent, end);
		} while (grandparent < end && grandparent->IsLogicalOperator() && (furthestParent == token || grandparent->value.operator_ == furthestParent->value.operator_));

		if (furthestParent != token && furthestParent->IsLogicalOperator())
		{
			token->shortCircuitParentType = furthestParent->value.operator_;
			token->shortCircuitDistance = furthestParent - token;

			auto* parent = GetOperatorParent(token, end);
			token->shortCircuitStackOffset = token + 1 == parent ? 2 : 1;
		}
		else
		{
			token->shortCircuitParentType = Expression::kOp_MAX;
			token->shortCircuitDistance = 0;
			token->shortCircuitStackOffset = 0;
		}
	}
}

bool kEvaluator::Token::IsLogicalOperator() const
{
	return this->value.operator_ == Expression::kOp_LogicalAnd || this->value.operator_ == Expression::kOp_LogicalOr;
}

void CopyShortCircuitInfo(kEvaluator::Token* to, kEvaluator::Token* from)
{
	to->shortCircuitParentType = from->shortCircuitParentType;
	to->shortCircuitDistance = from->shortCircuitDistance;
	to->shortCircuitStackOffset = from->shortCircuitStackOffset;
}

bool ParseBytecode(kEvaluator::TokenList& tokenList, Expression::Expression* expr, UInt8* scriptData, UInt32 numChars)
{
	char exprSubset[520];
	char tokenStringBuf[71];
	Expression::ScriptOperator operator_ = Expression::kOp_MAX;
	std::memcpy(exprSubset, scriptData, numChars);
	exprSubset[numChars] = 0;
	auto* exprSubsetPtr = exprSubset;
	unsigned int numBytesParsed;
	do
	{
		memset(tokenStringBuf, 0xCC, sizeof(tokenStringBuf));
		numBytesParsed = GameFunctions::Expression_Extract(expr, &exprSubsetPtr, tokenStringBuf, &operator_, false, &scriptData);
		if (!numBytesParsed)
			break;
		if (expr->lastErrorID)
			return false;
		if (operator_ >= 0x10 || operator_ <= 0x1) // operand
		{
			if (GameFunctions::IsStringInt(tokenStringBuf) || GameFunctions::IsStringFloat(tokenStringBuf))
			{
				// number
				const auto number = atof(tokenStringBuf);
				tokenList.tokens.Append(number);
			}
			else
			{
				// commandRef
				// auto* scriptDataSubset = new char[numBytesParsed];
				// std::memcpy(scriptDataSubset, tokenStringBuf, numBytesParsed);
				const auto offset = exprSubsetPtr - exprSubset - numBytesParsed;
				tokenList.tokens.Append(scriptData + offset);
			}
		}
		else // operator
		{
			tokenList.tokens.Append(operator_);
		}
	} while (numBytesParsed);
	ParseShortCircuit(tokenList);
	return true;
}

thread_local SmallObjectsAllocator::FastAllocator<kEvaluator::Token, 0x20> g_tempTokens;

double OperatorEvaluate(Expression::ScriptOperator opType, double lh, double rh, Expression::Expression* expr)
{
	switch (opType) {
	case Expression::kOp_LogicalAnd:
		return lh && rh;
	case Expression::kOp_LogicalOr:
		return lh || rh;
	case Expression::kOp_LessThanOrEqual:
		return lh <= rh;
	case Expression::kOp_LessThan:
		return lh < rh;
	case Expression::kOp_GreaterThanOrEqual:
		return lh >= rh;
	case Expression::kOp_GreaterThan:
		return lh > rh;
	case Expression::kOp_Equals:
		return lh == rh;
	case Expression::kOp_NotEquals:
		return lh != rh;
	case Expression::kOp_Minus:
		return lh - rh;
	case Expression::kOp_Plus:
		return lh + rh;
	case Expression::kOp_Multiply:
		return lh * rh;
	case Expression::kOp_Divide:
		if (rh == 0.0)
		{
			expr->lastErrorID = Expression::kScriptError_DivideByZero;
			return 0.0;
		}
		return lh / rh;
	case Expression::kOp_Modulo:
		if (rh == 0.0)
		{
			expr->lastErrorID = Expression::kScriptError_DivideByZero;
			return 0.0;
		}
		return static_cast<int>(lh) % static_cast<int>(rh);
	default:
		return 0.0;
	}
}

void ShortCircuit(FastStack<kEvaluator::Token*>& operands, Vector<kEvaluator::Token>::Iterator& iter)
{
	auto* lastToken = operands.Top();
	const auto type = lastToken->shortCircuitParentType;
	if (type == Expression::kOp_MAX)
		return;

	const bool eval = lastToken->value.operand;
	if (type == Expression::kOp_LogicalAnd && !eval || type == Expression::kOp_LogicalOr && eval)
	{
		iter += lastToken->shortCircuitDistance;
		for (UInt32 i = 0; i < lastToken->shortCircuitStackOffset; ++i)
		{
			// Make sure only one operand is left in RPN stack
			auto* operand = operands.Top();
			if (operand && operand != lastToken)
				delete operand;
			operands.Pop();
		}
		operands.Push(lastToken);
	}
}

void CleanUpStack(FastStack<kEvaluator::Token*>& stack)
{
	while (!stack.Empty())
	{
		delete stack.Top();
		stack.Pop();
	}
}

double __fastcall Hook_Expression_Eval(Expression::Expression* expr, UInt32 EDX, UInt8* scriptData, TESObjectREFR* ref, TESObjectREFR* containingObj, Script* scriptObj, ScriptEventList* eventList, std::size_t numChars, bool aFalse)
{
	g_lastScriptData = scriptData;
	if (numChars > 512)
	{
		expr->lastErrorID = Expression::kScriptError_OutOfMemory;
		return 0.0;
	}
	auto& tokenList = g_tokenListMap.map[scriptData];
	if (tokenList.tokens.Empty())
	{
		if (!ParseBytecode(tokenList, expr, scriptData, numChars))
			// error, expression::lastError contains error code
			return 0.0;
	}

	FastStack<kEvaluator::Token*> stack;
	for (auto iter = tokenList.tokens.Begin(); !iter.End() && expr->lastErrorID == 0; ++iter)
	{
		auto& token = iter.Get();
		switch (token.type)
		{
		case kEvaluator::Token::Type::kOperand:
		{
			stack.Push(&token);
			break;
		}
		case kEvaluator::Token::Type::kCommandRef:
		{
			double result = 0.0;
			UInt32 opcodeOffset = 0;
			if (GameFunctions::EvaluateCommandRef(&result, token.value.scriptData, &opcodeOffset, ref, containingObj, scriptObj, eventList, false))
			{
				auto* cmdRefResultToken = kEvaluator::Token::AllocateTemporary(result);
				CopyShortCircuitInfo(cmdRefResultToken, &token);
				stack.Push(cmdRefResultToken);
			}
			else
			{
				expr->lastErrorID = Expression::kScriptError_Syntax;
			}
			break;
		}
		case kEvaluator::Token::Type::kOperator:
		{
			if (token.value.operator_ == Expression::kOp_Tilde && stack.Size() != 0)
			{
				auto* rhToken = stack.Top();
				stack.Pop();
				auto* negatedToken = kEvaluator::Token::AllocateTemporary(-rhToken->value.operand);
				CopyShortCircuitInfo(negatedToken, rhToken);
				stack.Push(negatedToken);
				delete rhToken;
			}
			else if (stack.Size() < 2)
			{
				expr->lastErrorID = Expression::kScriptError_StackUnderflow;
			}
			else
			{
				auto* rhToken = stack.Top();
				stack.Pop();
				auto* lhToken = stack.Top();
				stack.Pop();

				const auto result = OperatorEvaluate(token.value.operator_, lhToken->value.operand, rhToken->value.operand, expr);
				if (expr->lastErrorID)
				{
					delete rhToken;
					delete lhToken;
					break;
				}
				auto* opResultToken = kEvaluator::Token::AllocateTemporary(result);
				CopyShortCircuitInfo(opResultToken, &token);
				stack.Push(opResultToken);

				delete rhToken;
				delete lhToken;
			}

		};
		default: break;
		}
		ShortCircuit(stack, iter);
	}
	if (expr->lastErrorID)
	{
		CleanUpStack(stack);
		return 0.0;
	}
	if (stack.Size() != 1)
	{
		expr->lastErrorID = Expression::kScriptError_Syntax;
		CleanUpStack(stack);
		return 0.0;
	}
	auto* resultToken = stack.Top();
	const auto result = resultToken->value.operand;
	delete resultToken;
	return result;
}

void* kEvaluator::Token::operator new(size_t)
{
	return g_tempTokens.Allocate();
}

void kEvaluator::Token::operator delete(void* ptr)
{
	if (static_cast<kEvaluator::Token*>(ptr)->temporary)
		g_tempTokens.Free(ptr);
}

void Hook_Evaluator()
{
	WriteRelJump(0x593DB0, UInt32(Hook_Expression_Eval));
}