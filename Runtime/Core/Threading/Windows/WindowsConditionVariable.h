#pragma once
#include "WindowsCriticalSection.h"

#include "Core/Logging/Log.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Generic/GenericConditionVariable.h"
#include "Core/Platform/PlatformMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsConditionVariable

class FWindowsConditionVariable final : public FGenericConditionVariable
{
public:

    typedef CONDITION_VARIABLE* PlatformHandle;

    FORCEINLINE FWindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable(&ConditionVariable);
    }

    FORCEINLINE ~FWindowsConditionVariable()
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

    FORCEINLINE bool Wait(TScopedLock<FCriticalSection>& Lock) noexcept
    {
        SetLastError(0);

        FWindowsCriticalSection::PlatformHandle CriticalSection = Lock.GetLock().GetPlatformHandle();

        const bool bResult = !!SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            FString ErrorString;
            FPlatformMisc::GetLastErrorString(ErrorString);

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