#include "AsyncThreadPool.h"
#include "AsyncTask.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/Console/ConsoleManager.h"
#include "Core/Platform/PlatformMisc.h"

TAutoConsoleVariable<bool> CVarEnableAsyncWork("Core.EnableAsyncWork", true);

FAsyncWorkThread::FAsyncWorkThread()
    : CurrentTask(nullptr)
    , Triggered(0)
    , Thread(nullptr)
    , WorkEvent(nullptr)
    , bIsRunning(false)
{ }

bool FAsyncWorkThread::Create(const CHAR* InThreadName)
{
    Thread = FPlatformThreadMisc::CreateThread(this);
    if (!Thread)
    {
        LOG_ERROR("[FAsyncWorkThread] Failed to Create Thread");
        return false;
    }

    Thread->SetName(InThreadName);

    if (!Thread->Start())
    {
        LOG_ERROR("[FAsyncWorkThread] Failed to Start Thread");
        return false;
    }

    WorkEvent = FPlatformThreadMisc::CreateEvent(false);
    if (!WorkEvent)
    {
        LOG_ERROR("[FAsyncWorkThread] Failed to Create Event");
        this->Stop();
        return false;
    }

    return true;
}

void FAsyncWorkThread::WakeUpAndStartTask(IAsyncTask* NewTask)
{
    // New task must be nullptr and current-task must be nullptr
    CHECK(NewTask != nullptr);
    CHECK(CurrentTask == nullptr);
    CurrentTask = NewTask;

    // Ensure that everyone can see the CurrentTask
    FPlatformMisc::MemoryBarrier();

    CHECK(WorkEvent != nullptr);
    WorkEvent->Trigger();
}

bool FAsyncWorkThread::Start()
{
    bIsRunning = true;
    return true;
}

int32 FAsyncWorkThread::Run()
{
    if (!WorkEvent)
    {
        return -1;
    }

    while(bIsRunning)
    {
        FPlatformMisc::MemoryBarrier();

        if (!CurrentTask)
        {
            // LOG_ERROR("Num Tasks = %d", FAsyncThreadPool::Get().GetNumTasks());
            WorkEvent->Wait(FTimespan::Infinity());
        }
        
        // Set the member to nullptr and save it locally
        IAsyncTask* LocalTask = CurrentTask;
        CurrentTask = nullptr;

        // Ensure that everyone can see the CurrentTask
        FPlatformMisc::MemoryBarrier();

        while (LocalTask)
        {
            // Perform the task
            LocalTask->DoAsyncWork();
            
            // Then return the thread to the pool and check if any new tasks has 
            // been submitted in that case start work on that
            LocalTask = FAsyncThreadPool::Get().ReturnThreadOrRetrieveNextTask(this);
        }
    }

    return 0;
}

void FAsyncWorkThread::Stop()
{
    bIsRunning = false;

    if (WorkEvent)
    {
        WorkEvent->Trigger();
    }

    if (Thread)
    {
        Thread->WaitForCompletion();
    }
}


FAsyncThreadPool* FAsyncThreadPool::GInstance = nullptr;

FAsyncThreadPool::FAsyncThreadPool()
    : AvailableWorkers()
    , TaskQueue()
    , TaskQueueCS()
    , bIsRunning(false)
{ }

FAsyncThreadPool::~FAsyncThreadPool()
{
    // Lock in-case we are currently trying to submit from another thread
    SCOPED_LOCK(TaskQueueCS);
    CHECK(TaskQueue.GetSize() == 0);
}

bool FAsyncThreadPool::Initialize(int32 NumThreads)
{
    if (!GInstance)
    {
        GInstance = dbg_new FAsyncThreadPool();

        if (!CVarEnableAsyncWork.GetValue())
        {
            NumThreads = 0;
        }

        if (GInstance->CreateWorkers(NumThreads))
        {
            return true;
        }
    }

    return false;
}

