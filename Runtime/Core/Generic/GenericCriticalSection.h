#pragma once
#include "Core/Core.h"

#include "Core/Templates/ClassUtilities.h"


struct FGenericCriticalSection
    : FNonCopyable
{
    typedef void* PlatformHandle;

    FGenericCriticalSection()  = default;
    ~FGenericCriticalSection() = default;

    /** @brief - Lock CriticalSection for other threads */
    FORCEINLINE void Lock() noexcept { }

    /**
     * @brief  - Try to lock CriticalSection for other threads
     * @return - Returns true if the lock is successful
     */
    FORCEINLINE bool TryLock() noexcept { return false; }

    /** @brief - Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept { }

    /**
     * @brief  - Retrieve platform specific handle
     * @return - Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle() { return nullptr; }
};
