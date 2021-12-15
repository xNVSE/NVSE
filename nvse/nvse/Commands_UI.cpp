#include "Commands_UI.h"

#include <algorithm>
#include <sstream>
#include <utility>


#include "containers.h"
#include "GameUI.h"
#include "GameAPI.h"
#include "MemoizedMap.h"
#include "common/ICriticalSection.h"

#define the_VATScamStruct		0x011F2250
#define the_VATSunclick			0x009C88A0
#define the_VATSexecute			0x00705780
#define	the_DoShowLevelUpMenu	0x00784C80

static const float fErrorReturnValue = -999;

enum eUICmdAction {
	kGetFloat,
	kSetFloat,
	kSetString,
	kSetFormattedString,
};

MemoizedMap<const char*, Tile::Value*> s_memoizedTileValues;

Tile::Value* GetCachedComponentValue(const char* component)
{
	return s_memoizedTileValues.Get(component, [](const char* component)
	{
		return InterfaceManager::GetMenuComponentValue(component);
	});
}

bool GetSetUIValue_Execute(COMMAND_ARGS, eUICmdAction action)
{
	char component[kMaxMessageLength];
	float newFloat;
	char newStr[kMaxMessageLength];
	*result = 0;

	bool bExtracted = false;
	switch (action)
	{
	case kGetFloat:
		bExtracted = ExtractArgs(EXTRACT_ARGS, &component);
		break;
	case kSetFloat:
		bExtracted = ExtractArgs(EXTRACT_ARGS, &component, &newFloat);
		break;
	case kSetString:
		bExtracted = ExtractArgs(EXTRACT_ARGS, &component, &newStr);
		break;
	case kSetFormattedString:
		bExtracted = ExtractFormatStringArgs(1, newStr, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, kCommandInfo_SetUIStringEx.numParams, &component);
		break;
	default:
		DEBUG_PRINT("Error: Invalid action in GetSetUIValue_Execute()");
		return true;
	}

	if (bExtracted)
	{
		Tile::Value* val = GetCachedComponentValue(component);
		if (val)
		{
			switch (action)
			{
			case kGetFloat:
				*result = val->num;
				if (IsConsoleMode())
					Console_Print("GetUIFloat >> %.4f", *result);
				break;
			case kSetFloat:
				CALL_MEMBER_FN(val->parent, SetFloatValue)(val->id, newFloat, true);
				break;
			case kSetString:
			case kSetFormattedString:
				CALL_MEMBER_FN(val->parent, SetStringValue)(val->id, newStr, true);
				break;
			}
		}
		else		// trait not found
		{
			if (action == kGetFloat)
				*result = fErrorReturnValue;

			if (IsConsoleMode())
				Console_Print("Trait not found");
		}
	}

	return true;
}

bool Cmd_GetUIFloat_Execute(COMMAND_ARGS)
{
	return GetSetUIValue_Execute(PASS_COMMAND_ARGS, kGetFloat);
}

bool Cmd_SetUIFloat_Execute(COMMAND_ARGS)
{
	return GetSetUIValue_Execute(PASS_COMMAND_ARGS, kSetFloat);
}

bool Cmd_SetUIString_Execute(COMMAND_ARGS)
{
	return GetSetUIValue_Execute(PASS_COMMAND_ARGS, kSetString);
}

bool Cmd_SetUIStringEx_Execute(COMMAND_ARGS)
{
	return GetSetUIValue_Execute(PASS_COMMAND_ARGS, kSetFormattedString);
}

enum {
	kReverseSort = 1,
	kNormalizeItemNames = 2,
	kAsFloat = 4,
};

typedef std::pair<std::string, int> SortKey;
struct SortValue {
	std::string str;
	float num;
	bool asFloat;
};
typedef std::pair<Tile *, std::vector<SortValue> > SortItem;

