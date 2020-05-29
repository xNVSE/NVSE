#include "IDirectoryIterator.h"
#include <string>

IDirectoryIterator::IDirectoryIterator(const char * path, const char * match)
:m_searchHandle(INVALID_HANDLE_VALUE), m_done(false)
{
	if(!match) match = "*";

	strcpy_s(m_path, sizeof(m_path), path);

	char	wildcardPath[MAX_PATH];
	sprintf_s(wildcardPath, sizeof(wildcardPath), "%s\\%s", path, match);

	m_searchHandle = FindFirstFile(wildcardPath, &m_result);
	if(m_searchHandle == INVALID_HANDLE_VALUE)
		m_done = true;
}

IDirectoryIterator::~IDirectoryIterator()
{
	if(m_searchHandle != INVALID_HANDLE_VALUE)
		FindClose(m_searchHandle);
}

void IDirectoryIterator::GetFullPath(char * out, UInt32 outLen)
{
	sprintf_s(out, outLen, "%s\\%s", m_path, m_result.cFileName);
}

std::string IDirectoryIterator::GetFullPath(void)
{
	return std::string(m_path) + "\\" + std::string(m_result.cFileName);
}

void IDirectoryIterator::Next(void)
{
	BOOL	result = FindNextFile(m_searchHandle, &m_result);
	if(!result)
		m_done = true;
}

bool IDirectoryIterator::Done(void)
{
	return m_done;
}
