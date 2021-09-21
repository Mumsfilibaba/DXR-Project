#pragma once
#include "Core.h"

#include "Core/RefCounted.h"
#include "Core/Time/Timestamp.h"

typedef void(*ThreadFunction)();
typedef uint64 ThreadID;

// Might need to be different on other platforms. However this is valid on windows. 
// See: https://docs.microsoft.com/en-us/windows/win32/procthread/thread-handles-and-identifiers
#define INVALID_THREAD_ID 0

class GenericThread : public CRefCounted
{
public:

    GenericThread() = default;
    virtual ~GenericThread() = default;

    virtual void Wait() = 0;

    virtual void SetName( const std::string& Name ) = 0;

    virtual ThreadID GetID() = 0;

    // TODO: Enable memberfunctions and lambdas
    static GenericThread* Create( ThreadFunction )
    {
        return nullptr;
    }
};
