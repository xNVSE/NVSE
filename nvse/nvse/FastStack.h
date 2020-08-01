#pragma once
#include <memory>
#include <stack>


template <class T, std::size_t S>
class FastStack
{
	T items_[S];
	std::size_t pos_ = 0;
	std::unique_ptr<std::stack<T>> fallbackStack_ = nullptr;

	void allocateStack()
	{
		if (fallbackStack_ == nullptr)
		{
			fallbackStack_ = std::make_unique<std::stack<T>>();
			//_MESSAGE("%s has ran out stack space", typeid(T).name());
			//ShowErrorMessageBox("FastStack Warning");
		}
	}
	
public:
	__forceinline void push(T t)
	{
		if (pos_ >= S)
		{
			allocateStack();
			fallbackStack_->push(std::move(t));
		}
		else
		{
			items_[pos_] = std::move(t);
		}
		++pos_;
	}

	T& top()
	{
		if (pos_ > S)
		{
			return fallbackStack_->top();
		}
		
		return items_[pos_ - 1];
	}

	void pop()
	{
		if (pos_ > S)
		{
			fallbackStack_->pop();
		}
		--pos_;
	}

	std::size_t size() const
	{
		return pos_;
	}

	void reset()
	{
		pos_ = 0;
	}
};