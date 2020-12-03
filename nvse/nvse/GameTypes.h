#pragma once

#include <list>
#include "Utilities.h"
#include "NiTypes.h"

// 8
class String
{
public:
	String();
	~String();

	char	* m_data;
	UInt16	m_dataLen;
	UInt16	m_bufLen;

	bool	Set(const char * src);
	bool	Includes(const char* toFind) const;
	bool	Replace(const char* toReplace, const char* replaceWith); // replaces instance of toReplace with replaceWith
	bool	Append(const char* toAppend);
	double	Compare(const String& compareTo, bool caseSensitive = false);

	const char *	CStr(void);
};

enum {
	eListCount = -3,
	eListEnd = -2,
	eListInvalid = -1,		
};

typedef void * (* _FormHeap_Allocate)(UInt32 size);
extern const _FormHeap_Allocate FormHeap_Allocate;

typedef void (* _FormHeap_Free)(void * ptr);
extern const _FormHeap_Free FormHeap_Free;

TESForm* __stdcall LookupFormByID(UInt32 refID);

template <class Item>
class tList
{
	typedef Item tItem;

public:
	struct _Node {
		
		tItem*	item;
		_Node*	next;

		tItem* Item() const { return item; }
		_Node* Next() const { return next; }

		// become the next entry and return my item
		tItem* RemoveMe() {
			tItem* pRemoved = item;
			_Node* pNext = next;
			if (pNext) {
				item = pNext->item;
				next = pNext->next;
				FormHeap_Free(pNext);
			} else {
				item = NULL;
				next = NULL;
			}
			return pRemoved;
		}
	};

private:
	_Node m_listHead;

	template <class Op>
	UInt32 FreeNodes(_Node* node, Op &compareOp) const
	{
		static UInt32 nodeCount = 0;
		static UInt32 numFreed = 0;
		static UInt32 lqstNumFreed = 0;

		if (node->Next())
		{
			nodeCount++;
			FreeNodes(node->Next(), compareOp);
			nodeCount--;
		}

		if (compareOp.Accept(node->Item()))
		{
			node->RemoveMe();
			numFreed++;
		}

		if (!nodeCount)	//reset vars after recursing back to head
		{
			lqstNumFreed = numFreed;
			numFreed = 0;
		}

		return lqstNumFreed;
	}


	struct NodePos {
		NodePos(): node(NULL), index(eListInvalid) {}

		_Node*	node;
		SInt32	index;
	};


	NodePos GetNthNode(SInt32 index) const {
		NodePos pos;
		SInt32 n = 0;
		_Node* pCur = Head();

		while (pCur && pCur->Item()) {
			if (n == index) break;
			if (eListEnd == index && !pCur->Next()) break;
			pCur = pCur->Next();
			++n;
		}

		pos.node = pCur;
		pos.index = n;

		return pos;
	}

public:

	void Init(Item *item = NULL)
	{
		m_listHead.item = item;
		m_listHead.next = NULL;
	}

	_Node* Head() const { return const_cast<_Node*>(&m_listHead); }

	class Iterator
	{
		_Node*	m_cur;
	public:
		Iterator() : m_cur(NULL) {}
		Iterator(_Node* node) : m_cur(node) { }
		Iterator operator++()	{ if (!End()) m_cur = m_cur->Next(); return *this;}
		bool End() { return (!m_cur || (!m_cur->item && !m_cur->next)); }
		const Item* operator->() { return (m_cur) ? m_cur->Item() : NULL; }
		const Item* operator*() { return (m_cur) ? m_cur->Item() : NULL; }
		const Iterator& operator=(const Iterator& rhs) {
			m_cur = rhs.m_cur;
			return *this;
		}
		Item* Get() { return (m_cur) ? m_cur->Item() : NULL; }
	};
	
	const Iterator Begin() const { return Iterator(Head()); }


	UInt32 Count() const {
		NodePos pos = GetNthNode(eListCount);
		return (pos.index > 0) ? pos.index : 0;
	};

