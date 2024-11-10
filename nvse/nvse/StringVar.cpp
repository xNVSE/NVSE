#include <string>
#include "StringVar.h"
#include "GameForms.h"
#include <algorithm>
#include "GameScript.h"
#include "Hooks_Script.h"
#include "ScriptUtils.h"
#include "GameData.h"
#include "GameApi.h"
#include <set>

#include "Core_Serialization.h"

StringVar::StringVar(const char* in_data, UInt8 modIndex)
{
	data = in_data;
	owningModIndex = modIndex;
}

StringVar::StringVar(StringVar&& other) noexcept: data(std::move(other.data)),
                                                  owningModIndex(other.owningModIndex)
{
}

StringVarMap* StringVarMap::GetSingleton()
{
	return &g_StringMap;
}

void StringVarMap::Delete(UInt32 varID)
{
	if (!IsFunctionResultCacheString(varID))
		VarMap<StringVar>::Delete(varID);
	else
	{
#if _DEBUG
		DebugBreak();
#endif
		tempIDs.Erase(varID);
	}
#if _DEBUG && 0
	else
		DebugBreak();
#endif
}

void StringVarMap::MarkTemporary(UInt32 varID, bool bTemporary)
{
	if (IsFunctionResultCacheString(varID) && bTemporary)
	{
#if _DEBUG
		DebugBreak();
#endif
		return;
	}
#if _DEBUG
	if (auto* strVar = Get(varID))
		strVar->temporary = bTemporary;
#endif
	VarMap<StringVar>::MarkTemporary(varID, bTemporary);
}

const char* StringVar::GetCString()
{
	return data.c_str();
}

void StringVar::Set(const char* newString)
{
	data = newString;
}

void StringVar::Set(StringVar&& other)
{
	data = std::move(other.StringRef());
}

SInt32 StringVar::Compare(char* rhs, bool caseSensitive)
{
	return caseSensitive ? strcmp(rhs, data.c_str()) : StrCompare(rhs, data.c_str());
}

void StringVar::Insert(const char* subString, UInt32 insertionPos)
{
	if (insertionPos < GetLength())
		data.insert(insertionPos, subString);
	else if (insertionPos == GetLength())
		data.append(subString);
}

#pragma warning(disable : 4996)	// disable checked iterator warning for std::transform with char*
UInt32 StringVar::Find(char* subString, UInt32 startPos, UInt32 numChars, bool bCaseSensitive)
{
	UInt32 pos = -1;

	if (numChars + startPos >= GetLength())
		numChars = GetLength() - startPos;

	if (startPos < GetLength())
	{
		std::string source = data.substr(startPos, numChars);
		if (!bCaseSensitive)
		{
			std::transform(source.begin(), source.end(), source.begin(), tolower);
			std::transform(subString, subString + strlen(subString), subString, tolower);
		}

		 //pos = data.substr(startPos, numChars).find(subString);	//returns -1 if not found
		pos = source.find(subString);
		if (pos != -1)
			pos += startPos;
	}

	return pos;
}

UInt32 StringVar::Count(char* subString, UInt32 startPos, UInt32 numChars, bool bCaseSensitive)
{
	if (numChars + startPos >= GetLength())
		numChars = GetLength() - startPos;

	if (startPos >= GetLength())
		return 0;

	std::string source = data.substr(startPos, numChars);	//only count occurences beginning before endPos
	UInt32 subStringLen = strlen(subString);
	if (!subStringLen)
		return 0;

	if (!bCaseSensitive)
	{
		std::transform(source.begin(), source.end(), source.begin(), tolower);
		std::transform(subString, subString + strlen(subString), subString, tolower);
	}

	UInt32 strIdx = 0;
	UInt32 count = 0;
	while (strIdx < GetLength() && ((strIdx = source.find(subString, strIdx)) != -1))
	{
		count++;
		strIdx += subStringLen;
	}

	return count;
}
#pragma warning(default : 4996)

UInt32 StringVar::GetLength()
{
	return data.length();
}

