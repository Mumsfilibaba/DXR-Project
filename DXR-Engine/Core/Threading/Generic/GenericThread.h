#pragma once
#include "Core.h"

#include "Core/RefCounted.h"
#include "Core/Time/Timestamp.h"

typedef void(*ThreadFunction)();
typedef uint64 ThreadID;

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

    virtual void Wait() = 0;

    virtual void SetName( const std::string& Name ) = 0;

    virtual ThreadID GetID() = 0;

protected:

    CGenericThread() = default;
    virtual ~CGenericThread() = default;
};
