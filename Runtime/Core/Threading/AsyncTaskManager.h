#pragma once
#include "AtomicInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 DispatchID;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CAsyncTask

class CAsyncTask
{
public:

    DECLARE_DELEGATE(CTaskDelegate);
    CTaskDelegate Delegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CAsyncTaskManager

class CORE_API CAsyncTaskManager
{
private:

    CAsyncTaskManager();
    ~CAsyncTaskManager();

public:

    /**
     * @return: Returns the DispatchQueue instance 
     */
    static CAsyncTaskManager& Get();

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
    DispatchID Dispatch(const CAsyncTask& NewTask);

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

    bool PopDispatch(CAsyncTask& OutTask);

    void KillWorkers();

    TArray<TSharedRef<CGenericThread>> WorkerThreads;

    TArray<CAsyncTask> Queue;
    CCriticalSection   QueueMutex;

    CConditionVariable WakeCondition;
    CCriticalSection   WakeMutex;

    AtomicInt64        DispatchAdded;
    AtomicInt64        DispatchCompleted;

    volatile bool      bIsRunning;

    static CAsyncTaskManager Instance;
};
