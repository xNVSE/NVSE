#pragma once

// Added to remove a cyclic dependency between GameForms.h and GameExtraData.h

// 0C
class BSExtraData
{
public:
	BSExtraData();
	~BSExtraData();

	virtual void	Destroy(bool doFree);
	virtual bool	IsDifferentType(BSExtraData *compareTo);

	static BSExtraData* Create(UInt8 xType, UInt32 size, UInt32 vtbl);

	UInt8			type;		// 04
	UInt8			pad05[3];	// 05
	BSExtraData		*next;		// 08
};

// 020
struct BaseExtraList
{
	virtual void	Destroy(bool doFree);
	void* vtable;

	BSExtraData		*m_data;					// 004
	UInt8			m_presenceBitfield[0x15];	// 008 - if a bit is set, then the extralist should contain that extradata
	UInt8			pad1D[3];					// 01D

	bool HasType(UInt32 type) const;
	BSExtraData *GetByType(UInt32 type) const;
	void MarkType(UInt32 type, bool bCleared);
	void Remove(BSExtraData *toRemove, bool doFree = false);
	void RemoveByType(UInt32 type);
	BSExtraData *Add(BSExtraData *xData);
	void RemoveAll(bool doFree = true);
	bool MarkScriptEvent(UInt32 eventMask, TESForm *eventTarget);
	void Copy(BaseExtraList *sourceList);
	void DebugDump() const;
	bool GetCannotWear() { return ((bool (__thiscall*)(BaseExtraList*))(0x418B10))(this); };
	bool IsWorn() const;
	char GetExtraFactionRank(TESFaction *faction) const;
	ExtraCount *GetExtraCount() const;
	SInt32 GetCount() const;
};

struct ExtraDataList : public BaseExtraList
{
	ExtraDataList *CreateCopy();
	static ExtraDataList* __stdcall Create(BSExtraData *xBSData = NULL);
};
STATIC_ASSERT(sizeof(ExtraDataList) == 0x020);
