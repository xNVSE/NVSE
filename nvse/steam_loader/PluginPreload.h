#pragma once

#include "loader_common/PluginChecker.h"

typedef bool (*_NVSEPlugin_Preload)();

void PreloadPlugins() {
	char folderPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, folderPath);
	strcat_s(folderPath, "\\Data\\NVSE\\Plugins");

	_MESSAGE("Preloading plugins from %s", folderPath);

	WIN32_FIND_DATA findData;
	HANDLE find = INVALID_HANDLE_VALUE;
	char searchPath[MAX_PATH];
	sprintf_s(searchPath, "%s\\*.dll", folderPath);
	find = FindFirstFile(searchPath, &findData);

	if (find == INVALID_HANDLE_VALUE) {
		_ERROR("Failed to find any plugins!");
		return;
	}

	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		if (!IsNVSEPreloadPlugin(findData.cFileName, folderPath))
			continue;

		// Load the plugin for real
		char pluginPath[MAX_PATH];
		sprintf_s(pluginPath, "%s\\%s", folderPath, findData.cFileName);
		_DMESSAGE("Loading \"%s\"", pluginPath);
		HMODULE module = LoadLibrary(pluginPath);
		if (!module) {
			_DMESSAGE("Failed to load \"%s\"", findData.cFileName);
			continue;
		}

		_NVSEPlugin_Preload preload = (_NVSEPlugin_Preload)GetProcAddress(module, "NVSEPlugin_Preload");
		if (!preload) {
			_DMESSAGE("Failed to get NVSEPlugin_Preload for \"%s\"", findData.cFileName);
			continue;
		}

		_MESSAGE("Preloading \"%s\"", findData.cFileName);
		preload();

	} while (FindNextFile(find, &findData));
}