#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include "common/ICriticalSection.h"

class Script;

#if NVSE_CORE && RUNTIME
constexpr std::string_view ScriptFilesPath = "data/nvse/user_defined_functions/";

extern UnorderedMap<char*, Script*> cachedFileUDFs;
extern ICriticalSection g_cachedUdfCS;
Script* CompileAndCacheScript(const char* relPath);

// Also checks in nested sub-folders in the path.
void CacheAllScriptsInPath(std::string_view pathStr);
#endif