#include "ScriptUtils.h"
#include "ArrayVar.h"
#include "GameForms.h"
#include <algorithm>
#include <intrin.h>
#include <set>

#include "MemoryTracker.h"
#include "Core_Serialization.h"

#if RUNTIME
#include "GameAPI.h"
#include "GameData.h"
#include "FunctionScripts.h"
#include <sstream>
#endif

ArrayVarMap g_ArrayMap;

const char* DataTypeToString(DataType dataType)
{
	switch (dataType) {
	case kDataType_Invalid: return "Invalid";
	case kDataType_Numeric: return "Numeric";
	case kDataType_Form: return "Form";
	case kDataType_String: return "String";
	case kDataType_Array: return "Array";
	default: return "Unknown";
	}
}

Script::VariableType DataTypeToVarType(DataType dataType)
{
	switch (dataType) { case kDataType_Invalid: return Script::eVarType_Invalid;
	case kDataType_Numeric: return Script::eVarType_Float;
	case kDataType_Form: return Script::eVarType_Ref;
	case kDataType_String: return Script::eVarType_String;
	case kDataType_Array: return Script::eVarType_Array;
	default: return Script::eVarType_Invalid;
	}
}

DataType VarTypeToDataType(Script::VariableType variableType)
{
	switch (variableType) {
	case Script::eVarType_Float: [[fallthrough]];
	case Script::eVarType_Integer:
		return DataType::kDataType_Numeric;
	case Script::eVarType_String:
		return DataType::kDataType_String;
	case Script::eVarType_Array:
		return DataType::kDataType_Array;
	case Script::eVarType_Ref:
		return DataType::kDataType_Form;
	case Script::eVarType_Invalid: [[fallthrough]];
	default:
		return DataType::kDataType_Invalid;
	}
}

// ArrayElement
ArrayElement::ArrayElement()
{
	m_data.dataType = kDataType_Invalid;
	m_data.owningArray = 0;
	m_data.arrID = 0;
}

ArrayElement::~ArrayElement()
{
	UnsetDefault();
}


ArrayElement::ArrayElement(ArrayElement& from)
{
	m_data.dataType = from.m_data.dataType;
	m_data.owningArray = from.m_data.owningArray;
	if (m_data.dataType == kDataType_String)
		m_data.SetStr(from.m_data.str);
	else m_data.num = from.m_data.num;
}


ArrayElement::ArrayElement(ArrayElement&& from) noexcept
{
	m_data.dataType = std::exchange(from.m_data.dataType, kDataType_Invalid);
	m_data.owningArray = std::exchange(from.m_data.owningArray, 0);
	m_data.num = std::exchange(from.m_data.num, 0);
}


bool ArrayElement::operator<(const ArrayElement& rhs) const
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


bool ArrayElement::operator==(const ArrayElement& rhs) const
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


bool ArrayElement::operator!=(const ArrayElement& rhs) const
{
	return !(*this == rhs);
}

bool ArrayElement::Equals(const ArrayElement& rhs, bool deepCmpArr) const
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
	{
		auto const lArrPtr = g_ArrayMap.Get(m_data.arrID);
		auto const rArrPtr = g_ArrayMap.Get(rhs.m_data.arrID);
		if (lArrPtr == rArrPtr) //two invalid arrays are considered equal.
			return true;
		if (!lArrPtr || !rArrPtr)
			return false;
		return lArrPtr->CompareArrays(rArrPtr, deepCmpArr);
	}
	default:
		return m_data.num == rhs.m_data.num;
	}
}


std::string ArrayElement::GetStringRepresentation() const
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


bool ArrayElement::CompareNames(const ArrayElement& lhs, const ArrayElement& rhs)
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


bool ArrayElement::SetFormID(UInt32 refID)
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

bool ArrayElement::SetString(const char* str)
{
	Unset();

	m_data.dataType = kDataType_String;
	m_data.SetStr(str);
	return true;
}

bool ArrayElement::SetString(std::string_view str)
{
	Unset();

	m_data.dataType = kDataType_String;
	m_data.SetStr(str);
	return true;
}

bool ArrayElement::SetArray(ArrayID arr)
{
	Unset();

	m_data.dataType = kDataType_Array;

	if (m_data.owningArray)
		g_ArrayMap.AddReference(&m_data.arrID, arr, GetArrayOwningModIndex(m_data.owningArray));
	else // this element is not inside any array, so it's just a temporary
		m_data.arrID = arr;

	return true;
}


bool ArrayElement::SetNumber(double num)
{
	Unset();

	m_data.dataType = kDataType_Numeric;
	m_data.num = num;
	return true;
}


bool ArrayElement::Set(const ArrayElement* elem)
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


bool ArrayElement::GetAsArray(ArrayID* out) const
{
	if (m_data.dataType != kDataType_Array)
		return false;
	if (m_data.arrID && !g_ArrayMap.Get(m_data.arrID)) // it's okay for arrayID to be 0, otherwise check if array exists
		return false;

	*out = m_data.arrID;
	return true;
}


bool ArrayElement::GetAsFormID(UInt32* out) const
{
	if (m_data.dataType != kDataType_Form)
		return false;
	*out = m_data.formID;
	return true;
}


bool ArrayElement::GetAsNumber(double* out) const
{
	if (m_data.dataType != kDataType_Numeric)
		return false;
	*out = m_data.num;
	return true;
}


bool ArrayElement::GetAsString(const char** out) const
{
	if (m_data.dataType != kDataType_String)
		return false;
	*out = m_data.GetStr();
	return true;
}

// Try to replicate bool ScriptToken::GetBool()

bool ArrayElement::GetBool() const
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


void ArrayElement::UnsetDefault()
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
	else if (m_data.dataType == kDataType_Array && m_data.owningArray)
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

void ArrayElement::Unset()
{
	UnsetDefault();
}


// ArrayData

ArrayData::~ArrayData()
{
	if ((dataType == kDataType_String) && str)
	{
		free(str);
		str = nullptr;
	}
	dataType = kDataType_Invalid;
}

const char* ArrayData::GetStr() const
{
	return str ? str : "";
}

void ArrayData::SetStr(const char* srcStr)
{
	str = (srcStr && *srcStr) ? CopyString(srcStr) : nullptr;
}

void ArrayData::SetStr(std::string_view srcStr)
{
	str = (!srcStr.empty() && srcStr[0]) ? CopyString(srcStr.data(), srcStr.length()) : nullptr;
}

ArrayData& ArrayData::operator=(const ArrayData& rhs)
{
	if (this != &rhs)
	{
		if (dataType == kDataType_String && str)
			free(str);
		dataType = rhs.dataType;
		if (dataType == kDataType_String)
			SetStr(rhs.str);
		else num = rhs.num;
	}
	return *this;
}

void* ArrayData::GetAsVoidArg() const
{
	switch (dataType)
	{
	case kDataType_Invalid: return nullptr;
	case kDataType_Numeric:
	{
		auto res = static_cast<float>(num);
		return *(void**)(&res);	//conversion: *((float *)&nthArg
	}
	case kDataType_Form: return LookupFormByID(formID);
	case kDataType_String: return str;
	case kDataType_Array: return reinterpret_cast<void*>(arrID);
	}
	return nullptr;
}

ArrayData::ArrayData(const ArrayData& from) : dataType(from.dataType), owningArray(from.owningArray)
{
	if (dataType == kDataType_String)
		SetStr(from.str);
	else num = from.num;
}

