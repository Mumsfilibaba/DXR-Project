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

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE CWindowsConditionVariable()
        : ConditionVariable()
    {
        InitializeConditionVariable(&ConditionVariable);
    }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~CWindowsConditionVariable()
    {
        NotifyAll();
    }

    /** Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept
    {
        WakeConditionVariable(&ConditionVariable);
    }

    /** Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept
    {
        WakeAllConditionVariable(&ConditionVariable);
    }

    /**
     * @brief: Make a CriticalSections wait until notified 
     * 
     * @param Lock: Lock that should wait for condition to be met
     * @return: Returns true if the wait is successful
     */
    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        SetLastError(0);

        CWindowsCriticalSection::PlatformHandle CriticalSection = Lock.GetLock().GetPlatformHandle();

        const bool bResult = !!SleepConditionVariableCS(&ConditionVariable, CriticalSection, INFINITE);
        if (!bResult)
        {
            String ErrorString;
            PlatformDebugMisc::GetLastErrorString(ErrorString);

            LOG_ERROR(ErrorString);

            return false;
        }
        else
        {
            return true;
        }
    }

    /**
     * @brief: Retrieve platform specific handle 
     * 
     * @return: Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &ConditionVariable;
    }

private:
    CONDITION_VARIABLE ConditionVariable;
};

#endif