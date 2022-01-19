#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for Critical Sections

class CPlatformCriticalSection
{
public:

    typedef void* PlatformHandle;

    CPlatformCriticalSection() = default;
    ~CPlatformCriticalSection() = default;

    /* Lock CriticalSection for other threads */
    FORCEINLINE void Lock() noexcept { }

    /* Try to lock CriticalSection for other threads */
    FORCEINLINE bool TryLock() noexcept { return false; }

    /* Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept { }

    /* Retrieve platform specific handle */
    FORCEINLINE PlatformHandle GetPlatformHandle() { return nullptr; }
};
