#include "ScriptTokenCache.h"

#include <atomic>

TokenCacheEntry& CachedTokens::Get(std::size_t key)
{
	return this->container_[key];
}

TokenCacheEntry* CachedTokens::Append(ScriptToken* scriptToken)
{
	return this->container_.Append(TokenCacheEntry(scriptToken));
}

std::size_t CachedTokens::Size() const
{
	return container_.Size();
}

bool CachedTokens::Empty() const
{
	return container_.Empty();
}

Vector<TokenCacheEntry>::Iterator CachedTokens::Begin()
{
	return container_.Begin();
}

TokenCacheEntry* CachedTokens::DataEnd()
{
	return container_.Data() + container_.Size();
}

void CachedTokens::Clear()
{
	for (auto iter = Begin(); !iter.End(); ++iter)
	{
		auto* token = iter.Get().token;
		token->cached = false;
		delete token;
	}
	this->container_.Clear();
}

CachedTokens& TokenCache::Get(UInt8* key)
{
	if (tlsClearAllCookie_ != tlsClearAllToken_)
	{
		tlsClearAllToken_ = tlsClearAllCookie_;
		Clear();
	} 
	return cache_[key];
}

void TokenCache::Clear()
{
	for (auto iter = cache_.Begin(); !iter.End(); ++iter)
	{
		iter.Get().Clear();
	}
	this->cache_.Clear();
}

std::size_t TokenCache::Size() const
{
	return cache_.Size();
}

bool TokenCache::Empty() const
{
	return cache_.Empty();
}

void TokenCache::MarkForClear()
{
	// Required since cache is thread_local and needs to be cleared on each thread
	++tlsClearAllCookie_;
}

std::atomic<int> TokenCache::tlsClearAllCookie_ = 0;
thread_local int TokenCache::tlsClearAllToken_ = 0;