UInt32 StringVar::Replace(const char* toReplace, const char* replaceWith, UInt32 startPos, UInt32 numChars, bool bCaseSensitive, UInt32 numToReplace)
{
	// calc length of substring
	if (startPos >= GetLength())
		return 0;
	else if (numChars + startPos > GetLength())
		numChars = GetLength() - startPos;

	UInt32 numReplaced = 0;
	UInt32 replacementLen = strlen(replaceWith);
	UInt32 toReplaceLen = strlen(toReplace);

	// create substring
	std::string srcStr = data.substr(startPos, numChars);

	// remove substring from original string
	data.erase(startPos, numChars);

	UInt32 strIdx = 0;
	while (numReplaced < numToReplace)// && (strIdx = srcStr.find(toReplace, strIdx)) != -1)
	{
		if (bCaseSensitive)
		{
			strIdx = srcStr.find(toReplace, strIdx);
			if (strIdx == -1)
				break;
		}
		else
		{
			std::string strToReplace = toReplace;
			std::string::iterator iter = std::search(srcStr.begin() + strIdx, srcStr.end(), strToReplace.begin(), strToReplace.end(), ci_equal);
			if (iter != srcStr.end())
				strIdx = iter - srcStr.begin();
			else
				break;
		}

		numReplaced++;
		srcStr.erase(strIdx, toReplaceLen);
		if (strIdx == srcStr.length())
		{
			srcStr.append(replaceWith);
			break;						// reached end of string so all done
		}
		else
		{
			srcStr.insert(strIdx, replaceWith);
			strIdx += replacementLen;
		}
	}

	// paste altered string back into original string
	if (startPos == GetLength())
		data.append(srcStr);
	else
		data.insert(startPos, srcStr);

	return numReplaced;
}

void StringVar::Erase(UInt32 startPos, UInt32 numChars)
{
	if (numChars + startPos >= GetLength())
		numChars = GetLength() - startPos;

	if (startPos < GetLength())
		data.erase(startPos, numChars);
}

std::string StringVar::SubString(UInt32 startPos, UInt32 numChars)
{
	if (numChars + startPos >= GetLength())
		numChars = GetLength() - startPos;

	if (startPos < GetLength())
		return data.substr(startPos, numChars);
	else
		return "";
}

UInt8 StringVar::GetOwningModIndex()
{
	return owningModIndex;
}

UInt32 StringVar::GetCharType(char ch)
{
	UInt32 charType = 0;
	if (isalpha(ch))
		charType |= kCharType_Alphabetic;
	if (isdigit(ch))
		charType |= kCharType_Digit;
	if (ispunct(ch))
		charType |= kCharType_Punctuation;
	if (isprint(ch))
		charType |= kCharType_Printable;
	if (isupper(ch))
		charType |= kCharType_Uppercase;

	return charType;
}

// Trims whitespace at beginning and end of string
void StringVar::Trim()
{
	// ltrim
	data.erase(data.begin(), ra::find_if(data, [](unsigned char ch) {
		return !std::isspace(ch);
	}));

	// rtrim
	data.erase(std::find_if(data.rbegin(), data.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), data.end());
}

char StringVar::At(UInt32 charPos)
{
	if (charPos < GetLength())
		return data[charPos];
	else
		return -1;
}

void StringVarMap::Save(NVSESerializationInterface* intfc)
{
	Clean();

	Serialization::OpenRecord('STVS', 0);

	for (auto iter = vars.Begin(); !iter.End(); ++iter)
	{
		if (IsTemporary(iter.Key()))	// don't save temp strings
			continue;
		StringVar* var = &iter.Get();
		if (var->GetOwningModIndex() == 0xFF)
			continue; // do not save function result cache
		Serialization::OpenRecord('STVR', 0);
		Serialization::WriteRecord8(var->GetOwningModIndex());
		Serialization::WriteRecord32(iter.Key());
		UInt16 len = var->GetLength();
		Serialization::WriteRecord16(len);
		Serialization::WriteRecordData(var->GetCString(), len);
	}

	Serialization::OpenRecord('STVE', 0);
}

