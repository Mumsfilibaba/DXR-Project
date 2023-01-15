#pragma once
#include "Core/Memory/Malloc.h"

#include <new>

#define IMPLEMENT_NEW_AND_DELETE_OPERATORS()                          \
    void* operator new(size_t Size)                                   \
    {                                                                 \
        return FMemory::Malloc(Size);                                 \
    }                                                                 \
    void* operator new[](size_t Size)                                 \
    {                                                                 \
        return FMemory::Malloc(Size);                                 \
    }                                                                 \
    void* operator new(size_t Size, const std::nothrow_t&) noexcept   \
    {                                                                 \
        return FMemory::Malloc(Size);                                 \
    }                                                                 \
    void* operator new[](size_t Size, const std::nothrow_t&) noexcept \
    {                                                                 \
        return FMemory::Malloc(Size);                                 \
    }                                                                 \
    void operator delete(void* Ptr) noexcept                          \
    {                                                                 \
        FMemory::Free(Ptr);                                           \
    }                                                                 \
    void operator delete[](void* Ptr) noexcept                        \
    {                                                                 \
        FMemory::Free(Ptr);                                           \
    }                                                                 \
    void operator delete(void* Ptr, size_t) noexcept                  \
    {                                                                 \
        FMemory::Free(Ptr);                                           \
    }                                                                 \
    void operator delete[](void* Ptr, size_t) noexcept                \
    {                                                                 \
        FMemory::Free(Ptr);                                           \
    }
