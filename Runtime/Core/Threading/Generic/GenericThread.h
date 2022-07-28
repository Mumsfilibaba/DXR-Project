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
// FGenericThread

class CORE_API FGenericThread 
    : public FRefCounted
{
    friend struct FGenericThreadMisc;

protected:
    FGenericThread(const TFunction<void()>& InFunction)
        : Function(InFunction)
    { }

    ~FGenericThread() = default;

public:

    /**
     * @brief: Waits for the thread to complete and joins with caller 
     * 
     * @return: Returns the return-value from the thread
     */
    virtual int32 WaitForCompletion(uint64 TimeoutInMs) { return 0; }
    
    virtual bool Start() { return true; }
	
    virtual void* GetPlatformHandle() { return nullptr; }

    virtual FString GetName() const { return ""; }
    virtual void    SetName(const FString& InName) { }

    TFunction<void()> GetFunction() const { return Function; }

protected:
    TFunction<void()> Function;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
