#include "ScriptDataCache.h"
#include "xxhash64.h"
#include "GameScript.h"
#include "Utilities.h"
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "ScriptUtils.h"

namespace ScriptDataCache
{
    // File format constants
    static constexpr UInt32 CACHE_MAGIC = 'NVSC';   // NVSE Script Cache
    static constexpr UInt32 CACHE_VERSION = PACKED_NVSE_VERSION;
    static bool g_enabled = true;

    // Header structure (64 bytes, padded for alignment)
#pragma pack(push, 1)
    struct CacheHeader
    {
        UInt32 magic;           // 00
        UInt32 version;         // 04
        UInt32 entryCount;      // 08
        UInt32 indexOffset;     // 0C
        UInt32 flags;           // 10
        UInt8 reserved[44];     // 14 - pad to 64 bytes
    };
    static_assert(sizeof(CacheHeader) == 64, "CacheHeader must be 64 bytes");

    // Index entry (20 bytes as specified in design)
    struct IndexEntry
    {
        UInt64 hash;            // 00
        UInt32 scriptLength;    // 08
        UInt32 blobOffset;      // 0C
        UInt32 blobSize;        // 10
    };
    static_assert(sizeof(IndexEntry) == 20, "IndexEntry must be 20 bytes");

    // Serialized variable info (fixed-size portion, followed by name bytes)
    struct SerializedVarInfo
    {
        UInt32 idx;
        UInt32 pad04;
        double data;
        UInt8 type;
        UInt16 nameLen;
        // followed by nameLen bytes of name data
    };
    static_assert(sizeof(SerializedVarInfo) == 19, "SerializedVarInfo must be 19 bytes");

    // Serialized ref variable (fixed-size portion, followed by name bytes)
    struct SerializedRefInfo
    {
        UInt32 varIdx;
        UInt32 formRefID;
        UInt16 nameLen;
        // followed by nameLen bytes of name data
    };
    static_assert(sizeof(SerializedRefInfo) == 10, "SerializedRefInfo must be 10 bytes");
#pragma pack(pop)

    // Pending entry for new compilations during runtime
    struct PendingEntry
    {
        UInt64 hash;
        UInt32 scriptLength;
        std::vector<UInt8> data;
    };

    namespace
    {
        // Global state for memory-mapped file
        HANDLE g_hFile = INVALID_HANDLE_VALUE;
        HANDLE g_hMapping = nullptr;
        void* g_basePtr = nullptr;
        size_t g_fileSize = 0;

        // Index loaded into memory for fast binary search
        std::vector<IndexEntry> g_index;

        // Pending entries to be written on save
        std::vector<PendingEntry> g_pendingEntries;

        // Hash set for O(1) duplicate checking in pending entries
        std::unordered_set<UInt64> g_pendingHashes;

        // Hash set to track which cached entries were actually accessed this session
        // Used to prune stale entries (removed scripts) on save
        std::unordered_set<UInt64> g_accessedHashes;

        // Initialization flag
        bool g_initialized = false;

        // Path is cached since it doesn't change during runtime
        const std::string& GetCacheFilePath()
        {
            static std::string path = []() {
                std::string p = GetFalloutDirectory();
                if (!p.empty())
                    p += "script_data_cache.bin";
                return p;
            }();
            return path;
        }

        // Cleanup memory mapping resources
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

        // Compare function for binary search
        bool CompareIndexEntry(const IndexEntry& entry, const std::pair<UInt64, UInt32>& key)
        {
            if (entry.hash != key.first)
                return entry.hash < key.first;
            return entry.scriptLength < key.second;
        }

        // ============================================================================
        // Size calculation helpers
        // ============================================================================

        size_t CalculateVarListBlobSize(const Script::VarInfoList& varList)
        {
            size_t size = sizeof(UInt32); // varCount
            for (auto* var : varList)
            {
                if (!var) continue;
                size += sizeof(SerializedVarInfo);
                if (var->name.m_data)
                    size += var->name.m_dataLen;
            }
            return size;
        }

        size_t CalculateRefListBlobSize(const Script::RefList& refList)
        {
            size_t size = sizeof(UInt32); // refCount
            for (auto* ref : refList)
            {
                if (!ref) continue;
                size += sizeof(SerializedRefInfo);
                if (ref->name.m_data)
                    size += ref->name.m_dataLen;
            }
            return size;
        }