	Item* GetNthItem(SInt32 n) const {
		NodePos pos = GetNthNode(n);
		return (pos.index == n && pos.node) ? pos.node->Item() : NULL;
	}

	Item *GetFirstItem() const
	{
		return m_listHead.item;
	}

	Item* GetLastItem() const {
		NodePos pos = GetNthNode(eListEnd);
		return pos.node->Item();
	}

	SInt32 AddAt(Item* item, SInt32 index) {
		if (!m_listHead.item) {
			m_listHead.item = item;
			return 0;
		}

		NodePos pos = GetNthNode(index);
		_Node* pTargetNode = pos.node;
		_Node* newNode = (_Node*)FormHeap_Allocate(sizeof(_Node));
		if (newNode && pTargetNode) {
			if (index == eListEnd) {
				pTargetNode->next = newNode;
				newNode->item = item;
				newNode->next = NULL;
			} else {
				newNode->item = pTargetNode->item;
				newNode->next = pTargetNode->next;
				pTargetNode->item = item;
				pTargetNode->next = newNode;
			}
			return pos.index;
		}
		return eListInvalid;
	}

	template <class Op>
	void Visit(Op& op, _Node* prev = NULL) const {
		const _Node* pCur = (prev) ? prev->next : Head();
		bool bContinue = true;
		while (pCur && bContinue) {
			bContinue = op.Accept(pCur->Item());
			if (bContinue) {
				pCur = pCur->next;
			}
		}
	}

	template <class Op>
	Item* Find(Op& op) const
	{
		const _Node* pCur = Head(); 

		bool bFound = false;
		while (pCur && !bFound)
		{
			if (!pCur->Item())
				pCur = pCur->Next();
			else
			{
				bFound = op.Accept(pCur->Item());
				if (!bFound)
					pCur = pCur->Next();
			}
		}
		return (bFound && pCur) ? pCur->Item() : NULL;
	}

	template <class Op>
	Iterator Find(Op& op, Iterator prev) const
	{
		Iterator curIt = (prev.End()) ? Begin() : ++prev;
		bool bFound = false;
		
		while(!curIt.End() && !bFound) {
			const tItem * pCur = *curIt;
			if (pCur) {
				bFound = op.Accept(pCur);
			}
			if (!bFound) {
				++curIt;
			}	
		}
		return curIt;
	}

	const _Node* FindString(char* str, Iterator prev) const
	{
		return Find(StringFinder_CI(str), prev);
	}

	template <class Op>
	UInt32 CountIf(Op& op) const
	{
		UInt32 count = 0;
		const _Node* pCur = Head();
		while (pCur)
		{
			if (pCur->Item() && op.Accept(pCur->Item()))
				count++;
			pCur = pCur->Next();
		}
		return count;
	}

	class AcceptAll {
	public:
		bool Accept(Item* item) {
			return true;
		}
	};

	void RemoveAll() const
	{
		FreeNodes(const_cast<_Node*>(Head()), AcceptAll());
	}

	Item* RemoveNth(SInt32 n) 
	{
		Item* pRemoved = NULL;
		if (n == 0) {
			pRemoved = m_listHead.RemoveMe();
		} else if (n > 0) {
			NodePos nodePos = GetNthNode(n);
			if (nodePos.node && nodePos.index == n) {
				pRemoved = nodePos.node->RemoveMe();
			}
		}
		return pRemoved;
	};

	Item* ReplaceNth(SInt32 n, tItem* item) 
	{
		Item* pReplaced = NULL;
		NodePos nodePos = GetNthNode(n);
		if (nodePos.node && nodePos.index == n) {
			pReplaced = nodePos.node->item;
			nodePos.node->item = item;
		}
		return pReplaced;
	}

	template <class Op>
	UInt32 RemoveIf(Op& op)
	{
		return FreeNodes(const_cast<_Node*>(Head()), op);
	}