///////////////////////
// SelfOwningArrayElement
//////////////////////

void SelfOwningArrayElement::UnsetSelfOwning()
{
	if (m_data.dataType == kDataType_Array && !m_data.owningArray)
	{
		g_ArrayMap.RemoveReference(&m_data.arrID, GetArrayOwningModIndex(m_data.arrID));
		m_data.dataType = kDataType_Invalid;
		return;
	}
	UnsetDefault();
}

SelfOwningArrayElement::~SelfOwningArrayElement()
{
	UnsetSelfOwning();
}

bool SelfOwningArrayElement::SetArray(ArrayID arr)
{
	if (m_data.owningArray)
		return ArrayElement::SetArray(arr);
	Unset();
	m_data.dataType = kDataType_Array;
	g_ArrayMap.AddReference(&m_data.arrID, arr, GetArrayOwningModIndex(arr));
	return true;
}

void SelfOwningArrayElement::Unset()
{
	UnsetSelfOwning();
}

///////////////////////
// ArrayKey
//////////////////////

UInt8 __fastcall GetArrayOwningModIndex(ArrayID arrID)
{
	ArrayVar* arr = g_ArrayMap.Get(arrID);
	return arr ? arr->OwningModIndex() : 0;
}

ArrayKey::ArrayKey()
{
	key.dataType = kDataType_Invalid;
}

ArrayKey::ArrayKey(const char* _key)
{
	key.dataType = kDataType_String;
	key.SetStr(_key);
}

ArrayKey::ArrayKey(double _key)
{
	key.dataType = kDataType_Numeric;
	key.num = _key;
}

ArrayKey::ArrayKey(const ArrayKey& from)
{
	key.dataType = from.key.dataType;
	if (key.dataType == kDataType_String)
		key.SetStr(from.key.str);
	else key.num = from.key.num;
}

ArrayKey::ArrayKey(DataType type)
{
	key.dataType = type;
	key.num = 0;
}

bool ArrayKey::operator<(const ArrayKey& rhs) const
{
	if (key.dataType != rhs.key.dataType)
	{
		//_MESSAGE("Error: ArrayKey operator< mismatched keytypes");
		return true;
	}

	switch (key.dataType)
	{
	case kDataType_Numeric:
		return key.num < rhs.key.num;
	case kDataType_String:
		return StrCompare(key.str, rhs.key.str) < 0;
	default:
		//_MESSAGE("Error: Invalid ArrayKey type %d", rhs.keyType);
		return true;
	}
}

bool ArrayKey::operator==(const ArrayKey& rhs) const
{
	if (key.dataType != rhs.key.dataType)
	{
		//_MESSAGE("Error: ArrayKey operator== mismatched keytypes");
		return true;
	}

	switch (key.dataType)
	{
	case kDataType_Numeric:
		return key.num == rhs.key.num;
	case kDataType_String:
		return !StrCompare(key.str, rhs.key.str);
	default:
		//_MESSAGE("Error: Invalid ArrayKey type %d", rhs.keyType);
		return true;
	}
}

thread_local ArrayKey s_arrNumKey(kDataType_Numeric), s_arrStrKey(kDataType_String);

///////////////////////
// ArrayVar
//////////////////////
#if _DEBUG && 0
MemoryLeakDebugCollector<ArrayVar> s_arrayDebugCollector;
#endif
ArrayVar::ArrayVar(UInt32 _keyType, bool _packed, UInt8 modIndex) : m_ID(0), m_keyType(_keyType), m_bPacked(_packed),
                                                                    m_owningModIndex(modIndex)
{
	if (m_keyType == kDataType_String)
		m_elements.m_type = kContainer_StringMap;
	else if (m_bPacked)
		m_elements.m_type = kContainer_Array;
	else
		m_elements.m_type = kContainer_NumericMap;

#if _DEBUG && 0
	s_arrayDebugCollector.Add(this);
#endif

}

ArrayVar::~ArrayVar()
{
#if _DEBUG && 0
	s_arrayDebugCollector.Remove(this);
#endif
}

ArrayElement* ArrayVar::Get(const ArrayKey* key, bool bCanCreateNew)
{
	if (m_keyType != key->KeyType())
		return nullptr;

	switch (GetContainerType())
	{
	default:
	case kContainer_Array:
		{
			auto* pArray = m_elements.getArrayPtr();
			int idx = key->key.num;
			if (idx < 0)
				idx += pArray->Size();
			ArrayElement* outElem = pArray->GetPtr((UInt32)idx);
			if (!outElem && bCanCreateNew)
			{
				outElem = pArray->Append();
				outElem->m_data.owningArray = m_ID;
			}
			return outElem;
		}
	case kContainer_NumericMap:
		{
			auto* pMap = m_elements.getNumMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement* newElem = pMap->Emplace(key->key.num);
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			return pMap->GetPtr(key->key.num);
		}
	case kContainer_StringMap:
		{
			auto* pMap = m_elements.getStrMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement* newElem = pMap->Emplace(key->key.str);
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			return pMap->GetPtr(key->key.str);
		}
	}
}

ArrayElement* ArrayVar::Get(double key, bool bCanCreateNew)
{
	if (m_keyType != kDataType_Numeric)
		return nullptr;

	switch (GetContainerType())
	{
	default:
	case kContainer_Array:
		{
			auto* pArray = m_elements.getArrayPtr();
			int idx = key;
			if (idx < 0)
				idx += pArray->Size();
			ArrayElement* outElem = pArray->GetPtr((UInt32)idx);
			if (!outElem && bCanCreateNew)
			{
				outElem = pArray->Append();
				outElem->m_data.owningArray = m_ID;
			}
			return outElem;
		}
	case kContainer_NumericMap:
		{
			auto* pMap = m_elements.getNumMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement* newElem = pMap->Emplace(key);
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			return pMap->GetPtr(key);
		}
	}
}

ArrayElement* ArrayVar::Get(const char* key, bool bCanCreateNew)
{
	if ((m_keyType != kDataType_String) || (GetContainerType() != kContainer_StringMap))
		return nullptr;

	auto* pMap = m_elements.getStrMapPtr();
	if (bCanCreateNew)
	{
		ArrayElement* newElem = pMap->Emplace(const_cast<char*>(key));
		newElem->m_data.owningArray = m_ID;
		return newElem;
	}
	return pMap->GetPtr(const_cast<char*>(key));
}

bool ArrayVar::HasKey(double key)
{
	return Get(key, false) != nullptr;
}

bool ArrayVar::HasKey(const char* key)
{
	return Get(key, false) != nullptr;
}

bool ArrayVar::HasKey(const ArrayKey* key)
{
	return Get(key, false) != nullptr;
}

bool ArrayVar::SetElementNumber(double key, double num)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetNumber(num);
	return true;
}

bool ArrayVar::SetElementNumber(const char* key, double num)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetNumber(num);
	return true;
}

bool ArrayVar::SetElementString(double key, const char* str)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetString(str);
	return true;
}

bool ArrayVar::SetElementString(double key, std::string_view str)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetString(str);
	return true;
}

bool ArrayVar::SetElementString(const char* key, const char* str)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetString(str);
	return true;
}

bool ArrayVar::SetElementFormID(double key, UInt32 refID)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetFormID(refID);
	return true;
}

bool ArrayVar::SetElementFormID(const char* key, UInt32 refID)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetFormID(refID);
	return true;
}