        size_t CalculateScriptBlobSize(const Script* script)
        {
            size_t size = sizeof(Script::ScriptInfo);
            size += script->info.dataLength;
            size += CalculateVarListBlobSize(script->varList);
            size += CalculateRefListBlobSize(script->refList);
            return size;
        }

        // ============================================================================
        // Serialization helpers (write)
        // ============================================================================

        void WriteVarList(const Script::VarInfoList& varList, UInt8*& ptr)
        {
            UInt32 varCount = varList.Count();
            memcpy(ptr, &varCount, sizeof(UInt32));
            ptr += sizeof(UInt32);

            for (auto* var : varList)
            {
                if (!var) continue;

                SerializedVarInfo serialized;
                serialized.idx = var->idx;
                serialized.pad04 = var->pad04;
                serialized.data = var->data;
                serialized.type = var->type;
                serialized.nameLen = var->name.m_data ? var->name.m_dataLen : 0;

                memcpy(ptr, &serialized, sizeof(SerializedVarInfo));
                ptr += sizeof(SerializedVarInfo);

                if (serialized.nameLen > 0)
                {
                    memcpy(ptr, var->name.m_data, serialized.nameLen);
                    ptr += serialized.nameLen;
                }
            }
        }

        void WriteRefList(const Script::RefList& refList, UInt8*& ptr)
        {
            UInt32 refCount = refList.Count();
            memcpy(ptr, &refCount, sizeof(UInt32));
            ptr += sizeof(UInt32);

            for (auto* ref : refList)
            {
                if (!ref) continue;

                SerializedRefInfo serialized;
                serialized.varIdx = ref->varIdx;
                serialized.formRefID = ref->form ? ref->form->refID : 0;
                serialized.nameLen = ref->name.m_data ? ref->name.m_dataLen : 0;

                memcpy(ptr, &serialized, sizeof(SerializedRefInfo));
                ptr += sizeof(SerializedRefInfo);

                if (serialized.nameLen > 0)
                {
                    memcpy(ptr, ref->name.m_data, serialized.nameLen);
                    ptr += serialized.nameLen;
                }
            }
        }

        void WriteScriptBlob(const Script* script, UInt8* dest)
        {
            UInt8* ptr = dest;

            // Write ScriptInfo
            memcpy(ptr, &script->info, sizeof(Script::ScriptInfo));
            ptr += sizeof(Script::ScriptInfo);

            // Write bytecode
            memcpy(ptr, script->data, script->info.dataLength);
            ptr += script->info.dataLength;

            // Write varList and refList
            WriteVarList(script->varList, ptr);
            WriteRefList(script->refList, ptr);
        }

        // ============================================================================
        // Deserialization helpers (read)
        // ============================================================================

        bool ReadVarList(const UInt8*& ptr, const UInt8* end, Script::VarInfoList& varList)
        {
            if (ptr + sizeof(UInt32) > end)
                return false;

            UInt32 varCount;
            memcpy(&varCount, ptr, sizeof(UInt32));
            ptr += sizeof(UInt32);

            // Initialize list head directly (skip Init() call overhead)
            varList.m_listHead.data = nullptr;
            varList.m_listHead.next = nullptr;

            if (varCount == 0)
                return true;

            // Validate minimum data size upfront to fail fast on corrupt data
            if (ptr + varCount * sizeof(SerializedVarInfo) > end)
                return false;

            // Track tail for O(1) appends instead of O(n) traversals
            using VarNode = ListNode<VariableInfo>;
            VarNode* tail = &varList.m_listHead;

            for (UInt32 i = 0; i < varCount; i++)
            {
                if (ptr + sizeof(SerializedVarInfo) > end)
                    return false;

                SerializedVarInfo serialized;
                memcpy(&serialized, ptr, sizeof(SerializedVarInfo));
                ptr += sizeof(SerializedVarInfo);

                if (ptr + serialized.nameLen > end)
                    return false;

                auto* var = static_cast<VariableInfo*>(FormHeap_Allocate(sizeof(VariableInfo)));
                if (!var)
                    return false;

                var->idx = serialized.idx;
                var->pad04 = serialized.pad04;
                var->data = serialized.data;
                var->type = serialized.type;

                if (serialized.nameLen > 0)
                {
                    var->name.m_data = static_cast<char*>(FormHeap_Allocate(serialized.nameLen + 1));
                    if (var->name.m_data)
                    {
                        memcpy(var->name.m_data, ptr, serialized.nameLen);
                        var->name.m_data[serialized.nameLen] = '\0';
                        var->name.m_dataLen = serialized.nameLen;
                        var->name.m_bufLen = serialized.nameLen + 1;
                    }
                    else
                    {
                        var->name.m_data = nullptr;
                        var->name.m_dataLen = 0;
                        var->name.m_bufLen = 0;
                    }
                }
                else
                {
                    var->name.m_data = nullptr;
                    var->name.m_dataLen = 0;
                    var->name.m_bufLen = 0;
                }
                ptr += serialized.nameLen;

                // O(1) append using tail tracking
                if (i == 0)
                {
                    // First item goes directly in list head (no node allocation)
                    tail->data = var;
                }
                else
                {
                    // Subsequent items: allocate node and link at tail
                    tail = tail->Append(var);
                }
            }

            return true;
        }

