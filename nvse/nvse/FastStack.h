#pragma once
#include <memory>
#include <stack>


template <class T, std::size_t arraySize>
class FastStack
{
	T items_[arraySize];
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
	void push(T t)
	{
		if (pos_ >= arraySize)
		{
			allocateStack();
			fallbackStack_->push(std::move(t));
		}
		else
		{
			items_[pos_] = std::move(t);
		}
		pos_++;
	}

	T& top()
	{
		if (pos_ > arraySize)
		{
			return fallbackStack_->top();
		}
		
		return items_[pos_ - 1];
	}

	void pop()
	{
		if (pos_ > arraySize)
		{
			fallbackStack_->pop();
		}
		pos_--;
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