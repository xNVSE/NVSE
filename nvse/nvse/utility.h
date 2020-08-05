#pragma once
#include <cfloat>
#include <cstdio>
#include <Windows.h>
#include <ctime>

typedef unsigned char		UInt8;		//!< An unsigned 8-bit integer value
typedef unsigned short		UInt16;		//!< An unsigned 16-bit integer value
typedef unsigned long		UInt32;		//!< An unsigned 32-bit integer value
typedef unsigned long long	UInt64;		//!< An unsigned 64-bit integer value
typedef signed char			SInt8;		//!< A signed 8-bit integer value
typedef signed short		SInt16;		//!< A signed 16-bit integer value
typedef signed long			SInt32;		//!< A signed 32-bit integer value
typedef signed long long	SInt64;		//!< A signed 64-bit integer value

const double
kDblZero = 0,
kDblOne = 1,
kDblPI = 3.141592653589793,
kDblPIx2 = 6.283185307179586,
kDblPIx3d2 = 4.71238898038469,
kDblPId2 = 1.5707963267948966,
kDblPId4 = 0.7853981633974483,
kDblPId6 = 0.5235987755982989,
kDblPId12 = 0.26179938779914946,
kDbl2dPI = 0.6366197723675814,
kDbl4dPI = 1.2732395447351628,
kDblTanPId6 = 0.5773502691896257,
kDblTanPId12 = 0.2679491924311227,
kDblPId180 = 0.017453292519943295;

const float
kFltZero = 0.0F,
kFltDot1 = 0.1F,
kFltHalf = 0.5F,
kFltOne = 1.0F,
kFltTwo = 2.0F,
kFltFour = 4.0F,
kFltFive = 5.0F,
kFltSix = 6.0F,
kFlt10 = 10.0F,
kFlt100 = 100.0F,
kFlt1000 = 1000.0F,
kFlt2048 = 2048.0F,
kFlt4096 = 4096.0F,
kFlt10000 = 10000.0F,
kFlt12288 = 12288.0F,
kFlt40000 = 40000.0F,
kFltMax = FLT_MAX;

union Coordinate
{
	UInt32		xy;
	struct
	{
		SInt16	y;
		SInt16	x;
	};

	Coordinate() {}
	Coordinate(SInt16 _x, SInt16 _y) : x(_x), y(_y) {}

	inline Coordinate& operator =(const Coordinate& rhs)
	{
		xy = rhs.xy;
		return *this;
	}
	inline Coordinate& operator =(const UInt32& rhs)
	{
		xy = rhs;
		return *this;
	}

	inline bool operator ==(const Coordinate& rhs) { return xy == rhs.xy; }
	inline bool operator !=(const Coordinate& rhs) { return xy != rhs.xy; }

	inline Coordinate operator +(const char* rhs)
	{
		return Coordinate(x + rhs[0], y + rhs[1]);
	}
};

template <typename T1, typename T2> inline T1 GetMin(T1 value1, T2 value2)
{
	return (value1 < value2) ? value1 : value2;
}

template <typename T1, typename T2> inline T1 GetMax(T1 value1, T2 value2)
{
	return (value1 > value2) ? value1 : value2;
}

template <typename T> inline T sqr(T value)
{
	return value * value;
}

UInt32 __fastcall GetNextPrime(UInt32 num);

UInt32 __fastcall RGBHexToDec(UInt32 rgb);

UInt32 __fastcall RGBDecToHex(UInt32 rgb);

void* __fastcall MemCopy(void* dest, const void* src, UInt32 length);

UInt32 __fastcall StrLen(const char* str);

char* __fastcall StrEnd(const char* str);

bool __fastcall MemCmp(const void* ptr1, const void* ptr2, UInt32 bsize);

void __fastcall MemZero(void* dest, UInt32 bsize);

char* __fastcall StrCopy(char* dest, const char* src);

char* __fastcall StrNCopy(char* dest, const char* src, UInt32 length);

char* __fastcall StrLenCopy(char* dest, const char* src, UInt32 length);

char* __fastcall StrCat(char* dest, const char* src);

UInt32 __fastcall StrHash(const char* inKey);

bool __fastcall CmprLetters(const char* lstr, const char* rstr);

bool __fastcall StrEqualCS(const char* lstr, const char* rstr);

bool __fastcall StrEqualCI(const char* lstr, const char* rstr);

char __fastcall StrCompare(const char* lstr, const char* rstr);

char __fastcall StrBeginsCS(const char* lstr, const char* rstr);

char __fastcall StrBeginsCI(const char* lstr, const char* rstr);

void __fastcall FixPath(char* str);

void __fastcall StrToLower(char* str);

void __fastcall ReplaceChr(char* str, char from, char to);

char* __fastcall FindChr(const char* str, char chr);

char* __fastcall FindChrR(const char* str, UInt32 length, char chr);

char* __fastcall SubStr(const char* srcStr, const char* subStr);

char* __fastcall SlashPos(const char* str);

char* __fastcall SlashPosR(const char* str);

char* __fastcall GetNextToken(char* str, char delim);

char* __fastcall GetNextToken(char* str, const char* delims);

char* __fastcall CopyString(const char* key);

char* __fastcall IntToStr(char* str, int num);

int FltToStr(char* str, double num);

int __fastcall StrToInt(const char* str);

float __fastcall StrToFlt(const char* str);

char* __fastcall UIntToHex(char* str, UInt32 num);

UInt32 __fastcall HexToUInt(const char* str);

int __stdcall GetRandomIntInRange(int from, int to);

const char kIndentLevelStr[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

bool __fastcall FileToBuffer(const char* filePath, char* buffer);

void ClearFolder(char* pathEndPtr);

// 10 bytes
void __stdcall WritePushRetRelJump(UInt32 baseAddr, UInt32 retAddr, UInt32 jumpTgt);

void __fastcall GetTimeStamp(char* buffer);

void DumpMemImg(void* data, UInt32 size, UInt8 extra = 0);

typedef long long int64; typedef unsigned long long uint64;

UInt32 __fastcall GetHighBit(UInt32 num);