#pragma once
/*
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
*/
#include <algorithm>

namespace SmallObjectsAllocator
{
	template <class T, std::size_t ObjectCount>
	class Allocator
	{
		std::size_t numElems_ = 0;
		T* memBlock_ = static_cast<T*>(::operator new (sizeof(T) * ObjectCount));
		bool allocated_[ObjectCount] = { false };
		std::size_t nextPos_ = 0;
	public:
		Allocator() = default;

		~Allocator()
		{
			::operator delete(memBlock_);
		}

		T* Allocate()
		{
			numElems_++;
			for (auto i = nextPos_; i < ObjectCount; i++)
			{
				if (!allocated_[i])
				{
					nextPos_ = i + 1;
					allocated_[i] = true;
					return &memBlock_[i];
				}
			}
			nextPos_ = -1;
			return static_cast<T*>(::operator new(sizeof(T)));
		}

		void Free(T* ptr)
		{
			numElems_--;
			ptrdiff_t pos = ptr - memBlock_;

			if (pos >= 0 && pos < ObjectCount)
			{
				nextPos_ = std::min<unsigned int>(static_cast<unsigned int>(pos), nextPos_);
				allocated_[pos] = false;
				return;
			}
			::operator delete(ptr);
		}

		void Free(void* ptr)
		{
			Free(static_cast<T*>(ptr));
		}
	};
}