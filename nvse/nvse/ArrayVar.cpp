#include "ScriptUtils.h"
#include "ArrayVar.h"
#include "GameForms.h"
#include <algorithm>

#if RUNTIME
#include "GameAPI.h"
#include "GameData.h"
#include "FunctionScripts.h"

#endif

ArrayVarMap g_ArrayMap;

ArrayData::~ArrayData()
{
	if ((dataType == kDataType_String) && str)
		free(str);
	dataType = kDataType_Invalid;
}

const char *ArrayData::GetStr() const
{
	return str ? str : "";
}

void ArrayData::SetStr(const char *srcStr)
{
	str = (srcStr && *srcStr) ? CopyString(srcStr) : NULL;
}

ArrayData& ArrayData::operator=(const ArrayData& rhs)
{
	if (this != &rhs)
	{
		if ((dataType == kDataType_String) && str)
			free(str);
		dataType = rhs.dataType;
		if (dataType == kDataType_String)
			SetStr(rhs.str);
		else num = rhs.num;
	}
	return *this;
}

//////////////////
// ArrayElement
/////////////////

ArrayElement::ArrayElement()
{
	m_data.dataType = kDataType_Invalid;
	m_data.owningArray = 0;
}

ArrayElement::ArrayElement(const ArrayElement& from)
{
	m_data.dataType = from.m_data.dataType;
	m_data.owningArray = from.m_data.owningArray;
	if (m_data.dataType == kDataType_String)
		m_data.SetStr(from.m_data.str);
	else m_data.num = from.m_data.num;
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
		case kDataType_Array:
			return m_data.formID == rhs.m_data.formID;
		case kDataType_String:
			return StrEqualCI(m_data.str, rhs.m_data.str);
		default:
			return m_data.num == rhs.m_data.num;
	}
}

UInt8 ArrayElement::GetOwningModIndex() const
{
	ArrayVar *arr = g_ArrayMap.Get(m_data.owningArray);
	return arr ? arr->m_owningModIndex : 0;
}

std::string ArrayElement::ToString() const
{
	char buf[0x50];

	switch (m_data.dataType)
	{
	case kDataType_Numeric:
		sprintf_s(buf, sizeof(buf), "%f", m_data.num);
		return buf;
	case kDataType_String:
		return m_data.GetStr();
	case kDataType_Array:
		sprintf_s(buf, sizeof(buf), "Array ID %d", m_data.arrID);
		return buf;
	case kDataType_Form:
		{
			UInt32 refID = m_data.formID;
			TESForm* form = LookupFormByID(refID);
			if (form)
			{
				const char* formName = GetFullName(form);
				if (formName)
					return formName;
			}
			sprintf_s(buf, sizeof(buf), "%08x", refID);
			return buf;
		}
	default:
		return "<INVALID>";
	}
}
			
bool ArrayElement::CompareAsString(const ArrayElement& lhs, const ArrayElement& rhs)
{
	// used by std::sort to sort elements by string representation
	// for forms, this is the name, or the actorvaluestring representation of formID if no name

	std::string lhStr = lhs.ToString();
	std::string rhStr = rhs.ToString();

	return StrCompare(lhStr.c_str(), rhStr.c_str()) < 0;
}

bool ArrayElement::SetForm(const TESForm* form)
{
	Unset();

	m_data.dataType = kDataType_Form;
	m_data.formID = form ? form->refID : 0;
	return true;
}

bool ArrayElement::SetFormID(UInt32 refID)
{
	Unset();

	m_data.dataType = kDataType_Form;
	m_data.formID = refID;
	return true;
}

bool ArrayElement::SetString(const char* str)
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
		g_ArrayMap.AddReference(&m_data.arrID, arr, GetOwningModIndex());
	else	// this element is not inside any array, so it's just a temporary
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
	if (m_data.arrID && !g_ArrayMap.Get(m_data.arrID))	// it's okay for arrayID to be 0, otherwise check if array exists
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

void ArrayElement::Unset()
{
	if (m_data.dataType == kDataType_String)
	{
		if (m_data.str)
			free(m_data.str);
	}
	else if (m_data.dataType == kDataType_Array)
		g_ArrayMap.RemoveReference(&m_data.arrID, GetOwningModIndex());
	
	m_data.dataType = kDataType_Invalid;
}

///////////////////////
// ArrayKey
//////////////////////

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
		return StrEqualCI(key.str, rhs.key.str);
	default:
		//_MESSAGE("Error: Invalid ArrayKey type %d", rhs.keyType);
		return true;
	}
}

///////////////////////
// ArrayVar
//////////////////////

