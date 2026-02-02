#pragma once

#include "loader_common/PluginChecker.h"
#include <vector>
#include <string>
#include <algorithm>

typedef bool (*_NVSEPlugin_Preload)();

void PreloadPlugins() {
	char folderPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, folderPath);
	strcat_s(folderPath, "\\Data\\NVSE\\Plugins");

	_MESSAGE("Preloading plugins from %s", folderPath);

	// The reason for recording names instead of full paths is because this code runs before MO2's filesystem fully resolves paths.
	// This can lead to cases where user has plugins visibly coming from both MO2's mods folder, and actual game folder.
	// This would break sorting.
	std::vector<std::string> pluginNames;

	{
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

			pluginNames.push_back(findData.cFileName);

		} while (FindNextFile(find, &findData));

		FindClose(find);
	}

	std::sort(pluginNames.begin(), pluginNames.end(),
		[](const std::string& a, const std::string& b) -> bool {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

	for (const auto& pluginName : pluginNames) {
		char pluginPath[MAX_PATH];
		sprintf_s(pluginPath, "%s\\%s", folderPath, pluginName.c_str());

		_DMESSAGE("Loading \"%s\"", pluginPath);
		HMODULE module = LoadLibrary(pluginPath);
		if (!module) {
			_DMESSAGE("Failed to load \"%s\"", pluginPath);
			continue;
		}

		// Load the plugin for real
		_NVSEPlugin_Preload preload = (_NVSEPlugin_Preload)GetProcAddress(module, "NVSEPlugin_Preload");
		if (!preload) {
			_DMESSAGE("Failed to get NVSEPlugin_Preload for \"%s\"", pluginPath);
			continue;
		}
		_MESSAGE("Preloading \"%s\"", pluginPath);
		preload();
	}
}