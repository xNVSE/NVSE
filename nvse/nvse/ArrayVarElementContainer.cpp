#include "ArrayVar.h"
#include "ScriptUtils.h"

ArrayVarElementContainer::~ArrayVarElementContainer()
{
	if (!m_container.pArray) return;
	clear();
	switch (m_type)
	{
		case kContainer_Array:
			delete m_container.pArray;
			break;
		case kContainer_NumericMap:
			delete m_container.pNumMap;
			break;
		case kContainer_StringMap:
			delete m_container.pStrMap;
			break;
	}
	m_container.pArray = NULL;
}

ArrayVarElementContainer::iterator::iterator(ElementVector *_array, ElementVector::iterator iter)
{
	m_type = kContainer_Array;
	m_container.pArray = _array;
	arrayIter = iter;
}

ArrayVarElementContainer::iterator::iterator(ElementNumMap *_numMap, ElementNumMap::iterator iter)
{
	m_type = kContainer_NumericMap;
	m_container.pNumMap = _numMap;
	numMapIter = iter;
}

ArrayVarElementContainer::iterator::iterator(ElementStrMap *_strMap, ElementStrMap::iterator iter)
{
	m_type = kContainer_StringMap;
	m_container.pStrMap = _strMap;
	strMapIter = iter;
}

void ArrayVarElementContainer::iterator::operator++()
{
	switch (m_type)
	{
		case kContainer_Array:
			++arrayIter;
			break;
		case kContainer_NumericMap:
			++numMapIter;
			break;
		case kContainer_StringMap:
			++strMapIter;
			break;
	}
}

void ArrayVarElementContainer::iterator::operator--()
{
	switch (m_type)
	{
		case kContainer_Array:
			--arrayIter;
			break;
		case kContainer_NumericMap:
			--numMapIter;
			break;
		case kContainer_StringMap:
			--strMapIter;
			break;
	}
}

ArrayVarElementContainer::iterator& ArrayVarElementContainer::iterator::operator+=(UInt32 incBy)
{
	switch (m_type)
	{
		case kContainer_Array:
		{
			UInt32 fromEnd = m_container.pArray->end() - arrayIter;
			if (incBy > fromEnd)
				incBy = fromEnd;
			arrayIter += incBy;
			break;
		}
		case kContainer_NumericMap:
		{
			auto mapEnd = m_container.pNumMap->end();
			while (incBy && (numMapIter != mapEnd))
			{
				++numMapIter;
				incBy--;
			}
			break;
		}
		case kContainer_StringMap:
		{
			auto mapEnd = m_container.pStrMap->end();
			while (incBy && (strMapIter != mapEnd))
			{
				++strMapIter;
				incBy--;
			}
			break;
		}
	}
	return *this;
}

ArrayVarElementContainer::iterator& ArrayVarElementContainer::iterator::operator-=(UInt32 decBy)
{
	switch (m_type)
	{
		case kContainer_Array:
		{
			UInt32 index = arrayIter - m_container.pArray->begin();
			if (decBy > index)
				decBy = index;
			arrayIter -= decBy;
			break;
		}
		case kContainer_NumericMap:
		{
			auto mapBgn = m_container.pNumMap->begin();
			while (decBy && (numMapIter != mapBgn))
			{
				--numMapIter;
				decBy--;
			}
			break;
		}
		case kContainer_StringMap:
		{
			auto mapBgn = m_container.pStrMap->begin();
			while (decBy && (strMapIter != mapBgn))
			{
				--strMapIter;
				decBy--;
			}
			break;
		}
	}
	return *this;
}

bool ArrayVarElementContainer::iterator::operator!=(const iterator& other) const
{
	if (m_type == other.m_type)
	{
		switch (m_type)
		{
			case kContainer_Array:
				return arrayIter != other.arrayIter;
			case kContainer_NumericMap:
				return numMapIter != other.numMapIter;
			case kContainer_StringMap:
				return strMapIter != other.strMapIter;
		}
	}
	return true;
}

const ArrayKey* ArrayVarElementContainer::iterator::first() const
{
	static ArrayKey arrNumKey(kDataType_Numeric), arrStrKey(kDataType_String);
	switch (m_type)
	{
		default:
		case kContainer_Array:
			arrNumKey.key.num = arrayIter - m_container.pArray->begin();
			return &arrNumKey;
		case kContainer_NumericMap:
			arrNumKey.key.num = numMapIter->first;
			return &arrNumKey;
		case kContainer_StringMap:
			arrStrKey.key.str = const_cast<char*>(strMapIter->first.Get());
			return &arrStrKey;
	}
}