// A sort specification looks like
//   "keySpec0[,keySpec1[,...]][$destinationPath[$incrementPath]]"
// where [] indicate optional parts.
//
// A key path looks like
//   "[-][#][@]path"
// where 
//   "-" reverses the normal sort order for this key
//   "#" sorts this string field numerically
//   "@" sorts this item name disregarding quanties and mod indicators
//       E.g. "9mm Pistol (2)" and "9mm Pistol+" sort as "9mm Pistol"
//
// The delimiters are chosen to hopefully not collide with anything
// that might actually be part of a path, or be specially interpreted by the
// console the way ";" is.
class SortUIListBox_SortSpec {
public:
	SortUIListBox_SortSpec(const char * spec) :
		keys(),
		destinationPath("_y"),
		incrementPath("height")
	{
		Tokenizer outerTokens(spec, "$");
		std::string keysToken;
		if (outerTokens.NextToken(keysToken) != -1)
		{
			Tokenizer innerTokens(keysToken.c_str(), ",");
			std::string keyPath;
			while (innerTokens.NextToken(keyPath) != -1)
			{
				int flags = 0;
				if (!keyPath.empty() && (*(keyPath.begin()) == '-'))
				{
					flags |= kReverseSort;
					keyPath.erase(0, 1);
				}
				if (!keyPath.empty() && (*(keyPath.begin()) == '#'))
				{
					flags |= kAsFloat;
					keyPath.erase(0, 1);
				}
				if (!keyPath.empty() && (*(keyPath.begin()) == '@'))
				{
					flags |= kNormalizeItemNames;
					keyPath.erase(0, 1);
				}
				DEBUG_PRINT("Parsed key #%d path %s flags %d", 
					keys.size(), keyPath.c_str(), flags);
				keys.push_back(std::make_pair(keyPath, flags));
			}
			if (outerTokens.NextToken(destinationPath) != -1)
			{
				DEBUG_PRINT("Parsed destination path: %s", destinationPath.c_str());
				if (outerTokens.NextToken(incrementPath) != -1)
				{
					DEBUG_PRINT("Parsed increment path: %s", incrementPath.c_str());
				}
			}
		}
	}

	// Comparison operator between two Tiles
	bool operator() (const SortItem * lhs, const SortItem * rhs)
	{
		for (int key_index=0; key_index<keys.size(); ++key_index)
		{
			const SortKey & key = keys[key_index];
			int flags = key.second;
			const SortValue & lhs_val = lhs->second[key_index];
			const SortValue & rhs_val = rhs->second[key_index];

			float cmp = 0;
			if (lhs_val.asFloat != rhs_val.asFloat)
			{
				return false;
			}
			
			if (lhs_val.asFloat)
			{
				cmp = lhs_val.num - rhs_val.num;
			}
			else
			{
				cmp = lhs_val.str.compare(rhs_val.str);
			}

			if (flags & kReverseSort)
			{
				cmp = -cmp;
			}

			if (cmp < 0)
			{
				return true;
			}
			else if (cmp > 0)
			{
				return false;
			}
		}

		return false; // Tiles are equal
	};

public:
	std::vector<SortKey> keys;
	std::string destinationPath;
	std::string incrementPath;
};

static void NormalizeItemName(std::string & str)
{
	// Strip quanities like "9mm Pistol (5)" -> "9mm Pistol"
	std::string::iterator last = str.end() - 1;
	if ((last > str.begin()) && (*last == ')'))
	{
		int n = 0;
		while (--last > str.begin())
		{
			if (('0' <= *last) && (*last <= '9'))
			{
				++n;
			}
			else
			{
				break;
			}
		}
		if ((n > 0) && (*last == '('))
		{
			while (--last > str.begin())
			{
				if (*last != ' ')
				{
					break;
				}
			}
			str.resize(last + 1 - str.begin());
		}
	}

	// Make modded items ending with "+" sort with unmodded items
	last = str.end() - 1;
	if ((last > str.begin()) && (*last == '+'))
	{
		str.erase(str.end() - 1);
	}
}	

