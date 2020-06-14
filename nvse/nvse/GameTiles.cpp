#pragma once

#include "nvse/GameTiles.h"

typedef NiTMapBase<const char*, int> TraitNameMap;
TraitNameMap *g_traitNameMap = (TraitNameMap*)0x11F32F4;
const _TraitNameToID TraitNameToID = (_TraitNameToID)0xA01860;
UInt32 (*TraitNameToIDAdd)(const char*, UInt32) = (UInt32 (*)(const char*, UInt32))0xA00940;

UInt32 Tile::TraitNameToID(const char *traitName)
{
	return ::TraitNameToID(traitName);
}

UInt32 Tile::TraitNameToIDAdd(const char *traitName)
{
	return ::TraitNameToIDAdd(traitName, 0xFFFFFFFF);
}

__declspec(naked) Tile::Value *Tile::GetValue(UInt32 typeID)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		mov		ebx, [ecx+0x14]
		xor		esi, esi
		mov		edi, [ecx+0x18]
		mov		edx, [esp+0x10]
	iterHead:
		cmp		esi, edi
		jz		iterEnd
		lea		ecx, [esi+edi]
		shr		ecx, 1
		mov		eax, [ebx+ecx*4]
		cmp		[eax], edx
		jz		done
		jb		isLT
		mov		edi, ecx
		jmp		iterHead
	isLT:
		lea		esi, [ecx+1]
		jmp		iterHead
	iterEnd:
		xor		eax, eax
	done:
		pop		edi
		pop		esi
		pop		ebx
		retn	4
	}
}

Tile::Value *Tile::GetValueName(const char *valueName)
{
	return GetValue(TraitNameToID(valueName));
}

DListNode<Tile> *Tile::GetNthChild(UInt32 index)
{
	return children.Tail()->Regress(index);
}

char *Tile::GetComponentFullName(char *resStr)
{
	if IS_TYPE(this, TileMenu)
		return StrLenCopy(resStr, name.m_data, name.m_dataLen);
	char *fullName = parent->GetComponentFullName(resStr);
	*fullName++ = '/';
	fullName = StrLenCopy(fullName, name.m_data, name.m_dataLen);
	DListNode<Tile> *node = parent->children.Tail();
	while (node->data != this)
		node = node->prev;
	int index = 0;
	while ((node = node->prev) && StrEqualCS(name.m_data, node->data->name.m_data))
		index++;
	if (index)
	{
		*fullName++ = ':';
		fullName = IntToStr(fullName, index);
	}
	return fullName;
}

Menu *Tile::GetParentMenu()
{
	Tile *tile = this;
	do
	{
		if IS_TYPE(tile, TileMenu)
			return ((TileMenu*)tile)->menu;
	}
	while (tile = tile->parent);
	return NULL;
}

__declspec(naked) void Tile::PokeValue(UInt32 valueID)
{
	__asm
	{
		push	dword ptr [esp+4]
		call	Tile::GetValue
		test	eax, eax
		jz		done
		push	eax
		push	1
		push	0x3F800000
		mov		ecx, eax
		mov		eax, 0xA0A270
		call	eax
		pop		ecx
		push	1
		push	0
		mov		eax, 0xA0A270
		call	eax
	done:
		retn	4
	}
}

__declspec(naked) void Tile::FakeClick()
{
	__asm
	{
		push	kTileValue_clicked
		call	Tile::GetValue
		test	eax, eax
		jz		done
		push	eax
		push	1
		push	0x3F800000
		mov		ecx, eax
		mov		eax, 0xA0A270
		call	eax
		pop		ecx
		push	1
		push	0
		mov		eax, 0xA0A270
		call	eax
	done:
		retn
	}
}

void Tile::DestroyAllChildren()
{
	DListNode<Tile> *node = children.Tail();
	Tile *child;
	while (node)
	{
		child = node->data;
		node = node->prev;
		if (child) child->Destroy(true);
	}
}

Tile *Tile::GetChild(const char *childName)
{
	int childIndex = 0;
	char *colon = FindChr(childName, ':');
	if (colon)
	{
		if (colon == childName) return NULL;
		*colon = 0;
		childIndex = StrToInt(colon + 1);
	}
	Tile *result = NULL;
	for (DListNode<Tile> *node = children.Head(); node; node = node->next)
	{
		if (node->data && ((*childName == '*') || StrEqualCI(node->data->name.m_data, childName)) && !childIndex--)
		{
			result = node->data;
			break;
		}
	}
	if (colon) *colon = ':';
	return result;
}

Tile *Tile::GetComponent(const char *componentPath, const char **trait)
{
	Tile *parentTile = this;
	char *slashPos;
	while (slashPos = SlashPos(componentPath))
	{
		*slashPos = 0;
		parentTile = parentTile->GetChild(componentPath);
		if (!parentTile) return NULL;
		componentPath = slashPos + 1;
	}
	if (*componentPath)
	{
		Tile *result = parentTile->GetChild(componentPath);
		if (result) return result;
		*trait = componentPath;
	}
	return parentTile;
}

Tile::Value *Tile::GetComponentValue(const char *componentPath)
{
	const char *trait = NULL;
	Tile *tile = GetComponent(componentPath, &trait);
	return (tile && trait) ? tile->GetValueName(trait) : NULL;
}

Tile *Tile::GetComponentTile(const char *componentPath)
{
	const char *trait = NULL;
	Tile *tile = GetComponent(componentPath, &trait);
	return (tile && !trait) ? tile : NULL;
}

void Tile::Dump()
{
	_MESSAGE("%08X\t%s", this, name.m_data);
	IDebugLog::Indent();
	
	_MESSAGE("Values:");

	IDebugLog::Indent();
	
	Value *value;
	const char *traitName;
	char traitID[9];
	for (BSSimpleArray<Value*>::Iterator iter(values); !iter.End(); ++iter)
	{
		value = *iter;
		traitName = TraitIDToName(value->id);

		if (!traitName)
		{
			UIntToHex(traitID, value->id);
			traitName = traitID;
		}
		if (value->str)
			_MESSAGE("%d  %s: %s", value->id, traitName, value->str);
		/*else if (value->action)
			_MESSAGE("%d  %s: Action %08X", value->id, traitName, value->action);*/
		else
			_MESSAGE("%d  %s: %.4f", value->id, traitName, value->num);
	}

	IDebugLog::Outdent();

	for (DListNode<Tile> *traverse = children.Tail(); traverse; traverse = traverse->prev)
		if (traverse->data) traverse->data->Dump();

	IDebugLog::Outdent();
}

// not a one-way mapping, so we just return the first
// also this is slow and sucks
const char *TraitIDToName(int id)
{
	for (UInt32 i = 0; i < g_traitNameMap->numBuckets; i++)
		for (TraitNameMap::Entry * bucket = g_traitNameMap->buckets[i]; bucket; bucket = bucket->next)
			if(bucket->data == id)
				return bucket->key;

	return NULL;
}
