#pragma once
#include "ThreadInterface.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/PriorityQueue.h"
#include "Core/Containers/Optional.h"

struct IAsyncTask;

class CORE_API FTaskWorkerThread : public FThreadInterface
{
public:
    FTaskWorkerThread();
    virtual ~FTaskWorkerThread() = default;
    
    virtual bool Start() override final;

    virtual int32 Run() override final;

    virtual void Stop() override final;
    
    bool Create(const CHAR* InThreadName);

    void WakeUpAndStartTask(IAsyncTask* NewTask);

private:
    IAsyncTask* volatile       CurrentTask;
    FAtomicInt32               Triggered;
    TSharedRef<FGenericEvent>  Event;
    TSharedRef<FGenericThread> Thread;
    bool bIsRunning : 1;
};

class CORE_API FTaskManager
{
public:
    static FTaskManager& Get()
    {
        CHECK(GInstance != nullptr);
        return *GInstance;
    }

    static bool Initialize();
    static void Release();

    static bool IsMultithreaded();

    bool SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority = EQueuePriority::Normal);

    bool AbandonTask(IAsyncTask* NewTask);

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
    bool bIsRunning;

    static FTaskManager* GInstance;
};
