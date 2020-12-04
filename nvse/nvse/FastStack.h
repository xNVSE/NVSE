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
	using Allocator = SmallObjectsAllocator::FastAllocator<Node, 16>;
	static thread_local Allocator s_allocator;
public:

	FastStack() : head(NULL), numItems(0) {}
	~FastStack() { Reset(); }

	bool Empty() const { return !numItems; }

	size_t Size() const { return numItems; }

	T_Data& Top()
	{
		return head->data;
	}

	T_Data* Push(Data_Arg item)
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
	T_Data* Push(Args && ...args)
	{
		Node* newNode = s_allocator.Allocate();
		T_Data* data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		newNode->next = head;
		head = newNode;
		numItems++;
		return data;
	}

	T_Data* Pop()
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

	void Reset()
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
thread_local typename FastStack<T>::Allocator FastStack<T>::s_allocator;