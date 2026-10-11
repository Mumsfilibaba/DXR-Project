#pragma once
#include "Core/PlatformInterface/IPlatformMemory.h"
#include <cstdlib>
#include <cstring>
#include <malloc/malloc.h>
#include <sys/mman.h>
#include <unistd.h>

struct CORE_API FMacPlatformMemory final : public IPlatformMemory
{
    static FORCEINLINE void* Malloc(uint64 Size, uint32 Alignment)
    {
        if (Alignment <= STANDARD_ALIGNMENT)
        {
            return ::malloc(Size);
        }

        void* Result = nullptr;
        return ::posix_memalign(&Result, Alignment, Size) == 0 ? Result : nullptr;
    }

    static FORCEINLINE void* Realloc(void* Block, uint64 Size, uint32 Alignment)
    {
        if (Alignment <= STANDARD_ALIGNMENT)
        {
            return ::realloc(Block, Size);
        }

        if (!Size)
        {
            ::free(Block);
            return nullptr;
        }

        void* Result = Malloc(Size, Alignment);
        if (Result)
        {
            const uint64 OldSize = static_cast<uint64>(::malloc_size(Block));
            ::memcpy(Result, Block, OldSize < Size ? OldSize : Size);
            ::free(Block);
        }

        return Result;
    }

    static FORCEINLINE void Free(void* Block)
    {
        ::free(Block);
    }

    static bool GetStats(FPlatformMemoryStats& OutStats);

    static FORCEINLINE uint64 GetPageSize()
    {
        return static_cast<uint64>(::getpagesize());
    }

    static FORCEINLINE void* AllocatePages(uint64 Size)
    {
        void* Pages = ::mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        return Pages != MAP_FAILED ? Pages : nullptr;
    }

    static FORCEINLINE void FreePages(void* Pages, uint64 Size)
    {
        ::munmap(Pages, Size);
    }
};
