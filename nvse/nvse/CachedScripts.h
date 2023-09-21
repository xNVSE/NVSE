#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include "common/ICriticalSection.h"

class Script;

#if NVSE_CORE
constexpr std::string_view ScriptFilesPath = "data/nvse/user_defined_functions/";

extern std::unordered_map<std::string, Script*> cachedFileUDFs;
extern ICriticalSection g_cachedUdfCS;
Script* CompileAndCacheScript(const char* path, Script* scriptObj);

// Also checks in nested sub-folders in the path.
void CacheAllScriptsInPath(std::string_view pathStr);
#endif