bool ArrayVar::SetElementArray(double key, ArrayID srcID)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetArray(srcID);
	return true;
}

bool ArrayVar::SetElementArray(const char* key, ArrayID srcID)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->SetArray(srcID);
	return true;
}

bool ArrayVar::SetElement(double key, const ArrayElement* val)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElement(const char* key, const ArrayElement* val)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElement(const ArrayKey* key, const ArrayElement* val)
{
	ArrayElement* elem = (key->KeyType() == kDataType_Numeric) ? Get(key->key.num, true) : Get(key->key.GetStr(), true);
	if (!elem)
		return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElementFromAPI(double key, const NVSEArrayVarInterface::Element* srcElem)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	switch (srcElem->type)
	{
	case NVSEArrayVarInterface::Element::kType_Numeric:
		elem->SetNumber(srcElem->num);
		break;
	case NVSEArrayVarInterface::Element::kType_Form:
		{
			UInt32 formID = srcElem->form ? srcElem->form->refID : 0;
			elem->SetFormID(formID);
			break;
		}
	case NVSEArrayVarInterface::Element::kType_String:
		elem->SetString(srcElem->str);
		break;
	case NVSEArrayVarInterface::Element::kType_Array:
		elem->SetArray((ArrayID)srcElem->arr);
		break;
	}
	return true;
}

bool ArrayVar::SetElementFromAPI(const char* key, const NVSEArrayVarInterface::Element* srcElem)
{
	ArrayElement* elem = Get(key, true);
	if (!elem)
		return false;
	switch (srcElem->type)
	{
	case NVSEArrayVarInterface::Element::kType_Numeric:
		elem->SetNumber(srcElem->num);
		break;
	case NVSEArrayVarInterface::Element::kType_Form:
		{
			UInt32 formID = srcElem->form ? srcElem->form->refID : 0;
			elem->SetFormID(formID);
			break;
		}
	case NVSEArrayVarInterface::Element::kType_String:
		elem->SetString(srcElem->str);
		break;
	case NVSEArrayVarInterface::Element::kType_Array:
		elem->SetArray((ArrayID)srcElem->arr);
		break;
	}
	return true;
}

bool ArrayVar::GetElementNumber(const ArrayKey* key, double* out)
{
	ArrayElement* elem = Get(key, false);
	return (elem && elem->GetAsNumber(out));
}

bool ArrayVar::GetElementString(const ArrayKey* key, const char** out)
{
	ArrayElement* elem = Get(key, false);
	return (elem && elem->GetAsString(out));
}

bool ArrayVar::GetElementFormID(const ArrayKey* key, UInt32* out)
{
	ArrayElement* elem = Get(key, false);
	return (elem && elem->GetAsFormID(out));
}

bool ArrayVar::GetElementForm(const ArrayKey* key, TESForm** out)
{
	ArrayElement* elem = Get(key, false);
	UInt32 refID;
	if (elem && elem->GetAsFormID(&refID))
	{
		*out = LookupFormByID(refID);
		return true;
	}
	return false;
}

bool ArrayVar::GetElementArray(const ArrayKey* key, ArrayID* out)
{
	ArrayElement* elem = Get(key, false);
	return (elem && elem->GetAsArray(out));
}

DataType ArrayVar::GetElementType(const ArrayKey* key)
{
	ArrayElement* elem = Get(key, false);
	return elem ? elem->DataType() : kDataType_Invalid;
}

const ArrayKey* ArrayVar::Find(const ArrayElement* toFind, const Slice* range)
{
	if (Empty())
		return nullptr;

	switch (GetContainerType())
	{
	default:
	case kContainer_Array:
		{
			ElementVector* pArray = m_elements.getArrayPtr();
			UInt32 arrSize = pArray->Size(), iLow, iHigh;
			if (range)
			{
				if (range->bIsString)
					return nullptr;
				iLow = (int)range->m_lower;
				iHigh = (int)range->m_upper;
				if (iHigh >= arrSize)
					iHigh = arrSize - 1;
				if ((iLow >= arrSize) || (iLow > iHigh))
					return nullptr;
			}
			else
			{
				iLow = 0;
				iHigh = arrSize - 1;
			}
			ArrayElement* elements = pArray->Data();
			for (int idx = iLow; idx <= iHigh; idx++)
			{
				if (elements[idx] != *toFind) continue;
				s_arrNumKey.key.num = idx;
				return &s_arrNumKey;
			}
			return nullptr;
		}
	case kContainer_NumericMap:
		{
			ElementNumMap::Iterator iter(*m_elements.getNumMapPtr());
			if (range)
			{
				if (range->bIsString)
					return nullptr;
				bool inRange = false;
				for (; !iter.End(); ++iter)
				{
					if (!inRange)
					{
						if (iter.Key() >= range->m_lower)
							inRange = true;
						else continue;
					}
					if (iter.Key() > range->m_upper)
						return nullptr;
					if (iter.Get() == *toFind) break;
				}
			}
			else
			{
				for (; !iter.End(); ++iter)
					if (iter.Get() == *toFind) break;
			}
			if (!iter.End())
			{
				s_arrNumKey.key.num = iter.Key();
				return &s_arrNumKey;
			}
			return nullptr;
		}
	case kContainer_StringMap:
		{
			ElementStrMap::Iterator iter(*m_elements.getStrMapPtr());
			if (range)
			{
				if (!range->bIsString)
					return nullptr;
				const char *sLow = range->m_lowerStr.c_str(), *sHigh = range->m_upperStr.c_str();
				bool inRange = false;
				for (; !iter.End(); ++iter)
				{
					if (!inRange)
					{
						if (StrCompare(iter.Key(), sLow) >= 0)
							inRange = true;
						else continue;
					}
					if (StrCompare(iter.Key(), sHigh) > 0)
						return nullptr;
					if (iter.Get() == *toFind) break;
				}
			}
			else
			{
				for (; !iter.End(); ++iter)
					if (iter.Get() == *toFind) break;
			}
			if (!iter.End())
			{
				s_arrStrKey.key.str = const_cast<char*>(iter.Key());
				return &s_arrStrKey;
			}
			return nullptr;
		}
	}
}

bool ArrayVar::GetFirstElement(ArrayElement** outElem, const ArrayKey** outKey)
{
	if (Empty())
		return false;

	ArrayIterator iter = m_elements.begin();
	*outKey = iter.first();
	*outElem = iter.second();
	return true;
}

bool ArrayVar::GetLastElement(ArrayElement** outElem, const ArrayKey** outKey)
{
	if (Empty())
		return false;

	ArrayIterator iter = m_elements.rbegin();
	*outKey = iter.first();
	*outElem = iter.second();
	return true;
}

bool ArrayVar::GetNextElement(const ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!prevKey || Empty())
		return false;

	ArrayIterator iter = m_elements.find(prevKey);
	if (!iter.End())
	{
		++iter;
		if (!iter.End())
		{
			*outKey = iter.first();
			*outElem = iter.second();
			return true;
		}
	}
	return false;
}

bool ArrayVar::GetPrevElement(const ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!prevKey || Empty())
		return false;

	ArrayIterator iter = m_elements.find(prevKey);
	if (!iter.End())
	{
		--iter;
		if (!iter.End())
		{
			*outKey = iter.first();
			*outElem = iter.second();
			return true;
		}
	}

	return false;
}

UInt32 ArrayVar::EraseElement(const ArrayKey* key)
{
	if (Empty() || (KeyType() != key->KeyType()))
		return -1;
	return m_elements.erase(key);
}

