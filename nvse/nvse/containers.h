#pragma once
#include "utility.h"
#include <iostream>
#include <memory>
#include <utility>
enum
{
	kMapDefaultAlloc = 8,

	kMapDefaultBucketCount = 8,
	kMapMaxBucketCount = 0x20000,

	kVectorDefaultAlloc = 2,
};

//extern AuxBuffer s_auxBuffers[3];

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

	bool GetIndex(T_Key key, UInt32* outIdx) const
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

	bool InsertKey(T_Key key, T_Data** outData)
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
		friend Map;

		Entry* entry;
		UInt32		count;

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

		void Find(Map* source, T_Key key)
		{
			UInt32 index;
			if (source->GetIndex(key, &index))
			{
				entry = source->entries + index;
				count = source->numEntries - index;
			}
			else count = 0;
		}

		Iterator() {}
		Iterator(Map& source) : entry(source.entries), count(source.numEntries) {}
		Iterator(Map& source, T_Key key) { Find(&source, key); }
	};

	class OpIterator : public Iterator
	{
		Map* table;

	public:
		Map* Table() const { return table; }

		void Remove(bool frwrd = true)
		{
			this->entry->Clear();
			Entry* pEntry = this->entry;
			UInt32 index;
			if (frwrd)
			{
				index = this->count - 1;
				entry--;
			}
			else index = table->numEntries - this->count;
			if (index) MemCopy(pEntry, pEntry + 1, sizeof(Entry) * index);
			table->numEntries--;
		}

		OpIterator(Map& source) : table(&source)
		{
			this->entry = source.entries;
			this->count = source.numEntries;
		}
		OpIterator(Map& source, T_Key key) : table(&source) { Find(&source, key); }
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

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Map& source)
		{
			count = source.numEntries;
			if (count)
			{
				UInt32 size = count * sizeof(Entry);
				//entry = (Entry*)MemCopy(s_auxBuffers[0].Employ(size), source.entries, size);
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

	bool GetIndex(T_Key key, UInt32* outIdx) const
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

	bool Insert(T_Key key)
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

	void CopyFrom(const Set& source)
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

		M_Key* pKey;
		UInt32		count;

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
		Set* table;

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
			{
				UInt32 size = count * sizeof(M_Key);
				pKey = (M_Key*)MemCopy(s_auxBuffers[1].Employ(size), source.keys, size);
			}
		}
	};
};

template <typename T_Key> class HashedKey
{
	T_Key		key;

public:
	static UInt32 Hash(T_Key inKey)
	{
		UInt32 uKey = *(UInt32*)&inKey;
		return (uKey * 0xD) ^ (uKey >> 0xF);
	}
	bool Match(T_Key inKey, UInt32) const { return key == inKey; }
	T_Key Get() const { return key; }
	void Set(T_Key inKey, UInt32) { key = inKey; }
	UInt32 GetHash() const { return Hash(key); }
	void Clear() {}
};

template <> class HashedKey<const char*>
{
	UInt32		hashVal;

public:
	static UInt32 Hash(const char* inKey) { return StrHash(inKey); }
	bool Match(const char*, UInt32 inHash) const { return hashVal == inHash; }
	const char* Get() const { return ""; }
	void Set(const char*, UInt32 inHash) { hashVal = inHash; }
	UInt32 GetHash() const { return hashVal; }
	void Clear() {}
};

