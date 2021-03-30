#pragma once
#include "WindowsMutex.h"

class WindowsConditionVariable
{
public:
    WindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable(&ConditionVariable);
    }

    void NotifyOne() noexcept
    {
        WakeConditionVariable(&ConditionVariable);
    }

    void NotifyAll() noexcept
    {
        WakeAllConditionVariable(&ConditionVariable);
    }

    void Wait(Mutex& Lock)
    {
        Lock.Lock();

        SleepConditionVariableCS(&ConditionVariable, &Lock.Section, INFINITE);

        Lock.Unlock();
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};

typedef WindowsConditionVariable ConditionVariable;