	template <class Op>
	SInt32 GetIndexOf(Op& op)
	{
		SInt32 idx = 0;
		const _Node* pCur = Head();
		while (pCur && pCur->Item() && !op.Accept(pCur->Item()))
		{
			idx++;
			pCur = pCur->Next();
		}

		if (pCur && pCur->Item())
			return idx;
		else
			return -1;
	}

};
STATIC_ASSERT(sizeof(tList<void *>) == 0x8);

template <typename T_Data> struct DListNode
{
	DListNode	*next;
	DListNode	*prev;
	T_Data		*data;

	DListNode *Advance(UInt32 times)
	{
		DListNode *result = this;
		while (result && times)
		{
			times--;
			result = result->next;
		}
		return result;
	}

	DListNode *Regress(UInt32 times)
	{
		DListNode *result = this;
		while (result && times)
		{
			times--;
			result = result->prev;
		}
		return result;
	}
};

template <class Item> class DList
{
public:
	typedef DListNode<Item> Node;

private:
	Node		*first;
	Node		*last;
	UInt32		count;

public:
	bool Empty() const {return !first;}
	Node *Head() {return first;}
	Node *Tail() {return last;}
	UInt32 Size() const {return count;}
};

// 010
template <class T>
class BSSimpleList
{
public:
	BSSimpleList<T>();
	~BSSimpleList<T>();

	void**		_vtbl;	// 000
	tList<T>	list;	// 004
};	// 00C
STATIC_ASSERT(sizeof(BSSimpleList<void *>) == 0xC);

//#if RUNTIME
//
//#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
//const UInt32 _NiTMap_Lookup = 0x00853130;
//#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
//const UInt32 _NiTMap_Lookup = 0x00844740;
//#else
//#error
//#endif
//
//#else
//const UInt32 _NiTMap_Lookup = 0;
//#endif

//// 10
//template <typename T_Key, typename T_Data>
//class NiTMapBase
//{
//public:
//	NiTMapBase();
//	virtual ~NiTMapBase();
//
//	virtual UInt32	CalcBucket(T_Key data);
//	virtual bool	CompareKey(T_Key lhs, T_Key rhs);
//	virtual void	Unk_03(void);
//	virtual void	Unk_04(void);
//	virtual void	AllocateNewData(void);
//	virtual void	FreeData(T_Data data);
//
//	DEFINE_MEMBER_FN_LONG(NiTMapBase, Lookup, bool, _NiTMap_Lookup, T_Key key, T_Data * dataOut);
//
//	struct Bucket
//	{
//		Bucket	* next;
//		T_Key	key;
//		T_Data	data;
//	};
//
//	// void		** vtbl				// 00
//	UInt32		m_numBuckets;		// 04
//	Bucket		** m_buckets;		// 08
//	UInt32		m_numItems;			// 0C
//};

//template <typename T>
//struct NiTArray
//{
//	void	* _vtbl;	// 00
//	T		* data;		// 04
//	UInt16	unk08;		// 08 - current size init'd to size of preallocation
//	UInt16	length;		// 0A - init'd to 0
//	UInt16	unk0C;		// 0C - count of not null elements init'd to 0
//	UInt16	unk0E;		// 0E - available size init'd to size of preallocation
//
//	T operator[](UInt32 idx) {
//		if (idx < length)
//			return data[idx];
//		return NULL;
//	}
//
//	T Get(UInt32 idx) { return (*this)[idx]; }
//};

template <typename T>
struct BSSimpleArray
{
	void	* _vtbl;		// 00
	T		* data;			// 04
	UInt32	size;			// 08
	UInt32	alloc;			// 0C

	// this only compiles for pointer types
	T operator[](UInt32 idx) { if (idx < size) 
		return data[idx]; 
	return NULL; }
};

