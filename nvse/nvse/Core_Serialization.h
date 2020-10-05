#pragma once

extern UInt8 s_preloadModRefIDs[0xFF];
extern UInt8 s_numPreloadMods;

UInt8 ResolveModIndexForPreload(UInt8 modIndexIn);
void Init_CoreSerialization_Callbacks();
