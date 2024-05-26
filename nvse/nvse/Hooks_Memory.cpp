#include "Hooks_Memory.h"
#include "SafeWrite.h"
#include "Utilities.h"

// greets to sheson for his work on this
// it must run before static initializers are run

enum HeapType {
	kHeapType_Default = 0,
	kHeapType_File,
	kHeapType_Max
};

const char* heapNames[kHeapType_Max] = {
	"Default",
	"File",
};

// In MB
const UInt32 vanillaHeapSizes[kHeapType_Max] = {
	200,
	64,
};

const UInt32 maxHeapSizes[kHeapType_Max] = {
	1024,
	192,
};

UInt32 userHeapSizes[kHeapType_Max] = {
	0,
	0,
};

constexpr UInt32 heapAddresses[kHeapType_Max] = {
	0x866EA0,
	0x866F89,
};

void Hooks_Memory_PreloadCommit(bool isNoGore)
{
	GetNVSEConfigOption_UInt32("Memory", "DefaultHeapInitialAllocMB", &userHeapSizes[kHeapType_Default]);
	GetNVSEConfigOption_UInt32("Memory", "FileHeapSizeMB", &userHeapSizes[kHeapType_File]);

	// Check if values differ from vanilla
	for (UInt32 i = 0; i < kHeapType_Max; i++) {
		if (userHeapSizes[i] > maxHeapSizes[i]) {
			_MESSAGE("Warning: %s heap size is greater than maximum allowed value. Using maximum value of %i", heapNames[i], maxHeapSizes[i]);
			userHeapSizes[i] = maxHeapSizes[i];
		}
		else if (userHeapSizes[i] < vanillaHeapSizes[i]) {
			_MESSAGE("Warning: %s heap size is less than vanilla value. Using vanilla value of %i", heapNames[i], vanillaHeapSizes[i]);
			userHeapSizes[i] = vanillaHeapSizes[i];
		}

		if (userHeapSizes[i] != vanillaHeapSizes[i]) {
			_MESSAGE("Overriding %s heap size: %dMB", heapNames[i], userHeapSizes[i]);
			SafeWrite32(heapAddresses[i], userHeapSizes[i] * 0x100000);
		}
	}
}
