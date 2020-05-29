#pragma once

#include "InventoryInfo.h"
#include <map>
#include <vector>
#include <queue>

class TESObjectREFR;

// InventoryReference represents a temporary reference to a stack of items in an inventory
// temp refs are valid only for the frame during which they were created

class InventoryReference {
public:
	struct Data {
		TESForm								* type;
		ExtraContainerChanges::EntryData	* entry;
		ExtraDataList						* xData;

		Data(TESForm* t, ExtraContainerChanges::EntryData* en, ExtraDataList* ex) : type(t), entry(en), xData(ex) { }
		Data(const Data& rhs) : type(rhs.type), entry(rhs.entry), xData(rhs.xData) { }
		Data() : type(NULL), entry(NULL), xData(NULL) { }

		static void CreateForUnextendedEntry(ExtraContainerChanges::EntryData* entry, SInt32 totalCount, std::vector<Data> &dataOut);
	};

	static InventoryReference* Create(TESObjectREFR* container, const Data &data, bool bValidate) { return new InventoryReference(container, data, bValidate); }

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
	bool Drop();
	bool SetEquipped(bool bEquipped);
	void SetRemoved() { m_bRemoved = true; }
	void Release();

	static InventoryReference* GetForRefID(UInt32 refID);
	static void Clean();									// called from main loop to destroy any temp refs
	static bool HasData() { return s_refmap.size() > 0; }	// provides a quick check from main loop to avoid unnecessary calls to Clean()
	static std::map<UInt32, InventoryReference*> *GetSingleton();	// returns the address of s_refmap

private:
	class DeferredAction 
	{
	public:
		// an operation which could potentially invalidate the extradatalist, deferred until inv ref is released
		DeferredAction(const Data& data) : m_itemData(data) { }
		virtual bool Execute(InventoryReference* iref) = 0;

		const Data& Data() { return m_itemData; }
	private:
		InventoryReference::Data	m_itemData;
	};

	class DeferredEquipAction : public DeferredAction
	{
	public:
		DeferredEquipAction(const InventoryReference::Data& data) : DeferredAction(data) { }
		virtual bool Execute(InventoryReference* iref);
	};

	class DeferredRemoveAction : public DeferredAction
	{
	public:
		DeferredRemoveAction(const InventoryReference::Data& data, TESObjectREFR* target = NULL) : DeferredAction(data), m_target(target) { }
		virtual bool Execute(InventoryReference* iref);
	private:
		TESObjectREFR*	m_target;
	};		

	InventoryReference(TESObjectREFR* container, const Data &data, bool bValidate);

	Data			m_data;
	TESObjectREFR	* m_containerRef;
	TESObjectREFR	* m_tempRef;
	std::queue<DeferredAction*>	m_deferredActions;
	bool			m_bDoValidation;
	bool			m_bRemoved;

	bool Validate();
	void QueueAction(DeferredAction* action);
	void DoDeferredActions();
	SInt16 GetCount();
	static std::map<UInt32, InventoryReference*>	s_refmap;	// maps refIDs of temp refs to InventoryReferences
};
