#pragma once
#include "GameScript.h"

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
#include "LambdaManager.h"
#include <string_view>

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

const char* DataTypeToString(DataType dataType);
Script::VariableType DataTypeToVarType(DataType dataType);
DataType VarTypeToDataType(Script::VariableType variableType);

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
	ArrayData() = default;
	[[nodiscard]] const char *GetStr() const;
	void SetStr(const char *srcStr);
	void SetStr(std::string_view srcStr);

	//Casts the data in the form InternalFunctionCaller::PopulateArgs() understands.
	[[nodiscard]] void* GetAsVoidArg() const;

	ArrayData& operator=(const ArrayData &rhs);
	ArrayData(const ArrayData &from);
};
STATIC_ASSERT(sizeof(ArrayData) == 0x10);

struct ArrayElement
{
protected:
	void UnsetDefault();	//to avoid calling virtual func in dtor.
public:
	friend class ArrayVar;
	friend class ArrayVarMap;
	
	virtual ~ArrayElement();
	ArrayElement();

	ArrayData	m_data;

	virtual void  Unset();

	[[nodiscard]] DataType DataType() const {return m_data.dataType;}

	bool GetAsNumber(double* out) const;
	bool GetAsString(const char **out) const;
	bool GetAsFormID(UInt32* out) const;
	bool GetAsArray(ArrayID* out) const;
	[[nodiscard]] bool GetBool() const;

	bool SetTESForm(const TESForm* form)
	{
		Unset();

		m_data.dataType = kDataType_Form;
		m_data.formID = form ? form->refID : 0;
		return true;
	}	//unlike SetFormID, will not store lambda info!
	bool SetFormID(UInt32 refID);
	bool SetString(const char* str);
	bool SetString(std::string_view str);
	virtual bool SetArray(ArrayID arr);	
	bool SetNumber(double num);
	bool Set(const ArrayElement* elem);

	//WARNING: Does not increment the reference count for a copied array;
	//consider move ctor or calling SetArray.
	ArrayElement(ArrayElement& from);

	ArrayElement& operator=(const ArrayElement& from)
	{
		if (this != &from)
		{
			m_data.dataType = from.m_data.dataType;
			m_data.owningArray = from.m_data.owningArray;
			if (m_data.dataType == kDataType_String)
				m_data.SetStr(from.m_data.str);
			else m_data.num = from.m_data.num;
		}
		return *this;
	}

	ArrayElement(ArrayElement&& from) noexcept;

	static bool CompareNames(const ArrayElement& lhs, const ArrayElement& rhs);
	static bool CompareNamesDescending(const ArrayElement& lhs, const ArrayElement& rhs) {return !CompareNames(lhs, rhs);}

	bool operator<(const ArrayElement& rhs) const;
	bool operator==(const ArrayElement& rhs) const;
	bool operator!=(const ArrayElement& rhs) const;

	// Unlike ==/!=, if both elems are Arrays, will cmp their contents instead of their IDs.
	bool Equals(const ArrayElement& rhs, bool deepCmpArr = false) const;

	[[nodiscard]] bool IsGood() const {return m_data.dataType != kDataType_Invalid;}

	[[nodiscard]] std::string GetStringRepresentation() const;
	[[nodiscard]] void* GetAsVoidArg() const { return m_data.GetAsVoidArg(); }
};

//Assumes owningArray is always null.
//Unlike ArrayElement, will increase ref counter for an array value even though owningArray is null.
class SelfOwningArrayElement : public ArrayElement
{
protected:
	void UnsetSelfOwning();	//to avoid calling virtual func in dtor.
public:
	~SelfOwningArrayElement() override;
	bool SetArray(ArrayID arr) override;
	void Unset() override;
	SelfOwningArrayElement() = default;
	SelfOwningArrayElement(ArrayElement&& from) noexcept
		: ArrayElement(std::forward<ArrayElement&&>(from))
	{}
};

struct ArrayKey
{
	ArrayData	key;

	ArrayKey();
	ArrayKey(double _key);
	ArrayKey(const char* _key);
	ArrayKey(const ArrayKey& from);
	ArrayKey(DataType type);

	UInt8 KeyType() const {return key.dataType;}
	bool IsValid() const {return key.dataType != kDataType_Invalid;}

	bool operator<(const ArrayKey& rhs) const;
	bool operator==(const ArrayKey& rhs) const;
	bool operator>=(const ArrayKey& rhs) const { return !(*this < rhs);	}
	bool operator>(const ArrayKey& rhs) const { return !(*this < rhs || *this == rhs); }
	bool operator<=(const ArrayKey& rhs) const { return !(*this > rhs); }
};

extern thread_local ArrayKey s_arrNumKey, s_arrStrKey;

enum ContainerType
{
	kContainer_Array,
	kContainer_NumericMap,
	kContainer_StringMap
};

typedef Vector<ArrayElement> ElementVector;
typedef Map<double, ArrayElement> ElementNumMap;
typedef Map<char*, ArrayElement> ElementStrMap;

class ArrayVarElementContainer
{
	friend class ArrayVar;

	struct GenericContainer
	{
		void *data;
		UInt32		 numItems;
		UInt32		 numAlloc;
	};

	ContainerType		m_type;
	GenericContainer	m_container;

	ElementVector& AsArray() const {return *(ElementVector*)&m_container;}
	ElementNumMap& AsNumMap() const {return *(ElementNumMap*)&m_container;}
	ElementStrMap& AsStrMap() const {return *(ElementStrMap*)&m_container;}

public:
	ArrayVarElementContainer();

	~ArrayVarElementContainer();

	UInt32 size() const {return m_container.numItems;}

