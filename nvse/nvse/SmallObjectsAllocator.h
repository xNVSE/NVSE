#pragma once
#include <algorithm>
#include <typeinfo>

namespace SmallObjectsAllocator
{
	template <class T, std::size_t Count>
	class Allocator
	{
		T* memBlock_ = static_cast<T*>(::operator new (sizeof(T) * Count));
		bool allocated_[Count] = { false };
		std::size_t nextPos_ = 0;
	public:
		Allocator() = default;

		~Allocator()
		{
			::operator delete(memBlock_);
		}

		T* Allocate()
		{
			for (auto i = nextPos_; i < Count; i++)
			{
				if (!allocated_[i])
				{
					nextPos_ = i + 1;
					allocated_[i] = true;
					return &memBlock_[i];
				}
			}
			nextPos_ = -1;
			_MESSAGE("%s has ran out of small objects", typeid(T).name());
			ShowErrorMessageBox("SmallObjectsAllocator Warning");
			return static_cast<T*>(::operator new(sizeof(T)));
		}

		void Free(T* ptr)
		{
			ptrdiff_t pos = ptr - memBlock_;

			if (pos >= 0 && pos < Count)
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