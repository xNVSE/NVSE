#pragma once
#include <stdexcept>

typedef UInt32 ArrayID;
class ArrayVar;
class ArrayVarMap;
struct Slice;
struct ArrayKey;
struct ArrayElement;
struct ScriptToken;

#if RUNTIME

#include "VarMap.h"
#include "Serialization.h"
#include "GameAPI.h"
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <map>

// NVSE array datatype, represented by std::map<ArrayKey, ArrayElement>
// Data elements can be of mixed types (string, UInt32/formID, float)
// Keys can be doubles or strings
// Can optionally be treated as vector (i.e. removal of an element shifts upper elements down)
// Commands such as GetEquippedItems can return arrays

// Array variable interface is based on ArrayIDs, which are simply integers identifying the array
// All array variable operations go through ArrayVarMap interface

/* Serialization layout
String	::= { UInt16 dataLen; char data[dataLen]; }
Element ::= { double num || string str || UInt32 arrayID || UInt32 formID }
Key		::= { string || double }

	ARVS - empty chunk indicating start of array variables
		ARVR
			UInt8	modIndex
			UInt32	ID
			UInt8	keyType
			bool	packed
** v1 **	UInt32  numRefs       <- references to this array (by variables or array element)
** v1 **	UInt8   refs[numRefs] <- mod indexes of each reference to array; on load, fix up, discard those from unloaded mods
			UInt32	numElements
			
			//for each element
			Key			key
			UInt8		elementType
			Element		data
		[ARVR]
			...
	ARVE - empty chunk indicating end of variables

As with string variables, array vars discarded on load if owning mod no longer present in modlist

*/

//#if RUNTIME

enum DataType : UInt8
{
	kDataType_Invalid,

	kDataType_Numeric,
	kDataType_Form,
	kDataType_String,
	kDataType_Array,
};

struct ArrayType {
	union
	{
		double		num;
		UInt32		formID;
	};
	std::string* str = nullptr;

	~ArrayType()
	{
		delete str;
	}
};

struct ArrayElement
{
	friend class ArrayVar;
	friend class ArrayVarMap;

	ArrayType	m_data;
	DataType	m_dataType;
	ArrayID		m_owningArray;

	void  Unset();
	std::string ToString() const;
public:
	DataType DataType() const { return m_dataType; }

	bool GetAsNumber(double* out) const;
	bool GetAsString(std::string& out) const;
	bool GetAsFormID(UInt32* out) const;
	bool GetAsArray(ArrayID* out) const;

	bool SetForm(const TESForm* form);
	bool SetFormID(UInt32 refID);
	bool SetString(const std::string& str);
	bool SetArray(ArrayID arr, UInt8 modIndex);	
	bool SetNumber(double num);
	bool Set(const ArrayElement& elem);

	ArrayElement();

	static bool CompareAsString(const ArrayElement& lhs, const ArrayElement& rhs);

	bool operator<(const ArrayElement& rhs) const;
	bool Equals(const ArrayElement& compareTo) const;

	bool IsGood() { return m_dataType != kDataType_Invalid;	}
};

struct ArrayKey
{
private:
	ArrayType	key;
	UInt8		keyType;
public:
	ArrayKey();
	ArrayKey(const std::string& _key);
	ArrayKey(double _key);
	ArrayKey(const char* _key);

	ArrayType	Key() const	{	return key;	}
	UInt8		KeyType() const { return keyType; }
	void		SetNumericKey(double newVal)	{	keyType = kDataType_Numeric; key.num = newVal;	}
	bool		IsValid() const { return keyType != kDataType_Invalid;	}

	bool operator<(const ArrayKey& rhs) const;
	bool operator==(const ArrayKey& rhs) const;
	bool operator>=(const ArrayKey& rhs) const { return !(*this < rhs);	}
	bool operator>(const ArrayKey& rhs) const { return !(*this < rhs || *this == rhs); }
	bool operator<=(const ArrayKey& rhs) const { return !(*this > rhs); }
};

class ArrayVarElementContainer
{
	
public:
	virtual ~ArrayVarElementContainer() = default;
	
	virtual ArrayElement& operator [](ArrayKey i) = 0;

	class iterator
	{		
	public:
		virtual ~iterator() = default;
		
		virtual void operator++() = 0;

