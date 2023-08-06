#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <memory>

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

// Calls Print_Long or Print depending on the size of the string.
void Console_Print_Str(const std::string& str);

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
	~Tokenizer() = default;

	// these return the offset of token in src, or -1 if no token
	UInt32 NextToken(std::string& outStr);
	std::string ToNewLine();
	UInt32 PrevToken(std::string& outStr);

private:
	std::string m_delims;
	size_t		m_offset;
	std::string m_data;
};


// For parsing lexical tokens inside script text line-by-line, while skipping over those inside comments.
// Strings are passed as a single token (including the '"' characters).
// Everything else will have to be manually handled.
class ScriptTokenizer
{
public:
	ScriptTokenizer(std::string_view scriptText);
	~ScriptTokenizer() = default;

	// Returns true if a new line could be read, false at the end of the script.
	// Skips over commented-out lines and empty lines.
	[[nodiscard]] bool TryLoadNextLine();

	// Gets the next space-separated token from the loaded line, ignoring tokens placed inside comments.
	// Returns an empty string_view if no line is loaded / end-of-line is reached.
	std::string_view GetNextLineToken();

	// Gets the entire line, for manual tokenizing.
	// Returns an empty string_view if no line is loaded.
	std::string_view GetLineText();

private:
	std::string_view m_scriptText;
	size_t			 m_scriptOffset = 0;
	std::vector<std::string_view> m_loadedLineTokens;
	size_t			 m_tokenOffset = 0;
	bool			 m_inMultilineComment = false;
};

#if RUNTIME

const char GetSeparatorChar(Script * script);
const char * GetSeparatorChars(Script * script);

#endif

const char * GetDXDescription(UInt32 keycode);

bool ci_equal(char ch1, char ch2);
bool ci_less(const char* lh, const char* rh);
void MakeUpper(std::string& str);
void MakeUpper(char* str);
void MakeLower(std::string& str);

// this copies the string onto the FormHeap - used to work around alloc/dealloc mismatch when passing
// data between nvse and plugins
char* CopyCString(const char* src);

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

// thread-safe template versions of ThisStdCall()

template <typename T_Ret = UInt32, typename ...Args>
__forceinline T_Ret ThisStdCall(UInt32 _addr, const void *_this, Args ...args)
{
	return ((T_Ret (__thiscall *)(const void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret StdCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__stdcall *)(Args...))_addr)(std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__cdecl *)(Args...))_addr)(std::forward<Args>(args)...);
}

void ShowErrorMessageBox(const char* message);

#if RUNTIME

const char* GetModName(TESForm* form);

void ShowRuntimeError(Script* script, const char* fmt, ...);

#endif

std::string FormatString(const char* fmt, ...);

#if EDITOR
void GeckExtenderMessageLog(const char* fmt, ...);
#endif

std::vector<void*> GetCallStack(int i);

bool FindStringCI(const std::string& strHaystack, const std::string& strNeedle);
void ReplaceAll(std::string &str, const std::string& from, const std::string& to);

extern bool g_warnScriptErrors;

template <typename L, typename T>
bool Contains(const L& list, T t)
{
	for (auto i : list)
	{
		if (i == t)
			return true;
	}
	return false;
}

template <typename T>
bool Contains(std::initializer_list<T> list, const T& t)
{
	for (auto i : list)
	{
		if (i == t)
			return true;
	}
	return false;
}

#define _L(x, y) [&](x) {return y;}
namespace ra = std::ranges;

bool IsProcessRunning(const char* processName);

void DisplayMessage(const char* msg);

std::string GetCurPath();

bool ValidString(const char* str);

#if _DEBUG


const char* GetFormName(TESForm* form);
const char* GetFormName(UInt32 formId);


#endif

typedef void* (*_FormHeap_Allocate)(UInt32 size);
extern const _FormHeap_Allocate FormHeap_Allocate;

template <typename T, const UInt32 ConstructorPtr = 0, typename... Args>
T* New(Args &&... args)
{
	auto* alloc = FormHeap_Allocate(sizeof(T));
	if constexpr (ConstructorPtr)
	{
		ThisStdCall(ConstructorPtr, alloc, std::forward<Args>(args)...);
	}
	else
	{
		memset(alloc, 0, sizeof(T));
	}
	return static_cast<T*>(alloc);
}

typedef void (*_FormHeap_Free)(void* ptr);
extern const _FormHeap_Free FormHeap_Free;

template <typename T, const UInt32 DestructorPtr = 0, typename... Args>
void Delete(T* t, Args &&... args)
{
	if constexpr (DestructorPtr)
	{
		ThisStdCall(DestructorPtr, t, std::forward<Args>(args)...);
	}
	FormHeap_Free(t);
}

template <typename T>
using game_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T, const UInt32 DestructorPtr = 0>
game_unique_ptr<T> MakeUnique(T* t)
{
	return game_unique_ptr<T>(t, [](T* t2) { Delete<T, DestructorPtr>(t2); });
}

template <typename T, const UInt32 ConstructorPtr = 0, const UInt32 DestructorPtr = 0, typename... ConstructorArgs>
game_unique_ptr<T> MakeUnique(ConstructorArgs &&... args)
{
	auto* obj = New<T, ConstructorPtr>(std::forward(args)...);
	return MakeUnique<T, DestructorPtr>(obj);
}

bool StartsWith(std::string left, std::string right);
std::string& ToLower(std::string&& data);
std::string& StripSpace(std::string&& data);
std::vector<std::string> SplitString(std::string s, std::string delimiter);

#define INLINE_HOOK(retnType, callingConv, ...) static_cast<retnType(callingConv*)(__VA_ARGS__)>([](__VA_ARGS__) [[msvc::forceinline]] -> retnType

UInt8* GetParentBasePtr(void* addressOfReturnAddress, bool lambda = false);

//Example in https://en.cppreference.com/w/cpp/utility/variant/visit
//Allows function overloading with c++ lambdas.
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

#if RUNTIME
inline int __cdecl game_toupper(int _C) { return CdeclCall<int>(0xECA7F4, _C); }
inline int __cdecl game_tolower(int _C) { return CdeclCall<int>(0xEC67AA, _C); }
#else 
// GECK and other non-runtime code (ex: steam_loader) probably don't need localized stuff...?
inline int __cdecl game_toupper(int _C) { return toupper(_C); }
inline int __cdecl game_tolower(int _C) { return tolower(_C); }
#endif