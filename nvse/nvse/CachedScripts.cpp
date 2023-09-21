#include "CachedScripts.h"
#include <filesystem>
#include <fstream>
#include "GameScript.h"
#include "GameAPI.h"

#if NVSE_CORE
std::unordered_map<std::string, Script*> cachedFileUDFs;
ICriticalSection g_cachedUdfCS;

Script* CompileAndCacheScript(std::filesystem::path fullPath, Script* scriptObj, bool useLocks)
{
	if (!fullPath.has_extension())
		return nullptr;

	std::ifstream ifs(fullPath);
	if (!ifs)
		return nullptr;

	std::ostringstream ss;
	ss << ifs.rdbuf();

	std::string udfName;
	{
		if (useLocks)
			g_cachedUdfCS.Enter();

		udfName = "nvseRuntimeScript"
			+ std::to_string(cachedFileUDFs.size()) // lock due to accessing this global
			+ fullPath.stem().string();

		if (useLocks)
			g_cachedUdfCS.Leave();
	}

	auto* script = CompileScriptEx(ss.str().c_str(), udfName.c_str());
	if (!script)
		return nullptr;

	if (scriptObj && scriptObj->GetModIndex() != 0xFF)
	{
		const auto nextFormId = GetNextFreeFormID(scriptObj->refID);
		if (nextFormId >> 24 == scriptObj->GetModIndex())
		{
			script->SetRefID(nextFormId, true);
		}
	}

	script->SetEditorID(udfName.c_str());

	// Cache result
	{
		if (useLocks)
			g_cachedUdfCS.Enter();

		cachedFileUDFs[fullPath.string()] = script;

		if (useLocks)
			g_cachedUdfCS.Leave();
	}
	return script;
}

Script* CompileAndCacheScript(const char* path, Script* scriptObj)
{
	// Pretend this is only for UDFs, since for 99% of use cases that's all this will be used for.
	std::filesystem::path fullPath = std::string(ScriptFilesPath) + path;
	if (!std::filesystem::exists(fullPath))
		return nullptr;

	return CompileAndCacheScript(fullPath, scriptObj, true);
}

void CacheAllScriptsInPath(std::string_view pathStr)
{
	std::filesystem::path path = pathStr;
	if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
	{
		for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(path))
		{
			if (dir_entry.is_regular_file())
			{
				CompileAndCacheScript(dir_entry.path(), nullptr, false);
			}
		}
	}

}
#endif