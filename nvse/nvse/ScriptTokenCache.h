#pragma once
#include "containers.h"
#include "ScriptTokens.h"
#include <atomic>
#include "FormExtraData.h"


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
	[[nodiscard]] TokenCacheEntry *DataBegin() const;
	[[nodiscard]] TokenCacheEntry *DataEnd() const;
	void Clear();
	~CachedTokens();
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
	[[nodiscard]] bool Empty() const;
	static void MarkForClear();
};

class ScriptTokenCacheFormExtraData : public FormExtraData
{
public:
	ScriptTokenCacheFormExtraData();
	virtual ~ScriptTokenCacheFormExtraData() override = default;

	TokenCache cache;

	static ScriptTokenCacheFormExtraData* Create();
	static ScriptTokenCacheFormExtraData* Get(Script* script);
	static const NiFixedString& GetName();
};