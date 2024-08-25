#include "PluginChecker.h"
#include "imagehlp.h"

#pragma comment(lib, "imagehlp.lib")

constexpr UInt32 exportThreshold = 50;

const PIMAGE_EXPORT_DIRECTORY GetExportDirectory(PLOADED_IMAGE image) {
	DWORD size = 0;
	return static_cast<const PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToData(image->MappedAddress, false, IMAGE_DIRECTORY_ENTRY_EXPORT, &size));
}

bool PluginHasExport(const char* fileName, const char* folderPath, const char* functionName) {
	bool validPlugin = false;

	LOADED_IMAGE image;
	if (!MapAndLoad(fileName, folderPath, &image, true, true)) {
		_DMESSAGE("Failed to load \"%s\"", fileName);
		return validPlugin;
	}

	const PIMAGE_EXPORT_DIRECTORY exportDir = GetExportDirectory(&image);
	if (!exportDir) {
		_DMESSAGE("Failed to get export directory for \"%s\"", fileName);
		goto UNLOAD;
	}

	if (exportDir->NumberOfNames == 0) {
		_DMESSAGE("No exports found for \"%s\"", image.ModuleName);
		goto UNLOAD;
	}

	if (exportDir->NumberOfNames > exportThreshold) {
		_DMESSAGE("Module \"%s\" over %i exports - are we sure it's an NVSE plugin? Skipping...", fileName, exportThreshold);
		goto UNLOAD;
	}

	{
		const DWORD* dNameRVAs = static_cast<const DWORD*>(ImageRvaToVa(image.FileHeader, image.MappedAddress, exportDir->AddressOfNames, nullptr));
		for (size_t i = 0; i < exportDir->NumberOfNames; i++) {
			const char* funcName = static_cast<const char*>(ImageRvaToVa(image.FileHeader, image.MappedAddress, dNameRVAs[i], nullptr));
			if (strcmp(funcName, functionName) == 0) {
				validPlugin = true;
				break;
			}
		}
	}

UNLOAD:
	UnMapAndLoad(&image);

	return validPlugin;
}

bool IsNVSEPlugin(const char* fileName, const char* folderPath) {
	return PluginHasExport(fileName, folderPath, "NVSEPlugin_Query");
}

bool IsNVSEPreloadPlugin(const char* fileName, const char* folderPath) {
	return PluginHasExport(fileName, folderPath, "NVSEPlugin_Preload");
}