        bool ReadRefList(const UInt8*& ptr, const UInt8* end, Script::RefList& refList)
        {
            if (ptr + sizeof(UInt32) > end)
                return false;

            UInt32 refCount;
            memcpy(&refCount, ptr, sizeof(UInt32));
            ptr += sizeof(UInt32);

            // Initialize list head directly (skip Init() call overhead)
            refList.m_listHead.data = nullptr;
            refList.m_listHead.next = nullptr;

            if (refCount == 0)
                return true;

            // Validate minimum data size upfront to fail fast on corrupt data
            if (ptr + refCount * sizeof(SerializedRefInfo) > end)
                return false;

            // Track tail for O(1) appends instead of O(n) traversals
            using RefNode = ListNode<Script::RefVariable>;
            RefNode* tail = &refList.m_listHead;

            for (UInt32 i = 0; i < refCount; i++)
            {
                if (ptr + sizeof(SerializedRefInfo) > end)
                    return false;

                SerializedRefInfo serialized;
                memcpy(&serialized, ptr, sizeof(SerializedRefInfo));
                ptr += sizeof(SerializedRefInfo);

                if (ptr + serialized.nameLen > end)
                    return false;

                auto* ref = static_cast<Script::RefVariable*>(FormHeap_Allocate(sizeof(Script::RefVariable)));
                if (!ref)
                    return false;

                ref->varIdx = serialized.varIdx;
                ref->form = serialized.formRefID ? LookupFormByID(serialized.formRefID) : nullptr;

                if (serialized.nameLen > 0)
                {
                    ref->name.m_data = static_cast<char*>(FormHeap_Allocate(serialized.nameLen + 1));
                    if (ref->name.m_data)
                    {
                        memcpy(ref->name.m_data, ptr, serialized.nameLen);
                        ref->name.m_data[serialized.nameLen] = '\0';
                        ref->name.m_dataLen = serialized.nameLen;
                        ref->name.m_bufLen = serialized.nameLen + 1;
                    }
                    else
                    {
                        ref->name.m_data = nullptr;
                        ref->name.m_dataLen = 0;
                        ref->name.m_bufLen = 0;
                    }
                }
                else
                {
                    ref->name.m_data = nullptr;
                    ref->name.m_dataLen = 0;
                    ref->name.m_bufLen = 0;
                }
                ptr += serialized.nameLen;

                // O(1) append using tail tracking
                if (i == 0)
                {
                    // First item goes directly in list head (no node allocation)
                    tail->data = ref;
                }
                else
                {
                    // Subsequent items: allocate node and link at tail
                    tail = tail->Append(ref);
                }
            }

            return true;
        }

        bool ReadScriptBlob(const UInt8* blobData, size_t blobSize, Script* script)
        {
            const UInt8* ptr = blobData;
            const UInt8* end = blobData + blobSize;

            // Read ScriptInfo
            if (ptr + sizeof(Script::ScriptInfo) > end)
                return false;

            Script::ScriptInfo cachedInfo;
            memcpy(&cachedInfo, ptr, sizeof(Script::ScriptInfo));
            ptr += sizeof(Script::ScriptInfo);

            // Validate and read bytecode
            if (ptr + cachedInfo.dataLength > end)
                return false;

            UInt8* newData = static_cast<UInt8*>(FormHeap_Allocate(cachedInfo.dataLength));
            if (!newData)
                return false;

            memcpy(newData, ptr, cachedInfo.dataLength);
            ptr += cachedInfo.dataLength;

            // Read varList
            if (!ReadVarList(ptr, end, script->varList))
            {
                FormHeap_Free(newData);
                return false;
            }

            // Read refList
            if (!ReadRefList(ptr, end, script->refList))
            {
                FormHeap_Free(newData);
                return false;
            }

            // Success - assign to script
            script->data = newData;
            script->info = cachedInfo;

            return true;
        }
    } // end anonymous namespace

