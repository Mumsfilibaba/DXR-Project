#pragma once
#include "Core/Time/Timespan.h"
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

typedef TSharedRef<class FGenericThread> FGenericThreadRef;

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

    /** @brief: Start the thread and start executing the entrypoint */
    virtual bool Start() { return true; }

    /** @return: Waits for the thread and returns the return-value from the thread  */
    virtual int32 WaitForCompletion(uint64 TimeoutInMs) { return 0; }

    /** @return: Waits for the thread and returns the return-value from the thread  */
    virtual int32 WaitForCompletion(FTimespan Timeout) { return WaitForCompletion(static_cast<uint64>(Timeout.AsMilliseconds())); }
    
    /** @return: Returns the native platform handle */
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
