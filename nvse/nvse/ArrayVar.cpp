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

//////////////////
// ArrayElement
/////////////////

ArrayElement::ArrayElement()
	: m_dataType(kDataType_Invalid), m_owningArray(0)
{	
	m_data.formID = 0;
	m_data.str = "";
}

bool ArrayElement::operator<(const ArrayElement& rhs) const
{
	// if we ever try to compare 2 elems of differing types (i.e. string and number) we violate strict weak
	// no reason to do that

	if (DataType() != rhs.DataType())
	{
		_MESSAGE("ArrayElement::operator< called with non-matching element types.");
		return false;
	}

	if (DataType() == kDataType_String)
		return (_stricmp(m_data.str.c_str(), rhs.m_data.str.c_str()) < 0);
	else if (DataType() == kDataType_Form)
		return m_data.formID < rhs.m_data.formID;
	else
		return m_data.num < rhs.m_data.num;
}

bool ArrayElement::Equals(const ArrayElement& compareTo) const
{
	if (DataType() != compareTo.DataType())
		return false;

	switch (DataType())
	{
	case kDataType_String:
		return (m_data.str.length() == compareTo.m_data.str.length()) ? !_stricmp(m_data.str.c_str(), compareTo.m_data.str.c_str()) : false;
	case kDataType_Form:
		return m_data.formID == compareTo.m_data.formID;
	default:
		return m_data.num == compareTo.m_data.num;
	}
}

