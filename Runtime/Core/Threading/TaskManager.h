#pragma once
#include "Runnable.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/PriorityQueue.h"
#include "Core/Containers/Optional.h"

struct IAsyncTask;

class CORE_API FTaskWorkerThread : public FRunnable
{
public:
    FTaskWorkerThread();
    virtual ~FTaskWorkerThread();
    
    virtual bool Start() override final;
    virtual int32 Run() override final;
    virtual void Stop() override final;
    
    bool Create(const CHAR* InThreadName);
    void WakeUpAndStartTask(IAsyncTask* NewTask);

private:
    IAsyncTask* volatile CurrentTask;
    FAtomicInt32         Triggered;
    FGenericEvent*       Event;
    FGenericThread*      Thread;
    bool                 bIsRunning;
};

class CORE_API FTaskManager
{
public:
    static bool Initialize();
    static void Release();
    static bool IsMultithreaded();

    static FTaskManager& Get()
    {
        CHECK(GInstance != nullptr);
        return *GInstance;
    }

    // Submits a new tasks, which gets picked up by a worker thread
    bool SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority = EQueuePriority::Normal);

    // Abondon the specified task
    bool AbandonTask(IAsyncTask* NewTask);

    // Returns a worker thread to the workthread pool, or returns a new task to process if there are any queued up
    IAsyncTask* ReturnThreadOrRetrieveNextTask(FTaskWorkerThread* InThread);

    int32 GetNumTasks() const { return TaskQueue.Size(); }

private:
    FTaskManager();
    ~FTaskManager();

    bool CreateWorkers(int32 NumWorkers);
    void DestroyWorkers();

    TArray<FTaskWorkerThread*>  AvailableWorkers;
    TArray<FTaskWorkerThread*>  Workers;
    TPriorityQueue<IAsyncTask*> TaskQueue;
    FCriticalSection            TaskQueueCS;
    bool                        bIsRunning;

    static FTaskManager* GInstance;
};
