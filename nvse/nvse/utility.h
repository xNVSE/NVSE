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

UInt32 __fastcall StrLen(const char* str);

bool __fastcall MemCmp(const void* ptr1, const void* ptr2, UInt32 bsize);

void __fastcall MemZero(void* dest, UInt32 bsize);

char* __fastcall StrCopy(char* dest, const char* src);

char* __fastcall StrNCopy(char* dest, const char* src, UInt32 length);

char* __fastcall StrCat(char* dest, const char* src);

bool __fastcall StrEqualCS(const char* lstr, const char* rstr);

bool __fastcall StrEqualCI(const char* lstr, const char* rstr);

char __fastcall StrCompare(const char* lstr, const char* rstr);

char __fastcall StrBeginsCS(const char* lstr, const char* rstr);

char __fastcall StrBeginsCI(const char* lstr, const char* rstr);

void __fastcall StrToLower(char* str);

void __fastcall ReplaceChr(char* str, char from, char to);

char* __fastcall FindChr(const char* str, char chr);

char* __fastcall FindChrR(const char* str, UInt32 length, char chr);

char* __fastcall SubStrCS(const char *srcStr, const char *subStr);

char* __fastcall SubStrCI(const char *srcStr, const char *subStr);

char* __fastcall SlashPos(const char *str);

char* __fastcall CopyString(const char* key);

char* __fastcall IntToStr(char *str, int num);

UInt32 __fastcall StrHashCS(const char* inKey);

UInt32 __fastcall StrHashCI(const char* inKey);

#define GetRandomUInt(n) ThisStdCall<UInt32, UInt32>(0xAA5230, (void*)0x11C4180, n)
