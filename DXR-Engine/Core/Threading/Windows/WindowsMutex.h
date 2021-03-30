#pragma once
#include "Windows/Windows.h"

class WindowsMutex
{
    friend class WindowsConditionVariable;

public:
    WindowsMutex()
        : Section()
    {
        InitializeCriticalSection(&Section);
    }

    ~WindowsMutex()
    {
        DeleteCriticalSection(&Section);
    }

    void Lock()
    {
        EnterCriticalSection(&Section);
    }

    bool TryLock()
    {
        return TryEnterCriticalSection(&Section);
    }

    void Unlock()
    {
        LeaveCriticalSection(&Section);
    }

private:
    CRITICAL_SECTION Section;
};

typedef WindowsMutex Mutex;