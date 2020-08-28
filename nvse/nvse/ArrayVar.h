#pragma once

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

struct ArrayData
{
	DataType	dataType;
	ArrayID		owningArray;
	union
	{
		double		num;
		UInt32		formID;
		char		*str;
		ArrayID		arrID;
	};

	~ArrayData();
	const char *GetStr() const;
	void SetStr(const char *srcStr);

	ArrayData& operator=(const ArrayData &rhs);
};
STATIC_ASSERT(sizeof(ArrayData) == 0x10);

struct ArrayElement
{
	friend class ArrayVar;
	friend class ArrayVarMap;

	ArrayData	m_data;

	void  Unset();
	std::string ToString() const;

	DataType DataType() const { return m_data.dataType; }
	UInt8 GetOwningModIndex() const;

	bool GetAsNumber(double* out) const;
	bool GetAsString(const char **out) const;
	bool GetAsFormID(UInt32* out) const;
	bool GetAsArray(ArrayID* out) const;

	bool SetForm(const TESForm* form);
	bool SetFormID(UInt32 refID);
	bool SetString(const char* str);
	bool SetArray(ArrayID arr);	
	bool SetNumber(double num);
	bool Set(const ArrayElement* elem);

	ArrayElement();
	ArrayElement(const ArrayElement& from);

	static bool CompareAsString(const ArrayElement& lhs, const ArrayElement& rhs);

	bool operator<(const ArrayElement& rhs) const;
	bool Equals(const ArrayElement& compareTo) const;

	bool IsGood() { return m_data.dataType != kDataType_Invalid;	}
};
STATIC_ASSERT(sizeof(ArrayElement) == 0x10);

struct ArrayKey
{
	ArrayData	key;

	ArrayKey();
	ArrayKey(double _key);
	ArrayKey(const char* _key);
	ArrayKey(const ArrayKey& from);

	UInt8		KeyType() const { return key.dataType; }
	void		SetNumericKey(double newVal)	{	key.dataType = kDataType_Numeric; key.num = newVal;	}
	bool		IsValid() const { return key.dataType != kDataType_Invalid;	}

	bool operator<(const ArrayKey& rhs) const;
	bool operator==(const ArrayKey& rhs) const;
	bool operator>=(const ArrayKey& rhs) const { return !(*this < rhs);	}
	bool operator>(const ArrayKey& rhs) const { return !(*this < rhs || *this == rhs); }
	bool operator<=(const ArrayKey& rhs) const { return !(*this > rhs); }
};
STATIC_ASSERT(sizeof(ArrayKey) == 0x10);

class ArrayVarElementContainer
{
	std::unique_ptr<std::vector<ArrayElement>> array_ = nullptr;
	std::unique_ptr<std::map<ArrayKey, ArrayElement>> map_ = nullptr;

	bool isArray_ = false;
	
public:
	ArrayVarElementContainer(bool isArray);

	ArrayVarElementContainer();

	//ArrayElement& operator [](const ArrayKey& i) const;

	class iterator
	{		
	public:
		bool isArray_ = false;
		std::map<ArrayKey, ArrayElement>::iterator mapIter_;
		std::vector<ArrayElement>::iterator arrIter_;
		ArrayVarElementContainer const* containerPtr_ = nullptr;

		iterator();

		explicit iterator(const std::map<ArrayKey, ArrayElement>::iterator iter);

		iterator(std::vector<ArrayElement>::iterator iter, ArrayVarElementContainer const* containerRef);

		void operator++();

		void operator--();

		bool operator!=(const iterator& other) const;

		[[nodiscard]] const ArrayKey* first() const;

		[[nodiscard]] ArrayElement* second() const;
	};

	[[nodiscard]] iterator find(const ArrayKey& key) const;

	[[nodiscard]] iterator begin() const;

	[[nodiscard]] iterator end() const;

	[[nodiscard]] std::size_t size() const;

	std::size_t count(const ArrayKey* key) const;

	std::size_t erase(const ArrayKey* key) const;

	std::size_t erase(const ArrayKey* low, const ArrayKey* high) const;

	void clear() const;

	std::vector<ArrayElement>& getVectorRef() const;

	std::map<ArrayKey, ArrayElement>& getMapRef() const;
};


typedef ArrayVarElementContainer::iterator ArrayIterator;

class ArrayVar
{
	friend struct ArrayElement;
	friend class ArrayVarMap;
	friend class Matrix;
	friend class PluginAPI::ArrayAPI;

	typedef ArrayVarElementContainer _ElementMap;
	_ElementMap			m_elements;
	ArrayID				m_ID;
	UInt8				m_owningModIndex;
	UInt8				m_keyType;
	bool				m_bPacked;
	std::vector<UInt8>	m_refs;		// data is modIndex of referring object; size() is number of references

