#pragma once
#include "WindowsCriticalSection.h"

#include "Core/Threading/ScopedLock.h"

class CWindowsConditionVariable
{
public:
    FORCEINLINE CWindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable( &ConditionVariable );
    }

    FORCEINLINE ~CWindowsConditionVariable()
    {
        NotifyAll();
    }

    FORCEINLINE void NotifyOne() noexcept
    {
        WakeConditionVariable( &ConditionVariable );
    }

    FORCEINLINE void NotifyAll() noexcept
    {
        WakeAllConditionVariable( &ConditionVariable );
    }

    FORCEINLINE bool Wait( TScopedLock<CCriticalSection>& Lock ) noexcept
    {
        SetLastError( 0 );

        bool Result = !!SleepConditionVariableCS( &ConditionVariable, &Lock.GetLock().GetSection(), INFINITE );
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
