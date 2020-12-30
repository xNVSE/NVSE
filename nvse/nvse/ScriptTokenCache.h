#pragma once
#include "containers.h"
#include "ScriptTokens.h"

struct TokenCacheEntry
{
	ScriptToken		token;
	Op_Eval			eval;
	bool			swapOrder;

	TokenCacheEntry(ExpressionEvaluator &expEval) : token(expEval), eval(nullptr), swapOrder(false) {}
};

class CachedTokens
{
	Vector<TokenCacheEntry> container_;
public:
	std::size_t incrementData;
	[[nodiscard]] TokenCacheEntry& Get(std::size_t key);
	TokenCacheEntry* Append(ExpressionEvaluator &expEval);
	[[nodiscard]] std::size_t Size() const;
	[[nodiscard]] bool Empty() const;
	Vector<TokenCacheEntry>::Iterator Begin();
	TokenCacheEntry *DataEnd();
};

class TokenCache
{
	UnorderedMap<UInt8*, CachedTokens> cache_;
public:
	TokenCache();
	CachedTokens& Get(UInt8* key);
	void Clear();
	[[nodiscard]] std::size_t Size() const;
	bool Empty() const;

	static Set<TokenCache*> tlsInstances;
	static void ClearAll();
};