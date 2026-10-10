#pragma once
#include "Core/Core.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformMemory
{
    /**
     * @brief Allocates a block from the system heap, bypassing GMalloc and the engine's tracking
     *
     * @param Size Number of bytes to allocate, which must be non-zero
     * @param Alignment Alignment of the block, a power of two of at least STANDARD_ALIGNMENT
     * @return Returns the new block, or nullptr when the allocation failed
     */
    static FORCEINLINE void* Malloc(uint64 Size, uint32 Alignment)
    {
        return nullptr;
    }

    /**
     * @brief Resizes a block from the system heap, keeping the contents up to the smaller of the old and new size
     *
     * @param Block Block returned by Malloc or Realloc, which must not be nullptr
     * @param Size Number of bytes to reallocate
     * @param Alignment Alignment of the block, which must match the alignment Block was allocated with
     * @return Returns the resized block, or nullptr when the allocation failed and Block is left untouched
     */
    static FORCEINLINE void* Realloc(void* Block, uint64 Size, uint32 Alignment)
    {
        return nullptr;
    }

    /**
     * @brief Returns a block to the system heap, whatever alignment it was allocated with
     *
     * @param Block Block returned by Malloc or Realloc, which must not be nullptr
     */
    static FORCEINLINE void Free(void* Block)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
