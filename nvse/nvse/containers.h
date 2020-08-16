#pragma once

#include <type_traits>

#define MAP_DEFAULT_ALLOC			8
#define MAP_DEFAULT_BUCKET_COUNT	8
#define MAP_MAX_BUCKET_COUNT		0x40000
#define VECTOR_DEFAULT_ALLOC		8

void __fastcall AllocDataArray(void *container, UInt32 dataSize);
UInt32 __fastcall AlignBucketCount(UInt32 count);
void* __fastcall AllocMapEntry(UInt32 entrySize);
void __fastcall ScrapMapEntry(void *entry, UInt32 entrySize);

template <typename T_Key> class MapKey
{
	T_Key		key;

public:
	T_Key Get() const {return key;}
	void Set(T_Key inKey) {key = inKey;}
	char Compare(T_Key inKey) const
	{
		if (inKey == key) return 0;
		return (inKey < key) ? -1 : 1;
	}
	void Clear() {key.~T_Key();}
};

template <> class MapKey<const char*>
{
	const char	*key;

public:
	const char *Get() const {return key;}
	void Set(const char *inKey) {key = inKey;}
	char Compare(const char *inKey) const {return StrCompare(inKey, key);}
	void Clear() {}
};

template <> class MapKey<char*>
{
	char		*key;

public:
	char *Get() const {return key;}
	void Set(char *inKey) {key = CopyString(inKey);}
	char Compare(char *inKey) const {return StrCompare(inKey, key);}
	void Clear() {free(key);}
};

template <typename T_Data> class MapValue
{
	T_Data		value;

public:
	T_Data *Init() {return &value;}
	T_Data Get() const {return value;}
	T_Data *Ptr() const {return &value;}
	void Clear() {value.~T_Data();}
};

template <typename T_Data> class MapValue_p
{
	T_Data		*value;

public:
	T_Data *Init()
	{
		value = (T_Data*)malloc(sizeof(T_Data));
		return value;
	}
	T_Data Get() const {return *value;}
	T_Data *Ptr() const {return value;}
	void Clear()
	{
		value->~T_Data();
		free(value);
	}
};

template <typename T_Key, typename T_Data> class Map
{
protected:
	typedef MapKey<T_Key> M_Key;
	typedef std::conditional_t<sizeof(T_Data) <= 4, MapValue<T_Data>, MapValue_p<T_Data>> M_Value;

	struct Entry
	{
		M_Key		key;
		M_Value		value;

		void Clear()
		{
			key.Clear();
			value.Clear();
		}
	};

	Entry		*entries;		// 00
	UInt32		numEntries;		// 04
	UInt32		numAlloc;		// 08

	bool GetIndex(T_Key key, UInt32 *outIdx) const
	{
		UInt32 lBound = 0, uBound = numEntries, index;
		char cmpr;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			cmpr = entries[index].key.Compare(key);
			if (!cmpr)
			{
				*outIdx = index;
				return true;
			}
			if (cmpr < 0) uBound = index;
			else lBound = index + 1;
		}
		*outIdx = lBound;
		return false;
	}

	bool InsertKey(T_Key key, T_Data **outData)
	{
		UInt32 index;
		if (GetIndex(key, &index))
		{
			*outData = entries[index].value.Ptr();
			return false;
		}
		AllocDataArray(this, sizeof(Entry));
		Entry *pEntry = entries + index;
		index = numEntries - index;
		if (index) MemCopy(pEntry + 1, pEntry, sizeof(Entry) * index);
		numEntries++;
		pEntry->key.Set(key);
		*outData = pEntry->value.Init();
		return true;
	}

	Entry *End() const {return entries + numEntries;}

