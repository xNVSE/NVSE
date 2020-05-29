#pragma once

// ILink members must be public
template <typename T>
struct ILink
{
	static const UInt32 s_offset;

	ILink <T>	* next;
	ILink <T>	* prev;

	T *	GetObj(void)	{ return (T *)(((uintptr_t)this) - s_offset); }

	static ILink <T> *	GetLink(T * obj)	{ return (ILink <T> *)(((uintptr_t)obj) + s_offset); }

	void	Unlink(void)
	{
		if(next)	next->prev = prev;
		if(prev)	prev->next = next;

		next = prev = NULL;
	}

	void	LinkBefore(T * obj)
	{
		LinkBefore(GetLink(obj));
	}

	void	LinkAfter(T * obj)
	{
		LinkAfter(GetLink(obj));
	}

	void	LinkBefore(ILink <T> * link)
	{
		link->next = this;
		link->prev = prev;

		if(prev)
		{
			prev->next = link;
		}

		prev = link;
	}

	void	LinkAfter(ILink <T> * link)
	{
		link->next = next;
		link->prev = this;

		if(next)
		{
			next->prev = link;
		}

		next = link;
	}
};

template <typename T>
struct ILinkedList
{
	ILink <T>	begin;
	ILink <T>	end;

	void	Reset(void)
	{
		begin.next = &end;
		begin.prev = NULL;
		end.next = NULL;
		end.prev = &begin;
	}

	void	PushFront(T * obj)
	{
		ILink <T>	* objLink = ILink <T>::GetLink(obj);

		objLink->next = begin.next;
		objLink->prev = &begin;

		if(objLink->next)
		{
			objLink->next->prev = objLink;
		}

		begin.next = objLink;
	}
};

#define ILINK_INIT(baseType, memberName)	template <typename T> const UInt32 ILink <T>::s_offset = offsetof(baseType, memberName)
