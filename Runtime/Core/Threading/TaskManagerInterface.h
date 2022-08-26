#pragma once
#include "AtomicInt.h"

#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Platform/PlatformThread.h"
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

class CORE_API FTaskManagerInterface
{
    FTaskManagerInterface();
    ~FTaskManagerInterface();

public:
    static FTaskManagerInterface& Get();

    bool Initialize();
    void Release();

    DispatchID Dispatch(const FAsyncTask& NewTask);

    void WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting = true);
    void WaitForAll(bool bUseThisThreadWhileWaiting = true);

private:
    static void WorkThread();

    bool PopDispatch(FAsyncTask& OutTask);

    void KillWorkers();

    TArray<FGenericThreadRef> WorkerThreads;

    TArray<FAsyncTask> Queue;
    FCriticalSection   QueueCS;

    FConditionVariable WakeCondition;
    FCriticalSection   WakeMutex;

    FAtomicInt64       DispatchAdded;
    FAtomicInt64       DispatchCompleted;

    volatile bool      bIsRunning;

    static FTaskManagerInterface Instance;
};
