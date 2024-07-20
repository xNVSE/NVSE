#pragma once

bool PluginHasExport(const char* fileName, const char* folderPath, const char* functionName);

bool IsNVSEPlugin(const char* fileName, const char* folderPath);

bool IsNVSEPreloadPlugin(const char* fileName, const char* folderPath);