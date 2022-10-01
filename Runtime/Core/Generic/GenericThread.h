#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Function.h"
#include "Core/Threading/ThreadInterface.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

typedef TSharedRef<class FGenericThread> FGenericThreadRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericThread

class CORE_API FGenericThread 
    : public FRefCounted
{
public:
    FGenericThread(FThreadInterface* InRunnable)
        : Runnable(InRunnable)
    { }

    /** @brief: Start the thread and start executing the entrypoint */
    virtual bool Start() { return true; }

    /** @brief: Waits for the thread and returns the return-value from the thread  */
    virtual void WaitForCompletion() { }
    
    /** @return: Returns the native platform handle */
    virtual void* GetPlatformHandle() { return nullptr; }

    /** @return: Returns the name of the thread */
    virtual FString GetName() const { return ""; }

    /** @breif: Set the name of the thread */
    virtual void SetName(const FString& InName) { }

    /** @return: Returns a pointer to the interface currently running on the thread */
    FThreadInterface* GetRunnable() const { return Runnable; }

protected:
    FThreadInterface* Runnable;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
