#pragma once
#include "Core/Core.h"

struct FMalloc;

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FUseStandardMalloc
{
    // Override the global new
    void* operator new(size_t InSize)
    {
        return ::malloc(InSize);
    }

    // Override the global new
    void* operator new[](size_t InSize)
    {
        return ::malloc(InSize);
    }

    // Override the global delete
    void operator delete(void* Object)
    {
        ::free(Object);
    }

    // Override the global delete
    void operator delete[](void* Object)
    {
        ::free(Object);
    }
};

struct CORE_API FMalloc : public FUseStandardMalloc
{
    // FMalloc instead of IMalloc since IMalloc is already defined in the WindowsHeaders
    virtual ~FMalloc() = default;

    /**
     * @brief Allocates memory
     *
     * @param InSize Number of bytes to allocate
     * @param InAlignment Alignment of the block, a power of two of at least STANDARD_ALIGNMENT
     * @return Returns a pointer with allocated memory, or nullptr when InSize is zero or the allocation failed
     */
    virtual void* Malloc(uint64 InSize, uint32 InAlignment) = 0;

    /**
     * @brief Allocates memory, from an existing block and copies over the old content to the new
     *
     * @param Block The old block to reallocate, or nullptr to allocate a new block
     * @param InSize Number of bytes to allocate
     * @param InAlignment Alignment of the block, which must match the alignment Block was allocated with
     * @return Returns a pointer with allocated memory, or nullptr when the allocation failed and Block is left untouched
     */
    virtual void* Realloc(void* Block, uint64 InSize, uint32 InAlignment) = 0;

    /**
     * @brief Frees a block of memory  
     * @param Block Block to free
     */
    virtual void Free(void* Block) = 0;
};

struct CORE_API FMallocANSI : public FMalloc
{
    virtual void* Malloc(uint64 InSize, uint32 InAlignment) override final;
    virtual void* Realloc(void* InBlock, uint64 InSize, uint32 InAlignment) override final;
    virtual void Free(void* InBlock) override final;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
