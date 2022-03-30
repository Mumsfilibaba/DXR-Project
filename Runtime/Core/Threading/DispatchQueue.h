#pragma once
#include "AtomicInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Information for a single dispatch

struct SDispatch
{
    DECLARE_DELEGATE(CTaskDelegate);
    CTaskDelegate Delegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DispatchQueue - Handles small tasks that are executed on another thread

typedef int64 DispatchID;

class CORE_API CDispatchQueue
{
public:

    /**
     * @brief: Retrieve the DispatchQueue instance
     * 
     * @return: Returns the DispatchQueue instance
     */
    static CDispatchQueue& Get();

    /**
     * @brief: Initialize DispatchQueue by starting worker threads 
     * 
     * @return: Returns true if the initialization was successful
     */
    bool Initialize();

    /**
     * @brief: Queue a new dispatch for being executed 
     * 
     * @param NewTask: A new task to dispatch when a worker thread is available
     * @return: Returns a dispatch ID that can be waited for
     */
    DispatchID Dispatch(const SDispatch& NewTask);

    /**
     * @brief: Wait for a specific task 
     * 
     * @param Task: A taskID to wait for
     */
    void WaitFor(DispatchID Task);

    /** Wait for all queued up tasks to be dispatched and finish */
    void WaitForAll();

    /** Release the DispatchQueue */
    void Release();

private:

    CDispatchQueue();
    ~CDispatchQueue();

    bool PopDispatch(SDispatch& OutTask);

    void KillWorkers();

    static void WorkThread();

    TArray<TSharedRef<CPlatformThread>> WorkerThreads;

    TArray<SDispatch> Queue;
    CCriticalSection QueueMutex;

    CConditionVariable WakeCondition;
    CCriticalSection WakeMutex;

    AtomicInt32 DispatchAdded;
    AtomicInt32 DispatchCompleted;

    volatile bool bIsRunning;

    static CDispatchQueue Instance;
};