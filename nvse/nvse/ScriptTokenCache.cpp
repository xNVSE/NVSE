#include "ScriptTokenCache.h"

#include <atomic>

TokenCacheEntry& CachedTokens::Get(std::size_t key)
{
	return this->container_[key];
}

TokenCacheEntry* CachedTokens::Append(ExpressionEvaluator &expEval)
{
	return this->container_.Append(expEval);
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

CachedTokens& TokenCache::Get(UInt8* key)
{
	if (tlsClearAllCookie_ != tlsClearAllToken_)
	{
		tlsClearAllToken_ = tlsClearAllCookie_;
		cache_.Clear();
	} 
	return cache_[key];
}

void TokenCache::Clear()
{
	if (!cache_.Empty())
		cache_.Clear();
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
	++tlsClearAllCookie_;
}

std::atomic<int> TokenCache::tlsClearAllCookie_ = 0;
thread_local int TokenCache::tlsClearAllToken_ = 0;