#pragma once

typedef UInt32 ArrayID;
class ArrayVar;
class ArrayVarMap;
struct Slice;
struct ArrayKey;


template <bool isSelfOwning>
struct ArrayElement_Templ;

using ArrayElement = ArrayElement_Templ<false>;

//Like ArrayElem, but assumes the data has no owning array.
//If it contains an array, will extend its lifetime (regular ArrayElement requires an owning arr for that).
using SelfOwningArrayElement = ArrayElement_Templ<true>;

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

	//Casts the data in the form InternalFunctionCaller::PopulateArgs() understands.
	[[nodiscard]] void* GetAsVoidArg() const;

	ArrayData& operator=(const ArrayData &rhs);
	ArrayData(const ArrayData &from);
};
STATIC_ASSERT(sizeof(ArrayData) == 0x10);

template <bool isSelfOwning = false>
struct ArrayElement_Templ
{
	friend class ArrayVar;
	friend class ArrayVarMap;
	
	~ArrayElement_Templ();
	ArrayElement_Templ();

	ArrayData	m_data;

	void  Unset();

	[[nodiscard]] DataType DataType() const {return m_data.dataType;}

	bool GetAsNumber(double* out) const;
	bool GetAsString(const char **out) const;
	bool GetAsFormID(UInt32* out) const;
	bool GetAsArray(ArrayID* out) const;
	[[nodiscard]] bool GetBool() const;

	bool SetForm(const TESForm* form);	//unlike SetFormID, will not store lambda info!
	bool SetFormID(UInt32 refID);
	bool SetString(const char* str);
	bool SetArray(ArrayID arr);	
	bool SetNumber(double num);
	bool Set(const ArrayElement_Templ<isSelfOwning>* elem);

	//WARNING: Does not increment the reference count for a copied array;
	//consider move ctor or calling SetArray.
	ArrayElement_Templ(const ArrayElement_Templ<isSelfOwning>& from);

	ArrayElement_Templ(ArrayElement_Templ<isSelfOwning>&& from) noexcept;

	static bool CompareNames(const ArrayElement_Templ<isSelfOwning>& lhs, const ArrayElement_Templ<isSelfOwning>& rhs);
	static bool CompareNamesDescending(const ArrayElement_Templ<isSelfOwning>& lhs, const ArrayElement_Templ<isSelfOwning>& rhs) {return !CompareNames(lhs, rhs);}

	bool operator<(const ArrayElement_Templ<isSelfOwning>& rhs) const;
	bool operator==(const ArrayElement_Templ<isSelfOwning>& rhs) const;
	bool operator!=(const ArrayElement_Templ<isSelfOwning>& rhs) const;

	[[nodiscard]] bool IsGood() const {return m_data.dataType != kDataType_Invalid;}

	[[nodiscard]] std::string GetStringRepresentation() const;
	[[nodiscard]] void* GetAsVoidArg() const { return m_data.GetAsVoidArg(); }
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
	ArrayVarElementContainer() : m_type(kContainer_Array)
	{
		m_container.data = NULL;
		m_container.numItems = 0;
		m_container.numAlloc = 2;
	}

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

		bool End() {return m_iter.index >= m_iter.contObj->numItems;}

		void operator++();
		void operator--();
		bool operator!=(const iterator& other) const;

		const ArrayKey* first();

		ArrayElement* second();
	};

	iterator begin() {return iterator(*this);}
	iterator rbegin() {return iterator(*this, true);}

	iterator find(const ArrayKey* key) {return iterator(*this, key);}

	ElementVector* getArrayPtr() const {return &AsArray();}
	ElementNumMap* getNumMapPtr() const {return &AsNumMap();}
	ElementStrMap* getStrMapPtr() const {return &AsStrMap();}
};

typedef ArrayVarElementContainer::iterator ArrayIterator;

class ArrayVar
{
	friend struct ArrayElement_Templ<false>;
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

	bool CompareArrays(ArrayVar* arr2, bool checkDeep);

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

	void Dump(const std::function<void(const std::string&)>& output = [&](const std::string& input){ Console_Print("%s", input.c_str()); });
	void DumpToFile(const char* filePath, bool append);

	ArrayVarElementContainer::iterator Begin();

	[[nodiscard]] std::string GetStringRepresentation() const;

