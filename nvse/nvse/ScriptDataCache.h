#pragma once

class Script;

namespace ScriptDataCache
{
    extern bool g_enabled;
    bool LoadScriptDataCacheFromFile();
    bool SaveScriptDataCacheToFile();
    bool LoadCachedDataToScript(const char* scriptText, Script* script);
    void AddCompiledScriptToCache(const char* scriptText, Script* script);
};
