#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Function.h"
#include "Core/Threading/ThreadInterface.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class CORE_API FGenericThread : public FRefCounted
{
public:
    FGenericThread(FThreadInterface* InRunnable)
        : FRefCounted()
        , Runnable(InRunnable)
    {
    }

    /** @brief - Start the thread and start executing the entrypoint */
    virtual bool Start() { return true; }

    /** @brief - Waits for the thread to finish */
    virtual void WaitForCompletion() { }
    
    /** @return - Returns the native platform handle */
    virtual void* GetPlatformHandle() { return nullptr; }

    /** @return - Returns the name of the thread */
    virtual FString GetName() const { return ""; }

    /** @brief - Set the name of the thread */
    virtual void SetName(const FString& InName) { }

    /** @return - Returns a pointer to the interface currently running on the thread */
    FThreadInterface* GetRunnable() const 
    { 
        return Runnable; 
    }

protected:
    FThreadInterface* Runnable;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
