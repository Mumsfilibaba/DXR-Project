#pragma once
#include "WindowsMutex.h"

#include "Core/Threading/ScopedLock.h"

class WindowsConditionVariable
{
public:
    WindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable( &ConditionVariable );
    }

    void NotifyOne() noexcept
    {
        WakeConditionVariable( &ConditionVariable );
    }

    void NotifyAll() noexcept
    {
        WakeAllConditionVariable( &ConditionVariable );
    }

    bool Wait( TScopedLock<Mutex>& Lock ) noexcept
    {
        SetLastError( 0 );

        BOOL Result = SleepConditionVariableCS( &ConditionVariable, &Lock.GetLock().Section, INFINITE );
        if ( !Result )
        {
            // TODO: Check Error
            return false;
        }
        else
        {
            return true;
        }
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};

typedef WindowsConditionVariable ConditionVariable;