	bool Equals(ArrayVar* arr2);
	bool DeepEquals(ArrayVar* arr2);

	ArrayVarElementContainer* GetRawContainer();
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


template <bool isSelfOwning>
ArrayElement_Templ<isSelfOwning>::ArrayElement_Templ()
{
	m_data.dataType = kDataType_Invalid;
	m_data.owningArray = 0;
	m_data.arrID = 0;
}

template <bool isSelfOwning>
ArrayElement_Templ<isSelfOwning>::~ArrayElement_Templ()
{
	Unset();
}

template <bool isSelfOwning>
ArrayElement_Templ<isSelfOwning>::ArrayElement_Templ(const ArrayElement_Templ<isSelfOwning>& from)
{
	m_data.dataType = from.m_data.dataType;
	m_data.owningArray = from.m_data.owningArray;
	if (m_data.dataType == kDataType_String)
		m_data.SetStr(from.m_data.str);
	else m_data.num = from.m_data.num;
}

template <bool isSelfOwning>
ArrayElement_Templ<isSelfOwning>::ArrayElement_Templ(ArrayElement_Templ<isSelfOwning>&& from) noexcept : m_data(from.m_data)
{
	from.m_data.dataType = kDataType_Invalid;

	//for extra safety; likely redundant
	from.m_data.owningArray = 0;
	from.m_data.str = nullptr;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::operator<(const ArrayElement_Templ<isSelfOwning>& rhs) const
{
	// if we ever try to compare 2 elems of differing types (i.e. string and number) we violate strict weak
	// no reason to do that

	if (m_data.dataType != rhs.m_data.dataType)
		return false;

	switch (m_data.dataType)
	{
	case kDataType_Form:
	case kDataType_Array:
		return m_data.formID < rhs.m_data.formID;
	case kDataType_String:
		return StrCompare(m_data.str, rhs.m_data.str) < 0;
	default:
		return m_data.num < rhs.m_data.num;
	}
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::operator==(const ArrayElement_Templ<isSelfOwning>& rhs) const
{
	if (m_data.dataType != rhs.m_data.dataType)
		return false;

	switch (m_data.dataType)
	{
	case kDataType_Form:
		return m_data.formID == rhs.m_data.formID;
	case kDataType_String:
		return !StrCompare(m_data.str, rhs.m_data.str);
	case kDataType_Array:
		return m_data.arrID == rhs.m_data.arrID;
	default:
		return m_data.num == rhs.m_data.num;
	}
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::operator!=(const ArrayElement_Templ<isSelfOwning>& rhs) const
{
	return !(*this == rhs);
}

template <bool isSelfOwning>
std::string ArrayElement_Templ<isSelfOwning>::GetStringRepresentation() const
{
	switch (this->DataType())
	{
	case kDataType_Invalid:
		return "invalid";
	case kDataType_Numeric:
	{
		double numeric;
		this->GetAsNumber(&numeric);
		return FormatString("%g", numeric);
	}
	case kDataType_Form:
	{
		UInt32 formId;
		this->GetAsFormID(&formId);
		auto* form = LookupFormByID(formId);
		if (form)
			return form->GetStringRepresentation();
		return "null";
	}
	case kDataType_String:
	{
		const char* str;
		this->GetAsString(&str);
		return '"' + std::string(str) + '"';
	}
	case kDataType_Array:
	{
		ArrayID id;
		this->GetAsArray(&id);
		const auto* arr = g_ArrayMap.Get(id);
		if (arr)
			return arr->GetStringRepresentation();
		return "invalid array";
	}
	default:
		return "unknown";
	}
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::CompareNames(const ArrayElement_Templ<isSelfOwning>& lhs, const ArrayElement_Templ<isSelfOwning>& rhs)
{
	TESForm* lform = LookupFormByID(lhs.m_data.formID);
	if (lform)
	{
		TESForm* rform = LookupFormByID(rhs.m_data.formID);
		if (rform)
		{
			const char* lName = lform->GetTheName();
			if (*lName)
			{
				const char* rName = rform->GetTheName();
				if (*rName) return StrCompare(lName, rName) < 0;
			}
		}
	}
	return lhs.m_data.formID < rhs.m_data.formID;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::SetForm(const TESForm* form)
{
	Unset();

	m_data.dataType = kDataType_Form;
	m_data.formID = form ? form->refID : 0;
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::SetFormID(UInt32 refID)
{
	Unset();

	m_data.dataType = kDataType_Form;
	m_data.formID = refID;

	TESForm* form;
	if ((form = LookupFormByRefID(refID)) && IS_ID(form, Script) && LambdaManager::IsScriptLambda(static_cast<Script*>(form)))
	{
		LambdaManager::SaveLambdaVariables(static_cast<Script*>(form));
	}

	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::SetString(const char* str)
{
	Unset();

	m_data.dataType = kDataType_String;
	m_data.SetStr(str);
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::SetArray(ArrayID arr)
{
	Unset();

	m_data.dataType = kDataType_Array;
	if constexpr (isSelfOwning)	//don't care about having an owning array.
	{
		g_ArrayMap.AddReference(&m_data.arrID, arr, GetArrayOwningModIndex(m_data.owningArray));
	}
	else
	{
		if (m_data.owningArray)
			g_ArrayMap.AddReference(&m_data.arrID, arr, GetArrayOwningModIndex(m_data.owningArray));
		else // this element is not inside any array, so it's just a temporary
			m_data.arrID = arr;
	}

	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::SetNumber(double num)
{
	Unset();

	m_data.dataType = kDataType_Numeric;
	m_data.num = num;
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::Set(const ArrayElement_Templ<isSelfOwning>* elem)
{
	switch (elem->m_data.dataType)
	{
	case kDataType_String:
		SetString(elem->m_data.str);
		break;
	case kDataType_Array:
		SetArray(elem->m_data.arrID);
		break;
	case kDataType_Numeric:
		SetNumber(elem->m_data.num);
		break;
	case kDataType_Form:
		SetFormID(elem->m_data.formID);
		break;
	default:
		Unset();
		return false;
	}

	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::GetAsArray(ArrayID* out) const
{
	if (m_data.dataType != kDataType_Array)
		return false;
	if (m_data.arrID && !g_ArrayMap.Get(m_data.arrID)) // it's okay for arrayID to be 0, otherwise check if array exists
		return false;

	*out = m_data.arrID;
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::GetAsFormID(UInt32* out) const
{
	if (m_data.dataType != kDataType_Form)
		return false;
	*out = m_data.formID;
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::GetAsNumber(double* out) const
{
	if (m_data.dataType != kDataType_Numeric)
		return false;
	*out = m_data.num;
	return true;
}

template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::GetAsString(const char** out) const
{
	if (m_data.dataType != kDataType_String)
		return false;
	*out = m_data.GetStr();
	return true;
}

// Try to replicate bool ScriptToken::GetBool()
template <bool isSelfOwning>
bool ArrayElement_Templ<isSelfOwning>::GetBool() const
{
	bool result;
	switch (DataType())
	{
	case kDataType_Array:
		result = m_data.arrID && g_ArrayMap.Get(m_data.arrID);
		break;
	case kDataType_Numeric:
		result = m_data.num != 0.0;
		break;
	case kDataType_Form:
		result = m_data.formID != 0;
		break;
	case kDataType_String:
		result = m_data.str && m_data.str[0];
		break;
	default:
		return false;
	}
	return result;
}

template <bool isSelfOwning>
void ArrayElement_Templ<isSelfOwning>::Unset()
{
	if (m_data.dataType == kDataType_Invalid)
		return;

	if (m_data.dataType == kDataType_String)
	{
		if (m_data.str)
		{
			free(m_data.str);
			m_data.str = nullptr;
		}
	}
	else if (m_data.dataType == kDataType_Array && (isSelfOwning || m_data.owningArray))
	{
		g_ArrayMap.RemoveReference(&m_data.arrID, GetArrayOwningModIndex(m_data.arrID));
	}
	else if (m_data.dataType == kDataType_Form)
	{
		auto* form = LookupFormByRefID(m_data.formID);
		if (form && IS_ID(form, Script) && LambdaManager::IsScriptLambda(static_cast<Script*>(form)))
			LambdaManager::UnsaveLambdaVariables(static_cast<Script*>(form));
		m_data.formID = 0;
	}

	m_data.dataType = kDataType_Invalid;
}


#endif