bool Cmd_SortUIListBox_Execute(COMMAND_ARGS)
{
	char tilePath[kMaxMessageLength];
	char sortSpecStr[kMaxMessageLength];
	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &tilePath, &sortSpecStr))
	{
		return true;
	}

	// Parse sort specification
	SortUIListBox_SortSpec sortSpec(sortSpecStr);

	// Find parent listbox
	Tile * parentTile = InterfaceManager::GetMenuComponentTile(tilePath);
	if (!parentTile)
	{
		return true;
	}

	// Find child tiles to sort
	std::vector<SortItem> sortItems;
	for(tList<Tile::ChildNode>::Iterator iter = parentTile->childList.Begin(); !iter.End(); ++iter)
	{
		if(!(*iter) || !(iter->child))
		{
			continue;
		}
		Tile * tile = iter->child;

		// Hidden?
		Tile::Value* listindexVal = tile->GetComponentValue("listindex");
		if ((!listindexVal) || (listindexVal->num < 0.0))
		{
			continue;
		}

		// Fetch tile values
		bool skip = false;
		std::vector<SortValue> values;
		for (std::vector<SortKey>::iterator it=sortSpec.keys.begin(); 
			 it != sortSpec.keys.end(); ++it)
		{
			Tile::Value* val = tile->GetComponentValue(it->first.c_str());
			int flags = it->second;
			if (!val)
			{
				// Skip things like scrollbars that don't have the specified traits
				skip = true;
				break;
			}
			SortValue sv;
			if (val->str) // String value
			{
				if (flags & kAsFloat) {
					sv.num = atof(val->str);
					sv.asFloat = true;
				}
				else
				{
					sv.str = val->str;
					sv.asFloat = false;
					if (flags & kNormalizeItemNames)
					{
						NormalizeItemName(sv.str);
					}
				}
			}
			else // Float value
			{
				sv.num = val->num;
				sv.asFloat = true;
			}

			values.push_back(sv);
		}
		if (!skip)
		{
			sortItems.push_back(std::make_pair(tile, values));
		}
	}

	// Sort the tiles by pointer so we don't spend all day copying strings around
	std::vector<SortItem*> sortPointers;
	for (std::vector<SortItem>::iterator it=sortItems.begin(); it != sortItems.end(); ++it)
	{
		sortPointers.push_back(&(*it));
	}
	sort(sortPointers.begin(), sortPointers.end(), sortSpec);

	// Reorder the positions of the menu tiles based on the sorted order
	float current = 0.0;
	for (int index=0; index < sortPointers.size(); ++index)
	{
		SortItem * sortItem = sortPointers[index];
		Tile * tile = sortItem->first;
		Tile::Value* incrementVal = tile->GetComponentValue(sortSpec.incrementPath.c_str());
		if (!incrementVal) {
			DEBUG_PRINT("Failed to find increment trait %s for tile %d", 
				sortSpec.incrementPath.c_str(), index)
			continue;
		}
		float increment = incrementVal->num;

		Tile::Value* destinationVal = tile->GetComponentValue(sortSpec.destinationPath.c_str());
		if (!destinationVal) {
			DEBUG_PRINT("Failed to find destination trait %s for tile %d", 
				sortSpec.destinationPath.c_str(), index)
			continue;
		}

		CALL_MEMBER_FN(destinationVal->parent, SetFloatValue)(destinationVal->id, current, true);
		current += increment;
	}

	return true;
}

// VATS camera

// End function provided and tested by Queued. Converted from asm to C++

typedef tList<void*>	VATStargetList;
struct VATScamStruct {
	VATStargetList *	targets;
	UInt32				unk004;
	UInt32				mode;
	// ...
	MEMBER_FN_PREFIX(VATScamStruct);
	DEFINE_MEMBER_FN(VATSunclick, void, the_VATSunclick, UInt32*, UInt32*);
};

