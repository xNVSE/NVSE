#pragma once

extern UInt8 s_preloadModRefIDs[0xFF];
extern UInt8 s_numPreloadMods;

void Core_PostLoadCallback(bool bLoadSucceeded);
UInt8 ResolveModIndexForPreload(UInt8 modIndexIn);
void Init_CoreSerialization_Callbacks();
