#pragma once
#include "Core/PlatformInterface/IPlatformMemory.h"
#include <malloc.h>

struct FWindowsPlatformMemory final : public IPlatformMemory
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
};
