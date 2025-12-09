#pragma once

class Script;

namespace ScriptDataCache
{
    bool LoadScriptDataCacheFromFile();
    bool SaveScriptDataCacheToFile();
    bool LoadCachedDataToScript(const char* scriptText, Script* script);
    void AddCompiledScriptToCache(const char* scriptText, Script* script);
};
