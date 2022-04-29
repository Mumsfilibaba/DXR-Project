#pragma once
#include "AtomicInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 DispatchID;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SAsyncTask

struct SAsyncTask
{
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
    DispatchID Dispatch(const SAsyncTask& NewTask);

    /**
     * @brief: Wait for a specific task 
     * 
     * @param Task: A taskID to wait for
     */
    void WaitFor(DispatchID Task);

    /**
     * @brief: Wait for all queued up tasks to be dispatched and finish 
     */
    void WaitForAll();

    /**
     * @brief: Release the DispatchQueue 
     */
    void Release();

private:

    static void WorkThread();

    bool PopDispatch(SAsyncTask& OutTask);

    void KillWorkers();

    TArray<TSharedRef<CGenericThread>> WorkerThreads;

    TArray<SAsyncTask>  Queue;
    CCriticalSection   QueueMutex;

    CConditionVariable WakeCondition;
    CCriticalSection   WakeMutex;

    AtomicInt32        DispatchAdded;
    AtomicInt32        DispatchCompleted;

    volatile bool      bIsRunning;

    static CAsyncTaskManager Instance;
};