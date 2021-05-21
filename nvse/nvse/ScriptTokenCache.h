#pragma once
#include "containers.h"
#include "ScriptTokens.h"
#include <atomic>
struct TokenCacheEntry
{
	ScriptToken*	token;
	Op_Eval			eval;
	bool			swapOrder;

	TokenCacheEntry(ScriptToken* scriptToken) : token(scriptToken), eval(nullptr), swapOrder(false) {}
};

class CachedTokens
{
	Vector<TokenCacheEntry> container_;
public:
	std::size_t incrementData;
	[[nodiscard]] TokenCacheEntry& Get(std::size_t key);
	TokenCacheEntry* Append(ScriptToken* scriptToken);
	[[nodiscard]] std::size_t Size() const;
	[[nodiscard]] bool Empty() const;
	Vector<TokenCacheEntry>::Iterator Begin();
	TokenCacheEntry *DataEnd();
	void Clear();
};

class TokenCache
{
	UnorderedMap<UInt8*, CachedTokens> cache_;
	static std::atomic<int> tlsClearAllCookie_;
	static thread_local int tlsClearAllToken_;
public:
	CachedTokens& Get(UInt8* key);
	void Clear();
	[[nodiscard]] std::size_t Size() const;
	bool Empty() const;
	static void MarkForClear();
};