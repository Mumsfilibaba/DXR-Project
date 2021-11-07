#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Platform/CriticalSection.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Generic ConditionVariable*/
class CPlatformConditionVariable
{
public:

    typedef void* PlatformHandle;

    CPlatformConditionVariable() = default;
    ~CPlatformConditionVariable() = default;

    FORCEINLINE void NotifyOne() noexcept
    {
    }

    FORCEINLINE void NotifyAll() noexcept
    {
    }

    FORCEINLINE bool Wait( TScopedLock<CCriticalSection>& Lock ) noexcept
    {
        return false;
    }

    FORCEINLINE PlatformHandle GetPlatformHandle()
    {
        return nullptr;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
