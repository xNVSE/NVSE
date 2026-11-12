#include "GameData.h"


#if RUNTIME
DataHandler* DataHandler::Get()
{
	DataHandler** g_dataHandler = (DataHandler**)0x011C3F2C;
	return *g_dataHandler;
}
#else

DataHandler* DataHandler::Get()
{
	DataHandler** g_dataHandler = (DataHandler**)0xED3B0C;
	return *g_dataHandler;
}

#endif

ModInfo* DataHandler::GetModByFormID(UInt32 formID) const {
	UInt32 modIndex = (formID >> 24) & 0xFF;
	if (SupportsNewFileTypes() && modIndex >= 0xFD)
		modIndex = formID;
	return GetMod(modIndex);
}

ModInfo* DataHandler::GetMod(UInt32 auiIndex) const {
#if RUNTIME
	return ThisStdCall<ModInfo*>(0x465010, this, auiIndex);
#else
	return ThisStdCall<ModInfo*>(0x4CDFA0, this, auiIndex);
#endif
}

const ModInfo * DataHandler::GetModByName(const char * modName) const {
	return ThisStdCall<const ModInfo*>(0x462F40, this, modName);
}

UInt8 DataHandler::GetModIndex(const char* modName) const {
	const ModInfo* mod = GetModByName(modName);
	if (mod)
		return mod->modIndex;

	return 0xFF;
}

const char* DataHandler::GetNthModName(UInt8 modIndex) const {
	if (SupportsNewFileTypes() && modIndex > 0xFD)
		return "";

	if (modList.GetNormalModCount() <= modIndex || modIndex == 0xFF)
		return "";

	ModInfo* modInfo = modList.GetMod(modIndex);
	if (modInfo)
		return modInfo->name;
	
	return "";
}

const char* DataHandler::GetNthModName(UInt8 modIndex, UInt16 smallIndex) const {
	UInt32 finalModIndex = modIndex;
	if (SupportsNewFileTypes()) {
		if (modIndex == 0xFE)
			finalModIndex = modIndex << 24 | smallIndex << 12;
		else if (modIndex == 0xFD)
			finalModIndex = modIndex << 24 | smallIndex << 16;
	}

	const ModInfo* mod = GetMod(finalModIndex);
	if (mod)
		return mod->name;
	else
		return "";
}

const char* DataHandler::GetModNameForForm(const TESForm* form) const {
	const UInt8 index = form->GetModIndex();
	const ModInfo* mod = nullptr;
	if (SupportsNewFileTypes() && index >= 0xFD)
		mod = GetMod(form->refID);
	else
		mod = GetMod(index);

	if (mod)
		return mod->name;
	else
		return "";
}

UInt32 DataHandler::GetNormalModCount() const {
	return modList.GetNormalModCount();
}

ModInfo* DataHandler::GetNormalMod(UInt32 auiIndex) const {
	return modList.GetMod(auiIndex);
}

UInt32 DataHandler::GetSmallModCount() const {
	if (SupportsNewFileTypes())
		return modList.GetSmallModCount();
	return 0;
}

ModInfo* DataHandler::GetSmallMod(UInt32 auiIndex) const {
	if (SupportsNewFileTypes())
		return modList.GetSmallMod(auiIndex);
	return nullptr;
}

UInt32 DataHandler::GetMediumModCount() const {
	if (SupportsNewFileTypes())
		return modList.GetMediumModCount();
	return 0;
}

ModInfo* DataHandler::GetMediumMod(UInt32 auiIndex) const {
	if (SupportsNewFileTypes())
		return modList.GetMediumMod(auiIndex);
	return nullptr;
}

UInt32 DataHandler::GetOverlayModCount() const {
	if (SupportsNewFileTypes())
		return modList.GetOverlayModCount();
	return 0;
}

ModInfo* DataHandler::GetOverlayMod(UInt32 auiIndex) const {
	if (SupportsNewFileTypes())
		return modList.GetOverlayMod(auiIndex);
	return nullptr;
}

void DataHandler::DisableAssignFormIDs(bool shouldAsssign)
{
	ThisStdCall(0x464D30, this, shouldAsssign);
}

bool DataHandler::IsFormIDInUse(UInt32 formID) const {
	return ThisStdCall<bool>(0x469760, this, formID);
}

UInt32 DataHandler::BuildFormID(UInt8 mainIndex, UInt32 formIndex, UInt16 secondIndex, bool useSecondary) {
	UInt32 formID = 0;
	if (useSecondary && HasNewFileTypeSupport()) {
		if (mainIndex == 0xFD)
			formID = 0xFD000000 | UInt32(secondIndex) << 16 | (formIndex & 0x0000FFFF);
		else if (mainIndex == 0xFE)
			formID = 0xFE000000 | UInt32(secondIndex) << 12 | (formIndex & 0x00000FFF);
		return formID;
	}
	
	formID = UInt32(mainIndex) << 24 | (formIndex & 0x00FFFFFF);
	return formID;
}

struct IsModLoaded
{
	bool Accept(ModInfo* pModInfo) const {
		return pModInfo->IsLoaded();
	}
};

UInt8 DataHandler::GetActiveModCount() const
{
	return modList.GetNormalModCount();
}

ModInfo::ModInfo() {
	//
};

ModInfo::~ModInfo() {
	//
};

ModInfo* ModInfo::GetIndexFile(UInt32 index) const {
#if RUNTIME
	return ThisStdCall<ModInfo*>(0x471A10, this, index);
#else
	return ThisStdCall<ModInfo*>(0x4DE2F0, this, index);
#endif
}

#if RUNTIME
ModInfo* ModInfo::GetFileForTempID(UInt32 formID) {
	return CdeclCall<ModInfo*>(0x474060, formID);
}
#endif

ModInfo* ModList::GetMod(UInt8 modIndex) const {
	if (modIndex >= GetNormalModCount())
		return nullptr;

	if (DataHandler::HasNewFileTypeSupport())
		return normalFiles.GetAt(modIndex);

	return loadedMods[modIndex];
}

ModInfo* ModList::GetSmallMod(UInt16 modIndex) const {
	if (modIndex >= GetSmallModCount())
		return nullptr;

	return smallFiles.GetAt(modIndex);
}

ModInfo* ModList::GetMediumMod(UInt16 modIndex) const {
	if (modIndex >= GetMediumModCount())
		return nullptr;

	return mediumFiles.GetAt(modIndex);
}

ModInfo* ModList::GetOverlayMod(UInt32 modIndex) const {
	if (modIndex >= GetOverlayModCount())
		return nullptr;

	return overlayFiles.GetAt(modIndex);
}

UInt32 ModList::GetNormalModCount() const {
	if (DataHandler::HasNewFileTypeSupport())
		return normalFiles.GetSize();

	return loadedModCount;
}

UInt32 ModList::GetSmallModCount() const {
	return smallFiles.GetSize();
}

UInt32 ModList::GetMediumModCount() const {
	return mediumFiles.GetSize();
}

UInt32 ModList::GetOverlayModCount() const {
	return overlayFiles.GetSize();
}