	bool empty() const {return !m_container.numItems;}

	void clear();

	UInt32 erase(const ArrayKey* key);

	UInt32 erase(UInt32 iLow, UInt32 iHigh);

	class iterator
	{
		friend ArrayVarElementContainer;

		struct GenericIterator
		{
			GenericContainer	*contObj;
			void				*pData;
			UInt32				index;
		};

		ContainerType	m_type;
		GenericIterator	m_iter;

		ElementVector::Iterator& AsArray() {return *(ElementVector::Iterator*)&m_iter;}
		ElementNumMap::Iterator& AsNumMap() {return *(ElementNumMap::Iterator*)&m_iter;}
		ElementStrMap::Iterator& AsStrMap() {return *(ElementStrMap::Iterator*)&m_iter;}

	public:
		iterator(ArrayVarElementContainer& container);
		iterator(ArrayVarElementContainer& container, bool reverse);
		iterator(ArrayVarElementContainer& container, const ArrayKey* key);
		iterator(ContainerType type, const GenericIterator& iterator);

		bool End() {return m_iter.index >= m_iter.contObj->numItems;}

		void operator++();
		void operator--();
		bool operator!=(const iterator& other) const;
		ArrayElement* operator*();

		const ArrayKey* first();

		ArrayElement* second();
	};

	iterator begin();
	iterator rbegin();
	iterator end() const;

	iterator find(const ArrayKey* key) {return iterator(*this, key);}

	ElementVector* getArrayPtr() const {return &AsArray();}
	ElementNumMap* getNumMapPtr() const {return &AsNumMap();}
	ElementStrMap* getStrMapPtr() const {return &AsStrMap();}
};

typedef ArrayVarElementContainer::iterator ArrayIterator;

class ArrayVar
{
	friend class ArrayVarMap;
	friend class Matrix;
	friend class PluginAPI::ArrayAPI;

	typedef ArrayVarElementContainer _ElementMap;
	_ElementMap			m_elements;
	ArrayID				m_ID;
	UInt8				m_owningModIndex;
	UInt8				m_keyType;
	bool				m_bPacked;
	Vector<UInt8>		m_refs;		// data is modIndex of referring object; size() is number of references

public:
	ICriticalSection m_cs;
#if _DEBUG
	std::string varName;
#endif
	ArrayVar(UInt32 keyType, bool packed, UInt8 modIndex);
	~ArrayVar();

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

	UInt32 ID()	const {return m_ID;}
	UInt8 KeyType() const {return m_keyType;}
	bool IsPacked() const {return m_bPacked;}
	UInt8 OwningModIndex() const {return m_owningModIndex;}
	UInt32 Size() const {return m_elements.size();}
	bool Empty() const {return m_elements.empty();}
	ContainerType GetContainerType() const {return m_elements.m_type;}

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
	bool SetElementString(double key, std::string_view str);

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
	bool GetNextElement(const ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey);
	bool GetPrevElement(const ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey);

	UInt32 EraseElement(const ArrayKey* key);
	UInt32 EraseElements(const Slice* slice);	// returns num erased
	UInt32 EraseAllElements();

	bool SetSize(UInt32 newSize, const ArrayElement* padWith);
	bool Insert(UInt32 atIndex, const ArrayElement* toInsert);
	bool Insert(UInt32 atIndex, ArrayID rangeID);

	ArrayVar *GetKeys(UInt8 modIndex);
	ArrayVar *Copy(UInt8 modIndex, bool bDeepCopy);
	ArrayVar *MakeSlice(const Slice* slice, UInt8 modIndex);

	void Sort(ArrayVar *result, SortOrder order, SortType type, Script* comparator = NULL);

	void Dump(const std::function<void(const std::string&)>& output = [&](const std::string& input){ Console_Print_Str(input); });
	void DumpToFile(const char* filePath, bool append);

	ArrayVarElementContainer::iterator Begin();

	[[nodiscard]] std::string GetStringRepresentation() const;

	bool CompareArrays(ArrayVar* arr2, bool checkDeep);
	bool Equals(ArrayVar* arr2);
	bool DeepEquals(ArrayVar* arr2);

	ArrayVarElementContainer* GetRawContainer();

	_ElementMap::iterator begin();

	_ElementMap::iterator end() const;
};

class ArrayVarMap : public VarMap<ArrayVar>
{
	// this gets incremented whenever serialization format changes
	static const UInt32 kVersion = 2;

	ArrayVar* Add(UInt32 varID, UInt32 keyType, bool packed, UInt8 modIndex, UInt32 numRefs, UInt8* refs);
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

	ArrayElement* GetElement(ArrayID id, const ArrayKey* key);

	void DumpAll(bool save);

	static ArrayVarMap * GetSingleton(void);

#if _DEBUG
	std::vector<ArrayVar*> GetByName(const char* name);
	std::vector<ArrayVar*> GetArraysContainingArrayID(ArrayID id);
#endif
};

extern ArrayVarMap g_ArrayMap;

UInt8 __fastcall GetArrayOwningModIndex(ArrayID arrID);

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
		static int GetContainerType(NVSEArrayVarInterface::Array* arr);
		static NVSEArrayVarInterface::Array* LookupArrayByID(UInt32 id);
		static bool GetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key,
			NVSEArrayVarInterface::Element& out);
		static bool GetElements(NVSEArrayVarInterface::Array* arr, NVSEArrayVarInterface::Element* elements,
			NVSEArrayVarInterface::Element* keys);
		static bool ArrayHasKey(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key);

		// helper fns
		static bool InternalElemToPluginElem(const ArrayElement* src, NVSEArrayVarInterface::Element* out);
	};
}

#endif