std::string ArrayElement::ToString() const
{
	char buf[0x50];

	switch (m_dataType)
	{
	case kDataType_Numeric:
		sprintf_s(buf, sizeof(buf), "%f", m_data.num);
		return buf;
	case kDataType_String:
		return m_data.str;
	case kDataType_Array:
		sprintf_s(buf, sizeof(buf), "Array ID %.0f", m_data.num);
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

	return _stricmp(lhStr.c_str(), rhStr.c_str()) < 0;
}



bool ArrayElement::SetForm(const TESForm* form)
{
	Unset();

	m_dataType = kDataType_Form;
	m_data.formID = form ? form->refID : 0;
	return true;
}

bool ArrayElement::SetFormID(UInt32 refID)
{
	Unset();

	m_dataType = kDataType_Form;
	m_data.formID = refID;
	return true;
}

bool ArrayElement::SetString(const std::string& str)
{
	Unset();

	m_dataType = kDataType_String;
	m_data.str = str;
	return true;
}

bool ArrayElement::SetArray(ArrayID arr, UInt8 modIndex)
{
	Unset();

	m_dataType = kDataType_Array;
	if (m_owningArray) {
		g_ArrayMap.AddReference(&m_data.num, arr, modIndex);
	}
	else {	// this element is not inside any array, so it's just a temporary
		m_data.num = arr;
	}

	return true;
}

bool ArrayElement::SetNumber(double num)
{
	Unset();

	m_dataType = kDataType_Numeric;
	m_data.num = num;
	return true;
}

bool ArrayElement::Set(const ArrayElement& elem)
{
	Unset();

	m_dataType = elem.DataType();
	switch (m_dataType)
	{
	case kDataType_String:
		SetString(elem.m_data.str);
		break;
	case kDataType_Array:
		SetArray(elem.m_data.num, g_ArrayMap.GetOwningModIndex(m_owningArray));
		break;
	case kDataType_Numeric:
		SetNumber(elem.m_data.num);
		break;
	case kDataType_Form:
		SetFormID(elem.m_data.formID);
		break;
	default:
		m_dataType = kTokenType_Invalid;
		return false;
	}

	return true;
}

bool ArrayElement::GetAsArray(ArrayID* out) const
{
	if (!out || m_dataType != kDataType_Array)
		return false;
	else if (m_data.num != 0 && !g_ArrayMap.Exists(m_data.num))	// it's okay for arrayID to be 0, otherwise check if array exists
		return false;

	*out = m_data.num;
	return true;
}

bool ArrayElement::GetAsFormID(UInt32* out) const
{
	if (!out || m_dataType != kDataType_Form)
		return false;
	*out = m_data.formID;
	return true;
}

bool ArrayElement::GetAsNumber(double* out) const
{
	if (!out || m_dataType != kDataType_Numeric)
		return false;
	*out = m_data.num;
	return true;
}

bool ArrayElement::GetAsString(std::string& out) const
{
	if (m_dataType != kDataType_String)
		return false;
	out = m_data.str;
	return true;
}

void ArrayElement::Unset()
{
	if (m_dataType == kDataType_Array)
		g_ArrayMap.RemoveReference(&m_data.num, g_ArrayMap.GetOwningModIndex(m_owningArray));
	
	m_dataType = kDataType_Invalid;
	m_data.num = 0;
}

///////////////////////
// ArrayKey
//////////////////////

ArrayKey::ArrayKey() : keyType(kDataType_Invalid)
{
	key.num = 0;
}

ArrayKey::ArrayKey(const std::string& _key)
{
	keyType = kDataType_String;
	key.str = _key;
}

ArrayKey::ArrayKey(const char* _key)
{
	keyType = kDataType_String;
	key.str = _key;
}

ArrayKey::ArrayKey(double _key)
{
	keyType = kDataType_Numeric;
	key.num = _key;
}

bool ArrayKey::operator<(const ArrayKey& rhs) const
{
	if (keyType != rhs.keyType)
	{
		_MESSAGE("Error: ArrayKey operator< mismatched keytypes");
		return true;
	}

	switch (keyType)
	{
	case kDataType_Numeric:
		return key.num < rhs.key.num;
	case kDataType_String:
		return _stricmp(key.str.c_str(), rhs.key.str.c_str()) < 0;
	default:
		_MESSAGE("Error: Invalid ArrayKey type %d", rhs.keyType);
		return true;
	}
}

bool ArrayKey::operator==(const ArrayKey& rhs) const
{
	if (keyType != rhs.keyType)
	{
		_MESSAGE("Error: ArrayKey operator== mismatched keytypes");
		return true;
	}

	switch (keyType)
	{
	case kDataType_Numeric:
		return key.num == rhs.key.num;
	case kDataType_String:
		return (key.str.length() == rhs.key.str.length()) ? !(_stricmp(key.str.c_str(), rhs.key.str.c_str())) : false;
	default:
		_MESSAGE("Error: Invalid ArrayKey type %d", rhs.keyType);
		return true;
	}
}

///////////////////////
// ArrayVar
//////////////////////


ArrayVar::ArrayVar(UInt8 modIndex)
	: m_ID(0), m_keyType(kDataType_Invalid), m_bPacked(false), m_owningModIndex(modIndex)
{
	//
}

ArrayVar::ArrayVar(UInt32 _keyType, bool _packed, UInt8 modIndex)
	: m_ID(0), m_keyType(_keyType), m_bPacked(_packed), m_owningModIndex(modIndex)
{
	//
}

ArrayVar::~ArrayVar()
{
	// erase all elements. Important because doing so decrements refCounts of arrays stored within this array
	for (ArrayIterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
	{
		ArrayKey key = iter->first;
		ArrayElement* elem = Get(key, false);
		if (elem)
			elem->Unset();
	}
}

ArrayElement* ArrayVar::Get(ArrayKey key, bool bCanCreateNew)
{
	if (IsPacked() && key.KeyType() == kDataType_Numeric)
	{
		double idx = key.Key().num;
		if (idx < 0)
			idx += Size();
		UInt32 intIdx = idx;
		key.SetNumericKey(intIdx);
	}

	_ElementMap::iterator it = m_elements.find(key);
	if (it != m_elements.end()) {
		return &it->second;
	}

	if (bCanCreateNew)
	{
		if (key.KeyType() == KeyType())
		{
			if (!IsPacked() || (key.Key().num <= Size()))
			{
				// create a new, uninitialized element
				ArrayElement* newElem = &m_elements[key];
				newElem->m_owningArray = m_ID;
				return newElem;
			}
		}
	}

	return NULL;
}

UInt32 ArrayVar::GetUnusedIndex()
{
	UInt32 id = 0;
	while (m_elements.find(id) != m_elements.end())
	{
		id++;
	}

	return id;
}

void ArrayVar::Dump()
{
	const char* owningModName = DataHandler::Get()->GetNthModName(m_owningModIndex);

	Console_Print("Refs: %d Owner %02X: %s", m_refs.size(), m_owningModIndex, owningModName);
	_MESSAGE("Refs: %d Owner %02X: %s", m_refs.size(), m_owningModIndex, owningModName);

	for (std::map<ArrayKey, ArrayElement>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
	{
		char numBuf[0x50] = { 0 };
		std::string elementInfo("[ ");

		switch (KeyType())
		{
		case kDataType_Numeric:
			sprintf_s(numBuf, sizeof(numBuf), "%f", iter->first.Key().num);
			elementInfo += numBuf;
			break;
		case kDataType_String:
			elementInfo += iter->first.Key().str;
			break;
		default:
			elementInfo += "?Unknown Key Type?";
		}

		elementInfo += " ] : ";

		switch (iter->second.m_dataType)
		{
		case kDataType_Numeric:
			sprintf_s(numBuf, sizeof(numBuf), "%f", iter->second.m_data.num);
			elementInfo += numBuf;
			break;
		case kDataType_String:
			elementInfo += iter->second.m_data.str;
			break;
		case kDataType_Array:
			elementInfo += "(Array ID #";
			sprintf_s(numBuf, sizeof(numBuf), "%.0f", iter->second.m_data.num);
			elementInfo += numBuf;
			elementInfo += ")";
			break;
		case kDataType_Form:
			{
				UInt32 refID = iter->second.m_data.formID;
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

void ArrayVar::Pack()
{
	if (!IsPacked() || !Size())
		return;

	// assume only one hole exists (i.e. we previously erased 0 or more contiguous elements)
	// these are double but will always hold integer values for packed arrays
	double curIdx = 0;		// last correct index

	ArrayIterator iter;
	for (iter = m_elements.begin(); iter != m_elements.end(); )
	{
		if (!(iter->first == curIdx))
		{
			ArrayElement* elem = Get(ArrayKey(curIdx), true);
			elem->Set(iter->second);
			iter->second.Unset();
			ArrayIterator toDelete = iter;
			++iter;
			m_elements.erase(toDelete);
		}
		else
			++iter;

		curIdx += 1;
	}
}

//////////////////////////
// ArrayVarMap
/////////////////////////

ArrayVarMap* ArrayVarMap::GetSingleton()
{
	return &g_ArrayMap;
}

UInt8 ArrayVarMap::GetOwningModIndex(ArrayID id)
{
	ArrayVar* arr = Get(id);
	if (arr)
		return arr->m_owningModIndex;

	return 0;
}

void ArrayVarMap::Erase(ArrayID toErase)
{
	ArrayVar* var = Get(toErase);
	if (var)
	{
		// delete any arrays contained in array
		for (ArrayIterator iter = var->m_elements.begin(); iter != var->m_elements.end(); ++iter)
		{
			iter->second.Unset();
		}
			
		// I think this is redundant: delete var;
	}

	Delete(toErase);
}

void ArrayVarMap::Add(ArrayVar* var, UInt32 varID, UInt32 numRefs, UInt8* refs)
{
#if _DEBUG
	if (Exists(varID))
	{
		DEBUG_PRINT("ArrayVarMap::Add() -> ArrayID %d already exists and will be overwritten.", varID);
	}
#endif

	VarMap::Insert(varID, var);
	var->m_ID = varID;
	if (!numRefs)		// nobody refers to this array, queue for deletion
		MarkTemporary(varID, true);
	else				// record references to this array
	{
		for (UInt32 i = 0; i < numRefs; i++)
			var->m_refs.push_back(refs[i]);
	}
}

ArrayID	ArrayVarMap::Create(UInt32 keyType, bool bPacked, UInt8 modIndex)
{
	ArrayVar* newVar = new ArrayVar(keyType, bPacked, modIndex);
	ArrayID varID = GetUnusedID();
	newVar->m_ID = varID;
	VarMap::Insert(varID, newVar);
	MarkTemporary(varID, true);		// queue for deletion until a reference to this array is made
	return varID;
}

ArrayID ArrayVarMap::Copy(ArrayID from, UInt8 modIndex, bool bDeepCopy)
{
	ArrayVar* src = Get(from);
	if (!src)
		return 0;

	ArrayID copyID = Create(src->KeyType(), src->IsPacked(), modIndex);
	for (ArrayIterator iter = src->m_elements.begin(); iter != src->m_elements.end(); ++iter)
	{
		if (iter->second.DataType() == kDataType_Array && bDeepCopy)
		{
			ArrayID innerID = 0;
			ArrayID innerCopyID = 0;
			if (iter->second.GetAsArray(&innerID))
				innerCopyID = Copy(innerID, modIndex, true);

			if (!SetElementArray(copyID, iter->first, innerCopyID))
			{
				DEBUG_PRINT("ArrayVarMap::Copy failed to make deep copy of inner array");
			}
		}
		else
		{
			if (!SetElement(copyID, iter->first, iter->second))
			{
				DEBUG_PRINT("ArrayVarMap::Copy failed to set element in copied array");
			}
		}
	}

	return copyID;
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
		for (std::vector<UInt8>::iterator iter = var->m_refs.begin(); iter != var->m_refs.end(); ++iter)
		{
			if (*iter == referringModIndex)
			{
				var->m_refs.erase(iter);
				break;
			}
		}
	}

	// if refcount is zero, queue for deletion
	if (var && var->m_refs.size() == 0)
	{
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

ArrayID ArrayVarMap::MakeSlice(ArrayID src, const Slice* slice, UInt8 modIndex)
{
	// keys in slice match those in source unless array packed, in which case they must start at zero

	ArrayVar* srcVar = Get(src);
	if (!srcVar)
		return 0;
	
	std::map<ArrayKey, ArrayElement>::iterator start, end;
	ArrayKey lo;
	ArrayKey hi;

	if (slice->bIsString && srcVar->KeyType() == kDataType_String)
	{
		lo = ArrayKey(slice->m_lowerStr);
		hi = ArrayKey(slice->m_upperStr);

	}
	else if (!slice->bIsString && srcVar->KeyType() == kDataType_Numeric)
	{
		lo = ArrayKey(slice->m_lower);
		hi = ArrayKey(slice->m_upper);
	}
	else
		return 0;

	ArrayID newID = Create(srcVar->KeyType(), srcVar->IsPacked(), modIndex);
	bool bPacked = srcVar->IsPacked();

	for (start = srcVar->m_elements.begin(); start != srcVar->m_elements.end(); ++start)
	{
		if (start->first >= lo)
			break;
	}

	UInt32 packedIndex = 0;

	for (end = start; end != srcVar->m_elements.end(); ++end)
	{
		if (end->first > hi)
			break;

		if (bPacked)
			SetElement(newID, packedIndex++, end->second);
		else
			SetElement(newID, end->first, end->second);
	}

	return newID;
}

UInt32	ArrayVarMap::GetKeyType(ArrayID id)
{
	ArrayVar* var = Get(id);
	return var ? var->KeyType() : kDataType_Invalid;
}

bool ArrayVarMap::Exists(ArrayID id)
{
	// redundant method, only name differs, preserved because it's used in several places
	return VarExists(id);
}

UInt32 ArrayVarMap::SizeOf(ArrayID id)
{
	ArrayVar* var = Get(id);
	return var ? var->Size() : -1;
}

UInt32 ArrayVarMap::IsPacked(ArrayID id)
{
	ArrayVar* var = Get(id);
	return var ? var->IsPacked() : false;
}

ArrayID ArrayVarMap::GetKeys(ArrayID id, UInt8 modIndex)
{
	// returns an array of all the keys
	ArrayVar* src = Get(id);
	if (!src)
		return 0;

	ArrayID keyArrID = Create(kDataType_Numeric, true, modIndex);
	UInt8 keysType = src->KeyType();
	UInt32 curIdx = 0;

	for (ArrayIterator iter = src->m_elements.begin(); iter != src->m_elements.end(); ++iter)
	{
		if (keysType == kDataType_Numeric)
			SetElementNumber(keyArrID, curIdx, iter->first.Key().num);
		else
			SetElementString(keyArrID, curIdx, iter->first.Key().str);
		curIdx++;
	}

	return keyArrID;
}

bool ArrayVarMap::HasKey(ArrayID id, const ArrayKey& key)
{
	ArrayVar* arr = Get(id);
	if (!arr || arr->KeyType() != key.KeyType())
		return false;

	return (arr->m_elements.find(key) != arr->m_elements.end());
}

bool ArrayVarMap::AsVector(ArrayID id, std::vector<const ArrayElement*> &vecOut)
{
	ArrayVar* arr = Get(id);
	if (!arr || !arr->IsPacked())
		return false;

	for (UInt32 i = 0; i < arr->Size(); i++)
	{
		vecOut.push_back(arr->Get(ArrayKey(i), false));
	}

	return true;
}

bool ArrayVarMap::SetSize(ArrayID id, UInt32 newSize, const ArrayElement& padWith)
{
	ArrayVar* arr = Get(id);
	if (!arr || !arr->IsPacked())
		return false;

	if (arr->Size() < newSize)
	{
		for (UInt32 i = arr->Size(); i < newSize; i++)
			SetElement(id, ArrayKey(i), padWith);
	}
	else if (arr->Size() > newSize)
		return EraseElements(id, ArrayKey(newSize), ArrayKey(arr->Size() - 1)) != -1;

	return true;
}

bool ArrayVarMap::Insert(ArrayID id, UInt32 atIndex, const ArrayElement& toInsert)
{
	ArrayVar* arr = Get(id);
	if (!arr || !arr->IsPacked() || atIndex > arr->Size())
		return false;
	
	if (atIndex < arr->Size())	
	{
		// shift higher elements up by one
		for (SInt32 i = arr->Size(); i >= (SInt32)atIndex; i--)
			SetElement(id, ArrayKey(i), i > 0 ? arr->m_elements[i-1] : ArrayElement());
	}

	// insert
	SetElement(id, ArrayKey(atIndex), toInsert);
	return true;
}

bool ArrayVarMap::Insert(ArrayID id, UInt32 atIndex, ArrayID rangeID)
{
	ArrayVar* dest = Get(id);
	ArrayVar* src = Get(rangeID);
	if (!dest || !src || !dest->IsPacked() || !src->IsPacked() || atIndex > dest->Size())
		return false;

	UInt32 shiftDelta = src->Size();

	// resize, pad with empty elements
	SetSize(id, dest->Size() + shiftDelta, ArrayElement());

	if (atIndex < dest->Size())
	{
		// shift up
		for (SInt32 i = dest->Size() - 1; i >= (SInt32)(atIndex + shiftDelta); i--)
			SetElement(id, ArrayKey(i), dest->m_elements[i-shiftDelta]);
	}

	// insert
	for (UInt32 i = 0; i < shiftDelta; i++)
		SetElement(id, ArrayKey(i + atIndex), src->m_elements[i]);

	return true;
}

class SortFunctionCaller : public FunctionCaller
{
	Script			* m_comparator;
	ArrayID			m_lhs;
	ArrayID			m_rhs;

public:
	SortFunctionCaller(Script* comparator) : m_comparator(comparator), m_lhs(0), m_rhs(0) { 
		if (comparator) {
			m_lhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
			m_rhs = g_ArrayMap.Create(kDataType_Numeric, true, comparator->GetModIndex());
		}
	}

	virtual ~SortFunctionCaller() { }

	virtual UInt8 ReadCallerVersion() { return UserFunctionManager::kVersion; }
	virtual Script * ReadScript() { return m_comparator; }
	virtual bool PopulateArgs(ScriptEventList* eventList, FunctionInfo* info) {
		DynamicParamInfo& dParams = info->ParamInfo();
		if (dParams.NumParams() == 2) {
			UserFunctionParam* lhs = info->GetParam(0);
			UserFunctionParam* rhs = info->GetParam(1);
			if (lhs && rhs && lhs->varType == Script::eVarType_Array && rhs->varType == Script::eVarType_Array) {
				ScriptEventList::Var* var = eventList->GetVariable(lhs->varIdx);
				if (!var) {
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				else {
					g_ArrayMap.AddReference(&var->data, m_lhs, m_comparator->GetModIndex());
				}

				var = eventList->GetVariable(rhs->varIdx);
				if (!var) {
					ShowRuntimeError(m_comparator, "Could not look up argument variable for function script");
					return false;
				}
				else {
					g_ArrayMap.AddReference(&var->data, m_rhs, m_comparator->GetModIndex());
				}

				return true;
			}
		}

		return false;
	}

	virtual TESObjectREFR* ThisObj() { return NULL; }
	virtual TESObjectREFR* ContainingObj() { return NULL; }

	bool operator()(const ArrayElement& lhs, const ArrayElement& rhs) {
		g_ArrayMap.SetElement(m_lhs, 0.0, lhs);
		g_ArrayMap.SetElement(m_rhs, 0.0, rhs);
		ScriptToken* result = UserFunctionManager::Call(*this);
		bool bResult = result ? result->GetBool() : false;
		delete result;
		return bResult;
	}
};

ArrayID ArrayVarMap::Sort(ArrayID src, SortOrder order, SortType type, UInt8 modIndex, Script* comparator)
{
	// result is a packed integer-based array of the elements in sorted order
	// if array cannot be sorted we return empty array
	ArrayVar* srcVar = Get(src);
	ArrayID result = Create(kDataType_Numeric, true, modIndex);
	if (!srcVar || !srcVar->Size())
		return result;

	// restriction: all elements of src must be of the same type for default sort
	// restriction not in effect for alpha sort (all values treated as strings) or custom sort (all values boxed as arrays)
	std::vector<ArrayElement> vec;
	ArrayIterator iter = srcVar->m_elements.begin();
	UInt32 dataType = iter->second.DataType();
	if (dataType == kDataType_Invalid || dataType == kDataType_Array)	// nonsensical to sort array of arrays
		return result;

	// copy elems to vec, verify all are of same type
	for ( ; iter != srcVar->m_elements.end(); ++iter)
	{
		if (type == kSortType_Default && iter->second.DataType() != dataType)
			return result;
		vec.push_back(iter->second);
	}

	// let STL do the sort
	if (type == kSortType_Default)
		std::sort(vec.begin(), vec.end());
	else if (type == kSortType_Alpha)
		std::sort(vec.begin(), vec.end(), ArrayElement::CompareAsString);
	else if (type == kSortType_UserFunction) {
		if (!comparator) {
			return result;
		}
		SortFunctionCaller sorter(comparator);
		std::sort(vec.begin(), vec.end(), sorter);
	}

	if (order == kSort_Descending)
		std::reverse(vec.begin(), vec.end());

	for (UInt32 i = 0; i < vec.size(); i++)
		SetElement(result, i, vec[i]);

	return result;
}

UInt32 ArrayVarMap::EraseElements(ArrayID id, const ArrayKey& lo, const ArrayKey& hi)
{
	ArrayVar* var = Get(id);
	if (!var || lo.KeyType() != hi.KeyType() || lo.KeyType() != var->KeyType())
		return -1;

	// find first elem to erase
	std::map<ArrayKey, ArrayElement>::iterator iter = var->m_elements.begin();
	while (iter != var->m_elements.end() && iter->first < lo)
		++iter;

	UInt32 numErased = 0;

	// erase. if element is an arrayID, clean up that array
	while (iter != var->m_elements.end() && iter->first <= hi)
	{
		iter->second.Unset();
		iter = var->m_elements.erase(iter);
		numErased++;
	}

	// if array is packed we must shift elements down
	if (var->IsPacked())
		var->Pack();

	return numErased;
}

UInt32 ArrayVarMap::EraseAllElements(ArrayID id)
{
	UInt32 numErased = -1;
	ArrayVar* var = Get(id);	
	if (var) {
		while (var->m_elements.begin() != var->m_elements.end())
		{
			var->m_elements.begin()->second.Unset();
			var->m_elements.erase(var->m_elements.begin());
			numErased++;
		}
	}

	return numErased;
}

bool ArrayVarMap::SetElementNumber(ArrayID id, const ArrayKey& key, double num)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, true);
	if (!elem || !elem->SetNumber(num))
		return false;

	return true;
}

bool ArrayVarMap::SetElementString(ArrayID id, const ArrayKey& key, const std::string& str)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, true);
	if (!elem || !elem->SetString(str))
		return false;

	return true;
}

bool ArrayVarMap::SetElementFormID(ArrayID id, const ArrayKey& key, UInt32 refID)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, true);
	if (!elem || !elem->SetFormID(refID))
		return false;

	return true;
}

bool ArrayVarMap::SetElementArray(ArrayID id, const ArrayKey& key, ArrayID srcID)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, true);
	if (!elem || !elem->SetArray(srcID, arr->m_owningModIndex))
		return false;

	return true;
}

bool ArrayVarMap::SetElement(ArrayID id, const ArrayKey& key, const ArrayElement& val)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, true);
	if (!elem || !elem->Set(val))
		return false;

	return true;
}