ArrayVar::ArrayVar(UInt32 _keyType, bool _packed, UInt8 modIndex) : m_ID(0), m_keyType(_keyType), m_bPacked(_packed), m_owningModIndex(modIndex)
{
	if (m_keyType == kDataType_String)
	{
		m_elements.m_type = kContainer_StringMap;
		m_elements.m_container.pStrMap = new ElementStrMap;
	}
	else if (m_bPacked)
	{
		m_elements.m_type = kContainer_Array;
		m_elements.m_container.pArray = new ElementVector;
	}
	else
	{
		m_elements.m_type = kContainer_NumericMap;
		m_elements.m_container.pNumMap = new ElementNumMap;
	}
}

ArrayElement* ArrayVar::Get(const ArrayKey* key, bool bCanCreateNew)
{
	if (m_keyType != key->KeyType())
		return NULL;

	switch (GetContainerType())
	{
		case kContainer_Array:
		{
			auto *pArray = m_elements.getArrayPtr();
			int idx = key->key.num;
			if (idx < 0)
				idx += pArray->size();
			UInt32 intIdx = idx;
			if (intIdx < pArray->size())
				return &(*pArray)[intIdx];
			if (bCanCreateNew)
			{
				pArray->push_back(ArrayElement());
				ArrayElement *newElem = &pArray->back();
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			break;
		}
		case kContainer_NumericMap:
		{
			auto *pMap = m_elements.getNumMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement *newElem = &(*pMap)[key->key.num];
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			auto findKey = pMap->find(key->key.num);
			if (findKey != pMap->end())
				return &findKey->second;
			break;
		}
		case kContainer_StringMap:
		{
			auto *pMap = m_elements.getStrMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement *newElem = &(*pMap)[*(StringKey*)&key->key.str];
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			auto findKey = pMap->find(*(StringKey*)&key->key.str);
			if (findKey != pMap->end())
				return &findKey->second;
			break;
		}
	}
	return NULL;
}

ArrayElement* ArrayVar::Get(double key, bool bCanCreateNew)
{
	if (m_keyType != kDataType_Numeric)
		return NULL;

	switch (GetContainerType())
	{
		case kContainer_Array:
		{
			auto *pArray = m_elements.getArrayPtr();
			int idx = key;
			if (idx < 0)
				idx += pArray->size();
			UInt32 intIdx = idx;
			if (intIdx < pArray->size())
				return &(*pArray)[intIdx];
			if (bCanCreateNew)
			{
				pArray->push_back(ArrayElement());
				ArrayElement *newElem = &pArray->back();
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			break;
		}
		case kContainer_NumericMap:
		{
			auto *pMap = m_elements.getNumMapPtr();
			if (bCanCreateNew)
			{
				ArrayElement *newElem = &(*pMap)[key];
				newElem->m_data.owningArray = m_ID;
				return newElem;
			}
			auto findKey = pMap->find(key);
			if (findKey != pMap->end())
				return &findKey->second;
			break;
		}
	}
	return NULL;
}

ArrayElement* ArrayVar::Get(const char* key, bool bCanCreateNew)
{
	if ((m_keyType != kDataType_String) || (GetContainerType() != kContainer_StringMap))
		return NULL;

	auto *pMap = m_elements.getStrMapPtr();
	if (bCanCreateNew)
	{
		ArrayElement *resElem = &(*pMap)[*(StringKey*)&key];
		resElem->m_data.owningArray = m_ID;
		return resElem;
	}
	auto findKey = pMap->find(*(StringKey*)&key);
	if (findKey != pMap->end())
		return &findKey->second;
	return NULL;
}

bool ArrayVar::HasKey(double key)
{
	return Get(key, false) != NULL;
}

bool ArrayVar::HasKey(const char* key)
{
	return Get(key, false) != NULL;
}

bool ArrayVar::HasKey(const ArrayKey* key)
{
	return Get(key, false) != NULL;
}

bool ArrayVar::SetElementNumber(double key, double num)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetNumber(num);
	return true;
}

bool ArrayVar::SetElementNumber(const char* key, double num)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetNumber(num);
	return true;
}

bool ArrayVar::SetElementString(double key, const char* str)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetString(str);
	return true;
}

bool ArrayVar::SetElementString(const char* key, const char* str)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetString(str);
	return true;
}

bool ArrayVar::SetElementFormID(double key, UInt32 refID)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetFormID(refID);
	return true;
}

bool ArrayVar::SetElementFormID(const char* key, UInt32 refID)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetFormID(refID);
	return true;
}

bool ArrayVar::SetElementArray(double key, ArrayID srcID)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetArray(srcID);
	return true;
}