void FAsyncThreadPool::Release()
{
    if (GInstance)
    {
        GInstance->DestroyWorkers();
        delete GInstance;
    }
}

bool FAsyncThreadPool::IsMultithreaded()
{
    if (GInstance)
    {
        return (GInstance->Workers.GetSize() > 0);
    }

    return false;
}

FAsyncThreadPool& FAsyncThreadPool::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}

bool FAsyncThreadPool::SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority)
{
    SCOPED_LOCK(TaskQueueCS);

    CHECK(NewTask != nullptr);

    // We can disable the async work pool, execute tasks here in these cases
    if (!CVarEnableAsyncWork.GetValue())
    {
        NewTask->DoAsyncWork();
        return true;
    }

    // If we have stopped running but a task still was submitted, abandon
    if (!bIsRunning)
    {
        NewTask->Abandon();
        return false;
    }

    if (AvailableWorkers.IsEmpty())
    {
        TaskQueue.Enqueue(NewTask, Priority);
    }
    else
    {
        FAsyncWorkThread* WorkerThread = AvailableWorkers.FirstElement();
        AvailableWorkers.RemoveAt(0);

        WorkerThread->WakeUpAndStartTask(NewTask);
    }

    return true;
}

bool FAsyncThreadPool::AbandonTask(IAsyncTask* NewTask)
{
    SCOPED_LOCK(TaskQueueCS);

    if (bIsRunning)
    {
        if (TaskQueue.Remove(NewTask))
        {
            return true;
        }
    }

    return false;
}

IAsyncTask* FAsyncThreadPool::ReturnThreadOrRetrieveNextTask(FAsyncWorkThread* InThread)
{
    SCOPED_LOCK(TaskQueueCS);

    CHECK(InThread != nullptr);

    IAsyncTask* NewTask = nullptr;
    if (TaskQueue.Dequeue(&NewTask))
    {
        return NewTask;
    }
    else
    {
        AvailableWorkers.Emplace(InThread);
        return nullptr;
    }
}

bool FAsyncThreadPool::CreateWorkers(int32 NumWorkers)
{
    bool bResult = true;
    for (int32 Index = 0; (Index < NumWorkers) && bResult; ++Index)
    {
        const FString Name = FString::CreateFormatted("Async Worker[%d]", Index);

        FAsyncWorkThread* NewWorker = dbg_new FAsyncWorkThread();
        if (NewWorker->Create(Name.GetCString()))
        {
            Workers.Emplace(NewWorker);
            AvailableWorkers.Emplace(NewWorker);
        }
        else
        {
            LOG_ERROR("[FAsyncThreadPool] Failed to create worker thread");
            
            delete NewWorker;
            bResult = false;
        }
    }

    if (!bResult)
    {
        DestroyWorkers();
    }
    else
    {
        bIsRunning = true;
    }

    return bResult;
}

void FAsyncThreadPool::DestroyWorkers()
{
    // Start by abandoning all the queued tasks
    {
        SCOPED_LOCK(TaskQueueCS);

        bIsRunning = false;

        // Ensure bIsRunning is visible for all threads
        FPlatformMisc::MemoryBarrier();

        IAsyncTask* Task = nullptr;
        while (TaskQueue.Dequeue(&Task))
        {
            if (Task)
            {
                Task->Abandon();
            }
        }

        TaskQueue.Reset();
    }

    // Wait for all the threads to finish
    {
        while (true)
        {
            {
                SCOPED_LOCK(TaskQueueCS);

                if (Workers.GetSize() == AvailableWorkers.GetSize())
                {
                    break;
                }
            }

            FPlatformThreadMisc::Sleep(FTimespan());
        }

    }

    // Wake all the threads up and stop them
    {
        SCOPED_LOCK(TaskQueueCS);

        for (FAsyncWorkThread* CurrentWorker : Workers)
        {
            CurrentWorker->Stop();
            delete CurrentWorker;
        }

        Workers.Clear();
        AvailableWorkers.Clear();
    }
}
