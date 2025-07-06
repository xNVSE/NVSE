#include "NiTypes.h"

// Copied from JIP LN NVSE
__declspec(naked) float __vectorcall Point3Distance(const NiVector3& pt1, const NiVector3& pt2)
{
	__asm
	{
		movaps	xmm2, PS_XYZ0Mask
		movups	xmm0, [ecx]
		andps	xmm0, xmm2
		movups	xmm1, [edx]
		andps	xmm1, xmm2
		subps	xmm0, xmm1
		mulps	xmm0, xmm0
		xorps	xmm1, xmm1
		haddps	xmm0, xmm1
		haddps	xmm0, xmm1
		comiss	xmm0, xmm1
		jz		done
		movaps	xmm1, xmm0
		rsqrtss	xmm2, xmm0
		mulss	xmm1, xmm2
		mulss	xmm1, xmm2
		movss	xmm3, SS_3
		subss	xmm3, xmm1
		mulss	xmm3, xmm2
		mulss	xmm3, PS_V3_Half
		mulss	xmm0, xmm3
	done :
		retn
	}
}

// NiFixedString

NiFixedString::NiFixedString() 
{
	m_kHandle = nullptr;
}

// GAME - 0x438170
NiFixedString::NiFixedString(const char* apcString) 
{
	if (apcString)
		m_kHandle = NiGlobalStringTable::AddString(apcString);
	else
		m_kHandle = nullptr;
}

NiFixedString::NiFixedString(const NiFixedString& arString) 
{
	NiGlobalStringTable::IncRefCount(const_cast<NiGlobalStringTable::GlobalStringHandle&>(arString.m_kHandle));
	m_kHandle = arString.m_kHandle;
}

// GAME - 0x4381B0
NiFixedString::~NiFixedString() 
{
	NiGlobalStringTable::DecRefCount(m_kHandle);
}

// GAME - 0xA2E750
NiFixedString& NiFixedString::operator=(const char* apcString) 
{
	if (m_kHandle != apcString) 
	{
		NiGlobalStringTable::GlobalStringHandle kHandle = m_kHandle;
		m_kHandle = NiGlobalStringTable::AddString(apcString);
		NiGlobalStringTable::DecRefCount(kHandle);
	}
	return *this;
}

NiFixedString& NiFixedString::operator=(const NiFixedString& arString) 
{
	if (m_kHandle != arString.m_kHandle) {
		NiGlobalStringTable::GlobalStringHandle kHandle = arString.m_kHandle;
		NiGlobalStringTable::IncRefCount(kHandle);
		NiGlobalStringTable::DecRefCount(m_kHandle);
		m_kHandle = kHandle;
	}
	return *this;
}

NiFixedString::operator const char* () const 
{
	return m_kHandle;
}

NiFixedString::operator bool() const 
{
	return m_kHandle != nullptr;
}

const char* NiFixedString::c_str() const 
{
	return m_kHandle;
}

uint32_t NiFixedString::GetLength() const 
{
	return NiGlobalStringTable::GetLength(m_kHandle);
}

bool NiFixedString::Includes(const char* apToFind) const 
{
	if (!m_kHandle || !apToFind)
		return false;

	return strstr(m_kHandle, apToFind) != nullptr;
}

bool operator==(const NiFixedString& arString1, const NiFixedString& arString2) 
{
	return arString1.m_kHandle == arString2.m_kHandle;
}

bool operator==(const NiFixedString& arString, const char* apcString) 
{
	if (arString.m_kHandle == apcString)
		return true;

	if (!arString.m_kHandle || !apcString)
		return false;

	return !strcmp(arString.m_kHandle, apcString);
}

bool operator==(const char* apcString, const NiFixedString& arString) 
{
	if (arString.m_kHandle == apcString)
		return true;

	if (!arString.m_kHandle || !apcString)
		return false;

	return !strcmp(arString.m_kHandle, apcString);
}
