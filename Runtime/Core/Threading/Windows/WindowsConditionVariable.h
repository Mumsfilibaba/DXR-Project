#pragma once

#if PLATFORM_WINDOWS
#include "WindowsCriticalSection.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Interface/PlatformConditionVariable.h"
#include "Core/Logging/Log.h"

#include "CoreApplication/Platform/PlatformDebugMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for condition variables

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

    /* Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept
    {
        WakeConditionVariable(&ConditionVariable);
    }

    /* Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept
    {
        WakeAllConditionVariable(&ConditionVariable);
    }

    /* Make a CriticalSections wait until notified */
    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        SetLastError(0);

        CWindowsCriticalSection::PlatformHandle CriticalSection = Lock.GetLock().GetPlatformHandle();

        bool bResult = !!SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            CString ErrorString;
            PlatformDebugMisc::GetLastErrorString(ErrorString);

            LOG_ERROR(ErrorString.CStr());

            return false;
        }
        else
        {
            return true;
        }
    }

    /* Retrieve platform specific handle */
    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &ConditionVariable;
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};

#endif