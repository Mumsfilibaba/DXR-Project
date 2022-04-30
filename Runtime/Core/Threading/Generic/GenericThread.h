#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Function.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

enum : uint64
{
    kWaitForThreadInfinity = uint64(~0)
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThread

class CORE_API CGenericThread : public CRefCounted
{
    friend class CGenericThreadMisc;

protected:

    CGenericThread(const TFunction<void()>& InFunction)
        : Function(InFunction)
    { }

    ~CGenericThread() = default;

public:

    virtual bool Start() { return true; }

    virtual void WaitUntilFinished(uint64 TimeoutInMilliseconds) { }
	
    virtual void SetName(const String& InName) { }

    virtual void* GetPlatformHandle() { return nullptr; }

    virtual String GetName() const { return ""; }

    TFunction<void()> GetFunction() const { return Function; }

protected:
    TFunction<void()> Function;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
