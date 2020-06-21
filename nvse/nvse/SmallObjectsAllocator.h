#pragma once
#include <algorithm>
#include <typeinfo>

namespace SmallObjectsAllocator
{
	template <class T, unsigned int S>
	class Allocator
	{
		T* memBlock_ = static_cast<T*>(malloc(sizeof(T) * S));
		bool allocated_[S] = { false };
		unsigned int nextPos_ = 0;
	public:
		Allocator() = default;

		~Allocator()
		{
			free(memBlock_);
		}

		T* Allocate()
		{
			for (auto i = nextPos_; i < S; i++)
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

		void Free(void* voidPtr)
		{
			auto* ptr = static_cast<T*>(voidPtr);
			ptrdiff_t pos = ptr - memBlock_;

			if (pos >= 0 && pos < S)
			{
				nextPos_ = std::min<unsigned int>(static_cast<unsigned int>(pos), nextPos_);
				allocated_[pos] = false;
				return;
			}
			::operator delete(static_cast<T*>(voidPtr));
		}
	};
}