		virtual void operator--() = 0;

		virtual bool operator!=(const iterator& other) const = 0;

		[[nodiscard]] virtual ArrayKey first() const = 0;

		[[nodiscard]] virtual ArrayElement& second() const = 0;

		virtual std::unique_ptr<iterator> clone() const = 0;
	};

	[[nodiscard]] virtual std::unique_ptr<iterator> find(ArrayKey key) = 0;

	[[nodiscard]] virtual std::unique_ptr<iterator> begin() = 0;

	[[nodiscard]] virtual std::unique_ptr<iterator> end() = 0;

	[[nodiscard]] virtual std::size_t size() const = 0;

	std::size_t virtual erase(const ArrayKey key) = 0;
};

class ArrayVarElementMap : public ArrayVarElementContainer
{
	std::map<ArrayKey, ArrayElement> map_;
	
public:
	ArrayElement& operator [](ArrayKey i) override
	{
		return map_[i];
	}

	class MapIterator : public iterator
	{
		std::map<ArrayKey, ArrayElement>::iterator iterator_;
	public:
		MapIterator() = delete;
		
		explicit MapIterator(std::map<ArrayKey, ArrayElement>::iterator iterator) : iterator_(std::move(iterator))
		{
		}
		
		void operator++() override
		{
			++iterator_;
		}

		void operator--() override
		{
			--iterator_;
		}

		bool operator!=(const iterator& other) const override
		{
			const auto& otherMapIter = dynamic_cast<const MapIterator&>(other);
			return otherMapIter.iterator_ != this->iterator_;
		}

		[[nodiscard]] ArrayKey first() const override
		{
			return iterator_->first;
		}

		[[nodiscard]] ArrayElement& second() const override
		{
			return iterator_->second;
		}

		std::unique_ptr<iterator> clone() const override
		{
			return std::make_unique<MapIterator>(iterator_);
		}

		void* operator new(std::size_t size);

		void operator delete(void* p);
		
	};

	[[nodiscard]] std::unique_ptr<iterator> find(ArrayKey key) override
	{
		return std::make_unique<MapIterator>(map_.find(key));
	}


	[[nodiscard]] std::unique_ptr<iterator> begin() override
	{
		return std::make_unique<MapIterator>(map_.begin());
	}

	[[nodiscard]] std::unique_ptr<iterator> end() override
	{
		return std::make_unique<MapIterator>(map_.end());
	}

	[[nodiscard]] std::size_t size() const override
	{
		return map_.size();
	}

	std::size_t erase(const ArrayKey key) override
	{
		return map_.erase(key);
	}

};

class ArrayVarElementVector : public ArrayVarElementContainer
{
	std::vector<ArrayElement> vector_;
	
public:
	ArrayElement& operator[](ArrayKey i) override
	{
		const std::size_t index = i.Key().num;
		if (index >= vector_.size())
		{
			vector_.emplace_back();
			return vector_.back();
		}
		return vector_[index];
	}

	class VectorIterator : public iterator
	{
		std::vector<ArrayElement>& vectorRef_;
		std::vector<ArrayElement>::iterator iterator_;
	public:
		VectorIterator() = delete;
		
		VectorIterator(std::vector<ArrayElement>::iterator iter, std::vector<ArrayElement>& vectorRef) : vectorRef_(vectorRef), iterator_(std::move(iter))
		{
		}
		
		void operator++() override
		{
			++iterator_;
		}
		void operator--() override
		{
			--iterator_;
		}
		bool operator!=(const iterator& other) const override
		{
			const auto& otherMapIter = dynamic_cast<const VectorIterator&>(other);
			return otherMapIter.iterator_ != this->iterator_;
		}
		[[nodiscard]] ArrayKey first() const override
		{
			return ArrayKey(iterator_ - vectorRef_.begin());
		}
		
		[[nodiscard]] ArrayElement& second() const override
		{
			return *iterator_;
		}

		std::unique_ptr<iterator> clone() const override
		{
			return std::make_unique<VectorIterator>(iterator_, vectorRef_);
		}

		void* operator new(std::size_t size);

		void operator delete(void* p);
	};
	
