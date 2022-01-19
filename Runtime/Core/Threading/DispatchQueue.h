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

    static CDispatchQueue& Get();

    /* Init DispatchQueue by starting worker threads */
    bool Init();

    /* Queue a new dispatch for being executed */
    DispatchID Dispatch(const SDispatch& NewTask);

    /* Wait for a specific task */
    void WaitFor(DispatchID Task);

    /* Wait for all queued up dispatched */
    void WaitForAll();

    /* Release the DispatchQueue */
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