UInt32 ArrayVar::EraseElements(const Slice* slice)
{
	if (slice->bIsString || Empty())
		return -1;
	return m_elements.erase((int)slice->m_lower, (int)slice->m_upper);
}

UInt32 ArrayVar::EraseAllElements()
{
	UInt32 numErased = m_elements.size();
	if (numErased) m_elements.clear();
	return numErased;
}

bool ArrayVar::SetSize(UInt32 newSize, const ArrayElement* padWith)
{
	if (!m_bPacked)
		return false;

	UInt32 varSize = m_elements.size();
	if (varSize < newSize)
	{
		double elemIdx = (int)varSize;
		for (UInt32 i = varSize; i < newSize; i++)
		{
			SetElement(elemIdx, padWith);
			elemIdx += 1;
		}
	}
	else if (varSize > newSize)
		return m_elements.erase(newSize, varSize - 1) > 0;

	return true;
}

bool ArrayVar::Insert(UInt32 atIndex, const ArrayElement* toInsert)
{
	if (!m_bPacked)
		return false;
	auto* pVec = m_elements.getArrayPtr();
	UInt32 varSize = pVec->Size();
	if (atIndex > varSize) return false;
	ArrayElement* newElem = pVec->Insert(atIndex);
	newElem->m_data.owningArray = m_ID;
	newElem->Set(toInsert);
	return true;
}

bool ArrayVar::Insert(UInt32 atIndex, ArrayID rangeID)
{
	ArrayVar* src = g_ArrayMap.Get(rangeID);
	if (!m_bPacked || !src || !src->m_bPacked)
		return false;

	auto *pDest = m_elements.getArrayPtr(), *pSrc = src->m_elements.getArrayPtr();
	UInt32 destSize = pDest->Size();
	if (atIndex > destSize)
		return false;

	UInt32 srcSize = pSrc->Size();
	if (!srcSize) return true;

	pDest->InsertSize(atIndex, srcSize);
	ArrayElement *pDestData = pDest->Data() + atIndex, *pSrcData = pSrc->Data();
	for (UInt32 idx = 0; idx < srcSize; idx++)
	{
		pDestData[idx].m_data.owningArray = m_ID;
		pDestData[idx].Set(&pSrcData[idx]);
	}

	return true;
}

ArrayVar* ArrayVar::GetKeys(UInt8 modIndex)
{
	ArrayVar* keysArr = g_ArrayMap.Create(kDataType_Numeric, true, modIndex);
	double currKey = 0;

	for (ArrayIterator iter = m_elements.begin(); !iter.End(); ++iter)
	{
		if (m_keyType == kDataType_Numeric)
			keysArr->SetElementNumber(currKey, iter.first()->key.num);
		else
			keysArr->SetElementString(currKey, iter.first()->key.GetStr());
		currKey += 1;
	}

	return keysArr;
}

ArrayVar* ArrayVar::Copy(UInt8 modIndex, bool bDeepCopy)
{
	ArrayVar* copyArr = g_ArrayMap.Create(m_keyType, m_bPacked, modIndex);
	const ArrayElement* arrElem;
	for (ArrayIterator iter = m_elements.begin(); !iter.End(); ++iter)
	{
		TempObject<ArrayKey> tempKey(*iter.first());
		// required as iterators pass static objects and this function is recursive
		arrElem = iter.second();
		if ((arrElem->DataType() == kDataType_Array) && bDeepCopy)
		{
			ArrayVar* innerArr = g_ArrayMap.Get(arrElem->m_data.arrID);
			if (innerArr)
			{
				ArrayVar* innerCopy = innerArr->Copy(modIndex, true);
				if (tempKey().KeyType() == kDataType_Numeric)
				{
					if (copyArr->SetElementArray(tempKey().key.num, innerCopy->ID()))
						continue;
				}
				else if (copyArr->SetElementArray(tempKey().key.GetStr(), innerCopy->ID()))
					continue;
			}
			DEBUG_PRINT("ArrayVarMap::Copy failed to make deep copy of inner array");
		}
		else if (!copyArr->SetElement(&tempKey(), arrElem))
		DEBUG_PRINT("ArrayVarMap::Copy failed to set element in copied array");
	}
	return copyArr;
}

ArrayVar* ArrayVar::MakeSlice(const Slice* slice, UInt8 modIndex)
{
	ArrayVar* newVar = g_ArrayMap.Create(m_keyType, m_bPacked, modIndex);

	if (Empty() || (slice->bIsString != (m_keyType == kDataType_String)))
		return newVar;

	switch (GetContainerType())
	{
	default:
	case kContainer_Array:
		{
			ElementVector* pArray = m_elements.getArrayPtr();
			UInt32 arrSize = pArray->Size(), iLow = (int)slice->m_lower, iHigh = (int)slice->m_upper;
			if (iHigh >= arrSize)
				iHigh = arrSize - 1;
			if ((iLow >= arrSize) || (iLow > iHigh))
				break;
			ArrayElement* elements = pArray->Data();
			double packedIndex = 0;
			for (int idx = iLow; idx <= iHigh; idx++)
			{
				newVar->SetElement(packedIndex, &elements[idx]);
				packedIndex += 1;
			}
			break;
		}
	case kContainer_NumericMap:
		{
			bool inRange = false;
			for (auto iter = m_elements.getNumMapPtr()->Begin(); !iter.End(); ++iter)
			{
				if (!inRange)
				{
					if (iter.Key() >= slice->m_lower)
						inRange = true;
					else continue;
				}
				if (iter.Key() > slice->m_upper)
					break;
				newVar->SetElement(iter.Key(), &iter.Get());
			}
			break;
		}
	case kContainer_StringMap:
		{
			const char *sLow = slice->m_lowerStr.c_str(), *sHigh = slice->m_upperStr.c_str();
			bool inRange = false;
			for (auto iter = m_elements.getStrMapPtr()->Begin(); !iter.End(); ++iter)
			{
				if (!inRange)
				{
					if (StrCompare(iter.Key(), sLow) >= 0)
						inRange = true;
					else continue;
				}
				if (StrCompare(iter.Key(), sHigh) > 0)
					break;
				newVar->SetElement(iter.Key(), &iter.Get());
			}
			break;
		}
	}
	return newVar;
}

class SortFunctionCaller : public FunctionCaller
{
	Script* m_comparator;
	ArrayVar* m_lhs;
	ArrayVar* m_rhs;
	bool descending;

public:
	SortFunctionCaller(Script* comparator, bool _descending) : m_comparator(comparator), m_lhs(nullptr), m_rhs(nullptr),
	                                                           descending(_descending)
	{
		if (comparator)
		{
			m_lhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
			m_rhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
		}
	}

	~SortFunctionCaller() override
	{
	}

	UInt8 ReadCallerVersion() override { return UserFunctionManager::kVersion; }
	Script* ReadScript() override { return m_comparator; }

	bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) override
	{
		DynamicParamInfo& dParams = info->ParamInfo();
		if (dParams.NumParams() == 2)
		{
			UserFunctionParam* lhs = info->GetParam(0);
			UserFunctionParam* rhs = info->GetParam(1);
			if (lhs && rhs && lhs->varType == Script::eVarType_Array && rhs->varType == Script::eVarType_Array)
			{
				ScriptLocal* var = eventList->GetVariable(lhs->varIdx);
				if (!var)
				{
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				g_ArrayMap.AddReference(&var->data, m_lhs->ID(), m_comparator->GetModIndex());
				AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_Array);

				var = eventList->GetVariable(rhs->varIdx);
				if (!var)
				{
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				g_ArrayMap.AddReference(&var->data, m_rhs->ID(), m_comparator->GetModIndex());
				AddToGarbageCollection(eventList, var, NVSEVarType::kVarType_Array);
				return true;
			}
		}

		return false;
	}

	TESObjectREFR* ThisObj() override { return nullptr; }
	TESObjectREFR* ContainingObj() override { return nullptr; }

	bool operator()(const ArrayElement& lhs, const ArrayElement& rhs)
	{
		m_lhs->SetElement(0.0, &lhs);
		m_rhs->SetElement(0.0, &rhs);
		if (auto const result = UserFunctionManager::Call(std::move(*this)))
		{
			bool const bResult = result->GetBool();
			return descending ? !bResult : bResult;
		}
		return false;
	}
};

void ArrayVar::Sort(ArrayVar* result, SortOrder order, SortType type, Script* comparator)
{
	// restriction: all elements of src must be of the same type

	if (Empty()) return;

	ArrayIterator iter = m_elements.begin();
	DataType dataType = iter.second()->DataType();
	if ((dataType == kDataType_Invalid) || (dataType == kDataType_Array)) // nonsensical to sort array of arrays
		return;

	if ((type == kSortType_Alpha) && (dataType != kDataType_Form))
		type = kSortType_Default;

	auto pOutArr = result->m_elements.getArrayPtr();
	result->m_elements.m_container.numAlloc = m_elements.size();
	TempObject<ArrayElement> tempElem;
	tempElem().m_data.owningArray = result->m_ID;
	bool descending = (order == kSort_Descending);
	switch (type)
	{
	case kSortType_Default:
		{
			for (; !iter.End(); ++iter)
			{
				if (iter.second()->DataType() != dataType)
					continue;
				tempElem().Set(iter.second());
				pOutArr->InsertSorted(tempElem(), descending);
				tempElem().m_data.dataType = kDataType_Invalid;
			}
			break;
		}
	case kSortType_Alpha:
		{
			for (; !iter.End(); ++iter)
			{
				if (iter.second()->DataType() != kDataType_Form)
					continue;
				tempElem().SetFormID(iter.second()->m_data.formID);
				pOutArr->InsertSorted(
					tempElem(), descending ? ArrayElement::CompareNamesDescending : ArrayElement::CompareNames);
				tempElem().m_data.dataType = kDataType_Invalid;
			}
			break;
		}
	case kSortType_UserFunction:
		{
			if (!comparator) break;
			SortFunctionCaller sorter(comparator, descending);
			for (; !iter.End(); ++iter)
			{
				if (iter.second()->DataType() != dataType)
					continue;
				tempElem().Set(iter.second());
				pOutArr->InsertSorted(tempElem(), sorter);
				tempElem().m_data.dataType = kDataType_Invalid;
			}
			break;
		}
	}
}

void ArrayVar::Dump(const std::function<void(const std::string&)>& output)
{
	const char* owningModName = DataHandler::Get()->GetNthModName(m_owningModIndex);

	auto const str = FormatString("** Dumping Array #%d **\nRefs: %d Owner %02X: %s", m_ID, m_refs.Size(), m_owningModIndex, owningModName);
	output(str);
	_MESSAGE("%s", str.c_str());

	for (ArrayIterator iter = m_elements.begin(); !iter.End(); ++iter)
	{
		char numBuf[0x50];
		std::string elementInfo("[ ");

		switch (KeyType())
		{
		case kDataType_Numeric:
			snprintf(numBuf, sizeof(numBuf), "%f", iter.first()->key.num);
			elementInfo += numBuf;
			break;
		case kDataType_String:
			elementInfo += iter.first()->key.GetStr();
			break;
		default:
			elementInfo += "?Unknown Key Type?";
		}

		elementInfo += " ] : ";

		switch (iter.second()->m_data.dataType)
		{
		case kDataType_Numeric:
			snprintf(numBuf, sizeof(numBuf), "%f", iter.second()->m_data.num);
			elementInfo += numBuf;
			break;
		case kDataType_String:
			elementInfo += iter.second()->m_data.GetStr();
			break;
		case kDataType_Array:
			{
				auto* arr = g_ArrayMap.Get(iter.second()->m_data.arrID);
				if (arr)
					elementInfo += arr->GetStringRepresentation();
				else
					elementInfo += "NULL Array";
				break;
			}
		case kDataType_Form:
			{
				UInt32 refID = iter.second()->m_data.formID;
				snprintf(numBuf, sizeof(numBuf), "%08X", refID);
				TESForm* form = LookupFormByID(refID);
				if (form)
					elementInfo += GetFullName(form);
				else
					elementInfo += "<?NAME?>";

				elementInfo += " (";
				elementInfo += numBuf;
				elementInfo += ")";
				break;
			}
		default:
			elementInfo += "?Unknown Element Type?";
		}

		output(elementInfo);
		_MESSAGE("%s", elementInfo.c_str());
	}
}

void ArrayVar::DumpToFile(const char* filePath, bool append)
{
	try
	{
		FILE* f;
		errno_t e;
		if (append)
		{
			e = fopen_s(&f, filePath, "a+");
		}
		else
		{
			e = fopen_s(&f, filePath, "w+");
		}

		if (!e)
		{
			Dump([&](const std::string& input){ fprintf(f, "%s\n", input.c_str()); });
			fclose(f);
		}
		else
			_MESSAGE("Cannot open file %s [%d]", filePath, e);

	}
	catch (...)
	{
		_MESSAGE("Cannot write to file %s", filePath);
	}
}

ArrayVarElementContainer::iterator ArrayVar::Begin()
{
	return m_elements.begin();
}

std::string ArrayVar::GetStringRepresentation() const
{
	switch (this->m_elements.m_type)
	{
	case kContainer_Array:
		{
			std::string result = "[";
			auto* container = this->m_elements.getArrayPtr();
			for (auto iter = container->Begin(); !iter.End(); ++iter)
			{
				result += iter.Get().GetStringRepresentation();
				if (iter.Index() != container->Size() - 1)
					result += ", ";
			}
			result += "]";
			return result;
		}
	case kContainer_NumericMap:
		{
			std::string result = "[";
			auto* container = this->m_elements.getNumMapPtr();
			for (auto iter = container->Begin(); !iter.End(); ++iter)
			{
				result += std::to_string(iter.Key()) + ": " + iter.Get().GetStringRepresentation();
				if (iter.Index() != container->Size() - 1)
					result += ", ";
			}
			result += "]";
			return result;
		}
	case kContainer_StringMap:
	{
		std::string result = "[";
		auto* container = this->m_elements.getStrMapPtr();
		for (auto iter = container->Begin(); !iter.End(); ++iter)
		{
			result += '"' + std::string(iter.Key()) + '"' + ": " + iter.Get().GetStringRepresentation();
			if (iter.Index() != container->Size() - 1)
				result += ", ";
		}
		result += "]";
		return result;
	}
	default:
		return "invalid array";
	}
}

