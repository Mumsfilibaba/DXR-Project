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

    FORCEINLINE CWindowsCriticalSection() noexcept
        : Section()
    {
        InitializeCriticalSection(&Section);
    }

    FORCEINLINE ~CWindowsCriticalSection()
    {
        DeleteCriticalSection(&Section);
    }

    /* Lock CriticalSection for other threads */
    FORCEINLINE void Lock() noexcept
    {
        EnterCriticalSection(&Section);
    }

    /* Try to lock CriticalSection for other threads */
    FORCEINLINE bool TryLock() noexcept
    {
        return !!TryEnterCriticalSection(&Section);
    }

    /* Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept
    {
        LeaveCriticalSection(&Section);
    }

    /* Retrieve platform specific handle */
    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &Section;
    }

private:
    CRITICAL_SECTION Section;
};

#endif