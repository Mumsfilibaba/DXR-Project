#pragma once
#include "PlatformThread.h"

#include "Core/Time/Timestamp.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CPlatformThreadMisc
{
public:

    /* Returns the number of logical processors on the system */
    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    /* Retrieves the current thread's system ID */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle() { return static_cast<PlatformThreadHandle>(INVALID_THREAD_ID); }

    /* Makes the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp Time ) { }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