bool ArrayVar::SetElementArray(const char* key, ArrayID srcID)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->SetArray(srcID);
	return true;
}

bool ArrayVar::SetElement(double key, const ArrayElement* val)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElement(const char* key, const ArrayElement* val)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElement(const ArrayKey* key, const ArrayElement* val)
{
	ArrayElement *elem = (key->KeyType() == kDataType_Numeric) ? Get(key->key.num, true) : Get(key->key.GetStr(), true);
	if (!elem) return false;
	elem->Set(val);
	return true;
}

bool ArrayVar::SetElementFromAPI(double key, const NVSEArrayVarInterface::Element* srcElem)
{
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
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
	ArrayElement *elem = Get(key, true);
	if (!elem) return false;
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
	ArrayElement *elem = Get(key, false);
	return (elem && elem->GetAsNumber(out));
}

bool ArrayVar::GetElementString(const ArrayKey* key, const char** out)
{
	ArrayElement *elem = Get(key, false);
	return (elem && elem->GetAsString(out));
}

bool ArrayVar::GetElementFormID(const ArrayKey* key, UInt32* out)
{
	ArrayElement *elem = Get(key, false);
	return (elem && elem->GetAsFormID(out));
}

bool ArrayVar::GetElementForm(const ArrayKey* key, TESForm** out)
{
	ArrayElement *elem = Get(key, false);
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
	ArrayElement *elem = Get(key, false);
	return (elem && elem->GetAsArray(out));
}

DataType ArrayVar::GetElementType(const ArrayKey* key)
{
	ArrayElement* elem = Get(key, false);
	return elem ? elem->DataType() : kDataType_Invalid;
}

const ArrayKey* ArrayVar::Find(const ArrayElement* toFind, const Slice* range)
{
	ArrayIterator start = m_elements.begin();
	ArrayIterator end = m_elements.end();
	if (range)
	{
		if (range->bIsString != (KeyType() == kDataType_String))
			return NULL;

		// locate lower and upper bounds
		ArrayKey lo;
		ArrayKey hi;
		range->GetArrayBounds(lo, hi);

		if (m_bPacked)
		{
			end = start;
			start += (int)lo.key.num;
			end += (int)hi.key.num;
		}
		else
		{
			ArrayIterator theEnd = m_elements.end();
			while ((start != theEnd) && (*start.first() < lo))
				++start;

			end = start;
			while ((end != theEnd) && (*end.first() <= hi))
				++end;
		}
	}

	// do the search
	while (start != end)
	{
		if (*start.second() == *toFind)
			return start.first();
		++start;
	}

	return NULL;
}

bool ArrayVar::GetFirstElement(ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!Size()) return false;

	ArrayIterator iter = m_elements.begin();
	*outKey = iter.first();
	*outElem = iter.second();
	return true;
}

bool ArrayVar::GetLastElement(ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!Size()) return false;

	ArrayIterator iter = m_elements.end();
	--iter;

	*outKey = iter.first();
	*outElem = iter.second();
	return true;
}

bool ArrayVar::GetNextElement(ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!prevKey || !Size())
		return false;

	ArrayIterator iter = m_elements.find(prevKey);
	if (iter != m_elements.end())
	{
		++iter;
		if (iter != m_elements.end())
		{
			*outKey = iter.first();
			*outElem = iter.second();
			return true;
		}
	}
	return false;
}

bool ArrayVar::GetPrevElement(ArrayKey* prevKey, ArrayElement** outElem, const ArrayKey** outKey)
{
	if (!prevKey || !Size())
		return false;

	ArrayIterator iter = m_elements.find(prevKey);
	if (iter != m_elements.end() && iter != m_elements.begin())
	{
		--iter;
		*outKey = iter.first();
		*outElem = iter.second();
		return true;
	}

	return false;
}

UInt32 ArrayVar::EraseElement(const ArrayKey* key)
{
	if (KeyType() != key->KeyType())
		return -1;
	return m_elements.erase(key);
}

UInt32 ArrayVar::EraseElements(const ArrayKey* lo, const ArrayKey* hi)
{
	if ((lo->KeyType() != hi->KeyType()) || (lo->KeyType() != KeyType()))
		return -1;
	return m_elements.erase(lo, hi);
}

UInt32 ArrayVar::EraseAllElements()
{
	UInt32 numErased = m_elements.size();
	if (numErased) m_elements.clear();
	return numErased;
}

bool ArrayVar::SetSize(UInt32 newSize, const ArrayElement* padWith)
{
	if (!m_bPacked) return false;

	UInt32 varSize = m_elements.getArrayPtr()->size();
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
	{
		ArrayKey lo(newSize), hi(varSize - 1);
		return EraseElements(&lo, &hi) != -1;
	}

	return true;
}

