#pragma once

double dCos(double angle);
double dSin(double angle);
double dTan(double angle);

double dAtan(double value);
double dAsin(double value);
double dAcos(double value);
double dAtan2(double y, double x);

void* __fastcall MemCopy(void* dest, const void* src, UInt32 length);

UInt32 __fastcall StrLen(const char* str);

char* __fastcall StrEnd(const char* str);

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

#define AUX_BUFFER_INIT_SIZE 0x8000

class AuxBuffer
{
	UInt8* ptr;
	UInt32 size;

public:
	AuxBuffer() : ptr(NULL), size(AUX_BUFFER_INIT_SIZE) {}
};

extern AuxBuffer s_auxBuffers[3];

UInt8* __fastcall GetAuxBuffer(AuxBuffer& buffer, UInt32 reqSize);

#define GetRandomUInt(n) ThisStdCall<UInt32, UInt32>(0xAA5230, (void*)0x11C4180, n)
