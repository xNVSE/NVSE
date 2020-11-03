#pragma once
#include "SmallObjectsAllocator.h"

template <typename T_Data> class FastStack
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	struct Node
	{
		Node* next;
		T_Data		data;

		void Clear() { data.~T_Data(); }
	};

	Node* head;
	size_t numItems;
	using Allocator = SmallObjectsAllocator::PerThreadAllocator<Node, 16>;
	static  Allocator s_allocator;
public:

	FastStack() : head(NULL), numItems(0) {}
	~FastStack() { reset(); }

	bool empty() const { return !numItems; }

	size_t size() const { return numItems; }

	T_Data& top()
	{
		return head->data;
	}

	T_Data* push(Data_Arg item)
	{
		Node* newNode = s_allocator.Allocate();
		T_Data* data = &newNode->data;
		RawAssign<T_Data>(*data, item);
		newNode->next = head;
		head = newNode;
		numItems++;
		return data;
	}

	template <typename ...Args>
	T_Data* push(Args && ...args)
	{
		Node* newNode = s_allocator.Allocate();
		T_Data* data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		newNode->next = head;
		head = newNode;
		numItems++;
		return data;
	}

	T_Data* pop()
	{
		if (!head) return NULL;
		T_Data* frontItem = &head->data;
		Node* toRemove = head;
		head = head->next;
		toRemove->Clear();
		s_allocator.Free(toRemove);
		numItems--;
		return frontItem;
	}

	void reset()
	{
		if (!head) return;
		do
		{
			Node* pNode = head;
			head = head->next;
			pNode->Clear();
			s_allocator.Free(pNode);
		} while (head);
		numItems = 0;
	}
};

template <typename T>
typename FastStack<T>::Allocator FastStack<T>::s_allocator;