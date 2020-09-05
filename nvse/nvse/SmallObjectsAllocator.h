#pragma once

#include "MemoryPool.h"

namespace SmallObjectsAllocator
{
	template <class T, std::size_t C>
	class Allocator
	{
#if _DEBUG
		std::size_t count_ = 0;
#endif
		MemoryPool<T, sizeof(T)* C> pool_;
	public:
		T* Allocate()
		{
#if _DEBUG
			++count_;
			if (count_ > C)
			{
				_MESSAGE("Warning, possible memory leak");
			}
#endif
			return pool_.allocate();
		}

		void Free(T* ptr)
		{
			pool_.deallocate(ptr);
		}

		void Free(void* ptr)
		{

			Free(static_cast<T*>(ptr));
#if _DEBUG
			--count_;
#endif
		}
	};
}