ArrayElement* ArrayVarElementContainer::iterator::second() const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			return arrayIter._Ptr;
		case kContainer_NumericMap:
			return &numMapIter->second;
		case kContainer_StringMap:
			return &strMapIter->second;
	}
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::begin() const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			return iterator(m_container.pArray, m_container.pArray->begin());
		case kContainer_NumericMap:
			return iterator(m_container.pNumMap, m_container.pNumMap->begin());
		case kContainer_StringMap:
			return iterator(m_container.pStrMap, m_container.pStrMap->begin());
	}
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::end() const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			return iterator(m_container.pArray, m_container.pArray->end());
		case kContainer_NumericMap:
			return iterator(m_container.pNumMap, m_container.pNumMap->end());
		case kContainer_StringMap:
			return iterator(m_container.pStrMap, m_container.pStrMap->end());
	}
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::find(const ArrayKey* key) const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
		{
			if (key->key.dataType == kDataType_Numeric)
			{
				UInt32 index = (int)key->key.num;
				if (index < m_container.pArray->size())
					return iterator(m_container.pArray, m_container.pArray->begin() + index);
			}
			return iterator(m_container.pArray, m_container.pArray->end());
		}
		case kContainer_NumericMap:
			if (key->key.dataType == kDataType_Numeric)
				return iterator(m_container.pNumMap, m_container.pNumMap->find(key->key.num));
			return iterator(m_container.pNumMap, m_container.pNumMap->end());
		case kContainer_StringMap:
			if (key->key.dataType == kDataType_String)
				return iterator(m_container.pStrMap, m_container.pStrMap->find(*(StringKey*)&key->key.str));
			return iterator(m_container.pStrMap, m_container.pStrMap->end());
	}
}

size_t ArrayVarElementContainer::size() const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
			return m_container.pArray->size();
		case kContainer_NumericMap:
			return m_container.pNumMap->size();
		case kContainer_StringMap:
			return m_container.pStrMap->size();
	}
}

size_t ArrayVarElementContainer::erase(const ArrayKey* key) const
{
	switch (m_type)
	{
		default:
		case kContainer_Array:
		{
			if (key->key.dataType != kDataType_Numeric)
				return 0;
			UInt32 idx = (int)key->key.num;
			if (idx >= m_container.pArray->size())
				return 0;
			(*m_container.pArray)[idx].Unset();
			m_container.pArray->erase(m_container.pArray->begin() + idx);
			return 1;
		}
		case kContainer_NumericMap:
		{
			if (key->key.dataType != kDataType_Numeric)
				return 0;
			auto findKey = m_container.pNumMap->find(key->key.num);
			if (findKey == m_container.pNumMap->end())
				return 0;
			findKey->second.Unset();
			m_container.pNumMap->erase(findKey);
			return 1;
		}
		case kContainer_StringMap:
		{
			if (key->key.dataType != kDataType_String)
				return 0;
			auto findKey = m_container.pStrMap->find(*(StringKey*)&key->key.str);
			if (findKey == m_container.pStrMap->end())
				return 0;
			findKey->second.Unset();
			m_container.pStrMap->erase(findKey);
			return 1;
		}
	}
}

size_t ArrayVarElementContainer::erase(UInt32 iLow, UInt32 iHigh) const
{
	if ((m_type != kContainer_Array) || m_container.pArray->empty())
		return 0;
	UInt32 arrSize = m_container.pArray->size();
	if (iHigh >= arrSize)
		iHigh = arrSize - 1;
	if ((iLow >= arrSize) || (iLow > iHigh))
		return 0;
	iHigh++;
	for (UInt32 idx = iLow; idx < iHigh; idx++)
		(*m_container.pArray)[idx].Unset();
	m_container.pArray->erase(m_container.pArray->begin() + iLow, m_container.pArray->begin() + iHigh);
	return iHigh - iLow;
}

void ArrayVarElementContainer::clear() const
{
	switch (m_type)
	{
		case kContainer_Array:
		{
			for (UInt32 idx = 0; idx < m_container.pArray->size(); idx++)
				(*m_container.pArray)[idx].Unset();
			m_container.pArray->clear();
			break;
		}
		case kContainer_NumericMap:
		{
			for (auto iter = m_container.pNumMap->begin(); iter != m_container.pNumMap->end(); ++iter)
				iter->second.Unset();
			m_container.pNumMap->clear();
			break;
		}
		case kContainer_StringMap:
		{
			for (auto iter = m_container.pStrMap->begin(); iter != m_container.pStrMap->end(); ++iter)
				iter->second.Unset();
			m_container.pStrMap->clear();
			break;
		}
	}
}