#pragma once
#include <string>

extern char* s_strArgBuffer;

class Script;

void DumpClass(void * theClassPtr, UInt32 nIntsToDump = 512);
const char * GetObjectClassName(void * obj);
const std::string & GetFalloutDirectory(void);
std::string GetNVSEConfigOption(const char * section, const char * key);
bool GetNVSEConfigOption_UInt32(const char * section, const char * key, UInt32 * dataOut);

// this has been tested to work for non-varargs functions
// varargs functions end up with 'this' passed as the last parameter (ie. probably broken)
// do NOT use with classes that have multiple inheritance

// if many member functions are to be declared, use MEMBER_FN_PREFIX to create a type with a known name
// so it doesn't need to be restated throughout the member list

// all of the weirdness with the _GetType function is because you can't declare a static const pointer
// inside the class definition. inlining automatically makes the function call go away since it's a const

#define MEMBER_FN_PREFIX(className)	\
	typedef className _MEMBER_FN_BASE_TYPE

#define DEFINE_MEMBER_FN_LONG(className, functionName, retnType, address, ...)		\
	typedef retnType (className::* _##functionName##_type)(__VA_ARGS__);			\
																					\
	inline _##functionName##_type * _##functionName##_GetPtr(void)					\
	{																				\
		static const UInt32 _address = address;										\
		return (_##functionName##_type *)&_address;									\
	}

#define DEFINE_MEMBER_FN(functionName, retnType, address, ...)	\
	DEFINE_MEMBER_FN_LONG(_MEMBER_FN_BASE_TYPE, functionName, retnType, address, __VA_ARGS__)

#define CALL_MEMBER_FN(obj, fn)	\
	((*(obj)).*(*((obj)->_##fn##_GetPtr())))


// ConsolePrint() limited to 512 chars; use this to print longer strings to console
void Console_Print_Long(const std::string& str);

// Macro for debug output to console at runtime
#if RUNTIME
#ifdef _DEBUG
#define DEBUG_PRINT(x, ...) { Console_Print((x), __VA_ARGS__); }
#define DEBUG_MESSAGE(x, ...) { _MESSAGE((x), __VA_ARGS__); }
#else
#define DEBUG_PRINT(x, ...) { }
#define DEBUG_MESSAGE(x, ...) { }
#endif	//_DEBUG
#else
#define DEBUG_PRINT(x, ...) { }
#define DEBUG_MESSAGE(x, ...) { }
// This is so we don't have to handle size change with EditorData :)
#undef STATIC_ASSERT
#define STATIC_ASSERT(a)
#endif	// RUNTIME

#define SIZEOF_ARRAY(arrayName, elementType) (sizeof(arrayName) / sizeof(elementType))

class TESForm;

class FormMatcher
{
public:
	virtual bool Matches(TESForm* pForm) const = 0;
};

namespace MersenneTwister
{

	/* initializes mt[N] with a seed */
	void init_genrand(unsigned long s);

	/* initialize by an array with array-length */
	void init_by_array(unsigned long init_key[], int key_length);

	/* generates a random number on [0,0xffffffff]-interval */
	unsigned long genrand_int32(void);

	/* generates a random number on [0,0x7fffffff]-interval */
	long genrand_int31(void);

	/* generates a random number on [0,1]-real-interval */
	double genrand_real1(void);

	/* generates a random number on [0,1)-real-interval */
	double genrand_real2(void);

	/* generates a random number on (0,1)-real-interval */
	double genrand_real3(void);

	/* generates a random number on [0,1) with 53-bit resolution*/
	double genrand_res53(void);

};

// alternative to strtok; doesn't modify src string, supports forward/backward iteration
class Tokenizer
{
public:
	Tokenizer(const char* src, const char* delims);
	~Tokenizer();

	// these return the offset of token in src, or -1 if no token
	UInt32 NextToken(std::string& outStr);
	UInt32 PrevToken(std::string& outStr);

private:
	std::string m_delims;
	size_t		m_offset;
	std::string m_data;
};

#if RUNTIME

const char GetSeparatorChar(Script * script);
const char * GetSeparatorChars(Script * script);

#endif

bool ci_equal(char ch1, char ch2);
bool ci_less(const char* lh, const char* rh);
void MakeUpper(std::string& str);
void MakeUpper(char* str);
void MakeLower(std::string& str);

// Generic error/warning output
// provides a common way to output errors and warnings
class ErrOutput
{
	typedef void (* _ShowError)(const char* msg);
	typedef bool (* _ShowWarning)(const char* msg);		// returns true if user requests to disable warning

	_ShowError		ShowError;
	_ShowWarning	ShowWarning;
public:
	ErrOutput(_ShowError errorFunc, _ShowWarning warningFunc);

	struct Message
	{
		const char*		fmt;
		bool			bCanDisable;
		bool			bDisabled;
	};

	void Show(Message msg, ...);
	void Show(const char* msg, ...);
	void vShow(Message& msg, va_list args);
	void vShow(const char* msg, va_list args);
};

char* __fastcall CopyCString(const char* src);

// thread-safe template versions of ThisStdCall()

__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(); } u = { _f };
    return ((T*)_t->*u.m)();
}

template <typename T1>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1); } u = { _f };
    return ((T*)_t->*u.m)(a1);
}

template <typename T1,typename T2>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1,T2 a2)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1,T2); } u = { _f };
    return ((T*)_t->*u.m)(a1,a2);
}

