#include "ScriptTokenCache.h"

TokenCacheEntry& CachedTokens::Get(std::size_t key)
{
	return this->container_[key];
}

void CachedTokens::Append(ExpressionEvaluator &expEval)
{
	this->container_.emplace_back(expEval);
}

std::size_t CachedTokens::Size() const
{
	return container_.size();
}

bool CachedTokens::Empty() const
{
	return container_.empty();
}

std::vector<TokenCacheEntry>::iterator CachedTokens::begin()
{
	return container_.begin();
}

std::vector<TokenCacheEntry>::iterator CachedTokens::end()
{
	return container_.end();
}

CachedTokens& TokenCache::Get(UInt8* key)
{
	return cache_[key];
}

void TokenCache::Clear()
{
	cache_.Clear();
}

std::size_t TokenCache::Size() const
{
	return cache_.Size();
}