//// this is a NiTPointerMap <UInt32, T_Data>
//// todo: generalize key
//template <typename T_Data>
//class NiTPointerMap
//{
//public:
//	NiTPointerMap();
//	virtual ~NiTPointerMap();
//
//	struct Entry
//	{
//		Entry	* next;
//		UInt32	key;
//		T_Data	* data;
//	};
//
//	// note: traverses in non-numerical order
//	class Iterator
//	{
//		friend NiTPointerMap;
//
//	public:
//		Iterator(NiTPointerMap * table, Entry * entry = NULL, UInt32 bucket = 0)
//			:m_table(table), m_entry(entry), m_bucket(bucket) { FindValid(); }
//		~Iterator() { }
//
//		T_Data *	Get(void);
//		UInt32		GetKey(void);
//		bool		Next(void);
//		bool		Done(void);
//
//	private:
//		void		FindValid(void);
//
//		NiTPointerMap	* m_table;
//		Entry		* m_entry;
//		UInt32		m_bucket;
//	};
//
//	virtual UInt32	CalculateBucket(UInt32 key);
//	virtual bool	CompareKey(UInt32 lhs, UInt32 rhs);
//	virtual void	Fn_03(void);
//	virtual void	Fn_04(void);
//	virtual void	Fn_05(void);
//	virtual void	Fn_06(void);
//
//	T_Data *	Lookup(UInt32 key);
//
//	UInt32	m_numBuckets;
//	Entry	** m_buckets;
//	UInt32	m_numItems;
//};

//template <typename T_Data>
//T_Data * NiTPointerMap <T_Data>::Lookup(UInt32 key)
//{
//	for(Entry * traverse = m_buckets[key % m_numBuckets]; traverse; traverse = traverse->next)
//		if(traverse->key == key)
//			return traverse->data;
//
//	return NULL;
//}
//
//template <typename T_Data>
//T_Data * NiTPointerMap <T_Data>::Iterator::Get(void)
//{
//	if(m_entry)
//		return m_entry->data;
//
//	return NULL;
//}
//
//template <typename T_Data>
//UInt32 NiTPointerMap <T_Data>::Iterator::GetKey(void)
//{
//	if(m_entry)
//		return m_entry->key;
//
//	return 0;
//}
//
//template <typename T_Data>
//bool NiTPointerMap <T_Data>::Iterator::Next(void)
//{
//	if(m_entry)
//		m_entry = m_entry->next;
//
//	while(!m_entry && (m_bucket < (m_table->m_numBuckets - 1)))
//	{
//		m_bucket++;
//
//		m_entry = m_table->m_buckets[m_bucket];
//	}
//
//	return m_entry != NULL;
//}
//
//template <typename T_Data>
//bool NiTPointerMap <T_Data>::Iterator::Done(void)
//{
//	return m_entry == NULL;
//}
//
//template <typename T_Data>
//void NiTPointerMap <T_Data>::Iterator::FindValid(void)
//{
//	// validate bucket
//	if(m_bucket >= m_table->m_numBuckets) return;
//
//	// get bucket
//	m_entry = m_table->m_buckets[m_bucket];
//
//	// find non-empty bucket
//	while(!m_entry && (m_bucket < (m_table->m_numBuckets - 1)))
//	{
//		m_bucket++;
//
//		m_entry = m_table->m_buckets[m_bucket];
//	}
//}

template <class Node, class Info>
class Visitor
{
	const Node* m_pHead;

	template <class Op>
	UInt32 FreeNodes(Node* node, Op &compareOp) const
	{
		static UInt32 nodeCount = 0;
		static UInt32 numFreed = 0;
		static Node* lastNode = NULL;
		static bool bRemovedNext = false;
		
		UInt32 returnCount;

		if (node->Next())
		{
			nodeCount++;
			FreeNodes(node->Next(), compareOp);
			nodeCount--;
		}

		if (compareOp.Accept(node->Info()))
		{
			if (nodeCount)
				node->Delete();
			else
				node->DeleteHead(lastNode);
			numFreed++;
			bRemovedNext = true;
		}
		else
		{
			if (bRemovedNext)
				node->SetNext(lastNode);
			bRemovedNext = false;
			lastNode = node;
		}

		returnCount = numFreed;

		if (!nodeCount)	//reset vars after recursing back to head
		{
			numFreed = 0;
			lastNode = NULL;
			bRemovedNext = false;
		}

		return returnCount;
	}