template <> class HashedKey<char*>
{
	UInt32		hashVal;
	char* key;

public:
	static UInt32 Hash(char* inKey) { return StrHash(inKey); }
	bool Match(char*, UInt32 inHash) const { return hashVal == inHash; }
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
		Entry* next;
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
		Entry* entries;

		void Insert(Entry* entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Clear()
		{
			if (!entries) return;
			Entry* entry;
			do
			{
				entry = entries;
				entries = entries->next;
				entry->Clear();
				free(entry);
			} while (entries);
		}
	};

	Bucket* buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numItems;		// 08

	void ResizeTable(UInt32 newCount)
	{
		Bucket* currBuckets = buckets, * newBuckets = (Bucket*)calloc(newCount + 1, sizeof(Bucket));
		Entry* entry, * temp;
		for (UInt32 count = numBuckets; count; count--, currBuckets++)
		{
			entry = currBuckets->entries;
			while (entry)
			{
				temp = entry;
				entry = entry->next;
				newBuckets[temp->key.GetHash() & (newCount - 1)].Insert(temp);
			}
		}
		newBuckets[newCount].entries = buckets[numBuckets].entries;
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount;
	}

	Entry* AddEntry()
	{
		numItems++;
		Bucket* scrap = &buckets[numBuckets];
		Entry* entry = scrap->entries;
		if (entry)
		{
			scrap->entries = entry->next;
			return entry;
		}
		return (Entry*)malloc(sizeof(Entry));
	}

	void RemoveEntry(Entry* entry)
	{
		numItems--;
		entry->Clear();
		Bucket* scrap = &buckets[numBuckets];
		entry->next = scrap->entries;
		scrap->entries = entry;
	}

	Entry* FindEntry(T_Key key) const
	{
		if (!numItems) return NULL;
		UInt32 hashVal = H_Key::Hash(key);
		for (Entry* entry = buckets[hashVal & (numBuckets - 1)].entries; entry; entry = entry->next)
			if (entry->key.Match(key, hashVal)) return entry;
		return NULL;
	}

	bool InsertKey(T_Key key, T_Data** outData)
	{
		if (!buckets)
		{
			numBuckets = GetHighBit(numBuckets);
			if (numBuckets < kMapDefaultBucketCount)
				numBuckets = kMapDefaultBucketCount;
			else if (numBuckets > kMapMaxBucketCount)
				numBuckets = kMapMaxBucketCount;
			buckets = (Bucket*)calloc(numBuckets + 1, sizeof(Bucket));
		}
		else if ((numItems > numBuckets) && (numBuckets < kMapMaxBucketCount))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry* entry = bucket->entries; entry; entry = entry->next)
		{
			if (!entry->key.Match(key, hashVal)) continue;
			*outData = &entry->value;
			return false;
		}
		Entry* newEntry = AddEntry();
		bucket->Insert(newEntry);
		newEntry->key.Set(key, hashVal);
		*outData = &newEntry->value;
		return true;
	}

public:
	UnorderedMap(UInt32 _numBuckets = kMapDefaultBucketCount) : buckets(NULL), numBuckets(_numBuckets), numItems(0) {}
	~UnorderedMap()
	{
		if (!buckets) return;
		Clear();
		Entry* entry = buckets[numBuckets].entries, * temp;
		while (entry)
		{
			temp = entry;
			entry = entry->next;
			free(temp);
		}
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const { return numItems; }
	bool Empty() const { return !numItems; }

	UInt32 BucketCount() const { return numBuckets; }

	void SetBucketCount(UInt32 newCount)
	{
		newCount = GetHighBit(newCount);
		if (newCount < kMapDefaultBucketCount)
			newCount = kMapDefaultBucketCount;
		else if (newCount > kMapMaxBucketCount)
			newCount = kMapMaxBucketCount;
		if (numBuckets != newCount)
		{
			if (buckets)
				ResizeTable(newCount);
			else numBuckets = newCount;
		}
	}

	float LoadFactor() const { return (float)numItems / (float)numBuckets; }

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
		Bucket* bucket = &buckets[hashVal & (numBuckets - 1)];
		Entry* entry = bucket->entries, * prev = NULL;
		while (entry)
		{
			if (entry->key.Match(key, hashVal))
			{
				if (prev) prev->next = entry->next;
				else bucket->entries = entry->next;
				RemoveEntry(entry);
				return true;
			}
			prev = entry;
			entry = entry->next;
		}
		return false;
	}

	T_Data GetErase(T_Key key)
	{
		if (!numItems) return NULL;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal & (numBuckets - 1)];
		Entry* entry = bucket->entries, * prev = NULL;
		while (entry)
		{
			if (entry->key.Match(key, hashVal))
			{
				if (prev) prev->next = entry->next;
				else bucket->entries = entry->next;
				T_Data outVal = entry->value;
				RemoveEntry(entry);
				return outVal;
			}
			prev = entry;
			entry = entry->next;
		}
		return NULL;
	}

	bool Clear()
	{
		if (!numItems) return false;
		numItems = 0;
		Bucket* bucket = buckets;
		for (UInt32 count = numBuckets; count; count--, bucket++)
			bucket->Clear();
		return true;
	}

	/*void DumpLoads()
	{
		UInt32 loadsArray[0x20];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *bucket = buckets;
		Entry *entry;
		UInt32 maxLoad = 0, entryCount;
		for (UInt32 count = numBuckets; count; count--, bucket++)
		{
			entryCount = 0;
			entry = bucket->entries;
			while (entry)
			{
				entryCount++;
				entry = entry->next;
			}
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numItems, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%d", iter, loadsArray[iter]);
	}*/

	class Iterator
	{
		friend UnorderedMap;

		UnorderedMap* table;		// 00
		UInt32			bucketIdx;	// 04
		Bucket* bucket;	// 08
		Entry* entry;		// 0C

		void FindValid()
		{
			for (; bucketIdx < table->numBuckets; bucketIdx++, bucket++)
				if (entry = bucket->entries) break;
		}

	public:
		void Init(UnorderedMap& _table)
		{
			table = &_table;
			entry = NULL;
			if (table->numItems)
			{
				bucketIdx = 0;
				bucket = table->buckets;
				FindValid();
			}
		}

		void Find(T_Key key)
		{
			if (!table->numItems)
			{
				entry = NULL;
				return;
			}
			UInt32 hashVal = H_Key::Hash(key);
			bucketIdx = hashVal & (table->numBuckets - 1);
			bucket = &table->buckets[bucketIdx];
			entry = bucket->entries;
			while (entry)
			{
				if (entry->key.Match(key, hashVal))
					break;
				entry = entry->next;
			}
		}

		UnorderedMap* Table() const { return table; }
		T_Key Key() const { return entry->key.Get(); }
		T_Data& Get() const { return entry->value; }
		T_Data& operator*() const { return entry->value; }
		T_Data operator->() const { return entry->value; }
		bool End() const { return !entry; }

		void operator++()
		{
			if (entry)
				entry = entry->next;
			else entry = bucket->entries;
			if (!entry && table->numItems)
			{
				bucketIdx++;
				bucket++;
				FindValid();
			}
		}

		bool IsValid()
		{
			if (!entry || (bucket != (table->buckets + bucketIdx)))
				return false;
			for (Entry* temp = bucket->entries; temp; temp = temp->next)
				if (temp == entry) return true;
			entry = NULL;
			return false;
		}

		void Remove()
		{
			Entry* curr = bucket->entries, * prev = NULL;
			do
			{
				if (curr == entry) break;
				prev = curr;
			} while (curr = curr->next);
			if (prev) prev->next = entry->next;
			else bucket->entries = entry->next;
			table->RemoveEntry(entry);
			entry = prev;
		}

		Iterator() : table(NULL), entry(NULL) {}
		Iterator(UnorderedMap& _table) { Init(_table); }
		Iterator(UnorderedMap& _table, T_Key key) : table(&_table) { Find(key); }
	};
};

