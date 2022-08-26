#pragma once
#include "TaskManagerInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTaskManager

class FTaskManager
    : public FTaskManagerInterface
{
public:
    FTaskManager();
    ~FTaskManager();

    virtual bool Initialize() override final;
    virtual void Release()    override final;

    virtual DispatchID Dispatch(const FAsyncTask& NewTask) override final;

    virtual void WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting = true) override final;
    virtual void WaitForAll(bool bUseThisThreadWhileWaiting = true) override final;

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
};