template <typename T1,typename T2,typename T3>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1,T2 a2,T3 a3)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1,T2,T3); } u = { _f };
    return ((T*)_t->*u.m)(a1,a2,a3);
}

template <typename T1,typename T2,typename T3,typename T4>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1,T2 a2,T3 a3,T4 a4)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1,T2,T3,T4); } u = { _f };
    return ((T*)_t->*u.m)(a1,a2,a3,a4);
}

template <typename T1,typename T2,typename T3,typename T4,typename T5>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1,T2 a2,T3 a3,T4 a4,T5 a5)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1,T2,T3,T4,T5); } u = { _f };
    return ((T*)_t->*u.m)(a1,a2,a3,a4,a5);
}

template <typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
__forceinline UInt32 ThisStdCall(UInt32 _f,void* _t,T1 a1,T2 a2,T3 a3,T4 a4,T5 a5, T6 a6)
{
    class T {}; union { UInt32 x; UInt32 (T::*m)(T1,T2,T3,T4,T5,T6); } u = { _f };
    return ((T*)_t->*u.m)(a1,a2,a3,a4,a5,a6);
}


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
kFlt2048 = 2048.0F,
kFlt4096 = 4096.0F,
kFlt10000 = 10000.0F,
kFlt12288 = 12288.0F,
kFlt40000 = 40000.0F,
kFltMax = FLT_MAX;

#define GameHeapAlloc(size) ThisStdCall(0xAA3E40, (void*)0x11F6238, size)
#define GameHeapFree(ptr) ThisStdCall(0xAA4060, (void*)0x11F6238, ptr)

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

bool fCompare(float lval, float rval);

int __stdcall lfloor(float value);
int __stdcall lceil(float value);

float __stdcall fSqrt(float value);
double __stdcall dSqrt(double value);

double dCos(double angle);
double dSin(double angle);
double dTan(double angle);

double dAtan(double value);
double dAsin(double value);
double dAcos(double value);
double dAtan2(double y, double x);

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

bool __fastcall FileExists(const char* path);

class FileStream
{
	FILE* theFile;

public:
	FileStream() : theFile(NULL) {}
	~FileStream() { if (theFile) fclose(theFile); }

	bool Open(const char* filePath);
	bool OpenAt(const char* filePath, UInt32 inOffset);
	bool OpenWrite(char* filePath, bool append);
	bool Create(const char* filePath);
	void SetOffset(UInt32 inOffset);

	void Close()
	{
		fclose(theFile);
		theFile = NULL;
	}

	UInt32 GetLength();
	char ReadChar();
	void ReadBuf(void* outData, UInt32 inLength);
	void WriteChar(char chr);
	void WriteStr(const char* inStr);
	void WriteBuf(const void* inData, UInt32 inLength);

	static void MakeAllDirs(char* fullPath);
};

const char kIndentLevelStr[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

class DebugLog
{
	FILE* theFile;
	UInt32			indent;

public:
	DebugLog() : theFile(NULL), indent(40) {}
	~DebugLog() { if (theFile) fclose(theFile); }

	bool Create(const char* filePath);
	void Message(const char* msgStr);
	void FmtMessage(const char* fmt, va_list args);
	void Indent() { if (indent) indent--; }
	void Outdent() { if (indent < 40) indent++; }
};

//void PrintLog(const char* fmt, ...);
//void PrintDebug(const char* fmt, ...);

class LineIterator
{
	char* dataPtr;

public:
	LineIterator(const char* filePath, char* buffer);

	bool End() const { return *dataPtr == 3; }
	void Next();
	char* Get() { return dataPtr; }
};

class DirectoryIterator
{
	HANDLE				handle;
	WIN32_FIND_DATA		fndData;

public:
	DirectoryIterator(const char* path) : handle(FindFirstFile(path, &fndData)) {}
	~DirectoryIterator() { Close(); }

	bool End() const { return handle == INVALID_HANDLE_VALUE; }
	void Next() { if (!FindNextFile(handle, &fndData)) Close(); }
	bool IsFile() const { return !(fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); }
	bool IsFolder() const
	{
		if (IsFile()) return false;
		if (fndData.cFileName[0] != '.') return true;
		if (fndData.cFileName[1] != '.') return fndData.cFileName[1] != 0;
		return fndData.cFileName[2] != 0;
	}
	const char* Get() const { return fndData.cFileName; }
	void Close()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
};

bool __fastcall FileToBuffer(const char* filePath, char* buffer);

void ClearFolder(char* pathEndPtr);

/*void __stdcall SafeWrite8(UInt32 addr, UInt32 data);
void __stdcall SafeWrite16(UInt32 addr, UInt32 data);
void __stdcall SafeWrite32(UInt32 addr, UInt32 data);
void __stdcall SafeWriteBuf(UInt32 addr, void* data, UInt32 len);

// 5 bytes
void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void __stdcall WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt);*/

// 10 bytes
void __stdcall WritePushRetRelJump(UInt32 baseAddr, UInt32 retAddr, UInt32 jumpTgt);

void __fastcall GetTimeStamp(char* buffer);

const char* __fastcall GetDXDescription(UInt32 keyID);

UInt32 __fastcall ByteSwap(UInt32 dword);

void DumpMemImg(void* data, UInt32 size, UInt8 extra = 0);

//void GetMD5File(const char *filePath, char *outHash);