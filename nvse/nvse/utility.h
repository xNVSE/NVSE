#pragma once

typedef void* (*memcpy_t)(void*, const void*, size_t);
extern memcpy_t _memcpy, _memmove;

//	Workaround for bypassing the compiler calling the d'tor on function-scope objects.
template <typename T> class TempObject
{
	friend T;

	struct Buffer
	{
		UInt8	bytes[sizeof(T)];
	}
	objData;

public:
	TempObject() {Reset();}
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

// These are used for 10h aligning segments in ASM code (massive performance gain, particularly with loops). VS compiler doesn't support multi-byte NOP.
#define NOP_0x2 __asm fnop
#define NOP_0x3 __asm lea esp, [esp]
#define NOP_0x4 __asm test ax, 0
#define NOP_0x5 __asm test eax, 0
#define NOP_0x6 __asm test esp, 0
#define NOP_0x7 __asm test dword ptr [esp], 0
#define NOP_0x8 __asm test dword ptr [esp-4], 0
#define NOP_0x9 NOP_0x4 NOP_0x5
#define NOP_0xA NOP_0x5 NOP_0x5
#define NOP_0xB NOP_0x5 NOP_0x6
#define NOP_0xC NOP_0x6 NOP_0x6
#define NOP_0xD NOP_0x6 NOP_0x7
#define NOP_0xE NOP_0x7 NOP_0x7
#define NOP_0xF NOP_0x7 NOP_0x8

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

#define GetRandomUInt(n) ThisStdCall<UInt32, UInt32>(0xAA5230, (void*)0x11C4180, n)
