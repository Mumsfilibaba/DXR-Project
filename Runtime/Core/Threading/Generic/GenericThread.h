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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThread

class CGenericThread : public CRefCounted
{
protected:

    CGenericThread()  = default;
    ~CGenericThread() = default;

public:

    // TODO: Enable member-functions and lambdas (Use TFunction to solve this)

    static TSharedRef<CGenericThread> Make(ThreadFunction InFunction) { return dbg_new CGenericThread(); }
   
    static TSharedRef<CGenericThread> Make(ThreadFunction InFunction, const String& InName) { return dbg_new CGenericThread(); }

    virtual bool Start() { return true; }

    virtual void WaitUntilFinished() { }
	
    virtual void SetName(const String& InName) { }

    virtual void* GetPlatformHandle() { return nullptr; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
