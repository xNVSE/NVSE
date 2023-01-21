#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <intrin.h>
#include "Utilities.h"

typedef void* (*memcpy_t)(void*, const void*, size_t);
extern memcpy_t _memcpy, _memmove;

//	Workaround for bypassing the compiler calling the d'tor on function-scope objects.
template <typename T, bool InitConstructor = true> class TempObject {
	friend T;

	struct Buffer { UInt8	bytes[sizeof(T)]; }
	objData;

public:
	TempObject() { if constexpr (InitConstructor) { Reset(); } }
	TempObject(const T &src) { objData = *reinterpret_cast<const Buffer *>(&src); }

	void Reset() {new (reinterpret_cast<T*>(&objData)) T();}

	T& operator()() {return *reinterpret_cast<T*>(&objData);}

	TempObject& operator=(const T &rhs) { objData = *reinterpret_cast<Buffer *>(&rhs); return &objData; }
	TempObject& operator=(const TempObject &rhs) { objData = rhs.objData; return &objData; }
};

//	Assign rhs to lhs, bypassing operator=
template <typename T> __forceinline void RawAssign(T &lhs, T &rhs) {
	struct Helper { UInt8	bytes[sizeof(T)]; };
	*reinterpret_cast<Helper *>(&lhs) = *reinterpret_cast<Helper *>(&rhs); // *(Helper*)&lhs = *(Helper*)&rhs;
}

//	Swap lhs and rhs, bypassing operator=
template <typename T> __forceinline void RawSwap(T &lhs, T &rhs) {
	struct Helper { UInt8	bytes[sizeof(T)]; }
	temp = *reinterpret_cast<Helper *>(&lhs);
	*reinterpret_cast<Helper *>(&lhs) = *reinterpret_cast<Helper *>(&rhs);
	*reinterpret_cast<Helper *>(&rhs) = temp;
}

// These are used for 10h aligning segments in ASM code (massive performance gain, particularly with loops).
#define EMIT(bt) __asm _emit bt
#define NOP_0x1 EMIT(0x90)
#define NOP_0x2 EMIT(0x66) EMIT(0x90)
#define NOP_0x3 EMIT(0x0F) EMIT(0x1F) EMIT(0x00)
#define NOP_0x4 EMIT(0x0F) EMIT(0x1F) EMIT(0x40) EMIT(0x00)
#define NOP_0x5 EMIT(0x0F) EMIT(0x1F) EMIT(0x44) EMIT(0x00) EMIT(0x00)
#define NOP_0x6 EMIT(0x66) EMIT(0x0F) EMIT(0x1F) EMIT(0x44) EMIT(0x00) EMIT(0x00)
#define NOP_0x7 EMIT(0x0F) EMIT(0x1F) EMIT(0x80) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
#define NOP_0x8 EMIT(0x0F) EMIT(0x1F) EMIT(0x84) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
#define NOP_0x9 EMIT(0x66) EMIT(0x0F) EMIT(0x1F) EMIT(0x84) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
#define NOP_0xA NOP_0x5 NOP_0x5
#define NOP_0xB NOP_0x5 NOP_0x6
#define NOP_0xC NOP_0x6 NOP_0x6
#define NOP_0xD NOP_0x6 NOP_0x7
#define NOP_0xE NOP_0x7 NOP_0x7
#define NOP_0xF NOP_0x7 NOP_0x8

class PrimitiveCS
{
	DWORD		m_owningThread;

public:
	PrimitiveCS() : m_owningThread(0) {}

	static PrimitiveCS *Enter();
	__forceinline void Leave() {m_owningThread = 0;}
};

class PrimitiveScopedLock {
	PrimitiveCS		*m_cs;

public:
	PrimitiveScopedLock(PrimitiveCS &cs) : m_cs(&cs) {cs.Enter();}
	~PrimitiveScopedLock() {m_cs->Leave();}
};

class TESForm;
TESForm* __stdcall LookupFormByRefID(UInt32 refID);

int __vectorcall ifloor(float value);

int __vectorcall iceil(float value);

UInt32 __fastcall StrLen(const char* str);

void __fastcall MemZero(void* dest, UInt32 bsize);

char* __fastcall StrCopy(char* dest, const char* src);

char* __fastcall StrNCopy(char* dest, const char* src, UInt32 length);

char* __fastcall StrCat(char* dest, const char* src);

char __fastcall StrCompare(const char* lstr, const char* rstr);

void __fastcall StrToLower(char* str);

char* __fastcall SubStrCI(const char *srcStr, const char *subStr);

char* __fastcall SlashPos(const char *str);

char* __fastcall CopyString(const char* key);

char* __fastcall IntToStr(char *str, int num);

UInt32 __fastcall StrHashCS(const char* inKey);

UInt32 __fastcall StrHashCI(const char* inKey);

class SpinLock
{
	UInt32	owningThread;
	UInt32	enterCount;

public:
	SpinLock() : owningThread(0), enterCount(0) {}

	void Enter();
	void EnterSleep();
	void Leave();
};

#define GetRandomUInt(n) ThisStdCall<UInt32, UInt32>(0xAA5230, (void*)0x11C4180, n)

#endif