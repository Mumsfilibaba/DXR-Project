#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Function.h"
#include "Core/Threading/Runnable.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct CORE_API IPlatformThread
{
public:

    /** @brief Creates a new thread. Each platform hides this with its own */
    static IPlatformThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true)
    {
        return nullptr;
    }

    /** @return Returns the thread-object for the current thread */
    static IPlatformThread* GetThread();

public:

    virtual ~IPlatformThread() = default;

    /** @brief Start the thread and start executing the entrypoint */
    virtual bool Start() = 0;

    /** @brief Kills the thread if the platform support the feature */
    virtual void Kill(bool bWaitUntilCompletion) = 0;

    /** @brief Suspends the thread if the platform support the feature */
    virtual void Suspend() = 0;

    /** @brief Resumes the thread after being suspended if the platform support the feature */
    virtual void Resume() = 0;

    /** @brief Waits for the thread to finish */
    virtual void WaitForCompletion() = 0;

    /** @return Returns the native platform handle */
    virtual void* GetPlatformHandle() = 0;

    /** @return Returns the name of the thread */
    virtual const String& GetName() const = 0;

    /** @return Returns a pointer to the interface currently running on the thread */
    virtual FRunnable* GetRunnable() const = 0;

protected:
    static uint32 AllocTLSSlot();

    static uint32 TLSSlot;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
