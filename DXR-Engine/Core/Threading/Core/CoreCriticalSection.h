#pragma once
#include "Core.h"

/* Generic CriticalSection */
class CCoreCriticalSection
{
public:

    typedef void* PlatformHandle;

    CCoreCriticalSection() = default;
    ~CCoreCriticalSection() = default;

    FORCEINLINE void Lock() noexcept
    {
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return false;
    }

    FORCEINLINE void Unlock() noexcept
    {
    }

    FORCEINLINE PlatformHandle GetPlatformHandle()
    {
        return nullptr;
    }
};
