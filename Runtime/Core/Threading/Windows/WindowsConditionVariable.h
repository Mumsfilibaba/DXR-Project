#pragma once
#include "WindowsCriticalSection.h"

#include "Core/Logging/Log.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Generic/GenericConditionVariable.h"
#include "Core/Platform/PlatformMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsConditionVariable

class CWindowsConditionVariable final : public CGenericConditionVariable
{
public:

    typedef CONDITION_VARIABLE* PlatformHandle;

    FORCEINLINE CWindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable(&ConditionVariable);
    }

    FORCEINLINE ~CWindowsConditionVariable()
    {
        NotifyAll();
    }

    FORCEINLINE void NotifyOne() noexcept
    {
        WakeConditionVariable(&ConditionVariable);
    }

    FORCEINLINE void NotifyAll() noexcept
    {
        WakeAllConditionVariable(&ConditionVariable);
    }

    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        SetLastError(0);

        CWindowsCriticalSection::PlatformHandle CriticalSection = Lock.GetLock().GetPlatformHandle();

        const bool bResult = !!SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            FString ErrorString;
            PlatformMisc::GetLastErrorString(ErrorString);

            LOG_ERROR("%s", ErrorString.CStr());

            return false;
        }
        else
        {
            return true;
        }
    }

    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &ConditionVariable;
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};