    bool LoadScriptDataCacheFromFile()
    {
        if (!g_enabled)
            return true;
        // Cleanup any previous state
        CleanupMapping();
        g_index.clear();
        g_pendingEntries.clear();
        g_pendingHashes.clear();
        g_accessedHashes.clear();
        g_initialized = true;

        std::string path = GetCacheFilePath();
        if (path.empty())
            return false;

        // Try to open existing cache file
        g_hFile = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (g_hFile == INVALID_HANDLE_VALUE)
        {
            // No cache file exists yet - that's OK, start fresh
            return true;
        }

        // Get file size
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(g_hFile, &fileSize))
        {
            CleanupMapping();
            return false;
        }
        g_fileSize = static_cast<size_t>(fileSize.QuadPart);

        // Validate minimum size for header
        if (g_fileSize < sizeof(CacheHeader))
        {
            CleanupMapping();
            return true; // Treat as empty/invalid cache
        }

        // Create file mapping
        g_hMapping = CreateFileMappingA(
            g_hFile,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (!g_hMapping)
        {
            CleanupMapping();
            return false;
        }

        // Map view of file
        g_basePtr = MapViewOfFile(
            g_hMapping,
            FILE_MAP_READ,
            0,
            0,
            0
        );

        if (!g_basePtr)
        {
            CleanupMapping();
            return false;
        }

        // Validate header
        const CacheHeader* header = static_cast<const CacheHeader*>(g_basePtr);
        if (header->magic != CACHE_MAGIC || header->version != CACHE_VERSION)
        {
            // Invalid or incompatible cache version - start fresh
            CleanupMapping();
            return true;
        }

        // Validate index bounds
        if (header->entryCount > 0)
        {
            size_t indexEnd = static_cast<size_t>(header->indexOffset) +
                              static_cast<size_t>(header->entryCount) * sizeof(IndexEntry);
            if (indexEnd > g_fileSize)
            {
                // Corrupted index - start fresh
                CleanupMapping();
                return true;
            }

            // Load index into memory for fast binary search
            const IndexEntry* indexStart = reinterpret_cast<const IndexEntry*>(
                static_cast<const UInt8*>(g_basePtr) + header->indexOffset
            );
            g_index.assign(indexStart, indexStart + header->entryCount);
        }

        return true;
    }

