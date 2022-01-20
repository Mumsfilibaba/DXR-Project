#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

typedef void(*ThreadFunction)();
typedef uint64 PlatformThreadHandle;

// Might need to be different on other platforms. However this is valid on windows. 
// See: https://docs.microsoft.com/en-us/windows/win32/procthread/thread-handles-and-identifiers
#define INVALID_THREAD_ID 0

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for threads

class CPlatformThread : public CRefCounted
{
public:

    // TODO: Enable member-functions and lambdas (Use TFunction to solve this)

    /* Create a new thread */
    static TSharedRef<CPlatformThread> Make(ThreadFunction InFunction) { return dbg_new CPlatformThread(); }
    /* Create a new thread with a name */
    static TSharedRef<CPlatformThread> Make(ThreadFunction InFunction, const CString& InName) { return dbg_new CPlatformThread(); }

    /* Start thread-execution */
    virtual bool Start() { return true; }

    /* Wait until function has finished running */
    virtual void WaitUntilFinished() { }

    /* Set name of thread. Needs to be called before start on some platforms for the changes to take effect */
    virtual void SetName(const CString& InName) { }

    /* Retrieve the platform specific handle for the thread */
    virtual PlatformThreadHandle GetPlatformHandle() { return static_cast<PlatformThreadHandle>(0); }

protected:

    CPlatformThread() = default;
    ~CPlatformThread() = default;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
