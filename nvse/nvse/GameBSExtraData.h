#pragma once

// Added to remove a cyclic dependency between GameForms.h and GameExtraData.h
#include "Utilities.h"

class TESForm;

// C+?
class BSExtraData
{
public:
	BSExtraData();
	virtual ~BSExtraData();

	virtual bool Differs(BSExtraData* extra);	// 001

	static BSExtraData* Create(UInt8 xType, UInt32 size, UInt32 vtbl);

//	void		** _vtbl;	// 000
	UInt8		type;		// 004
	UInt8		pad[3];		// 005
	BSExtraData	* next;		// 008
};

// 020
struct BaseExtraList
{
	bool HasType(UInt32 type) const;

	__forceinline BSExtraData *GetByType(UInt8 type) const
	{
		return ThisStdCall<BSExtraData*>(0x410220, this, type);
	}

	void MarkType(UInt32 type, bool bCleared);

	__forceinline void Remove(BSExtraData *toRemove, bool doFree)
	{
		ThisStdCall(0x410020, this, toRemove, doFree);
	}

	__forceinline void RemoveByType(UInt32 type)
	{
		ThisStdCall(0x410140, this, type);
	}

	__forceinline BSExtraData *Add(BSExtraData *toAdd)
	{
		return ThisStdCall<BSExtraData*>(0x40FF60, this, toAdd);
	}

	__forceinline void RemoveAll(bool doFree)
	{
		ThisStdCall(0x40FAE0, this, doFree);
	}

	bool MarkScriptEvent(UInt32 eventMask, TESForm* eventTarget);

	__forceinline void Copy(BaseExtraList *from)
	{
		ThisStdCall(0x411EC0, this, from);
	}

	void DebugDump() const;

	bool IsWorn();

	void		** m_vtbl;					// 000
	BSExtraData	* m_data;					// 004
	UInt8		m_presenceBitfield[0x15];	// 008 - if a bit is set, then the extralist should contain that extradata
											// bits are numbered starting from the lsb
	UInt8		pad1D[3];					// 01D
};

struct ExtraDataList : public BaseExtraList
{
	static ExtraDataList * Create(BSExtraData* xBSData = NULL);
};

STATIC_ASSERT(offsetof(BaseExtraList, m_presenceBitfield) == 0x008);
STATIC_ASSERT(sizeof(ExtraDataList) == 0x020);

