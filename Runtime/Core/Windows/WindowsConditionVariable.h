#pragma once
#include "WindowsCriticalSection.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Generic/GenericConditionVariable.h"
#include "Core/Platform/PlatformMisc.h"

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

        const BOOL bResult = SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            FString ErrorString;
            FPlatformMisc::GetLastErrorString(ErrorString);

            LOG_ERROR("%s", *ErrorString);

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