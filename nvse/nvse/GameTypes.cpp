#include "nvse/GameTypes.h"

String::String() {}

String::~String() {}

void String::Init(UInt32 bufSize)
{
	if (m_data) GameHeapFree(m_data);
	m_bufLen = bufSize;
	m_data = (char*)GameHeapAlloc(m_bufLen + 1);
	*m_data = 0;
	m_dataLen = 0;
}

bool String::Set(const char *src)
{
	m_dataLen = StrLen(src);
	if (!m_dataLen)
	{
		if (!src)
		{
			if (m_data)
			{
				GameHeapFree(m_data);
				m_data = NULL;
			}
			m_bufLen = 0;
		}
		else if (m_data) *m_data = 0;
		return true;
	}
	if (m_bufLen < m_dataLen)
	{
		m_bufLen = m_dataLen;
		if (m_data) GameHeapFree(m_data);
		m_data = (char*)GameHeapAlloc(m_dataLen + 1);
	}
	MemCopy(m_data, src, m_dataLen + 1);
	return true;
}

bool String::Append(const char *toAppend)
{
	UInt16 length = StrLen(toAppend);
	if (!length) return true;
	UInt16 newLen = m_dataLen + length;
	if (m_bufLen < newLen)
	{
		m_bufLen = newLen;
		char *newStr = (char*)GameHeapAlloc(m_bufLen + 1);
		if (m_data)
		{
			MemCopy(newStr, m_data, m_dataLen);
			GameHeapFree(m_data);
		}
		m_data = newStr;
	}
	MemCopy(m_data + m_dataLen, toAppend, length + 1);
	m_dataLen = newLen;
	return true;
}

void String::AppendChar(char toAppend)
{
	if (m_bufLen == m_dataLen)
	{
		m_bufLen++;
		char *newStr = (char*)GameHeapAlloc(m_bufLen + 1);
		if (m_data)
		{
			MemCopy(newStr, m_data, m_dataLen);
			GameHeapFree(m_data);
		}
		m_data = newStr;
	}
	m_data[m_dataLen++] = toAppend;
	m_data[m_dataLen] = 0;
}

void String::InsertChar(char toInsert, UInt32 index)
{
	if (index > m_dataLen) return;
	if (m_bufLen == m_dataLen)
	{
		m_bufLen++;
		char *newStr = (char*)GameHeapAlloc(m_bufLen + 1);
		if (m_data)
		{
			MemCopy(newStr, m_data, m_dataLen);
			GameHeapFree(m_data);
		}
		m_data = newStr;
	}
	if (index < m_dataLen)
		MemCopy(m_data + index + 1, m_data + index, m_dataLen - index);
	m_dataLen++;
	m_data[index] = toInsert;
	m_data[m_dataLen] = 0;
}

void String::EraseAt(UInt32 index)
{
	if (!m_data || (index >= m_dataLen)) return;
	m_dataLen--;
	if (m_dataLen)
	{
		UInt32 length = m_dataLen - index;
		if (!length)
			m_data[m_dataLen] = 0;
		else
			MemCopy(m_data + index, m_data + index + 1, length + 1);
	}
	else *m_data = 0;
}

void String::RemoveLastChar()
{
	if (!m_data || !m_dataLen) return;
	m_data[--m_dataLen] = '\0';
}

const char *String::CStr()
{
	return (m_data && m_dataLen) ? m_data : "";
}