bool ArrayVar::Insert(UInt32 atIndex, const ArrayElement* toInsert)
{
	if (!m_bPacked) return false;

	auto *pVec = m_elements.getArrayPtr();
	size_t varSize = pVec->size();
	if (atIndex > varSize) return false;
	if (atIndex < varSize)
		pVec->insert(pVec->begin() + atIndex, ArrayElement());
	else pVec->push_back(ArrayElement());

	(*pVec)[atIndex].Set(toInsert);

	return true;
}

bool ArrayVar::Insert(UInt32 atIndex, ArrayID rangeID)
{
	ArrayVar* src = g_ArrayMap.Get(rangeID);
	if (!m_bPacked || !src || !src->m_bPacked)
		return false;

	auto *pDest = m_elements.getArrayPtr(), *pSrc = src->m_elements.getArrayPtr();
	size_t destSize = pDest->size();
	if (atIndex > destSize)
		return false;

	size_t srcSize = pSrc->size();
	if (!srcSize) return true;

	pDest->insert(pDest->begin() + atIndex, srcSize, ArrayElement());

	ArrayElement *pDestData = pDest->data() + atIndex, *pSrcData = pSrc->data();

	for (size_t idx = 0; idx < srcSize; idx++)
		pDestData[idx].Set(&pSrcData[idx]);

	return true;
}

