#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericConditionVariable

struct FGenericConditionVariable 
    : FNonCopyable
{
    typedef void* PlatformHandle;

    FGenericConditionVariable()  = default;
    ~FGenericConditionVariable() = default;

    /** @brief: Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept { }

    /** @brief: Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept { }

    /**
     * @brief: Make a CriticalSections wait until notified 
     * 
     * @param Lock: Lock that should wait for condition to be met
     * @return: Returns true if the wait is successful
     */
    FORCEINLINE bool Wait(TScopedLock<FCriticalSection>& Lock) noexcept { return false; }

    /**
     * @brief: Retrieve platform specific handle 
     * 
     * @return: Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle() { return nullptr; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif