#include "ScriptDataCache.h"
#include "xxhash64.h"
#include "GameScript.h"
#include "Utilities.h"
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_set>
#include <unordered_map>

#include "ScriptUtils.h"
#include "GameData.h"

namespace ScriptDataCache
{
    bool g_enabled = true;

#pragma pack(push, 1)
    struct CacheHeader
    {
        UInt32 magic;
        UInt32 version;
        UInt32 entryCount;
        UInt32 indexOffset;
        UInt32 flags;
        UInt8 reserved[44];
    };
    static_assert(sizeof(CacheHeader) == 64);

    struct IndexEntry
    {
        UInt64 hash;
        UInt32 scriptLength;
        UInt32 blobOffset;
        UInt32 blobSize;
    };
    static_assert(sizeof(IndexEntry) == 20);

    // Followed by nameLen bytes of name data
    struct SerializedVarInfo
    {
        UInt32 idx;
        UInt32 pad04;
        double data;
        UInt8 type;
        UInt16 nameLen;
    };
    static_assert(sizeof(SerializedVarInfo) == 19);

    // Uses mod name index instead of full refID to handle load order changes
    // Followed by nameLen bytes of name data
    struct SerializedRefInfo
    {
        UInt32 varIdx;
        UInt16 modNameIndex;  // 0xFFFF = null form, 0xFFFE = dynamic form
        UInt32 baseFormID;    // Lower 24 bits for static forms, full ID for dynamic
        UInt16 nameLen;
    };
    static_assert(sizeof(SerializedRefInfo) == 12);
#pragma pack(pop)

    struct PendingEntry
    {
        UInt64 hash;
        UInt32 scriptLength;
        std::vector<UInt8> data;
    };

    namespace
    {
        constexpr UInt32 CACHE_MAGIC = 'NVSC';
        constexpr UInt32 CACHE_VERSION = 3;
        
        HANDLE g_hFile = INVALID_HANDLE_VALUE;
        HANDLE g_hMapping = nullptr;
        void* g_basePtr = nullptr;
        size_t g_fileSize = 0;

        std::vector<IndexEntry> g_index;
        std::vector<PendingEntry> g_pendingEntries;
        std::unordered_set<std::pair<UInt64, UInt32>, pair_hash> g_pendingHashes;
        std::unordered_set<std::pair<UInt64, UInt32>, pair_hash> g_accessedHashes;  // Tracks accessed entries for stale pruning

        bool g_initialized = false;

        constexpr auto CACHE_FILE_PATH = "script_data_cache.bin";

        template<typename T>
        bool ReadData(const UInt8*& ptr, const UInt8* end, T& out)
        {
            if (ptr + sizeof(T) > end)
                return false;
            memcpy(&out, ptr, sizeof(T));
            ptr += sizeof(T);
            return true;
        }

        template <typename T>
        class ScopedList
        {
            tList<T> list;

        public:
            ScopedList() = default;
            ~ScopedList() { list.DeleteAll(); }

            ScopedList(ScopedList&& other) noexcept
            {
                list.m_listHead = other.list.m_listHead;
                other.list.m_listHead = {};
            }

            ScopedList& operator=(ScopedList&& other) noexcept
            {
                if (this != &other)
                {
                    list.DeleteAll();
                    list.m_listHead = other.list.m_listHead;
                    other.list.m_listHead = {};
                }
                return *this;
            }

            ScopedList(const ScopedList&) = delete;
            ScopedList& operator=(const ScopedList&) = delete;

            tList<T>& Get() { return list; }

            void MoveTo(tList<T>& target)
            {
                target.m_listHead = list.m_listHead;
                list.m_listHead = {};
            }
        };

        template<typename T>
        void WriteData(std::vector<UInt8>& buf, const T& value)
        {
            const auto* bytes = reinterpret_cast<const UInt8*>(&value);
            buf.insert(buf.end(), bytes, bytes + sizeof(T));
        }

        void WriteBytes(std::vector<UInt8>& buf, const void* data, size_t len)
        {
            if (len > 0 && data)
                buf.insert(buf.end(), static_cast<const UInt8*>(data),
                           static_cast<const UInt8*>(data) + len);
        }

        // Caller must verify ptr + nameLen <= end before calling
        void ReadNameString(const UInt8*& ptr, UInt16 nameLen, String& name)
        {
            name.m_data = nullptr;
            name.m_dataLen = 0;
            name.m_bufLen = 0;

            if (nameLen == 0)
                return;

            name.m_data = static_cast<char*>(FormHeap_Allocate(nameLen + 1));
            if (!name.m_data)
            {
                ptr += nameLen;
                return;
            }

            memcpy(name.m_data, ptr, nameLen);
            name.m_data[nameLen] = '\0';
            name.m_dataLen = nameLen;
            name.m_bufLen = nameLen + 1;
            ptr += nameLen;
        }

        void CleanupMapping()
        {
            if (g_basePtr)
            {
                UnmapViewOfFile(g_basePtr);
                g_basePtr = nullptr;
            }
            if (g_hMapping)
            {
                CloseHandle(g_hMapping);
                g_hMapping = nullptr;
            }
            if (g_hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_hFile);
                g_hFile = INVALID_HANDLE_VALUE;
            }
            g_fileSize = 0;
        }

        void WriteVarList(std::vector<UInt8>& buf, const Script::VarInfoList& varList)
        {
            WriteData(buf, varList.Count());

            for (const auto* var : varList)
            {
                if (!var) continue;

                SerializedVarInfo serialized;
                serialized.idx = var->idx;
                serialized.pad04 = var->pad04;
                serialized.data = var->data;
                serialized.type = var->type;
                serialized.nameLen = var->name.m_data ? var->name.m_dataLen : 0;

                WriteData(buf, serialized);
                WriteBytes(buf, var->name.m_data, serialized.nameLen);
            }
        }

        void WriteRefList(std::vector<UInt8>& buf, const Script::RefList& refList)
        {
            auto* dataHandler = DataHandler::Get();

            // Build mod name table with mod index -> table index mapping
            std::vector<const char*> modNames;
            std::unordered_map<UInt8, UInt16> modIndexToTableIndex;

            for (const auto* ref : refList)
            {
                if (!ref || !ref->form)
                    continue;

                const auto modIndex = ref->form->GetModIndex();
                if (modIndex == 0xFF || modIndexToTableIndex.contains(modIndex))
                    continue;

                const char* modName = dataHandler ? dataHandler->GetNthModName(modIndex) : nullptr;
                if (!modName)
                    continue;

                modIndexToTableIndex[modIndex] = static_cast<UInt16>(modNames.size());
                modNames.push_back(modName);
            }

            WriteData(buf, static_cast<UInt16>(modNames.size()));
            for (const char* name : modNames)
            {
                const auto len = static_cast<UInt16>(strlen(name));
                WriteData(buf, len);
                WriteBytes(buf, name, len);
            }

            WriteData(buf, refList.Count());

            for (const auto* ref : refList)
            {
                if (!ref) continue;

                SerializedRefInfo serialized;
                serialized.varIdx = ref->varIdx;
                serialized.nameLen = ref->name.m_data ? ref->name.m_dataLen : 0;

                auto [modNameIndex, baseFormID] = [&]() -> std::pair<UInt16, UInt32>
                {
                    if (!ref->form)
                        return {0xFFFF, 0};

                    const auto modIndex = ref->form->GetModIndex();
                    if (modIndex == 0xFF)
                        return {0xFFFE, ref->form->refID};

                    const auto it = modIndexToTableIndex.find(modIndex);
                    if (it == modIndexToTableIndex.end())
                        return {0xFFFF, 0};

                    return {it->second, ref->form->refID & 0x00FFFFFF};
                }();

                serialized.modNameIndex = modNameIndex;
                serialized.baseFormID = baseFormID;

                WriteData(buf, serialized);
                WriteBytes(buf, ref->name.m_data, serialized.nameLen);
            }
        }

        void WriteScriptBlob(std::vector<UInt8>& buf, const Script* script)
        {
            WriteData(buf, script->info);
            WriteBytes(buf, script->data, script->info.dataLength);
            WriteVarList(buf, script->varList);
            WriteRefList(buf, script->refList);
        }

