#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Threading/Generic/GenericCriticalSection.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsCriticalSection

class CWindowsCriticalSection final : public CGenericCriticalSection
{
public:

    typedef CRITICAL_SECTION* PlatformHandle;

    FORCEINLINE CWindowsCriticalSection() noexcept
        : Section()
    {
        InitializeCriticalSection(&Section);
    }

    FORCEINLINE ~CWindowsCriticalSection()
    {
        DeleteCriticalSection(&Section);
    }

    FORCEINLINE void Lock() noexcept
    {
        EnterCriticalSection(&Section);
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return !!TryEnterCriticalSection(&Section);
    }

    FORCEINLINE void Unlock() noexcept
    {
        LeaveCriticalSection(&Section);
    }

    FORCEINLINE PlatformHandle GetPlatformHandle() noexcept
    {
        return &Section;
    }

private:
    CRITICAL_SECTION Section;
};