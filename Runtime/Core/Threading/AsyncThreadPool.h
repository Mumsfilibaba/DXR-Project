#pragma once
#include "ThreadInterface.h"

#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/PriorityQueue.h"
#include "Core/Containers/Optional.h"

struct IAsyncTask;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncWorkThread

class FAsyncWorkThread
    : public FThreadInterface
{
public:
    FAsyncWorkThread();
    ~FAsyncWorkThread() = default;

    bool Create(const CHAR* InThreadName);
    void WakeUpAndStartTask(IAsyncTask* NewTask);

    virtual bool Start() override final;

    virtual int32 Run() override final;

    virtual void Stop() override final;

private:
    IAsyncTask*       CurrentTask;
    FAtomicInt32      Triggered;

    FGenericThreadRef Thread;
    FGenericEventRef  WorkEvent;
    bool              bIsRunning;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncThreadPool

class CORE_API FAsyncThreadPool
{
private:
    FAsyncThreadPool();
    ~FAsyncThreadPool();

public:
    static bool Initialize(int32 NumThreads);
    static void Release();

    static FAsyncThreadPool& Get();

    bool SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority = EQueuePriority::Normal);
    bool AbandonTask(IAsyncTask* NewTask);

    IAsyncTask* ReturnThreadOrRetrieveNextTask(FAsyncWorkThread* InThread);

private:
    bool CreateWorkers(int32 NumWorkers);
    void DestroyWorkers();

    TArray<FAsyncWorkThread*>   AvailableWorkers;
    TArray<FAsyncWorkThread*>   Workers;

    TPriorityQueue<IAsyncTask*> TaskQueue;
    FCriticalSection            TaskQueueCS;

    bool                        bIsRunning;

    static FAsyncThreadPool* GInstance;
};