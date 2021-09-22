#pragma once
#include "Core/Windows/Windows.h"

class CWindowsCriticalSection
{
public:
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

    FORCEINLINE CRITICAL_SECTION& GetSection() noexcept
    {
        return Section;
    }

    FORCEINLINE const CRITICAL_SECTION& GetSection() const noexcept
    {
        return Section;
    }

private:
    CRITICAL_SECTION Section;
};