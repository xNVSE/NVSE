#pragma once
#include "containers.h"
#include "ScriptTokens.h"

struct TokenCacheEntry
{
	ScriptToken token;
	Op_Eval eval = nullptr;
	bool swapOrder;
};

class CachedTokens
{
	std::vector<TokenCacheEntry> container_;
public:
	std::size_t incrementData;
	[[nodiscard]] TokenCacheEntry& Get(std::size_t key);
	void Append(const TokenCacheEntry& entry);
	[[nodiscard]] std::size_t Size() const;
	[[nodiscard]] bool Empty() const;
	std::vector<TokenCacheEntry>::iterator begin();
	std::vector<TokenCacheEntry>::iterator end();
};

class TokenCache
{
	UnorderedMap<UInt8*, CachedTokens> cache_;
public:
	CachedTokens& Get(UInt8* key);
	void Clear();
	std::size_t Size() const;
};