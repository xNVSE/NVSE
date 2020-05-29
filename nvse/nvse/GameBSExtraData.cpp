#include "GameBSExtraData.h"
#include "GameAPI.h"
#include "GameExtraData.h"

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
static const UInt32 s_ExtraDataListVtbl							= 0x010143E8;	//	0x0100e3a8;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
static const UInt32 s_ExtraDataListVtbl							= 0x010143D8;
#elif EDITOR
#else
#error
#endif

bool BaseExtraList::HasType(UInt32 type) const
{
	UInt32 index = (type >> 3);
	UInt8 bitMask = 1 << (type % 8);
	return (m_presenceBitfield[index] & bitMask) != 0;
}

BSExtraData * BaseExtraList::GetByType(UInt32 type) const
{
	if (!HasType(type)) return NULL;

	for(BSExtraData * traverse = m_data; traverse; traverse = traverse->next)
		if(traverse->type == type)
			return traverse;

#if _DEBUG
	_MESSAGE("ExtraData HasType(%d) is true but it wasn't found!", type);
	DebugDump();
#endif
	return NULL;
}

void BaseExtraList::MarkType(UInt32 type, bool bCleared)
{
	UInt32 index = (type >> 3);
	UInt8 bitMask = 1 << (type % 8);
	UInt8& flag = m_presenceBitfield[index];
	if (bCleared) {
		flag &= ~bitMask;
	} else {
		flag |= bitMask;
	}
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

bool BaseExtraList::Add(BSExtraData* toAdd)
{
	if (!toAdd || HasType(toAdd->type)) return false;
	
	BSExtraData* next = m_data;
	m_data = toAdd;
	toAdd->next = next;
	MarkType(toAdd->type, false);
	return true;
}

ExtraDataList* ExtraDataList::Create(BSExtraData* xBSData)
{
	ExtraDataList* xData = (ExtraDataList*)FormHeap_Allocate(sizeof(ExtraDataList));
	memset(xData, 0, sizeof(ExtraDataList));
	((UInt32*)xData)[0] = s_ExtraDataListVtbl;
	if (xBSData)
		xData->Add(xBSData);
	return xData;
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

void BaseExtraList::Copy(BaseExtraList* from)
{
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	ThisStdCall(0x00411EC0, this, from);	// 428920 in last Fallout3
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	ThisStdCall(0x00411F10, this, from);	// 428920 in last Fallout3
#elif EDITOR
#else
#error
#endif
}

bool BaseExtraList::IsWorn()
{
	return (HasType(kExtraData_Worn) || HasType(kExtraData_WornLeft));
}

void BaseExtraList::DebugDump() const
{
	_MESSAGE("BaseExtraList Dump:");
	gLog.Indent();

	if (m_data)
	{
		for(BSExtraData * traverse = m_data; traverse; traverse = traverse->next) {
			_MESSAGE("%s", GetObjectClassName(traverse));
			_MESSAGE("Extra types %4x (%s) %s", traverse->type, GetExtraDataName(traverse->type), GetExtraDataValue(traverse));
		}
	}
	else
		_MESSAGE("No data in list");

	gLog.Outdent();
}

bool BaseExtraList::MarkScriptEvent(UInt32 eventMask, TESForm* eventTarget)
{
	return MarkBaseExtraListScriptEvent(eventTarget, this, eventMask);
}
