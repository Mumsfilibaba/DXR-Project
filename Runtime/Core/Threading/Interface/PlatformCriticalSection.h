#pragma once
#include "Core/Core.h"

/* Generic CriticalSection */
class CPlatformCriticalSection
{
public:

    typedef void* PlatformHandle;

    CPlatformCriticalSection() = default;
    ~CPlatformCriticalSection() = default;

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