#if _DEBUG
extern std::set<std::string> g_modsWithCosaveVars;
#endif

void StringVarMap::Load(NVSESerializationInterface* intfc)
{
	_MESSAGE("Loading strings");
	UInt32 type, length, version, stringID, tempRefID;
	UInt16 strLength;
	UInt8 modIndex;
	char buffer[kMaxMessageLength];

	Clean();

	// do some basic checking to weed out potential bloat caused by scripts creating large
	// numbers of string variables
	UInt32 modVarCounts[0x100] = {0};				// for each mod, # of string vars loaded
	static const UInt32 varCountThreshold = 100;	// what we'll consider a "large number" of vars; 
													// obviously a few mods may require more than this without it being a problem
	Set<UInt8> exceededMods;

	bool bContinue = true;
	while (bContinue && Serialization::GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'STVE':			//end of block
			bContinue = false;

			if (!exceededMods.Empty())
			{
				_MESSAGE("  WARNING: substantial numbers of string variables exist for the following files (may indicate savegame bloat):");
				for (auto iter = exceededMods.Begin(); !iter.End(); ++iter) {
					_MESSAGE("    %s (%d strings)", DataHandler::Get()->GetNthModName(*iter), modVarCounts[*iter]);
				}
			}

			break;
		case 'STVR':
			modIndex = Serialization::ReadRecord8();
#if _DEBUG
			g_modsWithCosaveVars.insert(g_modsLoaded.at(modIndex));
			modVarCounts[modIndex] += 1;
			if (modVarCounts[modIndex] == varCountThreshold) {
				exceededMods.Insert(modIndex);
				g_cosaveWarning.modIndices.insert(modIndex);
			}
#endif
			if (!Serialization::ResolveRefID(modIndex << 24, &tempRefID) || modIndex == 0xFF)
			{
				// owning mod is no longer loaded so discard
				continue;
			}
			modIndex = tempRefID >> 24;

			stringID = Serialization::ReadRecord32();
			strLength = Serialization::ReadRecord16();
			
			Serialization::ReadRecordData(buffer, strLength);
			buffer[strLength] = 0;

			Insert(stringID, buffer, modIndex);
#if !_DEBUG
			modVarCounts[modIndex] += 1;
			if (modVarCounts[modIndex] == varCountThreshold) {
				exceededMods.Insert(modIndex);
				g_cosaveWarning.modIndices.insert(modIndex);
			}
#endif
					
			break;
		default:
			_MESSAGE("Error loading string map: unhandled chunk type %d", type);
			break;
		}
	}
}

UInt32	StringVarMap::Add(UInt8 varModIndex, const char* data, bool bTemp, StringVar** svOut)
{
	ScopedLock lock(cs);
	UInt32 varID = GetUnusedID();
#if _DEBUG
	if (IsFunctionResultCacheString(varID))
		DebugBreak(); // this should never be allocated
	if (Get(varID))
		DebugBreak(); // trying to add existing var id
#endif
	auto* sv = Insert(varID, data, varModIndex);
	if (svOut)
		*svOut = sv;
	if (bTemp)
		MarkTemporary(varID, true);
	return varID;
}

UInt32 StringVarMap::Add(StringVar&& moveVar, bool bTemp, StringVar** svOut)
{
	ScopedLock lock(cs);
	const auto varID = GetUnusedID();
#if _DEBUG
	if (IsFunctionResultCacheString(varID))
		DebugBreak();
	if (Get(varID))
		DebugBreak();
#endif
	auto* sv = Insert(varID, std::move(moveVar));
	if (svOut)
		*svOut = sv;
	if (bTemp)
		MarkTemporary(varID, true);
	return varID;
}

StringVarMap g_StringMap;

thread_local FunctionResultStringVar s_functionResultStringVar;
static thread_local int svMapClearLocalToken = 0;
static std::atomic<int> svMapClearGlobalToken = 0;


