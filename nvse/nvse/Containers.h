#pragma once
#include "Utilities.h"

enum
{
	kMapDefaultAlloc = 8,

	kMapDefaultBucketNum = 11,
	kMapBucketSizeInc = 2,

	kVectorDefaultAlloc = 2,
};

extern UInt8 s_cpIteratorBuffer[0x1000];

template <typename T_Key> class MappedKey
{
	T_Key		key;

public:
	T_Key Get() const { return key; }
	void Set(T_Key inKey) { key = inKey; }
	char Compare(T_Key inKey) const
	{
		if (inKey == key) return 0;
		return (inKey < key) ? -1 : 1;
	}
	void Clear() {}
};

template <> class MappedKey<const char*>
{
	const char* key;

public:
	const char* Get() const { return key; }
	void Set(const char* inKey) { key = inKey; }
	char Compare(const char* inKey) const { return StrCompare(inKey, key); }
	void Clear() {}
};

template <> class MappedKey<char*>
{
	char* key;

public:
	char* Get() const { return key; }
	void Set(char* inKey) { key = CopyString(inKey); }
	char Compare(char* inKey) const { return StrCompare(inKey, key); }
	void Clear() { free(key); }
};

template <typename T_Key, typename T_Data> class Map
{
protected:
	typedef MappedKey<T_Key> M_Key;

	struct Entry
	{
		M_Key		key;
		T_Data		value;

		void Clear()
		{
			key.Clear();
			value.~T_Data();
		}
	};

	Entry* entries;		// 00
	UInt32		numEntries;		// 04
	UInt32		alloc;			// 08

	__declspec(noinline) bool GetIndex(T_Key key, UInt32* outIdx) const
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

	__declspec(noinline) bool InsertKey(T_Key key, T_Data** outData)
	{
		UInt32 index;
		if (GetIndex(key, &index))
		{
			*outData = &entries[index].value;
			return false;
		}
		if (!entries) entries = (Entry*)malloc(sizeof(Entry) * alloc);
		else if (numEntries == alloc)
		{
			alloc <<= 1;
			entries = (Entry*)realloc(entries, sizeof(Entry) * alloc);
		}
		Entry* entry = entries + index;
		index = numEntries - index;
		if (index) MemCopy(entry + 1, entry, sizeof(Entry) * index);
		numEntries++;
		entry->key.Set(key);
		*outData = &entry->value;
		return true;
	}

public:
	Map(UInt32 _alloc = kMapDefaultAlloc) : entries(NULL), numEntries(0), alloc(_alloc) {}
	~Map()
	{
		if (!entries) return;
		while (numEntries)
			entries[--numEntries].Clear();
		free(entries);
		entries = NULL;
	}

	UInt32 Size() const { return numEntries; }
	bool Empty() const { return !numEntries; }

	bool Insert(T_Key key, T_Data** outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](T_Key key)
	{
		T_Data* outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(T_Key key, Args&& ...args)
	{
		T_Data* outData;
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
		return GetIndex(key, &index) ? entries[index].value : NULL;
	}

	T_Data* GetPtr(T_Key key) const
	{
		UInt32 index;
		return GetIndex(key, &index) ? &entries[index].value : NULL;
	}

	bool Erase(T_Key key)
	{
		UInt32 index;
		if (!GetIndex(key, &index)) return false;
		Entry* entry = entries + index;
		entry->Clear();
		numEntries--;
		index = numEntries - index;
		if (index) MemCopy(entry, entry + 1, sizeof(Entry) * index);
		return true;
	}

	void Clear()
	{
		for (Entry* entry = entries; numEntries; numEntries--, entry++)
			entry->Clear();
	}

	class Iterator
	{
	protected:
		friend Map;

		Entry* entry;		// 00
		UInt32		count;		// 04

	public:
		T_Key Key() const { return entry->key.Get(); }
		T_Data& Get() const { return entry->value; }
		T_Data& operator*() const { return entry->value; }
		T_Data operator->() const { return entry->value; }
		bool End() const { return !count; }

		void operator++()
		{
			entry++;
			count--;
		}
		void operator--()
		{
			entry--;
			count--;
		}

		Iterator() {}
		Iterator(Map& source) : entry(source.entries), count(source.numEntries) {}
	};

	class OpIterator : public Iterator
	{
		Map* table;		// 08

	public:
		Map* Table() const { return table; }

		void Remove(bool frwrd = true)
		{
			entry->Clear();
			Entry* pEntry = entry;
			UInt32 index;
			if (frwrd)
			{
				index = count - 1;
				entry--;
			}
			else index = table->numEntries - count;
			if (index) MemCopy(pEntry, pEntry + 1, sizeof(Entry) * index);
			table->numEntries--;
		}

		OpIterator(Map& source) : table(&source)
		{
			entry = source.entries;
			count = source.numEntries;
		}
		OpIterator(Map& source, T_Key key) : table(&source)
		{
			UInt32 index;
			if (source.GetIndex(key, &index))
			{
				entry = source.entries + index;
				count = source.numEntries - index;
			}
			else count = 0;
		}
		OpIterator(Map& source, T_Key key, bool frwrd) : table(&source)
		{
			if (!source.numEntries)
			{
				count = 0;
				return;
			}
			UInt32 index;
			bool match = source.GetIndex(key, &index);
			if (frwrd)
			{
				entry = source.entries + index;
				count = source.numEntries - index;
			}
			else
			{
				entry = source.entries + (index - !match);
				count = index + match;
			}
		}
	};
};

template <typename T_Key> class Set
{
protected:
	typedef MappedKey<T_Key> M_Key;

	M_Key* keys;		// 00
	UInt32		numKeys;	// 04
	UInt32		alloc;		// 08

	__declspec(noinline) bool GetIndex(T_Key key, UInt32* outIdx) const
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

public:
	Set(UInt32 _alloc = kVectorDefaultAlloc) : keys(NULL), numKeys(0), alloc(_alloc) {}
	~Set()
	{
		if (!keys) return;
		while (numKeys)
			keys[--numKeys].Clear();
		free(keys);
		keys = NULL;
	}

	UInt32 Size() const { return numKeys; }
	bool Empty() const { return !numKeys; }
	T_Key* Keys() { return reinterpret_cast<T_Key*>(keys); }

	__declspec(noinline) bool Insert(T_Key key)
	{
		UInt32 index;
		if (GetIndex(key, &index)) return false;
		if (!keys) keys = (M_Key*)malloc(sizeof(M_Key) * alloc);
		else if (numKeys == alloc)
		{
			alloc <<= 1;
			keys = (M_Key*)realloc(keys, sizeof(M_Key) * alloc);
		}
		M_Key* pKey = keys + index;
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
		M_Key* pKey = keys + index;
		pKey->Clear();
		numKeys--;
		index = numKeys - index;
		if (index) MemCopy(pKey, pKey + 1, sizeof(M_Key) * index);
		return true;
	}

	void Clear(bool clrKeys = false)
	{
		if (clrKeys)
		{
			for (M_Key* pKey = keys; numKeys; numKeys--, pKey++)
				pKey->Clear();
		}
		else numKeys = 0;
	}

	__declspec(noinline) void CopyFrom(const Set& source)
	{
		numKeys = source.numKeys;
		if (!numKeys) return;
		if (!keys)
		{
			alloc = numKeys;
			keys = (M_Key*)malloc(sizeof(M_Key) * alloc);
		}
		else if (numKeys > alloc)
		{
			alloc = numKeys;
			free(keys);
			keys = (M_Key*)malloc(sizeof(M_Key) * alloc);
		}
		MemCopy(keys, source.keys, sizeof(M_Key) * numKeys);
	}

	bool CompareTo(const Set& source) const
	{
		if (numKeys != source.numKeys) return false;
		return !numKeys || MemCmp(keys, source.keys, sizeof(M_Key) * numKeys);
	}

	class Iterator
	{
		friend Set;

		M_Key* pKey;		// 00
		UInt32		count;		// 04

	public:
		T_Key operator*() const { return pKey->Get(); }
		T_Key operator->() const { return pKey->Get(); }
		bool End() const { return !count; }

		void operator++()
		{
			pKey++;
			count--;
		}

		Iterator() {}
		Iterator(Set& source) : pKey(source.keys), count(source.numKeys) {}
		Iterator(Set& source, T_Key key)
		{
			UInt32 index;
			if (source.GetIndex(key, &index))
			{
				pKey = source.keys + index;
				count = source.numKeys - index;
			}
			else count = 0;
		}
	};

	class OpIterator : public Iterator
	{
		Set* table;		// 08

	public:
		Set* Table() const { return table; }

		void Remove()
		{
			pKey->Clear();
			M_Key* _key = pKey;
			UInt32 index = count - 1;
			pKey--;
			if (index) MemCopy(_key, _key + 1, sizeof(M_Key) * index);
			table->numKeys--;
		}

		OpIterator(Set& source) : table(&source)
		{
			pKey = source.keys;
			count = source.numKeys;
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Set& source)
		{
			count = source.numKeys;
			if (count)
				pKey = (M_Key*)MemCopy(s_cpIteratorBuffer, source.keys, sizeof(M_Key) * count);
		}
	};
};

template <typename T_Key> class HashedKey
{
	T_Key		key;

public:
	static UInt32 Hash(T_Key inKey) { return *(UInt32*)&inKey; }
	T_Key Get() const { return key; }
	void Set(T_Key inKey, UInt32) { key = inKey; }
	UInt32 GetHash() const { return *(UInt32*)&key; }
	void Clear() {}
};

template <> class HashedKey<const char*>
{
	UInt32		key;

public:
	static UInt32 Hash(const char* inKey) { return StrHash(inKey); }
	UInt32 Get() const { return key; }
	void Set(const char*, UInt32 inHash) { key = inHash; }
	UInt32 GetHash() const { return key; }
	void Clear() {}
};

template <> class HashedKey<char*>
{
	UInt32		hashVal;
	char* key;

public:
	static UInt32 Hash(char* inKey) { return StrHash(inKey); }
	char* Get() const { return key; }
	void Set(char* inKey, UInt32 inHash)
	{
		hashVal = inHash;
		key = CopyString(inKey);
	}
	UInt32 GetHash() const { return hashVal; }
	void Clear() { free(key); }
};

template <typename T_Key, typename T_Data> class UnorderedMap
{
protected:
	typedef HashedKey<T_Key> H_Key;

	struct Entry
	{
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
		Entry* entries;		// 00
		UInt8		numEntries;		// 04
		UInt8		alloc;			// 05
		UInt8		pad06[2];		// 06

		__declspec(noinline) Entry* AddEntry()
		{
			if (alloc == numEntries)
			{
				alloc += kMapBucketSizeInc;
				entries = (Entry*)realloc(entries, sizeof(Entry) * alloc);
			}
			return entries + numEntries++;
		}

		__declspec(noinline) void Clear(bool doFree = true)
		{
			if (!entries) return;
			for (Entry* entry = entries; numEntries; numEntries--, entry++)
				entry->Clear();
			if (doFree)
			{
				free(entries);
				entries = NULL;
			}
		}
	};

	Bucket* buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numItems;		// 08

	__declspec(noinline) void ExpandTable()
	{
		UInt32 newCount = GetNextPrime(numBuckets << 1);
		Bucket* newBuckets = (Bucket*)calloc(newCount, sizeof(Bucket));
		Entry* entry;
		UInt8 count;
		for (Bucket* currBucket = buckets; numBuckets; numBuckets--, currBucket++)
		{
			if (entry = currBucket->entries)
			{
				for (count = currBucket->numEntries; count; count--, entry++)
					*(newBuckets[entry->key.GetHash() % newCount].AddEntry()) = *entry;
				free(currBucket->entries);
			}
		}
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount;
	}

	__declspec(noinline) Entry* FindEntry(T_Key key) const
	{
		if (!numItems) return NULL;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		Entry* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
			if (entry->key.GetHash() == hashVal) return entry;
		return NULL;
	}

	__declspec(noinline) bool InsertKey(T_Key key, T_Data** outData)
	{
		if (!buckets) buckets = (Bucket*)calloc(numBuckets, sizeof(Bucket));
		else if (numItems > numBuckets) ExpandTable();
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		Entry* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
		{
			if (entry->key.GetHash() != hashVal) continue;
			*outData = &entry->value;
			return false;
		}
		entry = bucket->AddEntry();
		numItems++;
		entry->key.Set(key, hashVal);
		*outData = &entry->value;
		return true;
	}

public:
	UnorderedMap(UInt32 _numBuckets = kMapDefaultBucketNum) : buckets(NULL), numBuckets(_numBuckets), numItems(0) {}
	~UnorderedMap()
	{
		if (!buckets) return;
		for (Bucket* bucket = buckets; numBuckets; numBuckets--, bucket++)
			bucket->Clear();
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const { return numItems; }
	bool Empty() const { return !numItems; }

	bool Insert(T_Key key, T_Data** outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](T_Key key)
	{
		T_Data* outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(T_Key key, Args&& ...args)
	{
		T_Data* outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	T_Data InsertNotIn(T_Key key, T_Data value)
	{
		T_Data* outData;
		if (InsertKey(key, &outData))
			*outData = value;
		return value;
	}

	bool HasKey(T_Key key) const { return FindEntry(key) ? true : false; }

	T_Data Get(T_Key key) const
	{
		Entry* entry = FindEntry(key);
		return entry ? entry->value : NULL;
	}

	T_Data* GetPtr(T_Key key) const
	{
		Entry* entry = FindEntry(key);
		return entry ? &entry->value : NULL;
	}

	bool Erase(T_Key key)
	{
		if (!numItems) return false;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		Entry* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
		{
			if (entry->key.GetHash() != hashVal) continue;
			entry->Clear();
			bucket->numEntries--;
			if (count > 1) *entry = bucket->entries[bucket->numEntries];
			numItems--;
			return true;
		}
		return false;
	}

	T_Data GetErase(T_Key key)
	{
		if (!numItems) return NULL;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		Entry* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
		{
			if (entry->key.GetHash() != hashVal) continue;
			T_Data outVal = entry->value;
			entry->Clear();
			bucket->numEntries--;
			if (count > 1) *entry = bucket->entries[bucket->numEntries];
			numItems--;
			return outVal;
		}
		return NULL;
	}

	bool Clear(bool clrBkt = true)
	{
		if (!numItems) return false;
		numItems = 0;
		Bucket* bucket = buckets;
		for (UInt32 count = numBuckets; count; count--, bucket++)
		{
			if (clrBkt) bucket->Clear(false);
			else bucket->numEntries = 0;
		}
		return true;
	}

	/*void DumpLoads()
	{
		UInt32 loadArray[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		UInt32 maxLoad = 0;
		Bucket *bucket = buckets;
		for (UInt32 count = numBuckets; count; count--, bucket++)
		{
			if (maxLoad < bucket->numEntries)
				maxLoad = bucket->numEntries;
			loadArray[bucket->numEntries]++;
		}
		_MESSAGE("Buckets = %d  Items = %d  Max Load = %d", numBuckets, numItems, maxLoad);
		_MESSAGE("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
			loadArray[0], loadArray[1], loadArray[2], loadArray[3], loadArray[4], loadArray[5], loadArray[6], loadArray[7],
			loadArray[8], loadArray[9], loadArray[10], loadArray[11], loadArray[12], loadArray[13], loadArray[14], loadArray[15]);
	}*/

	class Iterator
	{
		friend UnorderedMap;

		UnorderedMap* table;		// 00
		UInt32			bucketIdx;	// 04
		UInt8			entryIdx;	// 08
		bool			removed;	// 09
		UInt8			pad0A[2];	// 0A
		Bucket* bucket;	// 0C
		Entry* entry;		// 10

		void FindValid()
		{
			if (table->numItems)
				for (; bucketIdx < table->numBuckets; bucketIdx++, bucket++)
					if (bucket->numEntries && (entry = bucket->entries)) return;
			entry = NULL;
		}

	public:
		void Init(UnorderedMap& _table)
		{
			table = &_table;
			if (table)
			{
				bucketIdx = 0;
				entryIdx = 0;
				removed = false;
				bucket = table->buckets;
				FindValid();
			}
			else entry = NULL;
		}

		void Find(UnorderedMap& _table, T_Key key)
		{
			table = &_table;
			if (table && table->numItems)
			{
				UInt32 hashVal = H_Key::Hash(key);
				bucketIdx = hashVal % table->numBuckets;
				entryIdx = 0;
				removed = false;
				bucket = table->buckets + bucketIdx;
				entry = bucket->entries;
				for (UInt8 count = bucket->numEntries; count; count--, entryIdx++, entry++)
					if (entry->key.GetHash() == hashVal) return;
			}
			entry = NULL;
		}

		UnorderedMap* Table() const { return table; }
		T_Key Key() const { return entry->key.Get(); }
		T_Data& Get() const { return entry->value; }
		T_Data& operator*() const { return entry->value; }
		T_Data operator->() const { return entry->value; }
		bool End() const { return !entry; }

		void operator++()
		{
			if (removed) removed = false;
			else entryIdx++;
			if (entryIdx >= bucket->numEntries)
			{
				bucketIdx++;
				entryIdx = 0;
				bucket++;
				FindValid();
			}
			else entry++;
		}

		bool IsValid() const { return entry && (entryIdx < bucket->numEntries); }

		void Remove()
		{
			entry->Clear();
			if (--bucket->numEntries > entryIdx)
			{
				*entry = bucket->entries[bucket->numEntries];
				removed = true;
			}
			table->numItems--;
		}

		Iterator() : table(NULL), entry(NULL) {}
		Iterator(UnorderedMap& _table) { Init(_table); }
		Iterator(UnorderedMap& _table, T_Key key) { Find(_table, key); }
	};
};

template <typename T_Key> class UnorderedSet
{
protected:
	typedef HashedKey<T_Key> H_Key;

	struct Bucket
	{
		H_Key* entries;		// 00
		UInt8		numEntries;		// 04
		UInt8		alloc;			// 05
		UInt8		pad06[2];		// 06

		__declspec(noinline) H_Key* AddEntry()
		{
			if (alloc == numEntries)
			{
				alloc += kMapBucketSizeInc;
				entries = (H_Key*)realloc(entries, sizeof(H_Key) * alloc);
			}
			return entries + numEntries++;
		}

		__declspec(noinline) void Clear(bool doFree = true)
		{
			if (!entries) return;
			for (H_Key* entry = entries; numEntries; numEntries--, entry++)
				entry->Clear();
			if (doFree)
			{
				free(entries);
				entries = NULL;
			}
		}
	};

	Bucket* buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numItems;		// 08

	__declspec(noinline) void ExpandTable()
	{
		UInt32 newCount = GetNextPrime(numBuckets << 1);
		Bucket* newBuckets = (Bucket*)calloc(newCount, sizeof(Bucket));
		H_Key* entry;
		UInt8 count;
		for (Bucket* currBucket = buckets; numBuckets; numBuckets--, currBucket++)
		{
			if (entry = currBucket->entries)
			{
				for (count = currBucket->numEntries; count; count--, entry++)
					*(newBuckets[entry->GetHash() % newCount].AddEntry()) = *entry;
				free(currBucket->entries);
			}
		}
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount;
	}

	__declspec(noinline) bool InsertKey(T_Key key, H_Key** outEntry)
	{
		if (!buckets) buckets = (Bucket*)calloc(numBuckets, sizeof(Bucket));
		else if (numItems > numBuckets) ExpandTable();
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		H_Key* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
		{
			if (entry->GetHash() != hashVal) continue;
			*outEntry = entry;
			return false;
		}
		*outEntry = entry = bucket->AddEntry();
		numItems++;
		entry->Set(key, hashVal);
		return true;
	}

public:
	UnorderedSet(UInt32 _numBuckets = kMapDefaultBucketNum) : buckets(NULL), numBuckets(_numBuckets), numItems(0) {}
	~UnorderedSet()
	{
		if (!buckets) return;
		for (Bucket* bucket = buckets; numBuckets; numBuckets--, bucket++)
			bucket->Clear();
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const { return numItems; }
	bool Empty() const { return !numItems; }

	bool Insert(T_Key key)
	{
		H_Key* outEntry;
		return InsertKey(key, &outEntry);
	}

	T_Key InsertNotIn(T_Key key)
	{
		H_Key* outEntry;
		InsertKey(key, &outEntry);
		return outEntry->Get();
	}

	bool HasKey(T_Key key) const
	{
		if (!numItems) return false;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		H_Key* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
			if (entry->GetHash() == hashVal) return true;
		return false;
	}

	bool Erase(T_Key key)
	{
		if (!numItems) return false;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal % numBuckets];
		H_Key* entry = bucket->entries;
		for (UInt8 count = bucket->numEntries; count; count--, entry++)
		{
			if (entry->GetHash() != hashVal) continue;
			entry->Clear();
			bucket->numEntries--;
			if (count > 1) *entry = bucket->entries[bucket->numEntries];
			numItems--;
			return true;
		}
		return false;
	}

	bool Clear(bool clrBkt = false)
	{
		if (!numItems) return false;
		numItems = 0;
		Bucket* bucket = buckets;
		for (UInt32 count = numBuckets; count; count--, bucket++)
		{
			if (clrBkt) bucket->Clear(false);
			else bucket->numEntries = 0;
		}
		return true;
	}

	class Iterator
	{
		friend UnorderedSet;

		UnorderedSet* table;		// 00
		UInt32			bucketIdx;	// 04
		UInt8			entryIdx;	// 08
		UInt8			pad09[3];	// 09
		Bucket* bucket;	// 0C
		H_Key* entry;		// 10

		void FindValid()
		{
			if (table->numItems)
				for (; bucketIdx < table->numBuckets; bucketIdx++, bucket++)
					if (bucket->numEntries && (entry = bucket->entries)) return;
			entry = NULL;
		}

	public:
		void Init(UnorderedSet& _table)
		{
			table = &_table;
			if (table)
			{
				bucketIdx = 0;
				entryIdx = 0;
				bucket = table->buckets;
				FindValid();
			}
			else entry = NULL;
		}

		T_Key operator*() const { return entry->Get(); }
		T_Key operator->() const { return entry->Get(); }
		bool End() const { return !entry; }

		void operator++()
		{
			if (++entryIdx >= bucket->numEntries)
			{
				bucketIdx++;
				entryIdx = 0;
				bucket++;
				FindValid();
			}
			else entry++;
		}

		Iterator() : table(NULL), entry(NULL) {}
		Iterator(UnorderedSet& _table) { Init(_table); }
	};
};

template <typename T_Data> class Vector
{
protected:
	T_Data* data;		// 00
	UInt32		numItems;	// 04
	UInt32		alloc;		// 08

	__declspec(noinline) T_Data* AllocateData()
	{
		if (!data) data = (T_Data*)malloc(sizeof(T_Data) * alloc);
		else if (numItems == alloc)
		{
			alloc <<= 1;
			data = (T_Data*)realloc(data, sizeof(T_Data) * alloc);
		}
		return data + numItems++;
	}

public:
	Vector(UInt32 _alloc = kVectorDefaultAlloc) : alloc(_alloc), numItems(0), data(NULL) {}
	~Vector()
	{
		if (!data) return;
		while (numItems) data[--numItems].~T_Data();
		free(data);
		data = NULL;
	}

	UInt32 Size() const { return numItems; }
	bool Empty() const { return !numItems; }

	T_Data* Data() const { return data; }

	T_Data& operator[](UInt32 index) const { return data[index]; }

	T_Data* Get(UInt32 index) const { return (index < numItems) ? (data + index) : NULL; }

	void Append(const T_Data item)
	{
		T_Data* pData = AllocateData();
		*pData = item;
	}

	__declspec(noinline) void Concatenate(const Vector& source)
	{
		if (!source.numItems) return;
		UInt32 newCount = numItems + source.numItems;
		if (!data)
		{
			if (alloc < newCount) alloc = newCount;
			data = (T_Data*)malloc(sizeof(T_Data) * alloc);
		}
		else if (alloc < newCount)
		{
			alloc = newCount;
			data = (T_Data*)realloc(data, sizeof(T_Data) * alloc);
		}
		MemCopy(data + numItems, source.data, sizeof(T_Data) * source.numItems);
		numItems = newCount;
	}

	void Insert(const T_Data& item, UInt32 index)
	{
		if (index <= numItems)
		{
			UInt32 size = numItems - index;
			T_Data* pData = AllocateData();
			if (size)
			{
				pData = data + index;
				MemCopy(pData + 1, pData, sizeof(T_Data) * size);
			}
			*pData = item;
		}
	}

	UInt32 InsertSorted(const T_Data& item)
	{
		UInt32 lBound = 0, uBound = numItems, index;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			if (item < data[index]) uBound = index;
			else lBound = index + 1;
		}
		uBound = numItems - lBound;
		T_Data* pData = AllocateData();
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

	bool RemoveNth(UInt32 index)
	{
		if ((index < 0) || (index >= numItems)) return false;
		T_Data* pData = data + index;
		pData->~T_Data();
		numItems--;
		index = numItems - index;
		if (index) MemCopy(pData, pData + 1, sizeof(T_Data) * index);
		return true;
	}

	SInt32 GetIndexOf(T_Data item) const
	{
		T_Data* pData = data;
		for (UInt32 count = numItems; count; count--, pData++)
			if (*pData == item) return numItems - count;
		return -1;
	}

	bool AppendNotIn(const T_Data& item)
	{
		T_Data* pData = data;
		for (UInt32 count = numItems; count; count--, pData++)
			if (*pData == item) return false;
		pData = AllocateData();
		*pData = item;
		return true;
	}

	template <class Finder>
	SInt32 GetIndexOf(Finder& finder) const
	{
		T_Data* pData = data;
		for (UInt32 count = numItems; count; count--, pData++)
			if (finder.Accept(*pData)) return numItems - count;
		return -1;
	}

	template <class Finder>
	T_Data* Find(Finder& finder) const
	{
		T_Data* pData = data;
		for (UInt32 count = numItems; count; count--, pData++)
			if (finder.Accept(*pData)) return pData;
		return NULL;
	}

	bool Remove(T_Data item)
	{
		if (!numItems) return false;
		T_Data* pData = data + numItems;
		UInt32 count = numItems;
		for (; count; count--)
		{
			pData--;
			if (*pData != item) continue;
			pData->~T_Data();
			count = numItems - count;
			if (count) MemCopy(pData, pData + 1, sizeof(T_Data) * count);
			numItems--;
			return true;
		}
		return false;
	}

	template <class Finder>
	UInt32 Remove(Finder& finder)
	{
		if (!numItems) return 0;
		UInt32 removed = 0, size;
		T_Data* pData = data + numItems;
		for (UInt32 count = numItems; count; count--)
		{
			pData--;
			if (!finder.Accept(*pData)) continue;
			pData->~T_Data();
			size = numItems - count;
			if (size) MemCopy(pData, pData + 1, sizeof(T_Data) * size);
			numItems--;
			removed++;
		}
		return removed;
	}

	void RemoveRange(UInt32 beginIdx, UInt32 count)
	{
		if (beginIdx >= numItems) return;
		if (count > (numItems - beginIdx))
			count = numItems - beginIdx;
		T_Data* pData = data + beginIdx + count;
		for (UInt32 index = count; index; index--)
		{
			pData--;
			pData->~T_Data();
		}
		if ((beginIdx + count) < numItems)
			MemCopy(pData, pData + count, sizeof(T_Data) * (numItems - beginIdx - count));
		numItems -= count;
	}

	bool Clear(bool delData = false)
	{
		if (!numItems) return false;
		if (delData)
			while (numItems)
				data[--numItems].~T_Data();
		else numItems = 0;
		return true;
	}

	typedef bool (*CompareFunc)(T_Data&, T_Data&);

	void QuickSort(UInt32 p, UInt32 q, CompareFunc compareFunc)
	{
		if (p >= q) return;
		UInt32 i = p;
		T_Data temp;
		for (UInt32 j = p + 1; j < q; j++)
		{
			if (compareFunc(data[p], data[j])) continue;
			i++;
			temp = data[i];
			data[i] = data[j];
			data[j] = temp;
		}
		temp = data[i];
		data[i] = data[p];
		data[p] = temp;
		QuickSort(p, i, compareFunc);
		QuickSort(i + 1, q, compareFunc);
	}

	class Iterator
	{
	protected:
		friend Vector;

		UInt32		count;
		T_Data* pData;

	public:
		bool End() const { return !count; }
		void operator++()
		{
			count--;
			pData++;
		}

		T_Data& operator*() const { return *pData; }
		T_Data& operator->() const { return *pData; }
		T_Data& Get() const { return *pData; }

		Iterator() {}
		Iterator(Vector& source) : count(source.numItems), pData(source.data) {}
		Iterator(Vector& source, UInt32 index)
		{
			if (source.numItems > index)
			{
				count = source.numItems - index;
				pData = source.data + index;
			}
			else count = 0;
		}
	};

	class RvIterator : public Iterator
	{
		Vector* contObj;

	public:
		Vector* Container() const { return contObj; }

		void operator--()
		{
			count--;
			pData--;
		}

		void Remove()
		{
			pData->~T_Data();
			UInt32 size = contObj->numItems - count;
			if (size) MemCopy(pData, pData + 1, sizeof(T_Data) * size);
			contObj->numItems--;
		}

		RvIterator(Vector& source) : contObj(&source)
		{
			count = source.numItems;
			if (count) pData = source.data + (count - 1);
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Vector& source)
		{
			count = source.numItems;
			if (count)
				pData = (T_Data*)MemCopy(s_cpIteratorBuffer, source.data, sizeof(T_Data) * count);
		}
	};
};

template <typename T_Data, size_t N> class FixedTypeArray
{
protected:
	size_t		size;
	T_Data		data[N];

public:
	FixedTypeArray() : size(0) {}

	bool Empty() const { return size == 0; }
	size_t Size() const { return size; }
	T_Data* Data() { return data; }

	bool Append(T_Data item)
	{
		if (size >= N) return false;
		data[size++] = item;
		return true;
	}

	class Iterator
	{
	protected:
		friend FixedTypeArray;

		T_Data* pData;
		UInt32		count;

	public:
		bool End() const { return !count; }
		void operator++()
		{
			count--;
			pData++;
		}

		T_Data& operator*() const { return *pData; }
		T_Data& operator->() const { return *pData; }
		T_Data& Get() const { return *pData; }

		Iterator(FixedTypeArray& source) : pData(source.data), count(source.size) {}
	};
};