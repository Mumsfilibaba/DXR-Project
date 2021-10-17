#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Windows/Windows.h"
#include "Core/Threading/Core/CoreCriticalSection.h"

class CWindowsCriticalSection final : public CCoreCriticalSection
{
public:

    typedef CRITICAL_SECTION* PlatformHandle;

    CWindowsCriticalSection( const CWindowsCriticalSection& ) = delete;
    CWindowsCriticalSection& operator=( const CWindowsCriticalSection& ) = delete;

    FORCEINLINE CWindowsCriticalSection() noexcept
        : Section()
    {
        InitializeCriticalSection( &Section );
    }

    FORCEINLINE ~CWindowsCriticalSection()
    {
        DeleteCriticalSection( &Section );
    }

    FORCEINLINE void Lock() noexcept
    {
        EnterCriticalSection( &Section );
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return !!TryEnterCriticalSection( &Section );
    }

    FORCEINLINE void Unlock() noexcept
    {
        LeaveCriticalSection( &Section );
    }

    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &Section;
    }

private:
    CRITICAL_SECTION Section;
};

#endif