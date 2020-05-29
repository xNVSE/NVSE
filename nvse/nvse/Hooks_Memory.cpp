#include "Hooks_Memory.h"
#include "SafeWrite.h"
#include "Utilities.h"

// greets to sheson for his work on this
// it must run before static initializers are run

void Hooks_Memory_PreloadCommit(bool isNoGore)
{
	// these are the values used by the game
	const UInt32 kVanilla_defaultHeapInitialAllocMB = 200;
	const UInt32 kVanilla_scrapHeapSizeMB = 64;

	// clamp to these values because going over that is stupid
	const UInt32 kMax_defaultHeapInitialAllocMB = 500;	// issues have been reported while using 500
	const UInt32 kMax_scrapHeapSizeMB = 128;

	// in megabytes, -256 depending on bInitiallyLoadAllClips:Animation
	UInt32 defaultHeapInitialAllocMB = kVanilla_defaultHeapInitialAllocMB;
	UInt32 scrapHeapSizeMB = kVanilla_scrapHeapSizeMB;

	// Debug value defaultHeapInitialAllocMB = 400;
	GetNVSEConfigOption_UInt32("Memory", "DefaultHeapInitialAllocMB", &defaultHeapInitialAllocMB);
	GetNVSEConfigOption_UInt32("Memory", "ScrapHeapSizeMB", &scrapHeapSizeMB);

	if(	(defaultHeapInitialAllocMB != kVanilla_defaultHeapInitialAllocMB) ||
		(scrapHeapSizeMB != kVanilla_scrapHeapSizeMB))
	{
		_MESSAGE("overriding memory pool sizes");

		if(defaultHeapInitialAllocMB >= kMax_defaultHeapInitialAllocMB)
		{
			_MESSAGE("%dMB default heap is too large, clamping to %dMB. using your value will make the game unstable.", defaultHeapInitialAllocMB, kMax_defaultHeapInitialAllocMB);
			defaultHeapInitialAllocMB = kMax_defaultHeapInitialAllocMB;
		}

		if(scrapHeapSizeMB >= kMax_scrapHeapSizeMB)
		{
			_MESSAGE("%dMB scrap heap is too large, clamping to %dMB. using your value will make the game unstable.", kMax_scrapHeapSizeMB, kMax_defaultHeapInitialAllocMB);
			scrapHeapSizeMB = kMax_scrapHeapSizeMB;
		}

		if(defaultHeapInitialAllocMB < kVanilla_defaultHeapInitialAllocMB)
		{
			_MESSAGE("%dMB default heap is too small, clamping to %dMB. using your value will make the game unstable.", defaultHeapInitialAllocMB, kVanilla_defaultHeapInitialAllocMB);
			defaultHeapInitialAllocMB = kVanilla_defaultHeapInitialAllocMB;
		}

		if(scrapHeapSizeMB < kVanilla_scrapHeapSizeMB)
		{
			_MESSAGE("%dMB scrap heap is too small, clamping to %dMB. using your value will make the game unstable.", kMax_scrapHeapSizeMB, kVanilla_scrapHeapSizeMB);
			scrapHeapSizeMB = kVanilla_scrapHeapSizeMB;
		}

		_MESSAGE("default heap = %dMB", defaultHeapInitialAllocMB);
		//_MESSAGE("scrap heap = %dMB", scrapHeapSizeMB);

		if (isNoGore) {
			SafeWrite32(0x0086696F + 1, defaultHeapInitialAllocMB  * 1024 * 1024); // passed directly to FormHeap_allocate.
			// SafeWrite32(0x00zzzzz7 + 1, scrapHeapSizeMB * 1024 );
		}
		else {
			SafeWrite32(0x00866E9F + 1, defaultHeapInitialAllocMB  * 1024 * 1024); // passed directly to FormHeap_allocate.
			// SafeWrite32(0x00AA53F7 + 1, scrapHeapSizeMB * 1024 );
		}
	}
}
