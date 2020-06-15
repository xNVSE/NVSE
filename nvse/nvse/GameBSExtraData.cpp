#include "nvse/GameBSExtraData.h"
#include "nvse/GameExtraData.h"

bool BaseExtraList::HasType(UInt32 type) const
{
	return (m_presenceBitfield[type >> 3] & (1 << (type & 7))) != 0;
}

BSExtraData* BaseExtraList::GetByType(UInt32 type) const
{
	if (!HasType(type)) return NULL;

	for (BSExtraData* traverse = m_data; traverse; traverse = traverse->next)
		if (traverse->type == type)
			return traverse;

#if _DEBUG
	_MESSAGE("ExtraData HasType(%d) is true but it wasn't found!", type);
	DebugDump();
#endif
	return NULL;
}

void BaseExtraList::MarkType(UInt32 type, bool bCleared)
{
	UInt8 bitMask = 1 << (type & 7);
	UInt8 &flag = m_presenceBitfield[type >> 3];
	if (bCleared) flag &= ~bitMask;
	else flag |= bitMask;
}


ExtraDataList *ExtraDataList::Create(BSExtraData *xBSData)
{
	ExtraDataList *xData = (ExtraDataList*)GameHeapAlloc(sizeof(ExtraDataList));
	MemZero(xData, sizeof(ExtraDataList));
	*(UInt32*)xData = kVtbl_ExtraDataList;
	if (xBSData) xData->Add(xBSData);
	return xData;
}

bool BaseExtraList::Remove(BSExtraData* toRemove, bool free)
{
	if (!toRemove) return false;

	if (HasType(toRemove->type)) {
		bool bRemoved = false;
		if (m_data == toRemove) {
			m_data = m_data->next;
			bRemoved = true;
		}

		for (BSExtraData* traverse = m_data; traverse; traverse = traverse->next) {
			if (traverse->next == toRemove) {
				traverse->next = toRemove->next;
				bRemoved = true;
				break;
			}
		}
		if (bRemoved) {
			MarkType(toRemove->type, true);
			if (free)
				FormHeap_Free(toRemove);
		}
		return true;
	}

	return false;
}

bool BaseExtraList::RemoveByType(UInt32 type, bool free)
{
	if (HasType(type)) {
		return Remove(GetByType(type), free);
	}
	return false;
}

void BaseExtraList::RemoveAll()
{
	while (m_data) {
		BSExtraData* data = m_data;
		m_data = data->next;
		MarkType(data->type, true);
		FormHeap_Free(data);
	}
}

bool BaseExtraList::Add(BSExtraData* toAdd)
{
	if (!toAdd || HasType(toAdd->type)) return false;

	BSExtraData* next = m_data;
	m_data = toAdd;
	toAdd->next = next;
	MarkType(toAdd->type, false);
	return true;
}

/* Kormakur - Have no idea how to integrate this with existing NVSE code so I'm placing back the old NVSE code.
__declspec(naked) void BaseExtraList::Remove(BSExtraData *toRemove, bool doFree)
{
	static const UInt32 procAddr = 0x410020;
	__asm	jmp		procAddr
}

__declspec(naked) BSExtraData *BaseExtraList::Add(BSExtraData *xData)
{
	static const UInt32 procAddr = 0x40FF60;
	__asm	jmp		procAddr
}

__declspec(naked) void BaseExtraList::RemoveByType(UInt32 type)
{
	static const UInt32 procAddr = 0x410140;
	__asm	jmp		procAddr
}

__declspec(naked) void BaseExtraList::RemoveAll(bool doFree)
{
	static const UInt32 procAddr = 0x411FD0;
	__asm	jmp		procAddr
}

*/

__declspec(naked) void BaseExtraList::Copy(BaseExtraList* sourceList)
{
	static const UInt32 procAddr = 0x411EC0;
	__asm	jmp		procAddr
}

bool BaseExtraList::IsWorn() const
{
	return HasType(kExtraData_Worn);
}

char BaseExtraList::GetExtraFactionRank(TESFaction *faction) const
{
	ExtraFactionChanges *xFactionChanges = GetExtraType((*this), FactionChanges);
	if (xFactionChanges && xFactionChanges->data)
	{
		ListNode<FactionListData> *traverse = xFactionChanges->data->Head();
		FactionListData *pData;
		do
		{
			pData = traverse->data;
			if (pData && (pData->faction == faction))
				return pData->rank;
		}
		while (traverse = traverse->next);
	}
	return -1;
}

__declspec(naked) ExtraCount *BaseExtraList::GetExtraCount() const
{
	__asm
	{
		xor		eax, eax
		test	byte ptr [ecx+0xC], 0x10
		jz		done
		mov		eax, [ecx+4]
	iterHead:
		cmp		byte ptr [eax+4], kExtraData_Count
		jnz		iterNext
		retn
	iterNext:
		mov		eax, [eax+8]
		test	eax, eax
		jnz		iterHead
	done:
		retn
	}
}

__declspec(naked) SInt32 BaseExtraList::GetCount() const
{
	__asm
	{
		test	byte ptr [ecx+0xC], 0x10
		jz		retn1
		mov		ecx, [ecx+4]
	iterHead:
		cmp		byte ptr [ecx+4], kExtraData_Count
		jnz		iterNext
		movsx	eax, word ptr [ecx+0xC]
		retn
	iterNext:
		mov		ecx, [ecx+8]
		test	ecx, ecx
		jnz		iterHead
	retn1:
		mov		eax, 1
		retn
	}
}

void __fastcall ExtraValueStr(BSExtraData *xData, char *buffer)
{
	switch (xData->type)
	{
		case kExtraData_Ownership:
		{
			ExtraOwnership *xOwnership = (ExtraOwnership*)xData;
			sprintf_s(buffer, 0x20, "%08X", xOwnership->owner ? xOwnership->owner->refID : 0);
			break;
		}
		case kExtraData_Count:
		{
			ExtraCount *xCount = (ExtraCount*)xData;
			sprintf_s(buffer, 0x20, "%d", xCount->count);
			break;
		}
		default:
			sprintf_s(buffer, 0x20, "%08X", ((UInt32*)xData)[3]);
			break;
	}
}

void BaseExtraList::DebugDump() const
{
	_MESSAGE("\nBaseExtraList Dump:");
	Console_Print("BaseExtraList Dump:");
	IDebugLog::Indent();
	if (m_data)
	{
		char dataStr[0x20];
		for (BSExtraData *traverse = m_data; traverse; traverse = traverse->next)
		{
			ExtraValueStr(traverse, dataStr);
			_MESSAGE("%08X\t%02X\t%s\t%s", traverse, traverse->type, GetExtraDataName(traverse->type), dataStr);
			Console_Print("%08X  %02X  %s  %s", traverse, traverse->type, GetExtraDataName(traverse->type), dataStr);
		}
		Console_Print(" ");
	}
	else
	{
		_MESSAGE("No data in list");
		Console_Print("No data in list");
	}
	IDebugLog::Outdent();
}

bool BaseExtraList::MarkScriptEvent(UInt32 eventMask, TESForm* eventTarget)
{
	return MarkBaseExtraListScriptEvent(eventTarget, this, eventMask);
}