typedef void (__stdcall * _VATSunclick)(UInt32*, UInt32*);
typedef void (* _VATSexecute)(void);

VATScamStruct* g_VATScamStruct = (VATScamStruct*)the_VATScamStruct;
//_VATSunclick VATSunclick = (_VATSunclick)the_VATSunclick;
_VATSexecute VATSexecute = (_VATSexecute)the_VATSexecute;

bool Cmd_EndVATScam_Execute (COMMAND_ARGS)
{
	UInt32 const0;
	UInt32 value;

//	value = 0x016;
	for (; g_VATScamStruct->targets; ) {
		const0 = 0;
		value = 0x016;
		CALL_MEMBER_FN(g_VATScamStruct, VATSunclick)(&value, &const0);
//		VATSunclick(&value, &const0);
	}
	if (2 == g_VATScamStruct->mode)
		VATSexecute();
//		CALL_MEMBER_FN(g_VATScamStruct, VATSexecute)();
	return true;
}

typedef Menu* (* _DoShowLevelUpMenu)(void);
_DoShowLevelUpMenu DoShowLevelUpMenu = (_DoShowLevelUpMenu)the_DoShowLevelUpMenu;

bool Cmd_ShowLevelUpMenu_Execute (COMMAND_ARGS)
{
	DoShowLevelUpMenu();
	return true;
}

Tile::Value* GetCachedComponentValueAlt(const char* component)
{
	return s_memoizedTileValues.Get(component, [](const char* component)
	{
		return InterfaceManager::GetMenuComponentValueAlt(component);
	});
}

bool Cmd_GetUIFloatAlt_Execute(COMMAND_ARGS)
{
	*result = fErrorReturnValue;
	char component[0x200];
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &component))
	{
		Tile::Value *val = GetCachedComponentValueAlt(component);
		if (val)
		{
			*result = val->num;
			if (IsConsoleMode())
				Console_Print("GetUIFloatAlt >> %.4f", *result);
		}
	}
	return true;
}

bool Cmd_SetUIFloatAlt_Execute(COMMAND_ARGS)
{
	char component[0x200];
	float newFloat;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &component, &newFloat))
	{
		Tile::Value *val = GetCachedComponentValueAlt(component);
		if (val)
		{
			CALL_MEMBER_FN(val->parent, SetFloatValue)(val->id, newFloat, true);
			*result = 1;
		}
	}
	return true;
}

bool Cmd_SetUIStringAlt_Execute(COMMAND_ARGS)
{
	char component[0x200];
	char newStr[kMaxMessageLength];
	*result = 0;
	if (ExtractFormatStringArgs(1, newStr, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, kCommandInfo_SetUIStringAlt.numParams, &component))
	{
		Tile::Value *val = GetCachedComponentValueAlt(component);
		if (val)
		{
			CALL_MEMBER_FN(val->parent, SetStringValue)(val->id, newStr, true);
			*result = 1;
		}
	}
	return true;
}

bool Cmd_ModUIFloat_Execute(COMMAND_ARGS)
{
	char component[0x200];
	float newFloat;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &component, &newFloat))
	{
		Tile::Value* val = GetCachedComponentValueAlt(component);
		if (val)
		{
			CALL_MEMBER_FN(val->parent, SetFloatValue)(val->id, val->num + newFloat, true);
			*result = 1;
		}
	}
	return true;
}

bool Cmd_PrintActiveTile_Execute(COMMAND_ARGS)
{
	InterfaceManager* intfc = InterfaceManager::GetSingleton();
	if (intfc && intfc->activeTile)
	{
		std::string name = intfc->activeTile->GetQualifiedName();
		if (name.length() < 0x200)
			Console_Print("%s", name.c_str());
		else
		{
			Console_Print("Name too long to print in console, sending to nvse.log");
			_MESSAGE("%s", name.c_str());
		}
	}
	else
		Console_Print("Could not read active tile");

	return true;
}