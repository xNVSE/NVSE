#pragma once

#include "MemoryPool.h"
#include "common/ICriticalSection.h"


namespace SmallObjectsAllocator
{
	template <class T, std::size_t C>
	class Allocator
	{
#if _DEBUG
		std::size_t count_ = 0;
#endif
		MemoryPool<T, sizeof(T)* C> pool_;
		ICriticalSection criticalSection_;

		void Free(T* ptr)
		{
			pool_.deallocate(ptr);
		}
		
	public:
		T* Allocate()
		{
			ScopedLock lock(criticalSection_);
#if _DEBUG
			++count_;
			if (count_ > C)
			{
				_MESSAGE("Warning, possible memory leak");
			}
#endif
			
			return pool_.allocate();
		}

		void Free(void* ptr)
		{
			ScopedLock lock(criticalSection_);
			Free(static_cast<T*>(ptr));
#if _DEBUG
			--count_;
#endif
		}
	};
}
