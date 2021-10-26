#pragma once
#include "InterlockedInt.h"

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
    bool Init();

    DispatchID Dispatch( const SDispatch& NewTask );

    void WaitFor( DispatchID Task );
    void WaitForAll();

    void Release();

    static CDispatchQueue& Get();

private:
    CDispatchQueue();
    ~CDispatchQueue();

    bool PopDispatch( SDispatch& OutTask );

    void KillWorkers();

    static void WorkThread();

private:
    TArray<TSharedRef<CCoreThread>> WorkerThreads;

    TArray<SDispatch> Queue;
    CCriticalSection QueueMutex;

    CConditionVariable WakeCondition;
    CCriticalSection WakeMutex;

    InterlockedInt32 DispatchAdded;
    InterlockedInt32 DispatchCompleted;

    volatile bool IsRunning;

    static CDispatchQueue Instance;
};