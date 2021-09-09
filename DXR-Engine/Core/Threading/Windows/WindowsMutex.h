#pragma once
#include "Windows/Windows.h"

class CWindowsMutex
{
    friend class CWindowsConditionVariable;

public:

    FORCEINLINE CWindowsMutex() noexcept
        : Section()
    {
        InitializeCriticalSection( &Section );
    }

    FORCEINLINE ~CWindowsMutex()
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

private:
    CRITICAL_SECTION Section;
};