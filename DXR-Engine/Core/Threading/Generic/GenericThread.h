#pragma once
#include "Core.h"

#include "Core/RefCounted.h"
#include "Core/Time/Timestamp.h"

typedef void(*ThreadFunction)();
typedef uint64 PlatformThreadHandle;

// Might need to be different on other platforms. However this is valid on windows. 
// See: https://docs.microsoft.com/en-us/windows/win32/procthread/thread-handles-and-identifiers
#define INVALID_THREAD_ID 0

class CGenericThread : public CRefCounted
{
public:

    // TODO: Enable member-functions and lambdas
    static CGenericThread* Make( ThreadFunction )
    {
        return nullptr;
    }

    virtual bool Start() = 0;

    virtual void WaitUntilFinished() = 0;

    virtual void SetName( const std::string& Name ) = 0;

    virtual PlatformThreadHandle GetPlatformHandle() = 0;

protected:

    CGenericThread() = default;
    ~CGenericThread() = default;
};
