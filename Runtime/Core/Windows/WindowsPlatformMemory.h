#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformMemory.h"
#include <malloc.h>

struct CORE_API FWindowsPlatformMemory final : public IPlatformMemory
{
    static FORCEINLINE void* Malloc(uint64 Size, uint32 Alignment)
    {
        return ::_aligned_malloc(Size, Alignment);
    }

    static FORCEINLINE void* Realloc(void* Block, uint64 Size, uint32 Alignment)
    {
        return ::_aligned_realloc(Block, Size, Alignment);
    }

    static FORCEINLINE void Free(void* Block)
    {
        ::_aligned_free(Block);
    }

    static bool GetStats(FPlatformMemoryStats& OutStats);

    static FORCEINLINE uint64 GetPageSize()
    {
        SYSTEM_INFO SystemInfo;
        ::GetSystemInfo(&SystemInfo);
        return static_cast<uint64>(SystemInfo.dwPageSize);
    }

    static FORCEINLINE void* AllocatePages(uint64 Size)
    {
        return ::VirtualAlloc(nullptr, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    static FORCEINLINE void FreePages(void* Pages, MAYBE_UNUSED uint64 Size)
    {
        ::VirtualFree(Pages, 0, MEM_RELEASE);
    }
};
