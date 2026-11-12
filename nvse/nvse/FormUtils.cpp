#include "FormUtils.h"
#include "GameData.h"

namespace FormUtils {
#pragma optimize("y", on)

	__declspec(noinline) bool __fastcall AdjustFormIDForMod(const ModInfo* mod, UInt32& formID) {
		if (!mod)
			return false;

		const UInt32 modIndex = UInt32(mod->modIndex) << 24;
		UInt32 secondIndex = 0;
		UInt32 idMask = 0x00FFFFFF;
		if (DataHandler::HasNewFileTypeSupport()) {
			if (mod->IsOverlay()) [[unlikely]]
				return false;

			if (mod->IsSmall()) {
				idMask = 0x00000FFF;
				secondIndex = UInt32(mod->secondIndex) << 12;
			}
			else if (mod->IsMedium()) {
				idMask = 0x0000FFFF;
				secondIndex = UInt32(mod->secondIndex) << 16;
			}
		}
		
		formID = modIndex | secondIndex | (formID & idMask);
		return true;
	}

	__declspec(noinline) bool __fastcall AdjustFormIDForModByName(const char* modName, UInt32& formID) {
		const ModInfo* pIndexFile = DataHandler::Get()->GetModByName(modName);
		if (!pIndexFile) [[unlikely]]
			return false;

		return AdjustFormIDForMod(pIndexFile, formID);
	}

#pragma optimize("", on)
}