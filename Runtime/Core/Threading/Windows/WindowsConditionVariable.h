#pragma once

#if PLATFORM_WINDOWS
#include "WindowsCriticalSection.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Interface/PlatformConditionVariable.h"
#include "CoreApplication/Windows/WindowsDebugMisc.h"
#include "Core/Logging/Log.h"

class CWindowsConditionVariable final : public CPlatformConditionVariable
{
public:

    typedef CONDITION_VARIABLE* PlatformHandle;

    CWindowsConditionVariable(const CWindowsConditionVariable&) = delete;
    CWindowsConditionVariable& operator=(const CWindowsConditionVariable&) = delete;

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

        bool bResult = !!SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            CString ErrorString;
            CWindowsDebugMisc::GetLastErrorString(ErrorString);

            LOG_ERROR(ErrorString.CStr());

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

#endif