/*void ArrayVar::Pack()
{
	if (!IsPacked() || !Size())
		return;

	// assume only one hole exists (i.e. we previously erased 0 or more contiguous elements)
	// these are double but will always hold integer values for packed arrays
	double curIdx = 0;		// last correct index

	ArrayIterator iter;
	for (iter = m_elements.begin(); iter != m_elements.end(); )
	{
		if (!(iter.first() == curIdx))
		{
			ArrayElement* elem = Get(ArrayKey(curIdx), true);
			elem->Set(iter.second());
			iter.second().Unset();
			ArrayIterator toDelete = iter;
			++iter;
			m_elements.erase(toDelete);
		}
		else
			++iter;

		curIdx += 1;
	}
}*/

bool ArrayVar::CompareArrays(ArrayVar* arr2, bool checkDeep)
{
	if (this->Size() != arr2->Size())
		return false;

	if (this->ID() == arr2->ID())
		return true;

	auto iter2 = arr2->m_elements.begin();
	for (auto iter1 = this->m_elements.begin(); !iter1.End(); ++iter1, ++iter2)
	{
		//== Compare keys.
		TempObject<ArrayKey> tempKey1(*iter1.first());
		TempObject<ArrayKey> tempKey2(*iter2.first());
		// required as iterators pass static objects and this function is recursive   // copied note from ArrayVar::Copy()

		if (tempKey1().KeyType() != tempKey2().KeyType())
			return false;

		if (tempKey1() != tempKey2())  // Redundant KeyType check, but who knows when that error message might be re-enabled.
			return false;

		//== Compare elements.
		const ArrayElement& elem1 = *iter1.second();
		const ArrayElement& elem2 = *iter2.second();

		if (elem1.DataType() != elem2.DataType())
			return false;

		if (elem1.DataType() == kDataType_Array && checkDeep)  // recursion case
		{
			auto innerArr1 = g_ArrayMap.Get(elem1.m_data.arrID);
			auto innerArr2 = g_ArrayMap.Get(elem2.m_data.arrID);
			if (innerArr1 && innerArr2)
			{
				if (!innerArr1->CompareArrays(innerArr2, true))  // recursive call
					return false;
			}
			else if (innerArr1 || innerArr2)
			{
				return false;  // one of the inner arrays is null while the other is not.
			}
		}
		else if (elem1 != elem2)
			return false;
	}
	return true;
}

bool ArrayVar::Equals(ArrayVar* arr2)
{
	return this->CompareArrays(arr2, false);
}

bool ArrayVar::DeepEquals(ArrayVar* arr2)
{
	return this->CompareArrays(arr2, true);
}

ArrayVarElementContainer* ArrayVar::GetRawContainer()
{
	return &m_elements;
}

ArrayVarElementContainer::iterator ArrayVar::begin()
{
	return m_elements.begin();
}

ArrayVarElementContainer::iterator ArrayVar::end() const
{
	return m_elements.end();
}

//////////////////////////
// ArrayVarMap
/////////////////////////

ArrayVarMap* ArrayVarMap::GetSingleton()
{
	return &g_ArrayMap;
}
#if _DEBUG
std::vector<ArrayVar*> ArrayVarMap::GetByName(const char* name)
{
	std::vector<ArrayVar*> out;
	for (auto iter = vars.Begin(); !iter.End(); ++iter)
		if (FindStringCI(iter.Get().varName, name))
			out.push_back(&iter.Get());
	return out;
}

std::vector<ArrayVar*> ArrayVarMap::GetArraysContainingArrayID(ArrayID id)
{
	std::vector<ArrayVar*> out;
	for (auto iter = vars.Begin(); !iter.End(); ++iter)
	{
		for (auto elemIter = iter.Get().m_elements.begin(); !elemIter.End(); ++elemIter)
		{
			ArrayID iterId;
			if (elemIter.second()->DataType() == DataType::kDataType_Array && elemIter.second()->GetAsArray(&iterId))
			{
				if (iterId == id)
				{
					out.push_back(&iter.Get());
					break;
				}
			}
		}
	}
	return out;
}
#endif
ArrayVar* ArrayVarMap::Add(UInt32 varID, UInt32 keyType, bool packed, UInt8 modIndex, UInt32 numRefs, UInt8* refs)
{
	ArrayVar* var = VarMap::Insert(varID, keyType, packed, modIndex);
	ScopedLock lock(var->m_cs);
	availableIDs.Erase(varID);
	var->m_ID = varID;
	if (numRefs) // record references to this array
		var->m_refs.Concatenate(refs, numRefs);
	else // nobody refers to this array, queue for deletion
		MarkTemporary(varID, true);
	return var;
}

ArrayVar* ArrayVarMap::Create(UInt32 keyType, bool bPacked, UInt8 modIndex)
{
	ArrayID varID = GetUnusedID();
	ArrayVar* newVar = VarMap::Insert(varID, keyType, bPacked, modIndex);
	newVar->m_ID = varID;
	MarkTemporary(varID, true); // queue for deletion until a reference to this array is made
	return newVar;
}

void ArrayVarMap::AddReference(ArrayID* ref, ArrayID toRef, UInt8 referringModIndex)
{
	if (*ref) // refers to a different array, remove that reference
		RemoveReference(ref, referringModIndex);

	ArrayVar* arr = Get(toRef);
	if (arr)
	{
		ScopedLock lock(arr->m_cs);
		arr->m_refs.Append(referringModIndex); // record reference, increment refcount
		*ref = toRef; // store ref'ed ArrayID in reference
		MarkTemporary(toRef, false);
	}
}

void ArrayVarMap::RemoveReference(ArrayID* ref, UInt8 referringModIndex)
{
	ArrayVar* var = Get(*ref);
	if (var)
	{
		ScopedLock lock(var->m_cs);
		// decrement refcount
		var->m_refs.Remove(referringModIndex);

		// if refcount is zero, queue for deletion
		if (var->m_refs.Empty())
		{
#if _DEBUG // Deleting directly not suitable for Map (as opposed to UnorderedMap used in Release)
			MarkTemporary(var->ID(), true);
#else
			//Delete(var->ID());
			MarkTemporary(var->ID(), true);
#endif
		}
	}

	// store 0 in reference
	*ref = 0;
}

void ArrayVarMap::AddReference(double* ref, ArrayID toRef, UInt8 referringModIndex)
{
	ArrayID refID = *ref;
	AddReference(&refID, toRef, referringModIndex);
	*ref = refID;
}

void ArrayVarMap::RemoveReference(double* ref, UInt8 referringModIndex)
{
	ArrayID refID = *ref;
	RemoveReference(&refID, referringModIndex);
	*ref = refID;
}

ArrayElement* ArrayVarMap::GetElement(ArrayID id, const ArrayKey* key)
{
	ArrayVar* arr = Get(id);
	return arr ? arr->Get(key, false) : nullptr;
}

