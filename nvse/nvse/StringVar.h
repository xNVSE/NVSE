#pragma once
#include "Serialization.h"
#include "GameAPI.h"
#include "VarMap.h"

// String changes layout:
//
//	STVS - empty chunk indicating start of strings block
//		STVR
//			UInt8 modIndex
//			UInt32 stringID
//			UInt16 length
//			char data[length]
//		[STVR]
//		...
//	STVE - empty chunk indicating end of strings block
//
// Strings are discarded on load if the mod which created them is no longer present.

class StringVar
{
	std::string data;
	UInt8		owningModIndex;
public:
	StringVar(const char* in_data, UInt8 modIndex);

	StringVar(const StringVar& other) = delete;

	StringVar(StringVar&& other) noexcept;

	StringVar& operator=(const StringVar& other) = delete;

	StringVar& operator=(StringVar&& other) noexcept
	{
		if (this == &other)
			return *this;
		data = std::move(other.data);
		owningModIndex = other.owningModIndex;
		return *this;
	}

	// length of newString should respect kMaxMessageLength
	void		Set(const char* newString);
	void        Set(StringVar&& other);
	SInt32		Compare(char* rhs, bool caseSensitive);
	void		Insert(const char* subString, UInt32 insertionPos);
	UInt32		Find(char* subString, UInt32 startPos, UInt32 numChars, bool bCaseSensitive = false);	//returns position of substring
	UInt32		Count(char* subString, UInt32 startPos, UInt32 numChars, bool bCaseSensitive = false);
	UInt32		Replace(const char* toReplace, const char* replaceWith, UInt32 startPos, UInt32 numChars, bool bCaseSensitive, UInt32 numToReplace = -1);	//returns num replaced
	void		Erase(UInt32 startPos, UInt32 numChars);
	std::string	SubString(UInt32 startPos, UInt32 numChars);
	char		At(UInt32 charPos);
	static UInt32	GetCharType(char ch);
	void Trim();

	std::string String()					{	return data;	}
	std::string& StringRef() {return data;}
	const char*	GetCString();
	UInt32		GetLength();
	UInt8		GetOwningModIndex();	
};

enum {
	kCharType_Alphabetic	= 1 << 0,
	kCharType_Digit			= 1 << 1,
	kCharType_Punctuation	= 1 << 2,
	kCharType_Printable		= 1 << 3,
	kCharType_Uppercase		= 1 << 4,
};

class StringVarMap : public VarMap<StringVar>
{
public:
	void Save(NVSESerializationInterface* intfc);
	void Load(NVSESerializationInterface* intfc);
	void Clean();
	void Reset();
	UInt32 Add(UInt8 varModIndex, const char* data, bool bTemp = false, StringVar** svOut = nullptr);
	UInt32 Add(StringVar&& moveVar, bool bTemp, StringVar** svOut);
	static StringVarMap * GetSingleton(void);
	void Delete(UInt32 varID);
	void MarkTemporary(UInt32 varID, bool bTemporary);
};

extern StringVarMap g_StringMap;

bool AssignToStringVar(COMMAND_ARGS, const char* newValue);
bool IsFunctionResultCacheString(UInt32 strId);
bool AssignToStringVarLong(COMMAND_ARGS, const char* newValue);	// Increase the call count in the stack

namespace PluginAPI
{
	const char* GetString(UInt32 stringID);
	void SetString(UInt32 stringID, const char* newVal);
	UInt32 CreateString(const char* strVal, void* owningScript);
}

struct FunctionResultStringVar
{
	StringVar* var = nullptr;
	int id = -1;
	bool inUse = false;
};

__declspec(noinline) FunctionResultStringVar& GetFunctionResultCachedStringVar();
