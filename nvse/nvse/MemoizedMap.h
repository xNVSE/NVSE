#pragma once

#include <atomic>

#include "containers.h"

template <typename K, typename V>
class MemoizedMap
{
public:
	thread_local static UnorderedMap<K, V> map;
	template <typename F>
	V& Get(const K& key, F&& f)
	{
		if (localToken_ != globalToken_)
		{
			localToken_ = globalToken_;
			map.Clear();
		}
		V* value;
		if (map.Insert(key, &value))
			*value = 0; // Fix issue that affected Vanilla UI+ with lStewieAl's Tweaks bContainerDefaultToRightSide
		if (!*value)
			*value = f(key);
		return *value;
	}

	static void Clear()
	{
		++globalToken_;
	}

private:
	static std::atomic<int> globalToken_;
	thread_local static int localToken_;
};

template <typename K, typename V>
std::atomic<int> MemoizedMap<K, V>::globalToken_ = 0;

template <typename K, typename V>
thread_local int MemoizedMap<K, V>::localToken_ = 0;

template <typename K, typename V>
thread_local UnorderedMap<K, V> MemoizedMap<K, V>::map;
