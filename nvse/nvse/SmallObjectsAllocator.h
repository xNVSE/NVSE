#pragma once

#include "MemoryPool.h"

namespace SmallObjectsAllocator
{
	template <class T, std::size_t C>
	class Allocator
	{
		MemoryPool<T, sizeof(T)* C> pool_;
	public:
		T* Allocate()
		{
			return pool_.allocate();
		}

		void Free(T* ptr)
		{
			pool_.deallocate(ptr);
		}

		void Free(void* ptr)
		{
			Free(static_cast<T*>(ptr));
		}
	};
}
