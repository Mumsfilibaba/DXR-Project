#pragma once
#include "AtomicInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 DispatchID;

struct SDispatch
{
    DECLARE_DELEGATE( CTaskDelegate );
    CTaskDelegate Delegate;
};

class CORE_API CDispatchQueue
{
public:

    static CDispatchQueue& Get();

    /* Init DispatchQueue by starting worker threads */
    bool Init();

    /* Queue a new dispatch for being executed */
    DispatchID Dispatch( const SDispatch& NewTask );

    /* Wait for a specific task */
    void WaitFor( DispatchID Task );

    /* Wait for all queued up dispatched */
    void WaitForAll();

    /* Release the DispatchQueue */
    void Release();

private:

    CDispatchQueue();
    ~CDispatchQueue();

    bool PopDispatch( SDispatch& OutTask );

    void KillWorkers();

    static void WorkThread();

    TArray<TSharedRef<CPlatformThread>> WorkerThreads;

    TArray<SDispatch> Queue;
    CCriticalSection QueueMutex;

    CConditionVariable WakeCondition;
    CCriticalSection WakeMutex;

    AtomicInt32 DispatchAdded;
    AtomicInt32 DispatchCompleted;

    volatile bool IsRunning;

    static CDispatchQueue Instance;
};