ArrayVar *ArrayVar::GetKeys(UInt8 modIndex)
{
	ArrayVar *keysArr = g_ArrayMap.Create(kDataType_Numeric, true, modIndex);
	double currKey = 0;

	for (ArrayIterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
	{
		if (m_keyType == kDataType_Numeric)
			keysArr->SetElementNumber(currKey, iter.first()->key.num);
		else
			keysArr->SetElementString(currKey, iter.first()->key.GetStr());
		currKey += 1;
	}

	return keysArr;
}

ArrayVar *ArrayVar::Copy(UInt8 modIndex, bool bDeepCopy)
{
	ArrayVar *copyArr = g_ArrayMap.Create(m_keyType, m_bPacked, modIndex);
	const ArrayKey *arrKey;
	const ArrayElement *arrElem;
	for (ArrayIterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
	{
		arrKey = iter.first();
		arrElem = iter.second();
		if ((arrElem->DataType() == kDataType_Array) && bDeepCopy)
		{
			ArrayVar *innerArr = g_ArrayMap.Get(arrElem->m_data.arrID);
			if (innerArr)
			{
				ArrayVar *innerCopy = innerArr->Copy(modIndex, true);
				if (arrKey->KeyType() == kDataType_Numeric)
				{
					if (copyArr->SetElementArray(arrKey->key.num, innerCopy->ID()))
						continue;
				}
				else if (copyArr->SetElementArray(arrKey->key.GetStr(), innerCopy->ID()))
					continue;
			}
			DEBUG_PRINT("ArrayVarMap::Copy failed to make deep copy of inner array");
		}
		else if (!copyArr->SetElement(arrKey, arrElem))
			DEBUG_PRINT("ArrayVarMap::Copy failed to set element in copied array");
	}

	return copyArr;
}

ArrayVar *ArrayVar::MakeSlice(const Slice* slice, UInt8 modIndex)
{
	// keys in slice match those in source unless array packed, in which case they must start at zero

	ArrayKey lo;
	ArrayKey hi;

	if (slice->bIsString && (m_keyType == kDataType_String))
	{
		lo = ArrayKey(slice->m_lowerStr.c_str());
		hi = ArrayKey(slice->m_upperStr.c_str());

	}
	else if (!slice->bIsString && (m_keyType == kDataType_Numeric))
	{
		lo = ArrayKey(slice->m_lower);
		hi = ArrayKey(slice->m_upper);
	}
	else return 0;

	ArrayVar *newVar = g_ArrayMap.Create(m_keyType, m_bPacked, modIndex);
	ArrayIterator start = m_elements.begin();

	for (; start != m_elements.end(); ++start)
	{
		if (*start.first() >= lo)
			break;
	}

	double packedIndex = 0;

	for (ArrayIterator end = start; end != m_elements.end(); ++end)
	{
		if (*end.first() > hi)
			break;

		if (m_bPacked)
		{
			newVar->SetElement(packedIndex, end.second());
			packedIndex += 1;
		}
		else newVar->SetElement(end.first(), end.second());
	}

	return newVar;
}

class SortFunctionCaller : public FunctionCaller
{
	Script			*m_comparator;
	ArrayVar		*m_lhs;
	ArrayVar		*m_rhs;

public:
	SortFunctionCaller(Script* comparator) : m_comparator(comparator), m_lhs(NULL), m_rhs(NULL)
	{ 
		if (comparator)
		{
			m_lhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
			m_rhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
		}
	}

	virtual ~SortFunctionCaller() { }

	virtual UInt8 ReadCallerVersion() { return UserFunctionManager::kVersion; }
	virtual Script * ReadScript() { return m_comparator; }

	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info)
	{
		DynamicParamInfo& dParams = info->ParamInfo();
		if (dParams.NumParams() == 2)
		{
			UserFunctionParam* lhs = info->GetParam(0);
			UserFunctionParam* rhs = info->GetParam(1);
			if (lhs && rhs && lhs->varType == Script::eVarType_Array && rhs->varType == Script::eVarType_Array)
			{
				ScriptEventList::Var* var = eventList->GetVariable(lhs->varIdx);
				if (!var)
				{
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				else {
					g_ArrayMap.AddReference(&var->data, m_lhs->ID(), m_comparator->GetModIndex());
				}

				var = eventList->GetVariable(rhs->varIdx);
				if (!var) {
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				else {
					g_ArrayMap.AddReference(&var->data, m_rhs->ID(), m_comparator->GetModIndex());
				}

				return true;
			}
		}

		return false;
	}

	virtual TESObjectREFR* ThisObj() { return NULL; }
	virtual TESObjectREFR* ContainingObj() { return NULL; }

	bool operator()(const ArrayElement& lhs, const ArrayElement& rhs)
	{
		m_lhs->SetElement(0.0, &lhs);
		m_rhs->SetElement(0.0, &rhs);
		ScriptToken* result = UserFunctionManager::Call(*this);
		bool bResult = result ? result->GetBool() : false;
		delete result;
		return bResult;
	}
};

void ArrayVar::Sort(ArrayVar *result, SortOrder order, SortType type, Script* comparator)
{
	// restriction: all elements of src must be of the same type for default sort
	// restriction not in effect for alpha sort (all values treated as strings) or custom sort (all values boxed as arrays)
	ElementVector vec;
	ArrayIterator iter = m_elements.begin();
	UInt32 dataType = iter.second()->DataType();
	if (dataType == kDataType_Invalid || dataType == kDataType_Array)	// nonsensical to sort array of arrays
		return;

	// copy elems to vec, verify all are of same type
	for (; iter != m_elements.end(); ++iter)
	{
		if (type == kSortType_Default && iter.second()->DataType() != dataType)
			return;
		vec.push_back(*iter.second());
	}

	// let STL do the sort
	if (type == kSortType_Default)
		std::sort(vec.begin(), vec.end());
	else if (type == kSortType_Alpha)
		std::sort(vec.begin(), vec.end(), ArrayElement::CompareAsString);
	else if (type == kSortType_UserFunction)
	{
		if (!comparator) return;
		SortFunctionCaller sorter(comparator);
		std::sort(vec.begin(), vec.end(), sorter);
	}

	if (order == kSort_Descending)
		std::reverse(vec.begin(), vec.end());

	double elemIdx = 0;
	for (UInt32 i = 0; i < vec.size(); i++)
	{
		result->SetElement(elemIdx, &vec[i]);
		elemIdx += 1;
	}
}

void ArrayVar::Dump()
{
	const char* owningModName = DataHandler::Get()->GetNthModName(m_owningModIndex);

	Console_Print("** Dumping Array #%d **\nRefs: %d Owner %02X: %s", m_ID, m_refs.size(), m_owningModIndex, owningModName);
	_MESSAGE("** Dumping Array #%d **\nRefs: %d Owner %02X: %s", m_ID, m_refs.size(), m_owningModIndex, owningModName);

	for (ArrayIterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
	{
		char numBuf[0x50];
		std::string elementInfo("[ ");

		switch (KeyType())
		{
		case kDataType_Numeric:
			sprintf_s(numBuf, sizeof(numBuf), "%f", iter.first()->key.num);
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
			sprintf_s(numBuf, sizeof(numBuf), "%f", iter.second()->m_data.num);
			elementInfo += numBuf;
			break;
		case kDataType_String:
			elementInfo += iter.second()->m_data.GetStr();
			break;
		case kDataType_Array:
			elementInfo += "(Array ID #";
			sprintf_s(numBuf, sizeof(numBuf), "%d", iter.second()->m_data.arrID);
			elementInfo += numBuf;
			elementInfo += ")";
			break;
		case kDataType_Form:
			{
				UInt32 refID = iter.second()->m_data.formID;
				sprintf_s(numBuf, sizeof(numBuf), "%08X", refID);
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

		Console_Print("%s", elementInfo.c_str());
		_MESSAGE("%s", elementInfo.c_str());
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

//////////////////////////
// ArrayVarMap
/////////////////////////

ArrayVarMap* ArrayVarMap::GetSingleton()
{
	return &g_ArrayMap;
}

void ArrayVarMap::Add(ArrayVar* var, UInt32 varID, UInt32 numRefs, UInt8* refs)
{
	VarMap::Insert(varID, var);
	var->m_ID = varID;
	if (!numRefs)		// nobody refers to this array, queue for deletion
		MarkTemporary(varID, true);
	else				// record references to this array
	{
		var->m_refs.resize(numRefs);
		MemCopy(var->m_refs.data(), refs, numRefs);
	}
}

ArrayVar* ArrayVarMap::Create(UInt32 keyType, bool bPacked, UInt8 modIndex)
{
	ArrayVar* newVar = new ArrayVar(keyType, bPacked, modIndex);
	ArrayID varID = GetUnusedID();
	newVar->m_ID = varID;
	VarMap::Insert(varID, newVar);
	MarkTemporary(varID, true);		// queue for deletion until a reference to this array is made
	return newVar;
}

void ArrayVarMap::AddReference(ArrayID* ref, ArrayID toRef, UInt8 referringModIndex)
{
	if (*ref == toRef)
		return;			// already refers to this array

	if (*ref)			// refers to a different array, remove that reference
		RemoveReference(ref, referringModIndex);

	ArrayVar* arr = Get(toRef);
	if (arr)
	{
		arr->m_refs.push_back(referringModIndex);	// record reference, increment refcount
		*ref = toRef;								// store ref'ed ArrayID in reference
		MarkTemporary(toRef, false);
	}
}
		
void ArrayVarMap::RemoveReference(ArrayID* ref, UInt8 referringModIndex)
{
	ArrayVar* var = Get(*ref);
	if (var)
	{
		// decrement refcount
		for (auto iter = var->m_refs.begin(); iter != var->m_refs.end(); ++iter)
		{
			if (*iter == referringModIndex)
			{
				var->m_refs.erase(iter);
				break;
			}
		}

		// if refcount is zero, queue for deletion
		if (var->m_refs.empty())
			MarkTemporary(var->ID(), true);
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

ArrayElement *ArrayVarMap::GetElement(ArrayID id, const ArrayKey* key)
{
	ArrayVar *arr = Get(id);
	return arr ? arr->Get(key, false) : NULL;
}

void ArrayVarMap::Save(NVSESerializationInterface* intfc)
{
	Clean();

	intfc->OpenRecord('ARVS', kVersion);

	if (m_state)
	{
		_VarMap& vars = m_state->vars;
		ArrayVar *pVar;
		UInt32 numRefs;
		UInt8 keyType;
		const ArrayKey *pKey;
		const ArrayElement *pElem;
		char *str;
		UInt16 len;
		for (_VarMap::iterator iter = vars.begin(); iter != vars.end(); ++iter)
		{
			if (IsTemporary(iter->first))
				continue;

			pVar = iter->second;
			numRefs = pVar->m_refs.size();
			if (!numRefs) continue;
			keyType = pVar->m_keyType;

			intfc->OpenRecord('ARVR', kVersion);
			intfc->WriteRecordData(&pVar->m_owningModIndex, sizeof(UInt8));
			intfc->WriteRecordData(&iter->first, sizeof(UInt32));
			intfc->WriteRecordData(&keyType, sizeof(UInt8));
			intfc->WriteRecordData(&pVar->m_bPacked, sizeof(bool));
			intfc->WriteRecordData(&numRefs, sizeof(numRefs));
			intfc->WriteRecordData(pVar->m_refs.data(), numRefs);

			numRefs = pVar->Size();
			intfc->WriteRecordData(&numRefs, sizeof(UInt32));
			if (!numRefs) continue;

			for (ArrayIterator elems = pVar->m_elements.begin(); elems != pVar->m_elements.end(); ++elems)
			{
				pKey = elems.first();
				pElem = elems.second();

				if (keyType == kDataType_String)
				{
					str = pKey->key.str;
					len = StrLen(str);
					intfc->WriteRecordData(&len, sizeof(len));
					if (len) intfc->WriteRecordData(str, len);
				}
				else if (!pVar->m_bPacked)
					intfc->WriteRecordData(&pKey->key.num, sizeof(double));
				
				intfc->WriteRecordData(&pElem->m_data.dataType, sizeof(UInt8));
				switch (pElem->m_data.dataType)
				{
					case kDataType_Numeric:
						intfc->WriteRecordData(&pElem->m_data.num, sizeof(double));
						break;
					case kDataType_String:
					{
						str = pElem->m_data.str;
						len = StrLen(str);
						intfc->WriteRecordData(&len, sizeof(len));
						if (len) intfc->WriteRecordData(str, len);
						break;
					}
					case kDataType_Array:
						intfc->WriteRecordData(&pElem->m_data.arrID, sizeof(ArrayID));
						break;
					case kDataType_Form:
						intfc->WriteRecordData(&pElem->m_data.formID, sizeof(UInt32));
						break;
					default:
						_MESSAGE("Error in ArrayVarMap::Save() - unhandled element type %d. Element not saved.", pElem->m_data.dataType);
				}
			}
		}
	}
	intfc->OpenRecord('ARVE', kVersion);
}

void ArrayVarMap::Load(NVSESerializationInterface* intfc)
{
	_MESSAGE("Loading array variables");

	Clean();		// clean up any vars queued for garbage collection

	UInt32 type, length, version, arrayID, tempRefID, numElements;
	UInt16 strLength;
	UInt8 modIndex, keyType;
	bool bPacked;
	ContainerType contType;
	UInt8 buffer[kMaxMessageLength];

	//Reset(intfc);
	bool bContinue = true;
	UInt32 lastIndexRead = 0;

	UInt8 *bufferPtr = buffer;
	double numKey;

	while (bContinue && intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
			case 'ARVE':			//end of block
				bContinue = false;
				break;
			case 'ARVR':
			{
				intfc->ReadRecordData(&modIndex, sizeof(modIndex));
				if (!intfc->ResolveRefID(modIndex << 24, &tempRefID))
				{
					// owning mod was removed, but there may be references to it from other mods
					// assign ownership to the first mod which refers to it and is still loaded
					// if no loaded mods refer to it, discard
					// we handle all of that below
					_MESSAGE("Mod owning array was removed from load order; will attempt to assign ownership to a referring mod.");
					modIndex = 0;
				}
				else
					modIndex = (tempRefID >> 24);

				intfc->ReadRecordData(&arrayID, sizeof(arrayID));
				intfc->ReadRecordData(&keyType, sizeof(keyType));
				intfc->ReadRecordData(&bPacked, sizeof(bPacked));

				// read refs, fix up mod indexes, discard refs from unloaded mods
				UInt32 numRefs = 0;		// # of references to this array

				// reference-counting implemented in v1
				if (version >= 1)
				{
					intfc->ReadRecordData(&numRefs, sizeof(numRefs));
					if (numRefs)
					{
						UInt32 tempRefID = 0;
						UInt8 curModIndex = 0;
						UInt32 refIdx = 0;
						for (UInt32 i = 0; i < numRefs; i++)
						{
							intfc->ReadRecordData(&curModIndex, sizeof(curModIndex));
							if (!modIndex)
							{
								if (intfc->ResolveRefID(curModIndex << 24, &tempRefID))
								{
									modIndex = tempRefID >> 24;
									_MESSAGE("ArrayID %d was owned by an unloaded mod. Assigning ownership to mod #%d", arrayID, modIndex);
								}
							}

							if (intfc->ResolveRefID(curModIndex << 24, &tempRefID))
								buffer[refIdx++] = (tempRefID >> 24);
						}

						numRefs = refIdx;
					}
				}
				else		// v0 arrays assumed to have only one reference (the owning mod)
				{
					if (modIndex)		// owning mod is loaded
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
				ArrayVar* newArr = new ArrayVar(keyType, bPacked, modIndex);
				Add(newArr, arrayID, numRefs, buffer);

				// read the array elements			
				intfc->ReadRecordData(&numElements, sizeof(numElements));
				if (!numElements) continue;

				contType = newArr->GetContainerType();

				ArrayElement *elements, *elem;
				ElementNumMap *pNumMap;
				ElementStrMap *pStrMap;
				switch (contType)
				{
					case kContainer_Array:
					{
						auto *pArray = newArr->m_elements.getArrayPtr();
						pArray->resize(numElements);
						elements = pArray->data();
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
						intfc->ReadRecordData(&strLength, sizeof(strLength));
						if (strLength) intfc->ReadRecordData(buffer, strLength);
						buffer[strLength] = 0;
					}
					else if (!bPacked || (version < 2))
						intfc->ReadRecordData(&numKey, sizeof(double));
					
					UInt8 elemType;
					if (intfc->ReadRecordData(&elemType, sizeof(elemType)) == 0)
					{
						_MESSAGE("ArrayVarMap::Load() reading past end of file");
						return;
					}

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
							elem = &(*pStrMap)[*(StringKey*)&bufferPtr];
							break;
					}

					elem->m_data.dataType = (DataType)elemType;
					elem->m_data.owningArray = arrayID;

					switch (elemType)
					{
						case kDataType_Numeric:
							intfc->ReadRecordData(&elem->m_data.num, sizeof(double));
							break;
						case kDataType_String:
						{
							intfc->ReadRecordData(&strLength, sizeof(strLength));
							if (strLength)
							{
								char *strVal = (char*)malloc(strLength + 1);
								intfc->ReadRecordData(strVal, strLength);
								strVal[strLength] = 0;
								elem->m_data.str = strVal;
							}
							else elem->m_data.str = NULL;
							break;
						}
						case kDataType_Array:
							intfc->ReadRecordData(&elem->m_data.arrID, sizeof(ArrayID));
							break;
						case kDataType_Form:
						{
							UInt32 formID;
							intfc->ReadRecordData(&formID, sizeof(formID));
							if (!intfc->ResolveRefID(formID, &formID))
								formID = 0;
							elem->m_data.formID = formID;
							break;
						}
						default:
							_MESSAGE("Unknown element type %d encountered while loading array var, element discarded.", elemType);
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

void ArrayVarMap::Clean()		// garbage collection: delete unreferenced arrays
{
	// ArrayVar destructor may queue more IDs for deletion if deleted array contains other arrays
	// so on each pass through the loop we delete the first ID in the queue until none remain

	if (!m_state) return;

	while (m_state->tempVars.size())
		Delete(*(m_state->tempVars.begin()));
}

namespace PluginAPI
{
	NVSEArrayVarInterface::Array* ArrayAPI::CreateArray(const NVSEArrayVarInterface::Element* data, UInt32 size, Script* callingScript)
	{
		ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, callingScript->GetModIndex());
		if (!arr) return NULL;
		double elemIdx = 0;
		for (UInt32 i = 0; i < size; i++)
		{
			arr->SetElementFromAPI(elemIdx, &data[i]);
			elemIdx += 1;
		}
		return (NVSEArrayVarInterface::Array*)arr->m_ID;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateStringMap(const char** keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript)
	{
		ArrayVar *arr = g_ArrayMap.Create(kDataType_String, false, callingScript->GetModIndex());
		if (!arr) return NULL;
		for (UInt32 i = 0; i < size; i++)
			arr->SetElementFromAPI(keys[i], &values[i]);
		return (NVSEArrayVarInterface::Array*)arr->m_ID;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateMap(const double* keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript)
	{
		ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, false, callingScript->GetModIndex());
		if (!arr) return NULL;
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

	void ArrayAPI::SetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key, const NVSEArrayVarInterface::Element& value)
	{
		ArrayVar *arrVar = g_ArrayMap.Get((ArrayID)arr);
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
		ArrayVar *arrVar = g_ArrayMap.Get((ArrayID)arr);
		if (arrVar && (arrVar->KeyType() == kDataType_Numeric) && arrVar->IsPacked())
			arrVar->SetElementFromAPI((int)arrVar->m_elements.getArrayPtr()->size(), &value);
	}

	UInt32 ArrayAPI::GetArraySize(NVSEArrayVarInterface::Array* arr)
	{
		ArrayVar *arrVar = g_ArrayMap.Get((ArrayID)arr);
		return arrVar ? arrVar->Size() : 0;
	}

	UInt32 ArrayAPI::GetArrayPacked(NVSEArrayVarInterface::Array* arr)
	{
		ArrayVar *arrVar = g_ArrayMap.Get((ArrayID)arr);
		return arrVar ? arrVar->m_bPacked : 0;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::LookupArrayByID(UInt32 id)
	{
		ArrayVar *arrVar = g_ArrayMap.Get(id);
		return arrVar ? (NVSEArrayVarInterface::Array*)id : 0;
	}

	bool ArrayAPI::GetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key, NVSEArrayVarInterface::Element& out)
	{
		 ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		 if (var)
		 {
			 ArrayElement* data = NULL;
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
		
	bool ArrayAPI::GetElements(NVSEArrayVarInterface::Array* arr, NVSEArrayVarInterface::Element* elements, NVSEArrayVarInterface::Element* keys)
	{
		if (!elements)
			return false;

		ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		if (var)
		{
			UInt8 keyType = var->KeyType();
			UInt32 i = 0;
			for (ArrayIterator iter = var->m_elements.begin(); iter != var->m_elements.end(); ++iter)
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
