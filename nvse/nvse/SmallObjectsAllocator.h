#pragma once

#include "MemoryPool.h"
#include "common/ICriticalSection.h"
#include <vector>
#include <list>



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
			if (count_ > C * 10)
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

	template <class T, std::size_t C>
	class FastAllocator
	{
#if _DEBUG
		std::size_t count_ = 0;

		struct MemLeakDebugInfo
		{
			T* t;
			std::vector<UInt32> stack;
		};
		std::list<MemLeakDebugInfo> allocatedBlocks_;
#endif
		using MemPool = MemoryPool<T, sizeof(T)* C>;
		MemPool pool_;

	public:
		T* Allocate()
		{
#if _DEBUG
			++count_;
			if (count_ > C * 10)
			{
				_MESSAGE("Warning, possible memory leak");
			}
			auto* alloc = static_cast<T*>(operator new(sizeof(T)));
			UInt32 trace[12] = { 0 };
			CaptureStackBackTrace(0, 12, reinterpret_cast<PVOID*>(trace), nullptr);
			std::vector<UInt32> vecTrace(trace, trace + 12);

			MemLeakDebugInfo info{ alloc, vecTrace };
			allocatedBlocks_.push_back(info);
			return alloc;
#else
			return pool_.allocate();
#endif			
		}

		void Free(void* ptr)
		{
#if _DEBUG
			--count_;
			allocatedBlocks_.remove_if([ptr](auto& info) {return info.t == ptr; });
			::operator delete(ptr);
#else
			pool_.deallocate(static_cast<T*>(ptr));
#endif
		}
	};
}
