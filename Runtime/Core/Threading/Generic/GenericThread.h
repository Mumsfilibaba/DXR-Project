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

class CORE_API CGenericThread : public FRefCounted
{
    friend class CGenericThreadMisc;

protected:

    CGenericThread(const TFunction<void()>& InFunction)
        : Function(InFunction)
    { }

    ~CGenericThread() = default;

public:

    /**
     * @brief: Waits for the thread to complete and joins with caller 
     * 
     * @return: Returns the return-value from the thread
     */
    virtual int32 WaitForCompletion(uint64 TimeoutInMs) { return 0; }
    
    virtual bool Start() { return true; }
	
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