public:
	Map(UInt32 _alloc = MAP_DEFAULT_ALLOC) : entries(NULL), numEntries(0), numAlloc(_alloc) {}
	~Map()
	{
		if (!entries) return;
		Clear();
		free(entries);
		entries = NULL;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	bool Insert(T_Key key, T_Data **outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](T_Key key)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(T_Key key, Args&& ...args)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	bool HasKey(T_Key key) const
	{
		UInt32 index;
		return GetIndex(key, &index);
	}

	T_Data Get(T_Key key) const
	{
		UInt32 index;
		return GetIndex(key, &index) ? entries[index].value.Get() : NULL;
	}

	T_Data* GetPtr(T_Key key) const
	{
		UInt32 index;
		return GetIndex(key, &index) ? entries[index].value.Ptr() : NULL;
	}

	bool Erase(T_Key key)
	{
		UInt32 index;
		if (!GetIndex(key, &index)) return false;
		Entry *pEntry = entries + index;
		pEntry->Clear();
		numEntries--;
		index = numEntries - index;
		if (index) MemCopy(pEntry, pEntry + 1, sizeof(Entry) * index);
		return true;
	}

	void Clear()
	{
		if (!numEntries) return;
		Entry *pEntry = entries, *pEnd = End();
		do
		{
			pEntry->Clear();
			pEntry++;
		}
		while (pEntry != pEnd);
		numEntries = 0;
	}

	class Iterator
	{
		friend Map;

		Entry		*pEntry;
		Entry		*pEnd;

	public:
		T_Key Key() const {return pEntry->key.Get();}
		T_Data& Get() const {return *(pEntry->value.Ptr());}
		T_Data& operator*() const {return pEntry->value.Get();}
		T_Data operator->() const {return pEntry->value.Get();}
		bool End() const {return pEntry == pEnd;}

		void operator++() {pEntry++;}
		void operator--() {pEntry--;}

		void Find(Map *source, T_Key key)
		{
			pEnd = source->End();
			UInt32 index;
			pEntry = source->GetIndex(key, &index) ? (source->entries + index) : pEnd;
		}

		Iterator() {}
		Iterator(Map &source) : pEntry(source.entries), pEnd(source.End()) {}
		Iterator(Map &source, T_Key key) {Find(&source, key);}
	};

	class OpIterator : public Iterator
	{
		Map			*table;

	public:
		Map* Table() const {return table;}

		void Remove(bool frwrd = true)
		{
			table->numEntries--;
			pEntry->Clear();
			UInt32 size = (UInt32)table->End() - (UInt32)pEntry;
			if (size) MemCopy(pEntry, pEntry + 1, size);
			if (frwrd)
			{
				pEntry--;
				pEnd--;
			}
		}

		OpIterator(Map &source) : table(&source)
		{
			pEntry = table->entries;
			pEnd = table->End();
		}
		OpIterator(Map &source, T_Key key) : table(&source) {Find(&source, key);}
		OpIterator(Map &source, T_Key key, bool frwrd) : table(&source)
		{
			if (table->numEntries)
			{
				UInt32 index;
				bool match = table->GetIndex(key, &index);
				if (frwrd)
				{
					pEntry = table->entries + index;
					pEnd = table->End();
				}
				else
				{
					if (!match) index--;
					pEntry = table->entries + index;
					pEnd = table->entries - 1;
				}
			}
			else pEnd = pEntry = NULL;
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Map &source)
		{
			if (source.numEntries)
			{
				UInt32 size = source.numEntries * sizeof(Entry);
				pEntry = (Entry*)MemCopy(GetAuxBuffer(s_auxBuffers[0], size), source.entries, size);
				pEnd = pEntry + source.numEntries;
			}
			else pEnd = pEntry = NULL;
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(T_Key key) {return Iterator(*this, key);}

	OpIterator BeginOp() {return OpIterator(*this);}
	OpIterator FindOp(T_Key key) {return OpIterator(*this, key);}
	OpIterator FindOpDir(T_Key key, bool frwrd) {return OpIterator(*this, key, frwrd);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Key> class Set
{
protected:
	typedef MapKey<T_Key> M_Key;

	M_Key		*keys;		// 00
	UInt32		numKeys;	// 04
	UInt32		numAlloc;	// 08

	bool GetIndex(T_Key key, UInt32 *outIdx) const
	{
		UInt32 lBound = 0, uBound = numKeys, index;
		char cmpr;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			cmpr = keys[index].Compare(key);
			if (!cmpr)
			{
				*outIdx = index;
				return true;
			}
			if (cmpr < 0) uBound = index;
			else lBound = index + 1;
		}
		*outIdx = lBound;
		return false;
	}

	M_Key *End() const {return keys + numKeys;}

public:
	Set(UInt32 _alloc = MAP_DEFAULT_ALLOC) : keys(NULL), numKeys(0), numAlloc(_alloc) {}
	~Set()
	{
		if (!keys) return;
		Clear();
		free(keys);
		keys = NULL;
	}

	UInt32 Size() const {return numKeys;}
	bool Empty() const {return !numKeys;}
	T_Key *Keys() {return reinterpret_cast<T_Key*>(keys);}

	bool Insert(T_Key key)
	{
		UInt32 index;
		if (GetIndex(key, &index)) return false;
		AllocDataArray(this, sizeof(M_Key));
		M_Key *pKey = keys + index;
		index = numKeys - index;
		if (index) MemCopy(pKey + 1, pKey, sizeof(M_Key) * index);
		numKeys++;
		pKey->Set(key);
		return true;
	}

	bool HasKey(T_Key key) const
	{
		UInt32 index;
		return GetIndex(key, &index);
	}

	bool Erase(T_Key key)
	{
		UInt32 index;
		if (!GetIndex(key, &index)) return false;
		M_Key *pKey = keys + index;
		pKey->Clear();
		numKeys--;
		index = numKeys - index;
		if (index) MemCopy(pKey, pKey + 1, sizeof(M_Key) * index);
		return true;
	}

	void Clear()
	{
		if (!numKeys) return;
		M_Key *pKey = keys, *pEnd = End();
		do
		{
			pKey->Clear();
			pKey++;
		}
		while (pKey != pEnd);
		numKeys = 0;
	}

	class Iterator
	{
		friend Set;

		M_Key		*pKey;
		M_Key		*pEnd;

	public:
		T_Key operator*() const {return pKey->Get();}
		T_Key operator->() const {return pKey->Get();}
		bool End() const {return pKey == pEnd;}

		void operator++() {pKey++;}

		Iterator() {}
		Iterator(Set &source) : pKey(source.keys), pEnd(source.End()) {}
		Iterator(Set &source, T_Key key) : pEnd(source.End())
		{
			UInt32 index;
			pKey = source.GetIndex(key, &index) ? (source.keys + index) : pEnd;
		}
	};

	class OpIterator : public Iterator
	{
		Set			*table;

	public:
		Set* Table() const {return table;}

		void Remove()
		{
			table->numKeys--;
			pKey->Clear();
			pEnd--;
			UInt32 size = (UInt32)pEnd - (UInt32)pKey;
			if (size) MemCopy(pKey, pKey + 1, size);
			pKey--;
		}

		OpIterator(Set &source) : table(&source)
		{
			pKey = table->keys;
			pEnd = table->End();
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Set &source)
		{
			if (source.numKeys)
			{
				UInt32 size = source.numKeys * sizeof(M_Key);
				pKey = (M_Key*)MemCopy(GetAuxBuffer(s_auxBuffers[1], size), source.keys, size);
				pEnd = pKey + source.numKeys;
			}
			else pEnd = pKey = NULL;
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(T_Key key) {return Iterator(*this, key);}

	OpIterator BeginOp() {return OpIterator(*this);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Key> __forceinline UInt32 HashKey(T_Key inKey)
{
	UInt32 uKey = *(UInt32*)&inKey;
	if (sizeof(T_Key) > 4)
		uKey += uKey ^ ((UInt32*)&inKey)[1];
	return (uKey * 0xD) ^ (uKey >> 0xF);
}

template <> __forceinline UInt32 HashKey<const char*>(const char *inKey)
{
	return StrHashCI(inKey);
}

template <> __forceinline UInt32 HashKey<char*>(char *inKey)
{
	return StrHashCI(inKey);
}

template <typename T_Key> class HashedKey
{
	T_Key		key;

public:
	bool Match(T_Key inKey, UInt32) const {return key == inKey;}
	T_Key Get() const {return key;}
	void Set(T_Key inKey, UInt32) {key = inKey;}
	UInt32 GetHash() const {return HashKey<T_Key>(key);}
	void Clear() {key.~T_Key();}
};

template <> class HashedKey<const char*>
{
	UInt32		hashVal;

public:
	bool Match(const char*, UInt32 inHash) const {return hashVal == inHash;}
	const char *Get() const {return "";}
	void Set(const char*, UInt32 inHash) {hashVal = inHash;}
	UInt32 GetHash() const {return hashVal;}
	void Clear() {}
};

template <> class HashedKey<char*>
{
	UInt32		hashVal;
	char		*key;

public:
	bool Match(char*, UInt32 inHash) const {return hashVal == inHash;}
	char *Get() const {return key;}
	void Set(char *inKey, UInt32 inHash)
	{
		hashVal = inHash;
		key = CopyString(inKey);
	}
	UInt32 GetHash() const {return hashVal;}
	void Clear() {free(key);}
};

template <typename T_Key, typename T_Data> class UnorderedMap
{
protected:
	typedef HashedKey<T_Key> H_Key;

	struct Entry
	{
		Entry		*next;
		H_Key		key;
		T_Data		value;

		void Clear()
		{
			key.Clear();
			value.~T_Data();
		}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				ScrapMapEntry(pEntry, sizeof(Entry));
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)calloc(newCount, sizeof(Bucket));
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

	void RemoveEntry(Entry *entry)
	{
		numEntries--;
		entry->Clear();
		ScrapMapEntry(entry, sizeof(Entry));
	}

	Entry *FindEntry(T_Key key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return pEntry;
		}
		return NULL;
	}

	bool InsertKey(T_Key key, T_Data **outData)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)calloc(numBuckets, sizeof(Bucket));
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
		{
			if (!pEntry->key.Match(key, hashVal)) continue;
			*outData = &pEntry->value;
			return false;
		}
		numEntries++;
		Entry *newEntry = (Entry*)AllocMapEntry(sizeof(Entry));
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		*outData = &newEntry->value;
		return true;
	}

public:
	UnorderedMap(UInt32 _numBuckets = MAP_DEFAULT_BUCKET_COUNT) : buckets(NULL), numBuckets(_numBuckets), numEntries(0) {}
	~UnorderedMap()
	{
		if (!buckets) return;
		Clear();
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(T_Key key, T_Data **outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](T_Key key)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(T_Key key, Args&& ...args)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	T_Data InsertNotIn(T_Key key, T_Data value)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			*outData = value;
		return value;
	}

	bool HasKey(T_Key key) const {return FindEntry(key) ? true : false;}

	T_Data Get(T_Key key) const
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? pEntry->value : NULL;
	}

	T_Data* GetPtr(T_Key key) const
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? &pEntry->value : NULL;
	}

	bool Erase(T_Key key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = NULL;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					if (prev) prev->next = pEntry->next;
					else pBucket->entries = pEntry->next;
					RemoveEntry(pEntry);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	T_Data GetErase(T_Key key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = NULL;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					if (prev) prev->next = pEntry->next;
					else pBucket->entries = pEntry->next;
					T_Data outVal = pEntry->value;
					RemoveEntry(pEntry);
					return outVal;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return NULL;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	void DumpLoads()
	{
		UInt32 loadsArray[0x20];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *pBucket = buckets;
		UInt32 maxLoad = 0, entryCount;
		for (Bucket *pEnd = End(); pBucket != pEnd; pBucket++)
		{
			entryCount = pBucket->Size();
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numEntries, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%05d (%.4f%%)", iter, loadsArray[iter], 100.0 * (double)loadsArray[iter] / numEntries);
	}

	class Iterator
	{
		friend UnorderedMap;

		UnorderedMap	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedMap &_table)
		{
			table = &_table;
			entry = NULL;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		void Find(T_Key key)
		{
			if (!table->numEntries)
			{
				entry = NULL;
				return;
			}
			UInt32 hashVal = HashKey<T_Key>(key);
			bucket = &table->buckets[hashVal & (table->numBuckets - 1)];
			entry = bucket->entries;
			while (entry)
			{
				if (entry->key.Match(key, hashVal))
					break;
				entry = entry->next;
			}
		}

		UnorderedMap* Table() const {return table;}
		T_Key Key() const {return entry->key.Get();}
		T_Data& Get() const {return entry->value;}
		T_Data& operator*() const {return entry->value;}
		T_Data operator->() const {return entry->value;}
		bool End() const {return !entry;}

		void operator++()
		{
			if (entry)
				entry = entry->next;
			else entry = bucket->entries;
			if (!entry && table->numEntries)
			{
				bucket++;
				FindNonEmpty();
			}
		}

		bool IsValid()
		{
			if (entry)
			{
				for (Entry *temp = bucket->entries; temp; temp = temp->next)
					if (temp == entry) return true;
				entry = NULL;
			}
			return false;
		}

		void Remove()
		{
			Entry *curr = bucket->entries, *prev = NULL;
			do
			{
				if (curr == entry) break;
				prev = curr;
			}
			while (curr = curr->next);
			if (prev) prev->next = entry->next;
			else bucket->entries = entry->next;
			table->RemoveEntry(entry);
			entry = prev;
		}

		Iterator() : table(NULL), entry(NULL) {}
		Iterator(UnorderedMap &_table) {Init(_table);}
		Iterator(UnorderedMap &_table, T_Key key) : table(&_table) {Find(key);}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(T_Key key) {return Iterator(*this, key);}
};

template <typename T_Key> class UnorderedSet
{
protected:
	typedef HashedKey<T_Key> H_Key;

	struct Entry
	{
		Entry		*next;
		H_Key		key;

		void Clear() {key.Clear();}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				ScrapMapEntry(pEntry, sizeof(Entry));
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)calloc(newCount, sizeof(Bucket));
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

	void RemoveEntry(Entry *entry)
	{
		numEntries--;
		entry->Clear();
		ScrapMapEntry(entry, sizeof(Entry));
	}

public:
	UnorderedSet(UInt32 _numBuckets = MAP_DEFAULT_BUCKET_COUNT) : buckets(NULL), numBuckets(_numBuckets), numEntries(0) {}
	~UnorderedSet()
	{
		if (!buckets) return;
		Clear();
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(T_Key key)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)calloc(numBuckets, sizeof(Bucket));
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
			if (pEntry->key.Match(key, hashVal)) return false;
		numEntries++;
		Entry *newEntry = (Entry*)AllocMapEntry(sizeof(Entry));
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		return true;
	}

	bool HasKey(T_Key key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return true;
		}
		return false;
	}

	bool Erase(T_Key key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = NULL;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					if (prev) prev->next = pEntry->next;
					else pBucket->entries = pEntry->next;
					RemoveEntry(pEntry);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	void DumpLoads()
	{
		UInt32 loadsArray[0x20];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *pBucket = buckets;
		UInt32 maxLoad = 0, entryCount;
		for (Bucket *pEnd = End(); pBucket != pEnd; pBucket++)
		{
			entryCount = pBucket->Size();
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numEntries, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%05d (%.4f%%)", iter, loadsArray[iter], 100.0 * (double)loadsArray[iter] / numEntries);
	}

	class Iterator
	{
		friend UnorderedSet;

		UnorderedSet	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedSet &_table)
		{
			table = &_table;
			entry = NULL;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		T_Key operator*() const {return entry->key.Get();}
		T_Key operator->() const {return entry->key.Get();}
		bool End() const {return !entry;}

		void operator++()
		{
			if ((entry = entry->next) || !table->numEntries)
				return;
			bucket++;
			FindNonEmpty();
		}

		Iterator() : table(NULL), entry(NULL) {}
		Iterator(UnorderedSet &_table) {Init(_table);}
	};

	Iterator Begin() {return Iterator(*this);}
};

template <typename T_Data> class Vector
{
protected:
	T_Data		*data;		// 00
	UInt32		numItems;	// 04
	UInt32		numAlloc;	// 08

	T_Data *AllocateData()
	{
		AllocDataArray(this, sizeof(T_Data));
		return data + numItems++;
	}

	T_Data *End() const {return data + numItems;}

public:
	Vector(UInt32 _alloc = VECTOR_DEFAULT_ALLOC) : data(NULL), numItems(0), numAlloc(_alloc) {}
	~Vector()
	{
		if (!data) return;
		Clear();
		free(data);
		data = NULL;
	}

	UInt32 Size() const {return numItems;}
	bool Empty() const {return !numItems;}

	T_Data* Data() const {return data;}

	T_Data& operator[](UInt32 index) const {return data[index];}

	T_Data Get(UInt32 index) const {return (index < numItems) ? data[index] : NULL;}

	T_Data* GetPtr(UInt32 index) const {return (index < numItems) ? (data + index) : NULL;}

	T_Data Top() const {return numItems ? data[numItems - 1] : NULL;}

	void Append(const T_Data item)
	{
		T_Data *pData = AllocateData();
		*pData = item;
	}

	void Concatenate(const Vector &source)
	{
		if (!source.numItems) return;
		UInt32 newCount = numItems + source.numItems;
		if (!data)
		{
			if (numAlloc < newCount) numAlloc = newCount;
			data = (T_Data*)malloc(sizeof(T_Data) * numAlloc);
		}
		else if (numAlloc < newCount)
		{
			numAlloc = newCount;
			data = (T_Data*)realloc(data, sizeof(T_Data) * numAlloc);
		}
		MemCopy(data + numItems, source.data, sizeof(T_Data) * source.numItems);
		numItems = newCount;
	}

	void Insert(const T_Data &item, UInt32 index)
	{
		if (index <= numItems)
		{
			UInt32 size = numItems - index;
			T_Data *pData = AllocateData();
			if (size)
			{
				pData = data + index;
				MemCopy(pData + 1, pData, sizeof(T_Data) * size);
			}
			*pData = item;
		}
	}

	UInt32 InsertSorted(const T_Data &item)
	{
		UInt32 lBound = 0, uBound = numItems, index;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			if (item < data[index]) uBound = index;
			else lBound = index + 1;
		}
		uBound = numItems - lBound;
		T_Data *pData = AllocateData();
		if (uBound)
		{
			pData = data + lBound;
			MemCopy(pData + 1, pData, sizeof(T_Data) * uBound);
		}
		*pData = item;
		return lBound;
	}

	template <typename ...Args>
	T_Data* Emplace(Args&& ...args)
	{
		return new (AllocateData()) T_Data(std::forward<Args>(args)...);
	}

	bool AppendNotIn(const T_Data &item)
	{
		T_Data *pData;
		if (numItems)
		{
			pData = data;
			T_Data *pEnd = End();
			do
			{
				if (*pData == item)
					return false;
				pData++;
			}
			while (pData != pEnd);
		}
		pData = AllocateData();
		*pData = item;
		return true;
	}

	SInt32 GetIndexOf(T_Data item) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			UInt32 index = 0;
			do
			{
				if (*pData == item)
					return index;
				pData++;
				index++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Finder>
	SInt32 GetIndexOf(Finder &finder) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			UInt32 index = 0;
			do
			{
				if (finder.Match(*pData))
					return index;
				pData++;
				index++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Finder>
	T_Data* Find(Finder &finder) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (finder.Match(*pData))
					return pData;
				pData++;
			}
			while (pData != pEnd);
		}
		return NULL;
	}

	bool RemoveNth(UInt32 index)
	{
		if (index >= numItems) return false;
		T_Data *pData = data + index;
		pData->~T_Data();
		numItems--;
		index = numItems - index;
		if (index) MemCopy(pData, pData + 1, sizeof(T_Data) * index);
		return true;
	}

	bool Remove(T_Data item)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			do
			{
				pData--;
				if (*pData != item) continue;
				numItems--;
				pData->~T_Data();
				UInt32 size = (UInt32)End() - (UInt32)pData;
				if (size) MemCopy(pData, pData + 1, size);
				return true;
			}
			while (pData != pEnd);
		}
		return false;
	}

	template <class Finder>
	UInt32 Remove(Finder &finder)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			UInt32 removed = 0, size;
			do
			{
				pData--;
				if (!finder.Match(*pData)) continue;
				numItems--;
				pData->~T_Data();
				size = (UInt32)End() - (UInt32)pData;
				if (size) MemCopy(pData, pData + 1, size);
				removed++;
			}
			while (pData != pEnd);
			return removed;
		}
		return 0;
	}

	void RemoveRange(UInt32 beginIdx, UInt32 count)
	{
		if (beginIdx >= numItems) return;
		if (count > (numItems - beginIdx))
			count = numItems - beginIdx;
		T_Data *pData = data + beginIdx + count;
		for (UInt32 index = count; index; index--)
		{
			pData--;
			pData->~T_Data();
		}
		if ((beginIdx + count) < numItems)
			MemCopy(pData, pData + count, sizeof(T_Data) * (numItems - beginIdx - count));
		numItems -= count;
	}

	T_Data Pop()
	{
		if (!numItems) return NULL;
		numItems--;
		T_Data *pEnd = End();
		pEnd->~T_Data();
		return *pEnd;
	}

	void Clear()
	{
		if (!numItems) return;
		T_Data *pData = data, *pEnd = End();
		do
		{
			pData->~T_Data();
			pData++;
		}
		while (pData != pEnd);
		numItems = 0;
	}

	void QuickSort(UInt32 p, UInt32 q)
	{
		if (p >= q) return;
		UInt32 i = p;
		UInt8 buffer[sizeof(T_Data)];
		T_Data *temp = (T_Data*)&buffer;
		for (UInt32 j = p + 1; j < q; j++)
		{
			if (data[p] < data[j])
				continue;
			i++;
			*temp = data[i];
			data[i] = data[j];
			data[j] = *temp;
		}
		*temp = data[i];
		data[i] = data[p];
		data[p] = *temp;
		QuickSort(p, i);
		QuickSort(i + 1, q);
	}

	void Shuffle()
	{
		UInt8 buffer[sizeof(T_Data)];
		T_Data *temp = (T_Data*)&buffer;
		UInt32 idx = numItems, rand;
		while (idx > 1)
		{
			rand = GetRandomUInt(idx);
			idx--;
			if (rand == idx) continue;
			*temp = data[rand];
			data[rand] = data[idx];
			data[idx] = *temp;
		}
	}

	class Iterator
	{
		friend Vector;

		T_Data		*pData;
		T_Data		*pEnd;

	public:
		bool End() const {return pData == pEnd;}
		void operator++() {pData++;}

		T_Data& operator*() const {return *pData;}
		T_Data& operator->() const {return *pData;}
		T_Data& Get() const {return *pData;}

		Iterator() {}
		Iterator(Vector &source) : pData(source.data), pEnd(source.End()) {}
		Iterator(Vector &source, UInt32 index) : pEnd(source.End())
		{
			pData = (source.numItems > index) ? (source.data + index) : pEnd;
		}
	};

	class RvIterator : public Iterator
	{
		Vector		*contObj;

	public:
		Vector* Container() const {return contObj;}

		void operator--() {pData--;}

		void Remove()
		{
			contObj->numItems--;
			pData->~T_Data();
			UInt32 size = (UInt32)contObj->End() - (UInt32)pData;
			if (size) MemCopy(pData, pData + 1, size);
		}

		RvIterator(Vector &source) : contObj(&source)
		{
			pEnd = contObj->data - 1;
			pData = pEnd + contObj->numItems;
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Vector &source)
		{
			if (source.numItems)
			{
				UInt32 size = source.numItems * sizeof(T_Data);
				pData = (T_Data*)MemCopy(GetAuxBuffer(s_auxBuffers[2], size), source.data, size);
				pEnd = pData + source.numItems;
			}
			else pEnd = pData = NULL;
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator BeginAt(UInt32 index) {return Iterator(*this, index);}

	RvIterator BeginRv() {return RvIterator(*this);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Data, UInt32 size> class FixedTypeArray
{
protected:
	UInt32		numItems;
	T_Data		data[size];

public:
	FixedTypeArray() : numItems(0) {}

	UInt32 Size() const {return numItems;}
	bool Empty() const {return !numItems;}
	T_Data *Data() {return data;}

	bool Append(T_Data item)
	{
		if (numItems >= size) return false;
		data[numItems++] = item;
		return true;
	}

	T_Data PopBack()
	{
		return numItems ? data[--numItems] : 0;
	}

	class Iterator
	{
		friend FixedTypeArray;

		T_Data		*pData;
		T_Data		*pEnd;

	public:
		bool End() const {return pData == pEnd;}
		void operator++() {pData++;}

		T_Data& operator*() const {return *pData;}
		T_Data& operator->() const {return *pData;}
		T_Data& Get() const {return *pData;}

		Iterator(FixedTypeArray &source) : pData(source.data), pEnd(source.data + source.numItems) {}
	};
};
