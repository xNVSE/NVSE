#pragma once

// Added to remove a cyclic dependency between GameForms.h and GameExtraData.h

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
	bool			HasType(UInt32 type) const;
	BSExtraData *	GetByType(UInt32 type) const;

	void			MarkType(UInt32 type, bool bCleared);
	bool			Remove(BSExtraData* toRemove, bool free = false);
	bool			RemoveByType(UInt32 type, bool free = false);
	bool			Add(BSExtraData* toAdd);
	void			RemoveAll();

	bool			MarkScriptEvent(UInt32 eventMask, TESForm* eventTarget);

	void			Copy(BaseExtraList* from);

	void			DebugDump() const;

	bool			IsWorn();

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
