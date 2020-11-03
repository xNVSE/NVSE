#pragma once

#include "MemoryPool.h"
#include "common/ICriticalSection.h"


namespace SmallObjectsAllocator
{
	template <class T, std::size_t C>
	class LockBasedAllocator
	{
#if _DEBUG
		std::size_t count_ = 0;
#else
		MemoryPool<T, sizeof(T)* C> pool_;
#endif
		ICriticalSection criticalSection_;

		void Free(T* ptr)
		{
#if _DEBUG
			::operator delete(ptr);
#else
			pool_.deallocate(ptr);
#endif
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

			return static_cast<T*>(operator new(sizeof(T)));
#else
			return pool_.allocate();

#endif

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

	// only to be used where allocations are *not* shared between threads. 10x faster than lock-based allocator (from my testing)
	template <class T, std::size_t C>
	class PerThreadAllocator
	{
		struct MemObj
		{
			T t;
			void* poolPtr;
		};
		using MemPool = MemoryPool<MemObj, sizeof(MemObj)* C>;
		DWORD tlsIndex = TlsAlloc();

		MemPool* GetPool()
		{
			auto* pool = static_cast<MemPool*>(TlsGetValue(tlsIndex));
			if (!pool)
			{
				pool = new MemPool();
				TlsSetValue(tlsIndex, pool);
			}
			return pool;
		}

	public:
		T* Allocate()
		{
			auto* pool = GetPool();
			auto* memObj = static_cast<MemObj*>(pool->allocate());
			memObj->poolPtr = pool;
			return &memObj->t;
		}

		static void Free(void* ptr)
		{
			auto* mPtr = static_cast<MemObj*>(ptr);
			auto* pool = static_cast<MemPool*>(mPtr->poolPtr);
			pool->deallocate(mPtr);
		}
	};
}
