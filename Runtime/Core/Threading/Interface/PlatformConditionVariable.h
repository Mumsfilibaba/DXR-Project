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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for condition variables

class CPlatformConditionVariable
{
public:

    typedef void* PlatformHandle;

    CPlatformConditionVariable() = default;
    ~CPlatformConditionVariable() = default;

    /* Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept { }

    /* Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept { }

    /* Make a CriticalSections wait until notified */
    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept { return false; }

    /* Retrieve platform specific handle */
    FORCEINLINE PlatformHandle GetPlatformHandle() { return nullptr; }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
