#pragma once

#include "GameExtraData.h"

// 30
// Taken from JIP LN NVSE plugin, with some tweaks.
// This is just a stripped-down version of xNVSE's internal InventoryReference class.
// It exists so plugins have limited access to the internal functions.
struct InventoryRef
{
	struct Data
	{
		TESForm* type;	// 00
		ExtraContainerChanges::EntryData* entry;	// 04
		ExtraDataList* xData;	// 08
	} data;
	TESObjectREFR* containerRef;	// 0C
	TESObjectREFR* tempRef;		// 10
	UInt8				pad14[24];		// 14
	bool				doValidation;	// 2C
	bool				removed;		// 2D
	UInt8				pad2E[2];		// 2E

	SInt32 GetCount() const { return data.entry->countDelta; }
#if 0
	// todo: fix these by porting over some functions from JIP
	ExtraDataList* CreateExtraData();
	ExtraDataList* __fastcall SplitFromStack(SInt32 maxStack = 1);
#endif
};