        bool ReadVarList(const UInt8*& ptr, const UInt8* end, tList<VariableInfo>& varList)
        {
            UInt32 varCount;
            if (!ReadData(ptr, end, varCount))
                return false;

            varList.m_listHead.data = nullptr;
            varList.m_listHead.next = nullptr;

            if (varCount == 0)
                return true;

            auto* tail = &varList.m_listHead;

            for (UInt32 i = 0; i < varCount; i++)
            {
                SerializedVarInfo serialized;
                if (!ReadData(ptr, end, serialized))
                    return false;

                if (ptr + serialized.nameLen > end)
                    return false;

                auto* var = static_cast<VariableInfo*>(FormHeap_Allocate(sizeof(VariableInfo)));
                if (!var)
                    return false;

                var->idx = serialized.idx;
                var->pad04 = serialized.pad04;
                var->data = serialized.data;
                var->type = serialized.type;

                ReadNameString(ptr, serialized.nameLen, var->name);

                if (i == 0)
                    tail->data = var;
                else
                    tail = tail->Append(var);
            }

            return true;
        }

        bool ReadRefList(const UInt8*& ptr, const UInt8* end, tList<Script::RefVariable>& refList)
        {
            UInt16 modNameCount;
            if (!ReadData(ptr, end, modNameCount))
                return false;

            std::vector<std::string> modNames;
            modNames.reserve(modNameCount);

            for (UInt16 i = 0; i < modNameCount; i++)
            {
                UInt16 nameLen;
                if (!ReadData(ptr, end, nameLen))
                    return false;

                if (ptr + nameLen > end)
                    return false;

                modNames.emplace_back(reinterpret_cast<const char*>(ptr), nameLen);
                ptr += nameLen;
            }

            std::vector<UInt8> modIndices;
            modIndices.reserve(modNames.size());
            auto* dataHandler = DataHandler::Get();

            for (const auto& name : modNames)
            {
                UInt8 modIndex = dataHandler ? dataHandler->GetModIndex(name.c_str()) : 0xFF;
                modIndices.push_back(modIndex);
            }

            UInt32 refCount;
            if (!ReadData(ptr, end, refCount))
                return false;

            refList.m_listHead.data = nullptr;
            refList.m_listHead.next = nullptr;

            if (refCount == 0)
                return true;

            if (ptr + refCount * sizeof(SerializedRefInfo) > end)
                return false;

            auto* tail = &refList.m_listHead;

            for (UInt32 i = 0; i < refCount; i++)
            {
                SerializedRefInfo serialized;
                if (!ReadData(ptr, end, serialized))
                    return false;

                if (ptr + serialized.nameLen > end)
                    return false;

                auto* ref = static_cast<Script::RefVariable*>(FormHeap_Allocate(sizeof(Script::RefVariable)));
                if (!ref)
                    return false;

                ref->varIdx = serialized.varIdx;

                ref->form = [&]() -> TESForm*
                {
                    if (serialized.modNameIndex == 0xFFFF)
                        return nullptr;

                    if (serialized.modNameIndex == 0xFFFE)
                        return LookupFormByID(serialized.baseFormID);

                    if (serialized.modNameIndex >= modIndices.size())
                        return nullptr;

                    UInt8 currentModIndex = modIndices[serialized.modNameIndex];
                    if (currentModIndex == 0xFF)
                        return nullptr;

                    UInt32 reconstructedRefID = (static_cast<UInt32>(currentModIndex) << 24) | (serialized.baseFormID & 0x00FFFFFF);
                    return LookupFormByID(reconstructedRefID);
                }();

                ReadNameString(ptr, serialized.nameLen, ref->name);

                if (i == 0)
                    tail->data = ref;
                else
                    tail = tail->Append(ref);
            }

            return true;
        }

