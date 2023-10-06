#include "CachedScripts.h"
#include <filesystem>
#include <fstream>
#include "GameScript.h"
#include "GameAPI.h"

#if NVSE_CORE && RUNTIME
UnorderedMap<char*, Script*> cachedFileUDFs;
ICriticalSection g_cachedUdfCS;

Script* CompileAndCacheScript(std::filesystem::path fullPath, bool useLocks)
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
			+ std::to_string(cachedFileUDFs.Size()) // lock due to accessing this global
			+ fullPath.stem().string();

		if (useLocks)
			g_cachedUdfCS.Leave();
	}

	auto* script = CompileScriptEx(ss.str().c_str(), udfName.c_str(), true);
	if (!script)
		return nullptr;

	script->SetEditorID(udfName.c_str());

	// Cache result
	{
		if (useLocks)
			g_cachedUdfCS.Enter();

		auto relPath = std::filesystem::relative(fullPath, ScriptFilesPath);
		cachedFileUDFs[const_cast<char*>(relPath.string().c_str())] = script;

		if (useLocks)
			g_cachedUdfCS.Leave();
	}
	return script;
}

Script* CompileAndCacheScript(const char* relPath)
{
	// Pretend this is only for UDFs, since for 99% of use cases that's all this will be used for.
	std::filesystem::path fullPath = std::string(ScriptFilesPath) + relPath;
	if (!std::filesystem::exists(fullPath))
		return nullptr;

	return CompileAndCacheScript(fullPath, true);
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
				if (!CompileAndCacheScript(dir_entry.path(), false))
				{
					std::string errMsg = std::format("xNVSE: Failed to precompile script file {}", 
						dir_entry.path().string());
					Console_Print(errMsg.c_str());
					_ERROR(errMsg.c_str());
				}
			}
		}
	}

}
#endif