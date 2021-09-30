#pragma once
#include "Core/Threading/Core/CoreConditionVariable.h"
#include "Core/Application/Windows/WindowsDebugMisc.h"
#include "Core/Threading/ScopedLock.h"

#include "WindowsCriticalSection.h"

class CWindowsConditionVariable final : public CCoreConditionVariable
{
public:

    typedef CONDITION_VARIABLE* PlatformHandle;

    CWindowsConditionVariable( const CWindowsConditionVariable& ) = delete;
    CWindowsConditionVariable& operator=( const CWindowsConditionVariable& ) = delete;

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

        CWindowsCriticalSection::PlatformHandle CriticalSection = Lock.GetLock().GetPlatformHandle();

        bool Result = !!SleepConditionVariableCS( &ConditionVariable, CriticalSection, INFINITE );
        if ( !Result )
        {
            CString ErrorString;
            CWindowsDebugMisc::GetLastErrorString( ErrorString );

            LOG_ERROR( ErrorString.CStr() );

            return false;
        }
        else
        {
            return true;
        }
    }

    FORCEINLINE PlatformHandle GetSection() noexcept
    {
        return &ConditionVariable;
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};
