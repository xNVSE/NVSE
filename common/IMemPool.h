#pragma once

#include "common/ICriticalSection.h"

template <typename T, UInt32 size>
class IMemPool
{
public:
	IMemPool()
	:m_free(NULL), m_alloc(NULL)
	{
		Reset();
	}

	virtual ~IMemPool()	{ Clear(); }

	void	Reset(void)
	{
		for(UInt32 i = 0; i < size - 1; i++)
		{
			m_items[i].next = &m_items[i + 1];
		}

		m_items[size - 1].next = NULL;
		m_free = m_items;
		m_alloc = NULL;
	}

	T *		Allocate(void)
	{
		if(m_free)
		{
			PoolItem	* item = m_free;
			m_free = m_free->next;

			item->next = m_alloc;
			m_alloc = item;

			T	* obj = item->GetObj();

			new (obj) T;
			return obj;
		}

		return NULL;
	}

	void	Free(T * obj)
	{
		PoolItem	* item = reinterpret_cast <PoolItem *>(obj);

		if(item == m_alloc)
		{
			m_alloc = item->next;
		}
		else
		{
			PoolItem	* traverse = m_alloc;
			while(traverse->next != item)
				traverse = traverse->next;
			traverse->next = traverse->next->next;
		}

		item->next = m_free;
		m_free = item;

		obj->~T();
	}

	UInt32	GetSize(void)	{ return size; }

	T *		Begin(void)
	{
		T	* result = NULL;

		if(m_alloc)
			result = m_alloc->GetObj();
		
		return result;
	}

	T *		Next(T * obj)
	{
		PoolItem	* item = reinterpret_cast <PoolItem *>(obj);
		PoolItem	* next = item->next;
		T			* result = NULL;

		if(next)
			result = next->GetObj();

		return result;
	}

	void	Dump(void)
	{
		gLog.Indent();

		_DMESSAGE("free:");
		gLog.Indent();
		for(PoolItem * traverse = m_free; traverse; traverse = traverse->next)
			_DMESSAGE("%08X", traverse);
		gLog.Outdent();

		_DMESSAGE("alloc:");
		gLog.Indent();
		for(PoolItem * traverse = m_alloc; traverse; traverse = traverse->next)
			_DMESSAGE("%08X", traverse);
		gLog.Outdent();

		gLog.Outdent();
	}

	bool	Full(void)
	{
		return m_free == NULL;
	}

	bool	Empty(void)
	{
		return m_alloc == NULL;
	}

	void	Clear(void)
	{
		while(m_alloc)
			Free(m_alloc->GetObj());
	}

private:
	struct PoolItem
	{
		UInt8		obj[sizeof(T)];
		PoolItem	* next;

		T *			GetObj(void)	{ return reinterpret_cast <T *>(obj); }
	};

	PoolItem	m_items[size];
	PoolItem	* m_free;
	PoolItem	* m_alloc;
};

template <typename T, UInt32 size>
class IBasicMemPool
{
public:
	IBasicMemPool()
	:m_free(NULL)
	{
		Reset();
	}

	virtual ~IBasicMemPool()	{ }

	void	Reset(void)
	{
		for(UInt32 i = 0; i < size - 1; i++)
		{
			m_items[i].next = &m_items[i + 1];
		}

		m_items[size - 1].next = NULL;
		m_free = m_items;
	}

	T *		Allocate(void)
	{
		if(m_free)
		{
			PoolItem	* item = m_free;
			m_free = m_free->next;

			T	* obj = item->GetObj();

			new (obj) T;
			return obj;
		}

		return NULL;
	}

	void	Free(T * obj)
	{
		PoolItem	* item = reinterpret_cast <PoolItem *>(obj);

		item->next = m_free;
		m_free = item;

		obj->~T();
	}

	UInt32	GetSize(void)	{ return size; }

	bool	Full(void)
	{
		return m_free == NULL;
	}

	UInt32	GetIdx(T * obj)
	{
		PoolItem	* item = reinterpret_cast <PoolItem *>(obj);

		return item - m_items;
	}

	T *		GetByID(UInt32 id)
	{
		return m_items[id].GetObj();
	}

private:
	union PoolItem
	{
		UInt8		obj[sizeof(T)];
		PoolItem	* next;

		T *			GetObj(void)	{ return reinterpret_cast <T *>(obj); }
	};

	PoolItem	m_items[size];
	PoolItem	* m_free;
};

template <typename T, UInt32 size>
class IThreadSafeBasicMemPool
{
public:
	IThreadSafeBasicMemPool()
	:m_free(NULL)
	{
		Reset();
	}

	virtual ~IThreadSafeBasicMemPool()	{ }

	void	Reset(void)
	{
		m_mutex.Enter();

		for(UInt32 i = 0; i < size - 1; i++)
		{
			m_items[i].next = &m_items[i + 1];
		}

		m_items[size - 1].next = NULL;
		m_free = m_items;

		m_mutex.Leave();
	}

	T *		Allocate(void)
	{
		T	* result = NULL;

		m_mutex.Enter();

		if(m_free)
		{
			PoolItem	* item = m_free;
			m_free = m_free->next;

			m_mutex.Leave();

			result = item->GetObj();

			new (result) T;
		}
		else
		{
			m_mutex.Leave();
		}

		return result;
	}

	void	Free(T * obj)
	{
		PoolItem	* item = reinterpret_cast <PoolItem *>(obj);

		item->next = m_free;

		m_mutex.Enter();

		m_free = item;

		m_mutex.Leave();

		obj->~T();
	}

	UInt32	GetSize(void)	{ return size; }

	bool	Full(void)
	{
		return m_free == NULL;
	}

private:
	union PoolItem
	{
		UInt8		obj[sizeof(T)];
		PoolItem	* next;

		T *			GetObj(void)	{ return reinterpret_cast <T *>(obj); }
	};

	PoolItem	m_items[size];
	PoolItem	* m_free;

	ICriticalSection	m_mutex;
};

void Test_IMemPool(void);
