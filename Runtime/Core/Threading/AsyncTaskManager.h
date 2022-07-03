#pragma once
#include "AtomicInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 DispatchID;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTask

class FAsyncTask
{
public:

    DECLARE_DELEGATE(FTaskDelegate);
    FTaskDelegate Delegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTaskManager

class CORE_API FAsyncTaskManager
{
private:

    FAsyncTaskManager();
    ~FAsyncTaskManager();

public:

    /**
     * @return: Returns the DispatchQueue instance 
     */
    static FAsyncTaskManager& Get();

    /**
     * @return: Returns true if the initialization was successful 
     */
    bool Initialize();

    /**
     * @brief: Queue a new dispatch for being executed 
     * 
     * @param NewTask: A new task to dispatch when a worker thread is available
     * @return: Returns a dispatch ID that can be waited for
     */
    DispatchID Dispatch(const FAsyncTask& NewTask);

    /**
     * @brief: Wait for a specific task 
     * 
     * @param Task: A taskID to wait for
     */
    void WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting = true);

    /**
     * @brief: Wait for all queued up tasks to be dispatched and finish 
     */
    void WaitForAll(bool bUseThisThreadWhileWaiting = true);

    /**
     * @brief: Release the DispatchQueue 
     */
    void Release();

private:

    static void WorkThread();

    bool PopDispatch(FAsyncTask& OutTask);

    void KillWorkers();

    TArray<TSharedRef<FGenericThread>> WorkerThreads;

    TArray<FAsyncTask> Queue;
    FCriticalSection   QueueMutex;

    FConditionVariable WakeCondition;
    FCriticalSection   WakeMutex;

    FAtomicInt64        DispatchAdded;
    FAtomicInt64        DispatchCompleted;

    volatile bool      bIsRunning;

    static FAsyncTaskManager Instance;
};