template <typename T_Key> class UnorderedSet
{
protected:
	typedef HashedKey<T_Key> H_Key;

	struct Entry
	{
		Entry* next;
		H_Key		key;

		void Clear() { key.Clear(); }
	};

	struct Bucket
	{
		Entry* entries;

		void Insert(Entry* entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Clear()
		{
			if (!entries) return;
			Entry* entry;
			do
			{
				entry = entries;
				entries = entries->next;
				entry->Clear();
				free(entry);
			} while (entries);
		}
	};

	Bucket* buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numItems;		// 08

	void ResizeTable(UInt32 newCount)
	{
		Bucket* currBuckets = buckets, * newBuckets = (Bucket*)calloc(newCount + 1, sizeof(Bucket));
		Entry* entry, * temp;
		for (UInt32 count = numBuckets; count; count--, currBuckets++)
		{
			entry = currBuckets->entries;
			while (entry)
			{
				temp = entry;
				entry = entry->next;
				newBuckets[temp->key.GetHash() & (newCount - 1)].Insert(temp);
			}
		}
		newBuckets[newCount].entries = buckets[numBuckets].entries;
		free(buckets);
		buckets = newBuckets;
		numBuckets = newCount;
	}

	Entry* AddEntry()
	{
		numItems++;
		Bucket* scrap = &buckets[numBuckets];
		Entry* entry = scrap->entries;
		if (entry)
		{
			scrap->entries = entry->next;
			return entry;
		}
		return (Entry*)malloc(sizeof(Entry));
	}

	void RemoveEntry(Entry* entry)
	{
		numItems--;
		entry->Clear();
		Bucket* scrap = &buckets[numBuckets];
		entry->next = scrap->entries;
		scrap->entries = entry;
	}

	bool InsertKey(T_Key key)
	{
		if (!buckets)
		{
			numBuckets = GetHighBit(numBuckets);
			if (numBuckets < kMapDefaultBucketCount)
				numBuckets = kMapDefaultBucketCount;
			else if (numBuckets > kMapMaxBucketCount)
				numBuckets = kMapMaxBucketCount;
			buckets = (Bucket*)calloc(numBuckets + 1, sizeof(Bucket));
		}
		else if ((numItems > numBuckets) && (numBuckets < kMapMaxBucketCount))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry* entry = bucket->entries; entry; entry = entry->next)
			if (entry->key.Match(key, hashVal)) return false;
		Entry* newEntry = AddEntry();
		bucket->Insert(newEntry);
		newEntry->key.Set(key, hashVal);
		return true;
	}

public:
	UnorderedSet(UInt32 _numBuckets = kMapDefaultBucketCount) : buckets(NULL), numBuckets(_numBuckets), numItems(0) {}
	~UnorderedSet()
	{
		if (!buckets) return;
		Clear();
		Entry* entry = buckets[numBuckets].entries, * temp;
		while (entry)
		{
			temp = entry;
			entry = entry->next;
			free(temp);
		}
		free(buckets);
		buckets = NULL;
	}

	UInt32 Size() const { return numItems; }
	bool Empty() const { return !numItems; }

	UInt32 BucketCount() const { return numBuckets; }

	void SetBucketCount(UInt32 newCount)
	{
		newCount = GetHighBit(newCount);
		if (newCount < kMapDefaultBucketCount)
			newCount = kMapDefaultBucketCount;
		else if (newCount > kMapMaxBucketCount)
			newCount = kMapMaxBucketCount;
		if (numBuckets != newCount)
		{
			if (buckets)
				ResizeTable(newCount);
			else numBuckets = newCount;
		}
	}

	float LoadFactor() const { return (float)numItems / (float)numBuckets; }

	bool Insert(T_Key key)
	{
		return InsertKey(key);
	}

	bool HasKey(T_Key key) const
	{
		if (!numItems) return false;
		UInt32 hashVal = H_Key::Hash(key);
		for (Entry* entry = buckets[hashVal & (numBuckets - 1)].entries; entry; entry = entry->next)
			if (entry->key.Match(key, hashVal)) return true;
		return false;
	}

	bool Erase(T_Key key)
	{
		if (!numItems) return false;
		UInt32 hashVal = H_Key::Hash(key);
		Bucket* bucket = &buckets[hashVal & (numBuckets - 1)];
		Entry* entry = bucket->entries, * prev = NULL;
		while (entry)
		{
			if (entry->key.Match(key, hashVal))
			{
				if (prev) prev->next = entry->next;
				else bucket->entries = entry->next;
				RemoveEntry(entry);
				return true;
			}
			prev = entry;
			entry = entry->next;
		}
		return false;
	}

	bool Clear()
	{
		if (!numItems) return false;
		numItems = 0;
		Bucket* bucket = buckets;
		for (UInt32 count = numBuckets; count; count--, bucket++)
			bucket->Clear();
		return true;
	}

	/*void DumpLoads()
	{
		UInt32 loadsArray[0x20];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *bucket = buckets;
		Entry *entry;
		UInt32 maxLoad = 0, entryCount;
		for (UInt32 count = numBuckets; count; count--, bucket++)
		{
			entryCount = 0;
			entry = bucket->entries;
			while (entry)
			{
				entryCount++;
				entry = entry->next;
			}
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numItems, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%d", iter, loadsArray[iter]);
	}*/

	class Iterator
	{
		friend UnorderedSet;

		UnorderedSet* table;		// 00
		UInt32			bucketIdx;	// 04
		Bucket* bucket;	// 08
		Entry* entry;		// 0C

		void FindValid()
		{
			for (; bucketIdx < table->numBuckets; bucketIdx++, bucket++)
				if (entry = bucket->entries) break;
		}

	public:
		void Init(UnorderedSet& _table)
		{
			table = &_table;
			entry = NULL;
			if (table->numItems)
			{
				bucketIdx = 0;
				bucket = table->buckets;
				FindValid();
			}
		}

		T_Key operator*() const { return entry->key.Get(); }
		T_Key operator->() const { return entry->key.Get(); }
		bool End() const { return !entry; }

		void operator++()
		{
			if ((entry = entry->next) || !table->numItems)
				return;
			bucketIdx++;
			bucket++;
			FindValid();
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

	void Concatenate(const Vector& source)
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
		friend Vector;

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
			this->count = source.numItems;
			if (this->count)
			{
				UInt32 size = this->count * sizeof(T_Data);
				this->pData = (T_Data*)MemCopy(s_auxBuffers[2].Employ(size), source.data, size);
			}
		}
	};
};

template <typename T_Data, UInt32 N> class FixedTypeArray
{
protected:
	UInt32		size;
	T_Data		data[N];

public:
	FixedTypeArray() : size(0) {}

	bool Empty() const { return size == 0; }
	UInt32 Size() const { return size; }
	T_Data* Data() { return data; }

	bool Append(T_Data item)
	{
		if (size >= N) return false;
		data[size++] = item;
		return true;
	}

	T_Data PopBack()
	{
		return size ? data[--size] : 0;
	}

	class Iterator
	{
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