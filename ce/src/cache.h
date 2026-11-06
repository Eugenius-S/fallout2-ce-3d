#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

#include <mutex>

#include "heap.h"

namespace fallout {

#define INVALID_CACHE_ENTRY ((CacheEntry*)-1)

enum CacheEntryFlags : unsigned int {
    CACHE_ENTRY_NONE = 0x00,

    // Specifies that cache entry has no references as should be evicted during
    // the next sweep operation.
    CACHE_ENTRY_MARKED_FOR_EVICTION = 0x01,
};

constexpr inline CacheEntryFlags operator~(CacheEntryFlags rhs)
{
    return static_cast<CacheEntryFlags>(~static_cast<unsigned int>(rhs));
}

inline CacheEntryFlags& operator&=(CacheEntryFlags& lhs, CacheEntryFlags rhs)
{
    lhs = static_cast<CacheEntryFlags>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
    return lhs;
}

inline CacheEntryFlags& operator|=(CacheEntryFlags& lhs, CacheEntryFlags rhs)
{
    lhs = static_cast<CacheEntryFlags>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
    return lhs;
}

typedef int CacheSizeProc(int key, int* sizePtr);
typedef int CacheReadProc(int key, int* sizePtr, unsigned char* buffer);
typedef void CacheFreeProc(void* ptr);

typedef struct CacheEntry {
    int key;
    int size;
    unsigned char* data;
    unsigned int referenceCount;

    // Total number of hits that this cache entry received during it's
    // lifetime.
    unsigned int hits;

    CacheEntryFlags flags;

    // The most recent hit in terms of cache hit counter. Used to track most
    // recently used entries in eviction strategy.
    unsigned int mru;

    int heapHandleIndex;
} CacheEntry;

typedef struct Cache {
    // Current size of entries in cache.
    int size;

    // Maximum size of entries in cache.
    int maxSize;

    // The length of `entries` array.
    int entriesLength;

    // The capacity of `entries` array.
    int entriesCapacity;

    // Total number of hits during cache lifetime.
    unsigned int hits;

    // List of cache entries.
    CacheEntry** entries;

    CacheSizeProc* sizeProc;
    CacheReadProc* readProc;
    CacheFreeProc* freeProc;
    Heap heap;

    // Guards the entry table, the heap and the reference counts. The size and
    // read procs run outside it, so a warm thread pool spreads the file reads
    // (172); a read block lands in the heap and enters the table under it,
    // with a double check against another thread locking the same key.
    std::mutex mutex;
} Cache;

bool cacheInit(Cache* cache, CacheSizeProc* sizeProc, CacheReadProc* readProc, CacheFreeProc* freeProc, int maxSize);
bool cacheFree(Cache* cache);
bool cacheLock(Cache* cache, int key, void** data, CacheEntry** cacheEntryPtr);
bool cacheUnlock(Cache* cache, CacheEntry* cacheEntry);
bool cacheFlush(Cache* cache);
bool cachePrintStats(Cache* cache, char* dest, size_t size);

} // namespace fallout

#endif /* CACHE_H */