void ArrayVarMap::Save(NVSESerializationInterface* intfc)
{
	Clean();

	Serialization::OpenRecord('ARVS', kVersion);

	ArrayVar* pVar;
	UInt32 numRefs;
	UInt8 keyType;
	const ArrayKey* pKey;
	const ArrayElement* pElem;
	char* str;
	UInt16 len;
	for (auto iter = vars.Begin(); !iter.End(); ++iter)
	{
		if (IsTemporary(iter.Key()))
			continue;

		pVar = &iter.Get();
		numRefs = pVar->m_refs.Size();
		if (!numRefs) continue;
		keyType = pVar->m_keyType;

		Serialization::OpenRecord('ARVR', kVersion);
		Serialization::WriteRecord8(pVar->m_owningModIndex);
		Serialization::WriteRecord32(iter.Key());
		Serialization::WriteRecord8(keyType);
		Serialization::WriteRecord8(pVar->m_bPacked);
		Serialization::WriteRecord32(numRefs);
		Serialization::WriteRecordData(pVar->m_refs.Data(), numRefs);

		numRefs = pVar->Size();
		Serialization::WriteRecord32(numRefs);
		if (!numRefs) continue;

		for (ArrayIterator elems = pVar->m_elements.begin(); !elems.End(); ++elems)
		{
			pKey = elems.first();
			pElem = elems.second();

			if (keyType == kDataType_String)
			{
				str = pKey->key.str;
				len = StrLen(str);
				Serialization::WriteRecord16(len);
				if (len) Serialization::WriteRecordData(str, len);
			}
			else if (!pVar->m_bPacked)
				Serialization::WriteRecord64(&pKey->key.num);

			Serialization::WriteRecord8(pElem->m_data.dataType);
			switch (pElem->m_data.dataType)
			{
			case kDataType_Numeric:
				Serialization::WriteRecord64(&pElem->m_data.num);
				break;
			case kDataType_String:
				{
					str = pElem->m_data.str;
					len = StrLen(str);
					Serialization::WriteRecord16(len);
					if (len) Serialization::WriteRecordData(str, len);
					break;
				}
			case kDataType_Array:
			case kDataType_Form:
				Serialization::WriteRecord32(pElem->m_data.formID);
				break;
			default:
				_MESSAGE("Error in ArrayVarMap::Save() - unhandled element type %d. Element not saved.",
				         pElem->m_data.dataType);
			}
		}
	}

	Serialization::OpenRecord('ARVE', kVersion);
}
#if _DEBUG
std::set<std::string> g_modsWithCosaveVars;
#endif
void ArrayVarMap::Load(NVSESerializationInterface* intfc)
{
	_MESSAGE("Loading array variables");

	Clean(); // clean up any vars queued for garbage collection

	UInt32 type, length, version, arrayID, tempRefID, numElements;
	UInt16 strLength;
	UInt8 modIndex, keyType;
	bool bPacked;
	ContainerType contType;
	static UInt8 buffer[kMaxMessageLength];

	//Reset(intfc);
	bool bContinue = true;
	UInt32 lastIndexRead = 0;

	double numKey;
	std::unordered_map<UInt8, UInt32> varCountMap;
	while (bContinue && Serialization::GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'ARVE': //end of block
			bContinue = false;
			break;
		case 'ARVR':
			{
				modIndex = Serialization::ReadRecord8();
#if _DEBUG
				g_modsWithCosaveVars.insert(g_modsLoaded.at(modIndex));
#endif
				if (!Serialization::ResolveRefID(modIndex << 24, &tempRefID) || modIndex == 0xFF)
				{
					// owning mod was removed, but there may be references to it from other mods
					// assign ownership to the first mod which refers to it and is still loaded
					// if no loaded mods refer to it, discard
					// we handle all of that below
					_MESSAGE(
						"Mod owning array was removed from load order; will attempt to assign ownership to a referring mod.");
					modIndex = 0;
				}
				else
					modIndex = (tempRefID >> 24);

				arrayID = Serialization::ReadRecord32();
				keyType = Serialization::ReadRecord8();
				bPacked = Serialization::ReadRecord8();

				// read refs, fix up mod indexes, discard refs from unloaded mods
				UInt32 numRefs = 0; // # of references to this array

				// reference-counting implemented in v1
				if (version >= 1)
				{
					numRefs = Serialization::ReadRecord32();
					if (numRefs)
					{
						UInt32 tempRefID = 0;
						UInt8 curModIndex;
						UInt32 refIdx = 0;
						for (UInt32 i = 0; i < numRefs; i++)
						{
							curModIndex = Serialization::ReadRecord8();
							if (curModIndex == 0xFF)
								continue;
#if _DEBUG
							g_modsWithCosaveVars.insert(g_modsLoaded.at(curModIndex));
#endif
							if (!modIndex)
							{
								if (Serialization::ResolveRefID(curModIndex << 24, &tempRefID))
								{
									modIndex = tempRefID >> 24;
									_MESSAGE("ArrayID %d was owned by an unloaded mod. Assigning ownership to mod #%d",
									         arrayID, modIndex);
								}
							}

							if (Serialization::ResolveRefID(curModIndex << 24, &tempRefID))
								buffer[refIdx++] = (tempRefID >> 24);

							bool resolveRefID = true;
#if !_DEBUG
							resolveRefID = Serialization::ResolveRefID(curModIndex << 24, nullptr);
#endif
							if (g_showFileSizeWarning && resolveRefID)
							{
								auto& numVars = varCountMap[curModIndex];
								numVars++;
								if (numVars > 2000)
									g_cosaveWarning.modIndices.insert(curModIndex);
							}
						}

						numRefs = refIdx;
					}
				}
				else // v0 arrays assumed to have only one reference (the owning mod)
				{
					if (modIndex) // owning mod is loaded
					{
						numRefs = 1;
						buffer[0] = modIndex;
					}
				}

				if (!modIndex)
				{
					_MESSAGE("Array ID %d is referred to by no loaded mods. Discarding", arrayID);
					continue;
				}

				// record gaps between IDs for easy lookup later in GetUnusedID()
				lastIndexRead++;
				while (lastIndexRead < arrayID)
				{
					SetIDAvailable(lastIndexRead);
					lastIndexRead++;
				}

				// create array and add to map
				ArrayVar* newArr = Add(arrayID, keyType, bPacked, modIndex, numRefs, buffer);

				// read the array elements
				numElements = Serialization::ReadRecord32();
				if (!numElements) continue;

				contType = newArr->GetContainerType();

				ArrayElement *elements, *elem;
				ElementNumMap* pNumMap;
				ElementStrMap* pStrMap;
				switch (contType)
				{
				case kContainer_Array:
					{
						auto* pArray = newArr->m_elements.getArrayPtr();
						pArray->Resize(numElements);
						elements = pArray->Data();
						break;
					}
				case kContainer_NumericMap:
					pNumMap = newArr->m_elements.getNumMapPtr();
					break;
				case kContainer_StringMap:
					pStrMap = newArr->m_elements.getStrMapPtr();
					break;
				default:
					continue;
				}

				for (UInt32 i = 0; i < numElements; i++)
				{
					if (keyType == kDataType_String)
					{
						strLength = Serialization::ReadRecord16();
						if (strLength) Serialization::ReadRecordData(buffer, strLength);
						buffer[strLength] = 0;
					}
					else if (!bPacked || (version < 2))
						Serialization::ReadRecord64(&numKey);

					UInt8 elemType = Serialization::ReadRecord8();

					switch (contType)
					{
					default:
					case kContainer_Array:
						elem = &elements[i];
						break;
					case kContainer_NumericMap:
						elem = &(*pNumMap)[numKey];
						break;
					case kContainer_StringMap:
						elem = &(*pStrMap)[(char*)buffer];
						break;
					}

					elem->m_data.dataType = (DataType)elemType;
					elem->m_data.owningArray = arrayID;

					switch (elemType)
					{
					case kDataType_Numeric:
						Serialization::ReadRecord64(&elem->m_data.num);
						break;
					case kDataType_String:
						{
							strLength = Serialization::ReadRecord16();
							if (strLength)
							{
								char* strVal = (char*)malloc(strLength + 1);
								Serialization::ReadRecordData(strVal, strLength);
								strVal[strLength] = 0;
								elem->m_data.str = strVal;
							}
							else elem->m_data.str = nullptr;
							break;
						}
					case kDataType_Array:
						elem->m_data.arrID = Serialization::ReadRecord32();
						break;
					case kDataType_Form:
						{
							UInt32 formID = Serialization::ReadRecord32();
							if (!Serialization::ResolveRefID(formID, &formID))
								formID = 0;
							elem->m_data.formID = formID;
							break;
						}
					default:
						_MESSAGE("Unknown element type %d encountered while loading array var, element discarded.",
						         elemType);
						break;
					}
				}
				break;
			}
		default:
			_MESSAGE("Error loading array var map: unexpected chunk type %d", type);
			break;
		}
	}
}