	class AcceptAll {
	public:
		bool Accept(Info* info) {
			return true;
		}
	};

	class AcceptEqual {
		const Info	* m_toMatch;
	public:
		AcceptEqual(const Info* info) : m_toMatch(info) { }
		bool Accept(const Info* info) {
			return info == m_toMatch;
		}
	};

	class AcceptStriCmp {
		const char * m_toMatch;
	public:
		AcceptStriCmp(const char* info) : m_toMatch(info) { }
		bool Accept(const char* info) {
			if (m_toMatch && info)
				return _stricmp(info, m_toMatch) ? false : true;
			return false;
		}
	};
public:
	Visitor(const Node* pHead) : m_pHead(pHead) { }
	
	UInt32 Count() const {
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur && pCur->Info() != NULL) {
			++count;
			pCur = pCur->Next();
		}
		return count;
	}

	Info* GetNthInfo(UInt32 n) const {
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur && count < n && pCur->Info() != NULL) {
			++count;
			pCur = pCur->Next();
		}
		return (count == n && pCur) ? pCur->Info() : NULL;
	}

	template <class Op>
	void Visit(Op& op) const {
		const Node* pCur = m_pHead;
		bool bContinue = true;
		while (pCur && pCur->Info() && bContinue) {
			bContinue = op.Accept(pCur->Info());
			if (bContinue) {
				pCur = pCur->Next();
			}
		}
	}

	template <class Op>
	const Node* Find(Op& op, const Node* prev = NULL) const 
	{
		const Node* pCur;
		if (!prev)
			pCur = m_pHead;
		else
			pCur = prev->next;
		bool bFound = false;
		while (pCur && !bFound)
		{
			if (!pCur->Info())
				pCur = pCur->Next();
			else
			{
				bFound = op.Accept(pCur->Info());
				if (!bFound)
					pCur = pCur->Next();
			}
		}

		return pCur;
	}

	Node* FindInfo(const Info* toMatch) {
		return Find(AcceptEqual(toMatch));
	}

	const Node* FindString(char* str, const Node* prev = NULL) const
	{
		return Find(StringFinder_CI(str), prev);
	}

	template <class Op>
	UInt32 CountIf(Op& op) const
	{
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur)
		{
			if (pCur->Info() && op.Accept(pCur->Info()))
				count++;
			pCur = pCur->Next();
		}
		return count;
	}

	const Node* GetLastNode() const
	{
		const Node* pCur = m_pHead;
		while (pCur && pCur->Next())
			pCur = pCur->Next();
		return pCur;
	}

	void RemoveAll() const
	{
		FreeNodes(const_cast<Node*>(m_pHead), AcceptAll());
	}

	template <class Op>
	UInt32 RemoveIf(Op& op)
	{
		return FreeNodes(const_cast<Node*>(m_pHead), op);
	}

	bool Remove(const Info* toRemove)
	{
		return RemoveIf(AcceptEqual(toRemove)) ? true : false;
	}

	bool RemoveString(const char* toRemove)
	{
		return RemoveIf(AcceptStriCmp(toRemove)) ? true : false;
	}

	void Append(Node* newNode)
	{
		Node* lastNode = const_cast<Node*>(GetLastNode());
		if (lastNode == m_pHead && !m_pHead->Info())
			lastNode->DeleteHead(newNode);
		else
			lastNode->SetNext(newNode);
	}

	template <class Op>
	UInt32 GetIndexOf(Op& op)
	{
		UInt32 idx = 0;
		const Node* pCur = m_pHead;
		while (pCur && pCur->Info() && !op.Accept(pCur->Info()))
		{
			idx++;
			pCur = pCur->Next();
		}

		if (pCur && pCur->Info())
			return idx;
		else
			return -1;
	}
};