// no compiler optimizations on thread_local in msvc
__declspec(noinline) FunctionResultStringVar& GetFunctionResultCachedStringVar()
{
	if (svMapClearLocalToken != svMapClearGlobalToken)
	{
		s_functionResultStringVar = { nullptr, -1, false };
		svMapClearLocalToken = svMapClearGlobalToken;
	}
	return s_functionResultStringVar;
}

void ResetFunctionResultStringCache()
{
	++svMapClearGlobalToken;
}

bool IsFunctionResultCacheString(UInt32 strId)
{
	auto* strVar = g_StringMap.Get(strId);
	if (strVar)
		return strVar->isFunctionResultCache;
	return false;
}

bool AssignToStringVarLong(COMMAND_ARGS, const char* newValue)
{
	double strID = 0;
	UInt8 modIndex = 0;
	bool bTemp = true;
	StringVar* strVar = NULL;
	const auto isExpressionEvaluator = ExpressionEvaluator::Active();

	UInt32 len = (newValue) ? strlen(newValue) : 0;
	if (!newValue || len >= kMaxMessageLength)		//if null pointer or too long, assign an empty string
		newValue = "";

	if (!isExpressionEvaluator && ExtractSetStatementVar(scriptObj, eventList, scriptData, &strID, &bTemp, opcodeOffsetPtr, &modIndex, thisObj))
	{
		strVar = g_StringMap.Get(strID);
	}
	
	if (!modIndex)
		modIndex = scriptObj->GetModIndex();

	if (!isExpressionEvaluator) // set to statement
	{
		if (strVar)
		{
			strVar->Set(newValue);
			g_StringMap.MarkTemporary(strID, false);
		}
		else
		{
			strID = static_cast<int>(g_StringMap.Add(modIndex, newValue, bTemp));
		}
	}
	else
	{
#if 1
		auto& functionResult = GetFunctionResultCachedStringVar();
		if (!functionResult.inUse)
		{
			// optimizations, creating a new string var is slow
			if (!functionResult.var)
			{
				functionResult.id = static_cast<int>(g_StringMap.Add(0xFF, newValue, false, &functionResult.var));
				functionResult.var->isFunctionResultCache = true;
			}
			else
				functionResult.var->Set(newValue);

			functionResult.inUse = true;
			strID = functionResult.id;
		}
		else
#endif

			strID = static_cast<int>(g_StringMap.Add(modIndex, newValue, true, nullptr));
	}
	
	*result = strID;

#if _DEBUG	// console feedback disabled in release by request (annoying when called from batch scripts)
	if (IsConsoleMode() && !bTemp)
	{
		if (len < 480)
			Console_Print("Assigned string >> \"%s\"", newValue);
		else
			Console_Print("Assigned string (too long to print)");
	}
#endif

	return true;
}

bool AssignToStringVar(COMMAND_ARGS, const char* newValue) {	// Adds another call so ExtractSetStatementVar has a fixed number of calls to unwrap
	return AssignToStringVarLong(PASS_COMMAND_ARGS, newValue);
}

void StringVarMap::Clean()		// clean up any temporary vars
{
	while (!tempIDs.Empty())
		Delete(tempIDs.LastKey());
}


void StringVarMap::Reset()
{
	VarMap<StringVar>::Reset();
	ResetFunctionResultStringCache();
}

namespace PluginAPI
{
	const char* GetString(UInt32 stringID)
	{
		StringVar* var = g_StringMap.Get(stringID);
		if (var)
			return var->GetCString();
		else
			return NULL;
	}

	void SetString(UInt32 stringID, const char* newVal)
	{
		StringVar* var = g_StringMap.Get(stringID);
		if (var)
			var->Set(newVal);
	}

	UInt32 CreateString(const char* strVal, void* owningScript)
	{
		Script* script = (Script*)owningScript;
		if (script)
			return g_StringMap.Add(script->GetModIndex(), strVal);
		else
			return 0;
	}
}
