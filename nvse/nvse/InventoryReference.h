#pragma once

#include "InventoryInfo.h"

class TESObjectREFR;

// InventoryReference represents a temporary reference to a stack of items in an inventory
// temp refs are valid only for the frame during which they were created

class InventoryReference
{
public:
	struct Data
	{
		TESForm								* type;
		ExtraContainerChanges::EntryData	* entry;
		ExtraDataList						* xData;

		Data(TESForm* t, ExtraContainerChanges::EntryData* en, ExtraDataList* ex) : type(t), entry(en), xData(ex) { }
		Data(const Data& rhs) : type(rhs.type), entry(rhs.entry), xData(rhs.xData) { }
		Data() : type(NULL), entry(NULL), xData(NULL) { }

		static void CreateForUnextendedEntry(ExtraContainerChanges::EntryData* entry, SInt32 totalCount, Vector<Data> &dataOut);
	};

	~InventoryReference();

	TESObjectREFR* GetContainer() { return m_containerRef; }
	void SetContainer(TESObjectREFR* cont) { m_containerRef = cont; }
	bool SetData(const Data &data);
	TESObjectREFR* GetRef() { return Validate() ? m_tempRef : NULL; }
	static TESObjectREFR* GetRefBySelf(InventoryReference* self) { return self ? self->GetRef() : NULL; }	// Needed to get convert the address to a void*

	bool WriteRefDataToContainer();
	bool RemoveFromContainer();			// removes and frees Data pointers
	bool MoveToContainer(TESObjectREFR* dest);
	bool CopyToContainer(TESObjectREFR* dest);
	bool SetEquipped(bool bEquipped);
	void SetRemoved() { m_bRemoved = true; }
	void Release();

	static InventoryReference* GetForRefID(UInt32 refID);

	enum ActionType
	{
		kAction_Equip,
		kAction_Remove,
	};

	class DeferredAction 
	{
		ActionType					m_type;
		Data						m_itemData;
		TESObjectREFR				*m_target;

	public:
		// an operation which could potentially invalidate the extradatalist, deferred until inv ref is released
		DeferredAction(ActionType type, const Data& data, TESObjectREFR *target = nullptr) : m_type(type), m_itemData(data), m_target(target) {}
		
		const Data& Data() {return m_itemData;}
		bool Execute(InventoryReference* iref);
	};

	Data						m_data;
	TESObjectREFR				*m_containerRef;
	TESObjectREFR				*m_tempRef;
	Stack<DeferredAction>		m_deferredActions;
	ExtraContainerChanges::EntryData	*m_tempEntry;
	UInt8						pad1C[0x10];	// This used to be std::queue<DeferredAction*>; Padded to preserve sizeof == 0x30, for backward compatibility with plugins.
	bool						m_bDoValidation;
	bool						m_bRemoved;

	bool Validate();
	void DoDeferredActions();
	SInt32 GetCount();
};

typedef UnorderedMap<UInt32, InventoryReference> InventoryReferenceMap;
extern InventoryReferenceMap s_invRefMap;

InventoryReference* CreateInventoryRef(TESObjectREFR* container, const InventoryReference::Data &data, bool bValidate);
ExtraContainerChanges::EntryData *CreateTempEntry(TESForm *itemForm, SInt32 countDelta, ExtraDataList *xData);
TESObjectREFR* CreateInventoryRefEntry(TESObjectREFR *container, TESForm *itemForm, SInt32 countDelta, ExtraDataList *xData);