	[[nodiscard]] std::unique_ptr<iterator> find(ArrayKey key) override
	{
		std::size_t index = key.Key().num;
		if (index >= vector_.size())
		{
			return std::make_unique<VectorIterator>(vector_.end(), vector_);
		}
		return std::make_unique<VectorIterator>(vector_.begin() + key.Key().num, vector_);
	}
	
	[[nodiscard]] std::unique_ptr<iterator> begin() override
	{
		return std::make_unique<VectorIterator>(vector_.begin(), vector_);

	}

	[[nodiscard]] std::unique_ptr<iterator> end() override
	{
		return std::make_unique<VectorIterator>(vector_.end(), vector_);
	}
	
	[[nodiscard]] std::size_t size() const override
	{
		return vector_.size();
	}
	
	std::size_t erase(const ArrayKey key) override
	{
		vector_.erase(vector_.begin() + key.Key().num);
		return 1;
	}

	std::size_t erase(const ArrayKey low, const ArrayKey high)
	{
		if (high.Key().num > vector_.size() || low.Key().num > vector_.size())
		{
			throw std::out_of_range("Attempt to erase out of range array var");
		}
		vector_.erase(vector_.begin() + low.Key().num, vector_.begin() + high.Key().num);
		return high.Key().num - low.Key().num;
	}
};

typedef ArrayVarElementContainer::iterator ArrayIterator;

class ArrayVar
{
	friend class ArrayVarMap;
	friend class Matrix;
	friend class PluginAPI::ArrayAPI;

	typedef ArrayVarElementContainer _ElementMap;
	std::unique_ptr<ArrayVarElementContainer> m_elements;
	ArrayID				m_ID;
	UInt8				m_owningModIndex;
	UInt8				m_keyType;
	bool				m_bPacked;
	std::vector<UInt8>	m_refs;		// data is modIndex of referring object; size() is number of references

	UInt32 GetUnusedIndex();

	//explicit ArrayVar(UInt8 modIndex);
	

	ArrayElement* Get(ArrayKey key, bool bCanCreateNew);

	UInt32 ID()		{ return m_ID;	}
	//void Pack();

	void Dump();

public:
	ArrayVar() = delete;
	ArrayVar(UInt32 keyType, bool packed, UInt8 modIndex);
	~ArrayVar();

	UInt8 KeyType() const	{ return m_keyType; }
	bool IsPacked() const	{ return m_bPacked; }
	UInt32 Size() const		{ return m_elements->size(); }
};

class ArrayVarMap : public VarMap<ArrayVar>
{
	// this gets incremented whenever serialization format changes
	static const UInt32 kVersion = 1;

	void Add(ArrayVar* var, UInt32 varID, UInt32 numRefs, UInt8* refs);
public:
	enum SortOrder
	{
		kSort_Ascending,
		kSort_Descending,
	};

	enum SortType
	{
		kSortType_Default,
		kSortType_Alpha,
		kSortType_UserFunction,
	};

	void Save(NVSESerializationInterface* intfc);
	void Load(NVSESerializationInterface* intfc);
	void Clean();

	ArrayID	Create(UInt32 keyType, bool bPacked, UInt8 modIndex);
	ArrayID CreateArray(UInt8 modIndex) { return Create(kDataType_Numeric, true, modIndex); }
	ArrayID CreateMap(UInt8 modIndex)	{ return Create(kDataType_Numeric, false, modIndex); }
	ArrayID CreateStringMap(UInt8 modIndex)	{ return Create(kDataType_String, false, modIndex); }
	
