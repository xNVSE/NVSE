#include "nvse/GameTiles.h"
#include "nvse/Utilities.h"
#include "nvse/GameAPI.h"
#include <string>

typedef NiTMapBase <const char *, int>	TraitNameMap;
TraitNameMap	* g_traitNameMap = (TraitNameMap *)0x011F32F4;

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
const _TraitNameToID TraitNameToID = (_TraitNameToID)0x00A01860;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
const _TraitNameToID TraitNameToID = (_TraitNameToID)0x00A01730;
#elif EDITOR
#else
#error
#endif

UInt32 Tile::TraitNameToID(const char * traitName)
{
	return ::TraitNameToID(traitName);
}

Tile::Value * Tile::GetValue(UInt32 typeID)
{
	// values are sorted so could use binary search but these are not particularly large arrays
	for(UInt32 i = 0; i < values.size; i++)
	{
		Tile::Value * val = values[i];
		if(val && val->id == typeID)
			return val;
	}

	return NULL;
}

Tile::Value * Tile::GetValue(const char * valueName)
{
	UInt32 typeID = TraitNameToID(valueName);

	return GetValue(typeID);
}

Tile * Tile::GetChild(const char * childName)
{
	Tile * child = NULL;
	std::string tileName(childName);
	int childIndex = 0;
	
	// Allow child names like "foo:4" to select the 4th "foo" child. There 
	// must be a non-empty string before the colon, a single colon character, 
	// then a non-empty sequence of digits.
	const char * colon = strchr(childName, ':');
	if (colon && (colon != childName)) {
		childIndex = atoi(colon + 1);
		tileName = std::string(childName, (colon - childName));
		//DEBUG_PRINT("tileName: %s, childIndex: %d", tileName.c_str(), childIndex);
	}

	int foundCount = 0;
	for(tList<ChildNode>::Iterator iter = childList.Begin(); !iter.End(); ++iter)
	{
		// Allow child name "*" to match any child
		if(*iter && iter->child && (
			(tileName == "*") || (!_stricmp(iter->child->name.m_data, tileName.c_str()))))
		{
			if (foundCount == childIndex) {
				child = iter->child;
				break;
			} else {
				foundCount++;
			}
		}
	}

	return child;
}

// Find a tile or tile value by component path.
// Returns NULL if component path not found.
// Returns Tile* and clears "trait" if component was a tile.
// Returns Tile* and sets "trait" if component was a tile value.
Tile * Tile::GetComponent(const char * componentPath, std::string * trait)
{
	Tokenizer tokens(componentPath, "\\/");
	std::string curToken;
	Tile * parentTile = this;

	while(tokens.NextToken(curToken) != -1 && parentTile)
	{
		// DEBUG_PRINT("childName: %s", curToken.c_str());

		Tile * child = parentTile->GetChild(curToken.c_str());
		if(!child)
		{
			// didn't find child; is this last token?
			if(tokens.NextToken(curToken) == -1)
			{
				*trait = curToken;
				return parentTile;
			}
			else	// nope, error
				return NULL;
		}
		else
			parentTile = child;
	}

	trait->clear();
	return parentTile;
}

Tile::Value * Tile::GetComponentValue(const char * componentPath)
{
	std::string trait;
	Tile* tile = GetComponent(componentPath, &trait);
	if (tile && trait.length())
	{
		return tile->GetValue(trait.c_str());
	}
	return NULL;
}

Tile * Tile::GetComponentTile(const char * componentPath)
{
	std::string trait;
	Tile* tile = GetComponent(componentPath, &trait);
	if (tile && !trait.length())
	{
		return tile;
	}
	return NULL;
}

std::string Tile::GetQualifiedName(void)
{
	std::string qualifiedName;

	//if(parent && !parent->GetFloatValue(kTileValue_class, &parentClass))	// i.e., parent is not a menu
	if(parent)
		qualifiedName = parent->GetQualifiedName() + "\\";

	qualifiedName += name.m_data;

	return qualifiedName;
}

void Tile::Dump(void)
{
	_MESSAGE("%s", name.m_data);
	gLog.Indent();

	_MESSAGE("values:");

	gLog.Indent();
	
	for(UInt32 i = 0; i < values.size; i++)
	{
		Value		* val = values[i];
		const char	* traitName = TraitIDToName(val->id);
		char		traitNameIDBuf[16];

		if(!traitName)
		{
			sprintf_s(traitNameIDBuf, "%08X", val->id);
			traitName = traitNameIDBuf;
		}

		if(val->str)
			_MESSAGE("%s: %s", traitName, val->str);
		else if(val->action)
			_MESSAGE("%s: action %08X", traitName, val->action);
		else
			_MESSAGE("%s: %f", traitName, val->num);
	}

	gLog.Outdent();

	for(tList <ChildNode>::Iterator iter = childList.Begin(); !iter.End(); ++iter)
	{
		ChildNode	* node = iter.Get();
		if(node)
		{
			Tile	* child = node->child;
			if(child)
			{
				child->Dump();
			}
		}
	}

	gLog.Outdent();
}

void Debug_DumpTraits(void)
{
	for(UInt32 i = 0; i < g_traitNameMap->numBuckets; i++)
	{
		for(TraitNameMap::Entry * bucket = g_traitNameMap->buckets[i]; bucket; bucket = bucket->next)
		{
			_MESSAGE("%s,%08X,%d", bucket->key, bucket->data, bucket->data);
		}
	}
}

// not a one-way mapping, so we just return the first
// also this is slow and sucks
const char * TraitIDToName(int id)
{
	for(UInt32 i = 0; i < g_traitNameMap->numBuckets; i++)
		for(TraitNameMap::Entry * bucket = g_traitNameMap->buckets[i]; bucket; bucket = bucket->next)
			if(bucket->data == id)
				return bucket->key;

	return NULL;
}

void Debug_DumpTileImages(void) {};

