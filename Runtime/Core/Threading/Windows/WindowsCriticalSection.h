#pragma once

#if PLATFORM_WINDOWS
#include "Core/Windows/Windows.h"
#include "Core/Threading/Interface/PlatformCriticalSection.h"

class CWindowsCriticalSection final : public CPlatformCriticalSection
{
public:

    typedef CRITICAL_SECTION* PlatformHandle;

    CWindowsCriticalSection(const CWindowsCriticalSection&) = delete;
    CWindowsCriticalSection& operator=(const CWindowsCriticalSection&) = delete;

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE CWindowsCriticalSection() noexcept
        : Section()
    {
        InitializeCriticalSection(&Section);
    }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~CWindowsCriticalSection()
    {
        DeleteCriticalSection(&Section);
    }

    /** Lock CriticalSection for other threads */
    FORCEINLINE void Lock() noexcept
    {
        EnterCriticalSection(&Section);
    }

    /**
     * @brief: Try to lock CriticalSection for other threads
     * 
     * @return:; Returns true if the lock is successful
     */
    FORCEINLINE bool TryLock() noexcept
    {
        return !!TryEnterCriticalSection(&Section);
    }

    /** Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept
    {
        LeaveCriticalSection(&Section);
    }

    /**
     * @brief: Retrieve platform specific handle
     *
     * @return: Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &Section;
    }

private:
    CRITICAL_SECTION Section;
};

#endif