bool ArrayVarMap::GetElementNumber(ArrayID id, const ArrayKey& key, double* out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	return (elem && elem->GetAsNumber(out));
}

bool ArrayVarMap::GetElementString(ArrayID id, const ArrayKey& key, std::string& out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	return (elem && elem->GetAsString(out));
}

bool ArrayVarMap::GetElementCString(ArrayID id, const ArrayKey& key, const char** out)
{
	ArrayVar* arr = Get(id);
	if (arr)
	{
		ArrayElement* elem = arr->Get(key, false);
		if (elem && elem->DataType() == kDataType_String)
		{
			*out = elem->m_data.str.c_str();
			return true;
		}
	}

	return false;
}

bool ArrayVarMap::GetElementFormID(ArrayID id, const ArrayKey& key, UInt32* out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	return (elem && elem->GetAsFormID(out));
}

bool ArrayVarMap::GetElementForm(ArrayID id, const ArrayKey& key, TESForm** out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	UInt32 refID = 0;
	if (elem && elem->GetAsFormID(&refID))
	{
		*out = LookupFormByID(refID);
		return true;
	}

	return false;
}

UInt8 ArrayVarMap::GetElementType(ArrayID id, const ArrayKey& key)
{
	ArrayVar* arr = Get(id);
	if (arr)
	{
		ArrayElement* elem = arr->Get(key, false);
		if (elem)
			return elem->DataType();
	}

	return kDataType_Invalid;
}