    bool SaveScriptDataCacheToFile()
    {
        if (!g_initialized || !g_enabled)
            return false;

        // Optimization: Check if save is actually needed
        if (g_pendingEntries.empty())
        {
            // No new entries - check if any existing entries would be pruned
            if (g_index.empty())
                return true;  // Nothing to save or prune

            // If all existing entries were accessed, file content wouldn't change
            // If any entry was NOT accessed, we must save to prune stale entries
            const bool allEntriesAccessed = std::ranges::all_of(g_index, [](const IndexEntry& entry)
            {
                return g_accessedHashes.contains(entry.hash);
            });

            if (allEntriesAccessed)
                return true;  // No changes needed - skip save
        }

        std::string path = GetCacheFilePath();
        if (path.empty())
            return false;

        // Build temporary file path for atomic replace
        std::string tempPath = path + ".tmp";

        // Create/open temp file
        HANDLE hTempFile = CreateFileA(
            tempPath.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hTempFile == INVALID_HANDLE_VALUE)
            return false;

        // Prepare header (entryCount will be set after filtering)
        CacheHeader header = {};
        header.magic = CACHE_MAGIC;
        header.version = CACHE_VERSION;
        header.flags = 0;
        memset(header.reserved, 0, sizeof(header.reserved));

        // Track current write offset
        UInt32 currentOffset = sizeof(CacheHeader);

        // Write header placeholder (will update entryCount and indexOffset later)
        DWORD written;
        WriteFile(hTempFile, &header, sizeof(header), &written, nullptr);

        // New combined index - reserve estimate, actual count determined after filtering
        std::vector<IndexEntry> newIndex;
        newIndex.reserve(g_accessedHashes.size() + g_pendingEntries.size());

        // Copy existing blobs that were actually accessed this session
        // This prunes stale entries (removed/unused scripts) from the cache
        for (const auto& entry : g_index)
        {
            // Only keep entries that were accessed during this session
            if (!g_accessedHashes.contains(entry.hash) && !g_pendingHashes.contains(entry.hash))
                continue;

            if (g_basePtr && entry.blobOffset + entry.blobSize <= g_fileSize)
            {
                const UInt8* blobData = static_cast<const UInt8*>(g_basePtr) + entry.blobOffset;
                WriteFile(hTempFile, blobData, entry.blobSize, &written, nullptr);

                IndexEntry newEntry = entry;
                newEntry.blobOffset = currentOffset;
                newIndex.push_back(newEntry);
                currentOffset += entry.blobSize;
            }
        }

        // Write pending entries (new compilations)
        for (const auto& pending : g_pendingEntries)
        {
            WriteFile(hTempFile, pending.data.data(), pending.data.size(), &written, nullptr);

            IndexEntry newEntry;
            newEntry.hash = pending.hash;
            newEntry.scriptLength = pending.scriptLength;
            newEntry.blobOffset = currentOffset;
            newEntry.blobSize = static_cast<UInt32>(pending.data.size());
            newIndex.push_back(newEntry);
            currentOffset += newEntry.blobSize;
        }

        // Sort index by (hash, scriptLength) for binary search
        std::sort(newIndex.begin(), newIndex.end(),
            [](const IndexEntry& a, const IndexEntry& b) {
                if (a.hash != b.hash)
                    return a.hash < b.hash;
                return a.scriptLength < b.scriptLength;
            }
        );

        // Write sorted index
        header.indexOffset = currentOffset;
        header.entryCount = static_cast<UInt32>(newIndex.size());
        for (const auto& entry : newIndex)
        {
            WriteFile(hTempFile, &entry, sizeof(entry), &written, nullptr);
        }

        // Update header with correct entryCount and indexOffset
        SetFilePointer(hTempFile, 0, nullptr, FILE_BEGIN);
        WriteFile(hTempFile, &header, sizeof(header), &written, nullptr);

        CloseHandle(hTempFile);

        // Close existing mapping before replacing file
        CleanupMapping();

        // Atomic replace: rename temp file to final path
        if (!MoveFileExA(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
        {
            DeleteFileA(tempPath.c_str());
            return false;
        }
        // Clear pending entries and accessed tracking after successful save
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
        // Compute hash and length
        const size_t length = strlen(scriptText);
        UInt64 hash = XXHash64::hash(scriptText, length, 0);

        // Binary search in sorted index
        const auto key = std::make_pair(hash, static_cast<UInt32>(length));
        const auto it = std::lower_bound(g_index.begin(), g_index.end(), key, CompareIndexEntry);
        
        // Check if found (matching hash AND length)
        if (it == g_index.end() || it->hash != hash || it->scriptLength != static_cast<UInt32>(length))
            return false; // Cache miss

        // Cache hit - validate blob bounds
        if (!g_basePtr || it->blobOffset + it->blobSize > g_fileSize)
            return false;

        const auto* blobData = static_cast<const UInt8*>(g_basePtr) + it->blobOffset;
        if (!ReadScriptBlob(blobData, it->blobSize, script))
            return false;

        // Track this hash as accessed for stale entry pruning on save
        g_accessedHashes.insert(hash);
        return true;
    }

    void AddCompiledScriptToCache(const char* scriptText, Script* script)
    {
        if (!scriptText || !script || !script->data 
            || script->info.dataLength == 0 || !g_enabled || !g_initialized
            || GetLambdaParentScript(script))
            return;
        
        // Compute hash and length
        size_t length = strlen(scriptText);
        UInt64 hash = XXHash64::hash(scriptText, length, 0);

        // Check if already in pending entries using O(1) hash set lookup
        if (g_pendingHashes.contains(hash))
            return; // Already pending
        g_pendingHashes.insert(hash); // Track hash for O(1) duplicate check
        // Check if already in loaded index using O(log N) binary search
        auto key = std::make_pair(hash, static_cast<UInt32>(length));
        auto it = std::lower_bound(g_index.begin(), g_index.end(), key, CompareIndexEntry);
        if (it != g_index.end() && it->hash == hash && it->scriptLength == static_cast<UInt32>(length))
            return; // Already cached

        // Calculate blob size and allocate
        size_t blobSize = CalculateScriptBlobSize(script);

        PendingEntry entry;
        entry.hash = hash;
        entry.scriptLength = static_cast<UInt32>(length);
        entry.data.resize(blobSize);

        // Write script data to blob
        WriteScriptBlob(script, entry.data.data());

        g_pendingEntries.emplace_back(std::move(entry));
    }
}
