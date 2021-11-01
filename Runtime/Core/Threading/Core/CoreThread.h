#pragma once
#include "Core/Containers/SharedRef.h"

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

class CCoreThread : public CRefCounted
{
public:

    // TODO: Enable member-functions and lambdas
    static TSharedRef<CCoreThread> Make( ThreadFunction InFunction )
    {
        return dbg_new CCoreThread();
    }

    static TSharedRef<CCoreThread> Make( ThreadFunction InFunction, const CString& InName )
    {
        return dbg_new CCoreThread();
    }

    virtual bool Start() {
        return true;
    }

    virtual void WaitUntilFinished() {}

    virtual void SetName( const CString& Name ) {}

    virtual PlatformThreadHandle GetPlatformHandle() {
        return static_cast<PlatformThreadHandle>(0);
    }

protected:

    CCoreThread() = default;
    ~CCoreThread() = default;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
