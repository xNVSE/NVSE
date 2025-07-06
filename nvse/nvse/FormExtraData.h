#pragma once

#include "NiTypes.h"

class FormExtraData
{
public:
	NiFixedString	name;
	UInt32			refCount = 0;

	FormExtraData(const NiFixedString& aName) : name(aName), refCount(0) {}
	virtual ~FormExtraData() {};
	virtual void DeleteThis() {
		this->~FormExtraData();
		FormHeap_Free(this);
	};

	void IncRefCount() {
		InterlockedIncrement(&refCount);
	}

	void DecRefCount() {
		if (InterlockedDecrement(&refCount) == 0) {
			DeleteThis();
		}
	}

	static bool Add(TESForm* form, FormExtraData* formExtraData);

	static void RemoveByName(TESForm* form, const char* name);
	static void RemoveByPtr(TESForm* form, FormExtraData* formExtraData);

	static FormExtraData* Get(const TESForm* form, const char* name);

	static UInt32 GetAll(const TESForm* form, FormExtraData** outData);

	static void WriteHooks();
};