	// operations on ArrayVars
	void    AddReference(ArrayID* ref, ArrayID toRef, UInt8 referringModIndex);
	void    RemoveReference(ArrayID* ref, UInt8 referringModIndex);
	void    AddReference(double* ref, ArrayID toRef, UInt8 referringModIndex);
	void	RemoveReference(double* ref, UInt8 referringModIndex);
	ArrayID Copy(ArrayID from, UInt8 modIndex, bool bDeepCopy);
	ArrayID MakeSlice(ArrayID src, const Slice* slice, UInt8 modIndex);
	UInt32	GetKeyType(ArrayID id);
	bool	Exists(ArrayID id);
	void	Erase(ArrayID toErase);
	void	DumpArray(ArrayID toDump);
	UInt32	SizeOf(ArrayID id);
	UInt32	IsPacked(ArrayID id);
	UInt32  EraseElement(ArrayID id, const ArrayKey& key);
	UInt32  EraseElements(ArrayID id, const ArrayKey& lo, const ArrayKey& hi);	// returns num erased
	UInt32	EraseAllElements(ArrayID id);
	ArrayID Sort(ArrayID src, SortOrder order, SortType type, UInt8 modIndex, Script* comparator=NULL);
	ArrayKey Find(ArrayID toSearch, const ArrayElement& toFind, const Slice* range = NULL);
	std::string GetTypeString(ArrayID arr);
	UInt8	GetOwningModIndex(ArrayID id);
	ArrayID GetKeys(ArrayID id, UInt8 modIndex);
	bool	HasKey(ArrayID id, const ArrayKey& key);
	bool	AsVector(ArrayID id, std::vector<const ArrayElement*> &vecOut);
	bool	SetSize(ArrayID id, UInt32 newSize, const ArrayElement& padWith);
	bool	Insert(ArrayID id, UInt32 atIndex, const ArrayElement& toInsert);
	bool	Insert(ArrayID id, UInt32 atIndex, ArrayID rangeID);

	// operations on ArrayElements
	bool SetElementNumber(ArrayID id, const ArrayKey& key, double num);
	bool SetElementString(ArrayID id, const ArrayKey& key, const std::string& str);
	bool SetElementFormID(ArrayID id, const ArrayKey& key, UInt32 refID);
	bool SetElementArray(ArrayID id, const ArrayKey& key, ArrayID srcID);
	bool SetElement(ArrayID id, const ArrayKey& key, const ArrayElement& val);

	bool GetElementNumber(ArrayID id, const ArrayKey& key, double* out);
	bool GetElementString(ArrayID id, const ArrayKey& key, std::string& out);
	bool GetElementFormID(ArrayID id, const ArrayKey& key, UInt32* out);
	bool GetElementForm(ArrayID id, const ArrayKey& key, TESForm** out);
	bool GetElementArray(ArrayID id, const ArrayKey& key, ArrayID* out);
	bool GetElementAsBool(ArrayID id, const ArrayKey& key, bool* out);
	bool GetElementCString(ArrayID id, const ArrayKey& key, const char** out);

	bool GetFirstElement(ArrayID id, ArrayElement* outElem, ArrayKey* outKey);
	bool GetLastElement(ArrayID id, ArrayElement* outElem, ArrayKey* outKey);
	bool GetNextElement(ArrayID id, ArrayKey* prevKey, ArrayElement* outElem, ArrayKey* outKey);
	bool GetPrevElement(ArrayID id, ArrayKey* prevKey, ArrayElement* outElem, ArrayKey* outKey);

	DataType GetElementType(ArrayID id, const ArrayKey& key);

	static ArrayVarMap * GetSingleton(void);
};

extern ArrayVarMap g_ArrayMap;

namespace PluginAPI
{
	class ArrayAPI 
	{
	private:
		static bool SetElementFromAPI(UInt32 id, ArrayKey& key, const NVSEArrayVarInterface::Element& elem);

	public:
		static NVSEArrayVarInterface::Array* CreateArray(const NVSEArrayVarInterface::Element* data, UInt32 size, Script* callingScript);
		static NVSEArrayVarInterface::Array* CreateStringMap(const char** keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript);
		static NVSEArrayVarInterface::Array* CreateMap(const double* keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript);

		static bool AssignArrayCommandResult(NVSEArrayVarInterface::Array* arr, double* dest);
		static void SetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key, const NVSEArrayVarInterface::Element& value);
		static void AppendElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& value);

		static UInt32 GetArraySize(NVSEArrayVarInterface::Array* arr);
		static UInt32 GetArrayPacked(NVSEArrayVarInterface::Array* arr);
		static NVSEArrayVarInterface::Array* LookupArrayByID(UInt32 id);
		static bool GetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key,
			NVSEArrayVarInterface::Element& out);
		static bool GetElements(NVSEArrayVarInterface::Array* arr, NVSEArrayVarInterface::Element* elements,
			NVSEArrayVarInterface::Element* keys);

		// helper fns
		static bool InternalElemToPluginElem(ArrayElement& src, NVSEArrayVarInterface::Element& out);
	};
}

#endif
