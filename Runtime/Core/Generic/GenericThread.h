#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Function.h"
#include "Core/Threading/Runnable.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class CORE_API FGenericThread
{
public:

    // Creates a new thread
    static FGenericThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

    // Returns the thread-object for the current thread
    static FGenericThread* GetThread();

public:

    /** @brief Destructor */
    virtual ~FGenericThread();

    /** @brief Start the thread and start executing the entrypoint */
    virtual bool Start() { return true; }

    /** @brief Kills the thread if the platform support the feature */
    virtual void Kill(bool bWaitUntilCompletion) { }

    /** @brief Suspends the thread if the platform support the feature */
    virtual void Suspend() { }

    /** @brief Resumes the thread after being suspended if the platform support the feature */
    virtual void Resume() { }

    /** @brief Waits for the thread to finish */
    virtual void WaitForCompletion() { }
    
    /** @return Returns the native platform handle */
    virtual void* GetPlatformHandle() { return nullptr; }

    /** @return Returns the name of the thread */
    const FString& GetName() const
    {
        return ThreadName;
    }

    /** @return Returns a pointer to the interface currently running on the thread */
    FRunnable* GetRunnable() const 
    { 
        return Runnable; 
    }

protected:
    FGenericThread(FRunnable* InRunnable, const CHAR* InThreadName);

    // Returns and allocates a TLS slot for the local thread pointer
    static uint32 AllocTLSSlot();

    FRunnable* Runnable;
    FString    ThreadName;

    // Slot-Index for storing the current threads pointer
    static uint32 TLSSlot;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