bool ArrayVarMap::GetElementArray(ArrayID id, const ArrayKey& key, ArrayID* out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	return (elem && elem->GetAsArray(out));
}

bool ArrayVarMap::GetElementAsBool(ArrayID id, const ArrayKey& key, bool* out)
{
	ArrayVar* arr = Get(id);
	if (!arr)
		return false;

	ArrayElement* elem = arr->Get(key, false);
	if (!elem)
		return false;

	if (elem->DataType() == kDataType_String)
		*out = true;			// strings are always "true", whatever that means in this context
	else
		*out = elem->m_data.num ? true : false;

	return true;
}

void ArrayVarMap::DumpArray(ArrayID toDump)
{
	Console_Print("** Dumping Array #%d **", toDump);
	_MESSAGE("**\nDumping Array #%d **", toDump);
	
	ArrayVar* arr = Get(toDump);
	if (arr)
		arr->Dump();
	else
	{
		Console_Print("  Array does not exist");
		_MESSAGE("  Array does not exist");
	}
}

void ArrayVarMap::Save(NVSESerializationInterface* intfc)
{
	Clean();

	intfc->OpenRecord('ARVS', kVersion);

	if (m_state) {
		std::map<UInt32, ArrayVar*> & vars = m_state->vars;
		for (std::map<UInt32, ArrayVar*>::iterator iter = vars.begin(); iter != vars.end(); ++iter)
		{
			if (IsTemporary(iter->first))
				continue;

			intfc->OpenRecord('ARVR', kVersion);
			intfc->WriteRecordData(&iter->second->m_owningModIndex, sizeof(UInt8));
			intfc->WriteRecordData(&iter->first, sizeof(UInt32));
			intfc->WriteRecordData(&iter->second->m_keyType, sizeof(UInt8));
			intfc->WriteRecordData(&iter->second->m_bPacked, sizeof(bool));
			
			UInt32 numRefs = iter->second->m_refs.size();
			intfc->WriteRecordData(&numRefs, sizeof(numRefs));
			if (!numRefs)
				_MESSAGE("ArrayVarMap::Save(): saving array with no references");

			for (UInt32 i = 0; i < numRefs; i++)
				intfc->WriteRecordData(&iter->second->m_refs[i], sizeof(UInt8));

			UInt32 numElements = iter->second->Size();
			intfc->WriteRecordData(&numElements, sizeof(UInt32));

			UInt8 keyType = iter->second->m_keyType;
			for (std::map<ArrayKey, ArrayElement>::iterator elems = iter->second->m_elements.begin();
				elems != iter->second->m_elements.end(); ++elems)
			{
				ArrayType key = elems->first.Key();
				if (keyType == kDataType_Numeric)
					intfc->WriteRecordData(&key.num, sizeof(double));
				else
				{
					UInt16 len = key.str.length();
					intfc->WriteRecordData(&len, sizeof(len));
					intfc->WriteRecordData(key.str.c_str(), key.str.length());
				}

				intfc->WriteRecordData(&elems->second.m_dataType, sizeof(UInt8));
				switch (elems->second.m_dataType)
				{
				case kDataType_Numeric:
					intfc->WriteRecordData(&elems->second.m_data.num, sizeof(double));
					break;
				case kDataType_String:
					{
						UInt16 len = elems->second.m_data.str.length();
						intfc->WriteRecordData(&len, sizeof(len));
						intfc->WriteRecordData(elems->second.m_data.str.c_str(), elems->second.m_data.str.length());
						break;
					}
				case kDataType_Array:
					{
						ArrayID id = elems->second.m_data.num;
						intfc->WriteRecordData(&id, sizeof(id));
						break;
					}
				case kDataType_Form:
					intfc->WriteRecordData(&elems->second.m_data.formID, sizeof(UInt32));
					break;
				default:
					_MESSAGE("Error in ArrayVarMap::Save() - unhandled element type %d. Element not saved.", elems->second.m_dataType);
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
	char buffer[kMaxMessageLength] = { 0 };

	//Reset(intfc);
	bool bContinue = true;

	UInt32 lastIndexRead = 0;

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
				UInt8* refs = NULL;		// mod indexes of mods referring to this array

				// reference-counting implemented in v1
				if (version >= 1)
				{
					intfc->ReadRecordData(&numRefs, sizeof(numRefs));
					if (numRefs)
					{
						refs = new UInt8[numRefs];
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
								refs[refIdx++] = (tempRefID >> 24);
						}

						numRefs = refIdx;
					}
				}
				else		// v0 arrays assumed to have only one reference (the owning mod)
				{
					if (modIndex)		// owning mod is loaded
					{
						numRefs = 1;
						refs = new UInt8[1];
						refs[0] = modIndex;
					}
				}
				
				if (!modIndex)
				{
					_MESSAGE("Array ID %d is referred to by no loaded mods. Discarding", arrayID);
					delete[] refs;
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
				Add(newArr, arrayID, numRefs, refs);
				delete[] refs;

				// read the array elements			
				intfc->ReadRecordData(&numElements, sizeof(numElements));
				for (UInt32 i = 0; i < numElements; i++)
				{
					ArrayKey newKey;
					if (keyType == kDataType_Numeric)
					{
						double num;
						intfc->ReadRecordData(&num, sizeof(double));
						newKey = num;
					}
					else
					{
						intfc->ReadRecordData(&strLength, sizeof(strLength));
						intfc->ReadRecordData(buffer, strLength);
						buffer[strLength] = 0;
						newKey = std::string(buffer);
					}

					UInt8 elemType;
					if (intfc->ReadRecordData(&elemType, sizeof(elemType)) == 0)
					{
						_MESSAGE("ArrayVarMap::Load() reading past end of file");
						return;
					}

					switch (elemType)
					{
					case kDataType_Numeric:
						{
							double num;
							intfc->ReadRecordData(&num, sizeof(num));
							SetElementNumber(arrayID, newKey, num);
							break;
						}
					case kDataType_String:
						{
							intfc->ReadRecordData(&strLength, sizeof(strLength));
							intfc->ReadRecordData(buffer, strLength);
							buffer[strLength] = 0;
							SetElementString(arrayID, newKey, std::string(buffer));
							break;
						}
					case kDataType_Array:
						{
							ArrayID id;
							intfc->ReadRecordData(&id, sizeof(id));
							if (newArr)
							{
								ArrayElement* elem = newArr->Get(newKey, true);
								if (elem)
								{
									elem->m_dataType = kDataType_Array;
									elem->m_data.num = id;
									elem->m_owningArray = arrayID;
								}
							}

							break;
						}
					case kDataType_Form:
						{
							UInt32 formID;
							intfc->ReadRecordData(&formID, sizeof(formID));
							if (!intfc->ResolveRefID(formID, &formID))
								formID = 0;

							SetElementFormID(arrayID, newKey, formID);
							break;
						}
						break;
					default:
						_MESSAGE("Unknown element type %d encountered while loading array var, element discarded.", elemType);
						break;
					}
				}
			}
			break;
		default:
			_MESSAGE("Error loading array var map: unexpected chunk type %d", type);
			break;
		}
	}
}

bool ArrayVarMap::GetFirstElement(ArrayID id, ArrayElement* outElem, ArrayKey* outKey)
{
	ArrayVar* var = Get(id);
	if (!var || !var->Size() || !outElem || !outKey)
		return false;

	std::map<ArrayKey, ArrayElement>::iterator iter = var->m_elements.begin();
	*outKey = iter->first;
	*outElem = iter->second;
	return true;
}

bool ArrayVarMap::GetLastElement(ArrayID id, ArrayElement* outElem, ArrayKey* outKey)
{
	ArrayVar* var = Get(id);
	if (!var || !var->Size() || !outElem || !outKey)
		return false;

	ArrayIterator iter = var->m_elements.end();
	if (var->Size() > 1)
		--iter;
	else		// only one element
		iter= var->m_elements.begin();

	*outKey = iter->first;
	*outElem = iter->second;
	return true;
}

bool ArrayVarMap::GetNextElement(ArrayID id, ArrayKey* prevKey, ArrayElement* outElem, ArrayKey* outKey)
{
	ArrayVar* var = Get(id);
	if (!var || !var->Size() || !outElem || !outKey || !prevKey)
		return false;

	std::map<ArrayKey, ArrayElement>::iterator iter = var->m_elements.find(*prevKey);
	if (iter != var->m_elements.end())
	{
		++iter;
		if (iter != var->m_elements.end())
		{
			//var->m_cachedIterator = iter;

			*outKey = iter->first;
			*outElem = iter->second;
			return true;
		}
	}

	return false;
}

bool ArrayVarMap::GetPrevElement(ArrayID id, ArrayKey* prevKey, ArrayElement* outElem, ArrayKey* outKey)
{
	ArrayVar* var = Get(id);
	if (!var || !var->Size() || !outElem || !outKey || !prevKey)
		return false;

	ArrayIterator iter = var->m_elements.find(*prevKey);
	if (iter != var->m_elements.end() && iter != var->m_elements.begin())
	{
		--iter;
		*outKey = iter->first;
		*outElem = iter->second;
		return true;
	}

	return false;
}

ArrayKey ArrayVarMap::Find(ArrayID toSearch, const ArrayElement& toFind, const Slice* range)
{
	ArrayKey foundIndex;
	ArrayVar* var = Get(toSearch);
	if (!var)
		return foundIndex;

	ArrayIterator start = var->m_elements.begin();
	ArrayIterator end = var->m_elements.end();
	if (range)
	{
		if ((range->bIsString && var->KeyType() != kDataType_String) || (!range->bIsString && var->KeyType() != kDataType_Numeric))
			return foundIndex;
		
		// locate lower and upper bounds
		ArrayKey lo;
		ArrayKey hi;
		range->GetArrayBounds(lo, hi);

		while (start != var->m_elements.end() && start->first < lo)
			++start;

		end = start;
		while (end != var->m_elements.end() && end->first <= hi)
			++end;
	}

	// do the search
	for (ArrayIterator iter = start; iter != end; ++iter)
	{
		if (iter->second.Equals(toFind))
		{
			foundIndex = iter->first;
			break;
		}
	}

	return foundIndex;
}

std::string ArrayVarMap::GetTypeString(ArrayID arr)
{
	std::string result = "<Bad Array>";
	ArrayVar* a = Get(arr);
	if (a)
	{
		if (a->KeyType() == kDataType_Numeric)
			result = a->IsPacked() ? "Array" : "Map";
		else if (a->KeyType() == kDataType_String)
			result = "StringMap";
	}

	return result;
}

void ArrayVarMap::Clean()		// garbage collection: delete unreferenced arrays
{
	// ArrayVar destructor may queue more IDs for deletion if deleted array contains other arrays
	// so on each pass through the loop we delete the first ID in the queue until none remain

	if (m_state) {
		while (m_state->tempVars.size())
		{
			UInt32 idToDelete = *(m_state->tempVars.begin());
			Delete(idToDelete);
		}
	}
}

namespace PluginAPI
{
	bool ArrayAPI::SetElementFromAPI(UInt32 id, ArrayKey& key, const NVSEArrayVarInterface::Element& elem)
	{
		switch (elem.type)
		{
		case elem.kType_Array:
			return g_ArrayMap.SetElementArray(id, key, (ArrayID)elem.arr);
		case elem.kType_Numeric:
			return g_ArrayMap.SetElementNumber(id, key, elem.num);
		case elem.kType_String:
			return g_ArrayMap.SetElementString(id, key, elem.str);
		case elem.kType_Form:
			return g_ArrayMap.SetElementFormID(id, key, elem.form ? elem.form->refID : 0);
		default:
			return false;
		}
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateArray(const NVSEArrayVarInterface::Element* data, UInt32 size, Script* callingScript)
	{
		ArrayID id = g_ArrayMap.Create(kDataType_Numeric, true, callingScript->GetModIndex());
		for (UInt32 i = 0; i < size; i++)
		{
			if (!SetElementFromAPI(id, ArrayKey(i), data[i]))
			{
				_MESSAGE("Error: An attempt by a plugin to set an array element failed.");
				return NULL;
			}
		}

		return (NVSEArrayVarInterface::Array*)id;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateStringMap(const char** keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript)
	{
		ArrayID id = g_ArrayMap.Create(kDataType_String, false, callingScript->GetModIndex());

		for (UInt32 i = 0; i < size; i++) {
			if (!SetElementFromAPI(id, ArrayKey(keys[i]), values[i])) {
				_MESSAGE("Error: An attempt by a plugin to set an array element failed.");
				return NULL;
			}
		}

		return (NVSEArrayVarInterface::Array*)id;
	}

	NVSEArrayVarInterface::Array* ArrayAPI::CreateMap(const double* keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript)
	{
		ArrayID id = g_ArrayMap.Create(kDataType_Numeric, false, callingScript->GetModIndex());

		for (UInt32 i = 0; i < size; i++) {
			if (!SetElementFromAPI(id, ArrayKey(keys[i]), values[i])) {
				_MESSAGE("Error: An attempt by a plugin to set an array element failed.");
				return NULL;
			}
		}

		return (NVSEArrayVarInterface::Array*)id;
	}

	bool ArrayAPI::AssignArrayCommandResult(NVSEArrayVarInterface::Array* arr, double* dest)
	{
		if (!g_ArrayMap.Get((ArrayID)arr))
		{
			_MESSAGE("Error: A plugin is attempting to return a non-existent array.");
			return false;
		}

		*dest = (UInt32)arr;
		return true;
	}

	void ArrayAPI::SetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key, const NVSEArrayVarInterface::Element& value)
	{
		ArrayID arrID = (ArrayID)arr;

		switch (key.type)
		{
		case NVSEArrayVarInterface::Element::kType_Numeric:
			SetElementFromAPI(arrID, ArrayKey(key.num), value);
			break;
		case NVSEArrayVarInterface::Element::kType_String:
			SetElementFromAPI(arrID, ArrayKey(key.str), value);
			break;
		default:
			_MESSAGE("Error: a plugin is attempting to create an array element with an invalid key type.");
		}
	}

	void ArrayAPI::AppendElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& value)
	{
		ArrayID arrID = (ArrayID)arr;
		ArrayVar* theArray = g_ArrayMap.Get(arrID);

		// verify array is the packed, numeric kind
		if (theArray && theArray->KeyType() == kDataType_Numeric && theArray->IsPacked()) {
			SetElementFromAPI(arrID, ArrayKey(theArray->Size()), value);
		}
	}

	UInt32 ArrayAPI::GetArraySize(NVSEArrayVarInterface::Array* arr)
	{
		return g_ArrayMap.SizeOf((ArrayID)arr);
	}

	UInt32 ArrayAPI::GetArrayPacked(NVSEArrayVarInterface::Array* arr)
	{
		return g_ArrayMap.IsPacked((ArrayID)arr);
	}

	NVSEArrayVarInterface::Array* ArrayAPI::LookupArrayByID(UInt32 id)
	 {
		 if (g_ArrayMap.Exists(id))
			 return (NVSEArrayVarInterface::Array*)id;
		 else
			 return 0;
	 }

	bool ArrayAPI::GetElement(NVSEArrayVarInterface::Array* arr, const NVSEArrayVarInterface::Element& key, NVSEArrayVarInterface::Element& out)
	 {
		 ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		 ArrayElement* data = NULL;
		 if (var) {
			 switch (key.type) {
				 case key.kType_String:
					 if (var->KeyType() == kDataType_String) {
						 data = var->Get(key.str, false);
					 }
					 break;
				 case key.kType_Numeric:
					 if (var->KeyType() == kDataType_Numeric) {
						 data = var->Get(key.num, false);
					 }
					 break;
			 }
		 
			 if (data) {
				 return InternalElemToPluginElem(*data, out);
			 }
		 }

		 return false;
	}
		
	bool ArrayAPI::GetElements(NVSEArrayVarInterface::Array* arr, NVSEArrayVarInterface::Element* elements,
		NVSEArrayVarInterface::Element* keys)
	{
		if (!elements)
			return false;

		ArrayVar* var = g_ArrayMap.Get((ArrayID)arr);
		if (var) {
			UInt32 size = var->Size();
			UInt8 keyType = var->KeyType();

			if (size != -1) {
				UInt32 i = 0;
				for (ArrayIterator iter = var->m_elements.begin(); iter != var->m_elements.end(); ++iter) {
					if (keys) {
						switch (keyType) {
							case kDataType_Numeric:
								keys[i] = iter->first.Key().num;
								break;
							case kDataType_String:
								{
									keys[i] = NVSEArrayVarInterface::Element(iter->first.Key().str.c_str());
									break;
								}
						}
					}
					
					InternalElemToPluginElem(iter->second, elements[i]);
					i++;
				}

				return true;
			}
		}

		return false;
	}

	// helpers
	bool ArrayAPI::InternalElemToPluginElem(ArrayElement& src, NVSEArrayVarInterface::Element& out)
	{
		switch (src.DataType()) {
			case kDataType_Numeric:
				{
					double num;
					src.GetAsNumber(&num);
					out = num;
				}
				break;
			case kDataType_Form:
				{
					UInt32 formID;
					src.GetAsFormID(&formID);
					out = LookupFormByID(formID);
				}
				break;
			case kDataType_Array:
				{
					ArrayID arrID;
					src.GetAsArray(&arrID);
					out = (NVSEArrayVarInterface::Array*)arrID;
				}
				break;
			case kDataType_String:
				{
					std::string str;
					src.GetAsString(str);
					out = NVSEArrayVarInterface::Element(str.c_str());
				}
				break;
		}

		return true;
	}
}