        bool ReadScriptBlob(const UInt8* blobData, size_t blobSize, Script* script)
        {
            const UInt8* ptr = blobData;
            const UInt8* end = blobData + blobSize;

            Script::ScriptInfo cachedInfo;
            if (!ReadData(ptr, end, cachedInfo))
                return false;

            if (ptr + cachedInfo.dataLength > end)
                return false;

            auto newData = MakeUnique(static_cast<UInt8*>(FormHeap_Allocate(cachedInfo.dataLength)));
            if (!newData)
                return false;

            memcpy(newData.get(), ptr, cachedInfo.dataLength);
            ptr += cachedInfo.dataLength;

            ScopedList<VariableInfo> varList;
            if (!ReadVarList(ptr, end, varList.Get()))
                return false;

            ScopedList<Script::RefVariable> refList;
            if (!ReadRefList(ptr, end, refList.Get()))
                return false;

            varList.MoveTo(script->varList);
            refList.MoveTo(script->refList);
            script->data = newData.release();
            script->info = cachedInfo;

            return true;
        }

        const IndexEntry* FindEntry(UInt64 hash, UInt32 length)
        {
            const auto key = std::make_pair(hash, length);
            const auto it = std::ranges::lower_bound(g_index, key, std::less{}, [](const IndexEntry& e) {
                return std::make_pair(e.hash, e.scriptLength);
            });
            if (it != g_index.end() && it->hash == hash && it->scriptLength == length)
                return &*it;
            return nullptr;
        }
    } // end anonymous namespace

    bool LoadScriptDataCacheFromFile()
    {
        CleanupMapping();

        if (!g_enabled)
            return true;

        g_index.clear();
        g_pendingEntries.clear();
        g_pendingHashes.clear();
        g_accessedHashes.clear();
        g_initialized = true;

        g_hFile = CreateFileA(CACHE_FILE_PATH,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);

        if (g_hFile == INVALID_HANDLE_VALUE)
            return true;  // No cache file yet

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(g_hFile, &fileSize))
        {
            CleanupMapping();
            return false;
        }
        g_fileSize = static_cast<size_t>(fileSize.QuadPart);

        if (g_fileSize < sizeof(CacheHeader))
        {
            CleanupMapping();
            return true;
        }

        g_hMapping = CreateFileMappingA(g_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);

        if (!g_hMapping)
        {
            CleanupMapping();
            return false;
        }

        g_basePtr = MapViewOfFile(g_hMapping,FILE_MAP_READ,0,0,0);

        if (!g_basePtr)
        {
            CleanupMapping();
            return false;
        }

        const auto* header = static_cast<const CacheHeader*>(g_basePtr);
        if (header->magic != CACHE_MAGIC || header->version != CACHE_VERSION)
        {
            CleanupMapping();
            return true;
        }

        if (header->entryCount == 0)
            return true;

        const auto indexEnd = static_cast<size_t>(header->indexOffset) + static_cast<size_t>(header->entryCount) * sizeof(IndexEntry);
        if (indexEnd > g_fileSize)
        {
            CleanupMapping();
            return true;
        }

        const auto* indexStart = reinterpret_cast<const IndexEntry*>(
            static_cast<const UInt8*>(g_basePtr) + header->indexOffset
        );
        g_index.assign_range(std::ranges::subrange(indexStart, indexStart + header->entryCount));
        return true;
    }

    bool SaveScriptDataCacheToFile()
    {
        if (!g_initialized || !g_enabled)
            return false;

        // Skip save if no changes needed
        if (g_pendingEntries.empty())
        {
            if (g_index.empty())
                return true;

            // Must save if any entry was not accessed (to prune stale entries)
            const bool allEntriesAccessed = std::ranges::all_of(g_index, [](const IndexEntry& entry)
            {
                return g_accessedHashes.contains({entry.hash, entry.scriptLength});
            });

            if (allEntriesAccessed)
                return true;
        }

        const auto tempPath = std::string(CACHE_FILE_PATH) + ".tmp";

        const auto hTempFile = CreateFileA(tempPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);

        if (hTempFile == INVALID_HANDLE_VALUE)
            return false;

        CacheHeader header = {
            .magic = CACHE_MAGIC,
            .version = CACHE_VERSION,
            .entryCount = 0,
            .indexOffset = 0,
            .flags = 0,
            .reserved = {}
        };
        memset(header.reserved, 0, sizeof(header.reserved));

        auto currentOffset = sizeof(CacheHeader);

        DWORD written;
        
        const auto write = [&](const void* data, DWORD size) -> bool
        {
            if (!WriteFile(hTempFile, data, size, &written, nullptr))
            {
                CloseHandle(hTempFile);
                DeleteFileA(tempPath.c_str());
                return false;
            }
            return true;
        };

        if (!write(&header, sizeof(header)))
            return false;

        std::vector<IndexEntry> newIndex;
        newIndex.reserve(g_accessedHashes.size() + g_pendingEntries.size());

        // Copy accessed entries (prunes stale/unused scripts)
        for (const auto& entry : g_index)
        {
            if (!g_accessedHashes.contains({entry.hash, entry.scriptLength}) && !g_pendingHashes.contains({entry.hash, entry.scriptLength}))
                continue;

            if (!g_basePtr || entry.blobOffset + entry.blobSize > g_fileSize)
                continue;

            const auto* blobData = static_cast<const UInt8*>(g_basePtr) + entry.blobOffset;
            if (!write(blobData, entry.blobSize))
                return false;

            auto newEntry = entry;
            newEntry.blobOffset = currentOffset;
            newIndex.emplace_back(newEntry);
            currentOffset += entry.blobSize;
        }

        for (const auto& pending : g_pendingEntries)
        {
            if (!write(pending.data.data(), pending.data.size()))
                return false;

            IndexEntry newEntry;
            newEntry.hash = pending.hash;
            newEntry.scriptLength = pending.scriptLength;
            newEntry.blobOffset = currentOffset;
            newEntry.blobSize = static_cast<UInt32>(pending.data.size());
            newIndex.emplace_back(newEntry);
            currentOffset += newEntry.blobSize;
        }

        std::ranges::sort(newIndex, [](const IndexEntry& a, const IndexEntry& b) {
            if (a.hash != b.hash)
                return a.hash < b.hash;
            return a.scriptLength < b.scriptLength;
        });

        header.indexOffset = currentOffset;
        header.entryCount = static_cast<UInt32>(newIndex.size());
        for (const auto& entry : newIndex)
        {
            if (!write(&entry, sizeof(entry)))
                return false;
        }

        SetFilePointer(hTempFile, 0, nullptr, FILE_BEGIN);
        if (!write(&header, sizeof(header)))
            return false;

        CloseHandle(hTempFile);
        CleanupMapping();

        if (!MoveFileExA(tempPath.c_str(), CACHE_FILE_PATH, MOVEFILE_REPLACE_EXISTING))
        {
            DeleteFileA(tempPath.c_str());
            return false;
        }

        g_pendingEntries.clear();
        g_pendingHashes.clear();
        g_accessedHashes.clear();
        g_initialized = false;

        return true;
    }

    bool LoadCachedDataToScript(const char* scriptText, Script* script)
    {
        if (!scriptText || !script || !g_enabled || !g_initialized || g_index.empty())
            return false;

        const auto length = strlen(scriptText);
        const auto hash = XXHash64::hash(scriptText, length, 0);

        const auto* entry = FindEntry(hash, length);
        if (!entry)
            return false;

        if (!g_basePtr || entry->blobOffset + entry->blobSize > g_fileSize)
            return false;

        const auto* blobData = static_cast<const UInt8*>(g_basePtr) + entry->blobOffset;
        if (!ReadScriptBlob(blobData, entry->blobSize, script))
            return false;

        g_accessedHashes.insert({hash, static_cast<UInt32>(length)});
        return true;
    }

    void AddCompiledScriptToCache(const char* scriptText, Script* script)
    {
        if (!scriptText || !script || !script->data
            || script->info.dataLength == 0 || !g_enabled || !g_initialized
            || GetLambdaParentScript(script))
            return;

        const auto length = strlen(scriptText);
        const auto hash = XXHash64::hash(scriptText, length, 0);

        if (g_pendingHashes.contains({hash, length}) || FindEntry(hash, length))
            return;
        g_pendingHashes.insert({hash, length});

        PendingEntry entry = {
            .hash = hash,
            .scriptLength = static_cast<UInt32>(length),
            .data = {},
        };
        WriteScriptBlob(entry.data, script);

        g_pendingEntries.emplace_back(std::move(entry));
    }
}
