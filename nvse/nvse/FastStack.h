#pragma once
#include <stack>

template <class T, std::size_t arraySize>
class FastStack
{
	T items_[arraySize];
	std::size_t pos_ = 0;
	std::stack<T> fallbackStack_;
public:
	void push(T t)
	{
		if (pos_ >= arraySize)
		{
			fallbackStack_.push(t);
		}
		else
		{
			items_[pos_] = t;
		}
		pos_++;
	}

	T top()
	{
		if (pos_ > arraySize)
		{
			return fallbackStack_.top();
		}
		
		return items_[pos_ - 1];
	}

	void pop()
	{
		if (pos_ > arraySize)
		{
			fallbackStack_.pop();
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