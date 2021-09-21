#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

/* Generic CriticalSection */
class CGenericCriticalSection
{
public:
    CGenericCriticalSection() = default;
    ~CGenericCriticalSection() = default;

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
};