	explicit ArrayVar(UInt8 modIndex);
	ArrayVar(UInt32 keyType, bool packed, UInt8 modIndex);

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

	~ArrayVar();

	UInt32 ID()	const	{ return m_ID;	}
	UInt8 KeyType() const	{ return m_keyType; }
	bool IsPacked() const	{ return m_bPacked; }
	UInt32 Size() const		{ return m_elements.size(); }

	ArrayElement* Get(const ArrayKey* key, bool bCanCreateNew);
	ArrayElement* Get(double key, bool bCanCreateNew);
	ArrayElement* Get(const char* key, bool bCanCreateNew);

	bool HasKey(double key);
	bool HasKey(const char* key);
	bool HasKey(const ArrayKey* key);

	bool SetElementNumber(double key, double num);
	bool SetElementNumber(const char* key, double num);

	bool SetElementString(double key, const char* str);
	bool SetElementString(const char* key, const char* str);

	bool SetElementFormID(double key, UInt32 refID);
	bool SetElementFormID(const char* key, UInt32 refID);

	bool SetElementArray(double key, ArrayID srcID);
	bool SetElementArray(const char* key, ArrayID srcID);

	bool SetElement(double key, const ArrayElement* val);
	bool SetElement(const char* key, const ArrayElement* val);
	bool SetElement(const ArrayKey* key, const ArrayElement* val);

	bool SetElementFromAPI(double key, const NVSEArrayVarInterface::Element* srcElem);
	bool SetElementFromAPI(const char* key, const NVSEArrayVarInterface::Element* srcElem);

	bool GetElementNumber(const ArrayKey* key, double* out);
	bool GetElementString(const ArrayKey* key, const char** out);
	bool GetElementFormID(const ArrayKey* key, UInt32* out);
	bool GetElementForm(const ArrayKey* key, TESForm** out);
	bool GetElementArray(const ArrayKey* key, ArrayID* out);
	DataType GetElementType(const ArrayKey* key);

	const ArrayKey* Find(const ArrayElement* toFind, const Slice* range = NULL);

	bool GetFirstElement(ArrayElement** outElem, const ArrayKey** outKey);
	bool GetLastElement(ArrayElement** outElem, const ArrayKey** outKey);
	bool GetNextElement(ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey);
	bool GetPrevElement(ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey);

	UInt32 EraseElement(const ArrayKey* key);
	UInt32 EraseElements(const ArrayKey* lo, const ArrayKey* hi);	// returns num erased
	UInt32 EraseAllElements();

	bool SetSize(UInt32 newSize, const ArrayElement* padWith);
	bool Insert(UInt32 atIndex, const ArrayElement* toInsert);
	bool Insert(UInt32 atIndex, ArrayID rangeID);

	ArrayVar *GetKeys(UInt8 modIndex);
	ArrayVar *Copy(UInt8 modIndex, bool bDeepCopy);
	ArrayVar *MakeSlice(const Slice* slice, UInt8 modIndex);

	void Sort(ArrayVar *result, SortOrder order, SortType type, Script* comparator = NULL);

	void Dump();
};

class ArrayVarMap : public VarMap<ArrayVar>
{
	// this gets incremented whenever serialization format changes
	static const UInt32 kVersion = 2;

	void Add(ArrayVar* var, UInt32 varID, UInt32 numRefs, UInt8* refs);
public:
	void Save(NVSESerializationInterface* intfc);
	void Load(NVSESerializationInterface* intfc);
	void Clean();

	ArrayVar* Create(UInt32 keyType, bool bPacked, UInt8 modIndex);
	ArrayVar* CreateArray(UInt8 modIndex) { return Create(kDataType_Numeric, true, modIndex); }
	ArrayVar* CreateMap(UInt8 modIndex)	{ return Create(kDataType_Numeric, false, modIndex); }
	ArrayVar* CreateStringMap(UInt8 modIndex)	{ return Create(kDataType_String, false, modIndex); }
	
	// operations on ArrayVars
	void    AddReference(ArrayID* ref, ArrayID toRef, UInt8 referringModIndex);
	void    RemoveReference(ArrayID* ref, UInt8 referringModIndex);
	void    AddReference(double* ref, ArrayID toRef, UInt8 referringModIndex);
	void	RemoveReference(double* ref, UInt8 referringModIndex);
	void	Erase(ArrayID toErase);

	ArrayElement* GetElement(ArrayID id, const ArrayKey* key);

	static ArrayVarMap * GetSingleton(void);
};

extern ArrayVarMap g_ArrayMap;

namespace PluginAPI
{
	class ArrayAPI 
	{
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
		static bool InternalElemToPluginElem(const ArrayElement* src, NVSEArrayVarInterface::Element* out);
	};
}

#endif
