#include "ArrayVar.h"

#if RUNTIME

ArrayVarElementContainer::~ArrayVarElementContainer()
{
	clear();
	switch (m_type)
	{
		case kContainer_Array:
			AsArray().~ElementVector();
			break;
		case kContainer_NumericMap:
			AsNumMap().~ElementNumMap();
			break;
		case kContainer_StringMap:
			AsStrMap().~ElementStrMap();
			break;
	}
}

void ArrayVarElementContainer::clear()
{
	if (empty()) return;
	switch (m_type)
	{
		default:
		case kContainer_Array:
		{
			for (auto iter = AsArray().Begin(); !iter.End(); ++iter)
				iter.Get().Unset();
			m_container.numItems = 0;
			break;
		}
		case kContainer_NumericMap:
		{
			for (auto iter = AsNumMap().Begin(); !iter.End(); ++iter)
				iter.Get().Unset();
			AsNumMap().Clear();
			break;
		}
		case kContainer_StringMap:
		{
			for (auto iter = AsStrMap().Begin(); !iter.End(); ++iter)
				iter.Get().Unset();
			AsStrMap().Clear();
			break;
		}
	}
}

UInt32 ArrayVarElementContainer::erase(const ArrayKey *key)
{
	if (empty()) return 0;
	switch (m_type)
	{
		default:
		case kContainer_Array:
		{
			if (key->key.dataType != kDataType_Numeric)
				return 0;
			UInt32 idx = (int)key->key.num;
			if (idx >= m_container.numItems)
				return 0;
			AsArray()[idx].Unset();
			AsArray().RemoveNth(idx);
			return 1;
		}
		case kContainer_NumericMap:
		{
			if (key->key.dataType != kDataType_Numeric)
				return 0;
			auto findKey = AsNumMap().Find(key->key.num);
			if (findKey.End())
				return 0;
			findKey.Get().Unset();
			findKey.Remove(false);
			return 1;
		}
		case kContainer_StringMap:
		{
			if (key->key.dataType != kDataType_String)
				return 0;
			auto findKey = AsStrMap().Find(key->key.str);
			if (findKey.End())
				return 0;
			findKey.Get().Unset();
			findKey.Remove(false);
			return 1;
		}
	}
}

UInt32 ArrayVarElementContainer::erase(UInt32 iLow, UInt32 iHigh)
{
	if (empty() || (m_type != kContainer_Array && m_type != kContainer_NumericMap))
		return 0;
	UInt32 arrSize = m_container.numItems;
	if (iHigh >= arrSize)
		iHigh = arrSize - 1;
	if ((iLow >= arrSize) || (iLow > iHigh))
		return 0;
	iHigh++;
	if (m_type == kContainer_Array)
	{
		ArrayElement* elements = AsArray().Data();
		for (UInt32 idx = iLow; idx < iHigh; idx++)
			elements[idx].Unset();
		iHigh -= iLow;
		AsArray().RemoveRange(iLow, iHigh);
		return iHigh;
	}
	if (m_type == kContainer_NumericMap)
	{
		auto& elements = AsNumMap();
		auto numErased = 0U;
		for (auto i = iLow; i < iHigh; ++i)
		{
			auto* element = elements.GetPtr(i);
			if (element)
			{
				element->Unset();
				elements.Erase(i);
				++numErased;
			}
		}
		return numErased;
	}
	return -1;
}

ArrayVarElementContainer::iterator::iterator(ArrayVarElementContainer& container)
{
	m_type = container.m_type;
	switch (m_type)
	{
		default:
		case kContainer_Array:
			AsArray().Init(container.AsArray());
			break;
		case kContainer_NumericMap:
			AsNumMap().Init(container.AsNumMap());
			break;
		case kContainer_StringMap:
			AsStrMap().Init(container.AsStrMap());
			break;
	}
}

ArrayVarElementContainer::iterator::iterator(ArrayVarElementContainer& container, bool reverse)
{
	m_type = container.m_type;
	switch (m_type)
	{
		default:
		case kContainer_Array:
			AsArray().Find(container.AsArray(), container.m_container.numItems - 1);
			break;
		case kContainer_NumericMap:
			AsNumMap().Last(container.AsNumMap());
			break;
		case kContainer_StringMap:
			AsStrMap().Last(container.AsStrMap());
			break;
	}
}

ArrayVarElementContainer::iterator::iterator(ArrayVarElementContainer& container, const ArrayKey* key)
{
	m_type = container.m_type;
	switch (m_type)
	{
		default:
		case kContainer_Array:
			AsArray().Find(container.AsArray(), (int)key->key.num);
			break;
		case kContainer_NumericMap:
			AsNumMap().Find(container.AsNumMap(), key->key.num);
			break;
		case kContainer_StringMap:
			AsStrMap().Find(container.AsStrMap(), key->key.str);
			break;
	}
}

void ArrayVarElementContainer::iterator::operator++()
{
	switch (m_type)
	{
		case kContainer_Array:
			AsArray().operator++();
			break;
		case kContainer_NumericMap:
			AsNumMap().operator++();
			break;
		case kContainer_StringMap:
			AsStrMap().operator++();
			break;
	}
}

void ArrayVarElementContainer::iterator::operator--()
{
	switch (m_type)
	{
		case kContainer_Array:
			AsArray().operator--();
			break;
		case kContainer_NumericMap:
			AsNumMap().operator--();
			break;
		case kContainer_StringMap:
			AsStrMap().operator--();
			break;
	}
}

const ArrayKey* ArrayVarElementContainer::iterator::first()
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			s_arrNumKey.key.num = (int)m_iter.index;
			return &s_arrNumKey;
		case kContainer_NumericMap:
			s_arrNumKey.key.num = AsNumMap().Key();
			return &s_arrNumKey;
		case kContainer_StringMap:
			s_arrStrKey.key.str = const_cast<char*>(AsStrMap().Key());
			return &s_arrStrKey;
	}
}

ArrayElement* ArrayVarElementContainer::iterator::second()
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			return &AsArray().Get();
		case kContainer_NumericMap:
			return &AsNumMap().Get();
		case kContainer_StringMap:
			return &AsStrMap().Get();
	}
}

#endif