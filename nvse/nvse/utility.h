#pragma once

#include <intrin.h>
#include "Utilities.h" // for ThisStdCall
#include <bit>

typedef void* (*memcpy_t)(void*, const void*, size_t);
extern memcpy_t _memcpy, _memmove;

//	Workaround for bypassing the compiler calling the d'tor on function-scope objects.
template <typename T, bool InitConstructor = true> class TempObject
{
	friend T;

	struct Buffer
	{
		UInt8	bytes[sizeof(T)];
	}
	objData;

public:
	TempObject()
	{
		if constexpr (InitConstructor)
			Reset();
	}
	TempObject(const T &src) {objData = *(Buffer*)&src;}

	void Reset() {new ((T*)&objData) T();}

	T& operator()() {return *(T*)&objData;}

	TempObject& operator=(const T &rhs) {objData = *(Buffer*)&rhs;}
	TempObject& operator=(const TempObject &rhs) {objData = rhs.objData;}
};

//	Assign rhs to lhs, bypassing operator=
template <typename T> __forceinline void RawAssign(const T &lhs, const T &rhs)
{
	struct Helper
	{
		UInt8	bytes[sizeof(T)];
	};
	*(Helper*)&lhs = *(Helper*)&rhs;
}

//	Swap lhs and rhs, bypassing operator=
template <typename T> __forceinline void RawSwap(const T &lhs, const T &rhs)
{
	struct Helper
	{
		UInt8	bytes[sizeof(T)];
	}
	temp = *(Helper*)&lhs;
	*(Helper*)&lhs = *(Helper*)&rhs;
	*(Helper*)&rhs = temp;
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

	PrimitiveCS *Enter();
	__forceinline void Leave() {m_owningThread = 0;}
};

class PrimitiveScopedLock
{
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
char* __fastcall CopyString(const char* key, UInt32 length);

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

// From JIP
#define PS_DUP_1(a)	a, 0UL, 0UL, 0UL
#define PS_DUP_2(a)	a, a, 0UL, 0UL
#define PS_DUP_3(a)	a, a, a, 0UL
#define PS_DUP_4(a)	a, a, a, a

// From JIP
#define HEX(a) std::bit_cast<UInt32>(a)
#define UBYT(a) *((UInt8*)&a)
#define USHT(a) *((UInt16*)&a)
#define ULNG(a) *((UInt32*)&a)

// From JIP
extern const UInt32 kPackedValues[];

// From JIP
#define GET_PS(i)	((const __m128*)kPackedValues)[i]
#define GET_SS(i)	((const float*)kPackedValues)[i << 2]

// From JIP
#define PS_AbsMask			kPackedValues
#define PS_AbsMask0			kPackedValues+0x10
#define PS_FlipSignMask		kPackedValues+0x20
#define PS_FlipSignMask0	kPackedValues+0x30
#define PS_XYZ0Mask			kPackedValues+0x40
#define PD_AbsMask			kPackedValues+0x50
#define PD_FlipSignMask		kPackedValues+0x60

// From JIP
#define PS_Epsilon			kPackedValues+0x70
#define PS_V3_PId180		kPackedValues+0x80
#define PS_V3_180dPI		kPackedValues+0x90
#define PS_V3_PId2			kPackedValues+0xA0
#define PS_V3_PI			kPackedValues+0xB0
#define PS_V3_PIx2			kPackedValues+0xC0
#define PS_V3_Half			kPackedValues+0xD0
#define PS_V3_One			kPackedValues+0xE0
#define PS_HKUnitCnvrt		kPackedValues+0xF0

// From JIP
#define SS_1d1K				kPackedValues+0x100
#define SS_1d100			kPackedValues+0x104
#define SS_1d10				kPackedValues+0x108
#define SS_1d4				kPackedValues+0x10C
#define SS_3				kPackedValues+0x110
#define SS_10				kPackedValues+0x114
#define SS_100				kPackedValues+0x118

// From JIP
#define FltPId2		1.570796371F
#define FltPI		3.141592741F
#define FltPIx2		6.283185482F
#define FltPId180	0.01745329238F
#define Flt180dPI	57.29578018F
#define DblPId180	0.017453292519943295
#define Dbl180dPI	57.29577951308232