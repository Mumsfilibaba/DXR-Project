#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Memory/Malloc.h"
#include <new>

namespace NewOperatorsInternal
{
    FORCEINLINE void* Allocate(size_t Size)
    {
        const uint64 AllocationSize = Size ? static_cast<uint64>(Size) : 1;
        if (void* Result = Memory::Malloc(AllocationSize))
        {
            return Result;
        }

        Memory::OnOutOfMemory(AllocationSize);
    }

    FORCEINLINE void* TryAllocate(size_t Size) noexcept
    {
        return Memory::Malloc(Size ? static_cast<uint64>(Size) : 1);
    }
}

#define IMPLEMENT_NEW_AND_DELETE_OPERATORS() \
    void* operator new(size_t Size) \
    { \
        return NewOperatorsInternal::Allocate(Size); \
    } \
    void* operator new[](size_t Size) \
    { \
        return NewOperatorsInternal::Allocate(Size); \
    } \
    void* operator new(size_t Size, const std::nothrow_t&) noexcept \
    { \
        return NewOperatorsInternal::TryAllocate(Size); \
    } \
    void* operator new[](size_t Size, const std::nothrow_t&) noexcept \
    { \
        return NewOperatorsInternal::TryAllocate(Size); \
    } \
    void operator delete(void* Ptr) noexcept \
    { \
        Memory::Free(Ptr); \
    } \
    void operator delete[](void* Ptr) noexcept \
    { \
        Memory::Free(Ptr); \
    } \
    void operator delete(void* Ptr, size_t) noexcept \
    { \
        Memory::Free(Ptr); \
    } \
    void operator delete[](void* Ptr, size_t) noexcept \
    { \
        Memory::Free(Ptr); \
    } \
    void operator delete(void* Ptr, const std::nothrow_t&) noexcept \
    { \
        Memory::Free(Ptr); \
    } \
    void operator delete[](void* Ptr, const std::nothrow_t&) noexcept \
    { \
        Memory::Free(Ptr); \
    }
