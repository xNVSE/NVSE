#pragma once

enum ModListType : UInt32 {
	kNormal = 0,
	kSmall = 1,
	kMedium = 2,
	kOverlay = 3,

	kCount
};

extern std::vector<const class ModInfo*> g_saveModList[ModListType::kCount];
extern std::vector<std::string> g_saveModListNames[ModListType::kCount];

void Init_CoreSerialization_Callbacks();

