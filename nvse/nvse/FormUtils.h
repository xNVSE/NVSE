#pragma once

class ModInfo;

namespace FormUtils {

	bool __fastcall AdjustFormIDForMod(const ModInfo* mod, UInt32& formID);

	bool __fastcall AdjustFormIDForModByName(const char* modName, UInt32& formID);

}