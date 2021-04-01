#pragma once
#include "Windows/Windows.h"

class WindowsMutex
{
    friend class WindowsConditionVariable;

public:
    WindowsMutex() noexcept
        : Section()
    {
        InitializeCriticalSection(&Section);
    }

    ~WindowsMutex()
    {
        DeleteCriticalSection(&Section);
    }

    void Lock() noexcept
    {
        EnterCriticalSection(&Section);
    }

    bool TryLock() noexcept
    {
        return TryEnterCriticalSection(&Section);
    }

    void Unlock() noexcept
    {
        LeaveCriticalSection(&Section);
    }

private:
    CRITICAL_SECTION Section;
};

typedef WindowsMutex Mutex;