void ArrayVarMap::Clean() // garbage collection: delete unreferenced arrays
{
	// ArrayVar destructor may queue more IDs for deletion if deleted array contains other arrays
	// so on each pass through the loop we delete the first ID in the queue until none remain

	while (!tempIDs.Empty())
		Delete(tempIDs.LastKey());
}

void ArrayVarMap::DumpAll(bool save)
{
	auto append = false;
	for (auto iter = vars.Begin(); !iter.End(); ++iter)
	{
		iter.Get().DumpToFile(save ? "array_save.txt" : "array_load.txt", append);
		append = true;
	}
}

namespace PluginAPI
{
	NVSEArrayVarInterface::Array* ArrayAPI::CreateArray(const NVSEArrayVarInterface::Element* data, UInt32 size,
	                                                    Script* callingScript)
	{
		ArrayVar* arr = g_ArrayMap.Create(kDataType_Numeric, true, callingScript->GetModIndex());
		if (!arr) return nullptr;
		double elemIdx = 0;
		for (UInt32 i = 0; i < size; i++)
		{
			arr->SetElementFromAPI(elemIdx, &data[i]);
			elemIdx += 1;
		}
		return (NVSEArrayVarInterface::Array*)arr->m_ID;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateStringMap(const char** keys,
	                                                        const NVSEArrayVarInterface::Element* values, UInt32 size,
	                                                        Script* callingScript)
	{
		ArrayVar* arr = g_ArrayMap.Create(kDataType_String, false, callingScript->GetModIndex());
		if (!arr) return nullptr;
		for (UInt32 i = 0; i < size; i++)
			arr->SetElementFromAPI(keys[i], &values[i]);
		return (NVSEArrayVarInterface::Array*)arr->m_ID;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateMap(const double* keys, const NVSEArrayVarInterface::Element* values,
	                                                  UInt32 size, Script* callingScript)
	{
		ArrayVar* arr = g_ArrayMap.Create(kDataType_Numeric, false, callingScript->GetModIndex());
		if (!arr) return nullptr;
		for (UInt32 i = 0; i < size; i++)
			arr->SetElementFromAPI(keys[i], &values[i]);
		return (NVSEArrayVarInterface::Array*)arr->m_ID;
	}

	bool ArrayAPI::AssignArrayCommandResult(NVSEArrayVarInterface::Array* arr, double* dest)
	{
		if (!g_ArrayMap.Get((ArrayID)arr))
		{
			_MESSAGE("Error: A plugin is attempting to return a non-existent array.");
			return false;
		}
		*dest = (int)arr;
		return true;
	}

	void ArrayAPI::SetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key,
	                          const NVSEArrayVarInterface::Element& value)
	{
		ArrayVar* arrVar = g_ArrayMap.Get((ArrayID)arr);
		if (!arrVar) return;
		switch (key.type)
		{
		case NVSEArrayVarInterface::Element::kType_Numeric:
			arrVar->SetElementFromAPI(key.num, &value);
			break;
		case NVSEArrayVarInterface::Element::kType_String:
			arrVar->SetElementFromAPI(key.str, &value);
			break;
		}
	}

	void ArrayAPI::AppendElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& value)
	{
		ArrayVar* arrVar = g_ArrayMap.Get((ArrayID)arr);
		if (arrVar && (arrVar->KeyType() == kDataType_Numeric) && arrVar->IsPacked())
			arrVar->SetElementFromAPI((int)arrVar->m_elements.getArrayPtr()->Size(), &value);
	}

	UInt32 ArrayAPI::GetArraySize(NVSEArrayVarInterface::Array* arr)
	{
		ArrayVar* arrVar = g_ArrayMap.Get((ArrayID)arr);
		return arrVar ? arrVar->Size() : 0;
	}

	UInt32 ArrayAPI::GetArrayPacked(NVSEArrayVarInterface::Array* arr)
	{
		ArrayVar* arrVar = g_ArrayMap.Get((ArrayID)arr);
		return arrVar ? arrVar->m_bPacked : 0;
	}

	int ArrayAPI::GetContainerType(NVSEArrayVarInterface::Array* arr)
	{
		ArrayVar* arrVar = g_ArrayMap.Get((ArrayID)arr);
		return arrVar ? arrVar->GetContainerType() : -1;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::LookupArrayByID(UInt32 id)
	{
		ArrayVar* arrVar = g_ArrayMap.Get(id);
		return arrVar ? (NVSEArrayVarInterface::Array*)id : nullptr;
	}

	bool ArrayAPI::GetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key,
	                          NVSEArrayVarInterface::Element& out)
	{
		ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		if (var)
		{
			ArrayElement* data = nullptr;
			switch (key.type)
			{
			case key.kType_String:
				if (var->KeyType() == kDataType_String)
					data = var->Get(key.str, false);
				break;
			case key.kType_Numeric:
				if (var->KeyType() == kDataType_Numeric)
					data = var->Get(key.num, false);
				break;
			}

			if (data)
				return InternalElemToPluginElem(data, &out);
		}

		return false;
	}

	bool ArrayAPI::GetElements(NVSEArrayVarInterface::Array* arr, NVSEArrayVarInterface::Element* elements,
	                           NVSEArrayVarInterface::Element* keys)
	{
		if (!elements)
			return false;

		ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		if (var)
		{
			UInt8 keyType = var->KeyType();
			UInt32 i = 0;
			for (ArrayIterator iter = var->m_elements.begin(); !iter.End(); ++iter)
			{
				if (keys)
				{
					switch (keyType)
					{
					case kDataType_Numeric:
						keys[i] = iter.first()->key.num;
						break;
					case kDataType_String:
						keys[i] = iter.first()->key.GetStr();
						break;
					}
				}
				InternalElemToPluginElem(iter.second(), &elements[i]);
				i++;
			}
			return true;
		}
		return false;
	}

	bool ArrayAPI::ArrayHasKey(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key)
	{
		ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		if (var)
		{
			if (key.type == key.kType_String)
				return var->HasKey(key.str);
			if (key.type == key.kType_Numeric)
				return var->HasKey(key.num);
		}
		return false;
	}

	// helpers
	bool ArrayAPI::InternalElemToPluginElem(const ArrayElement* src, NVSEArrayVarInterface::Element* out)
	{
		switch (src->DataType())
		{
		case kDataType_Numeric:
			*out = src->m_data.num;
			break;
		case kDataType_Form:
			*out = LookupFormByID(src->m_data.formID);
			break;
		case kDataType_Array:
			{
				ArrayID arrID = 0;
				src->GetAsArray(&arrID);
				*out = (NVSEArrayVarInterface::Array*)arrID;
				break;
			}
		case kDataType_String:
			*out = src->m_data.GetStr();
			break;
		}
		return true;
	}
}
