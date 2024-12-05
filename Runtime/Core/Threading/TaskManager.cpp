#include "TaskManager.h"
#include "AsyncTask.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Platform/PlatformThread.h"

static TAutoConsoleVariable<int32> CVarNumTaskThreads(
    "Core.NumTaskThreads",
    "Sets the number of worker threads available to perform Async tasks",
    TNumericLimits<int32>::Max());

FTaskWorkerThread::FTaskWorkerThread()
    : CurrentTask(nullptr)
    , Triggered(0)
    , Event(nullptr)
    , Thread(nullptr)
    , bIsRunning(false)
{
}

FTaskWorkerThread::~FTaskWorkerThread()
{
    if (Thread)
    {
        Thread->WaitForCompletion();
        delete Thread;
    }
}

bool FTaskWorkerThread::Create(const CHAR* InThreadName)
{
    Event = FPlatformEvent::Create(false);
    if (!Event)
    {
        LOG_ERROR("[FTaskWorkerThread] Failed to Create Event");
        return false;
    }

    Thread = FPlatformThread::Create(this, InThreadName);
    if (!Thread)
    {
        LOG_ERROR("[FTaskWorkerThread] Failed to Create Thread");
        return false;
    }

    if (!Thread->Start())
    {
        LOG_ERROR("[FTaskWorkerThread] Failed to Start Thread");
        return false;
    }

    return true;
}

void FTaskWorkerThread::WakeUpAndStartTask(IAsyncTask* NewTask)
{
    // New task must be nullptr and current-task must be nullptr
    CHECK(NewTask != nullptr);
    CHECK(CurrentTask == nullptr);

    CurrentTask = NewTask;
    FPlatformMisc::MemoryBarrier();

    CHECK(Event != nullptr);
    Event->Trigger();
}

bool FTaskWorkerThread::Start()
{
    bIsRunning = true;
    return true;
}

int32 FTaskWorkerThread::Run()
{
    CHECK(Event != nullptr);

    while(bIsRunning)
    {
        FPlatformMisc::MemoryBarrier();

        if (!CurrentTask)
        {
            Event->Wait(FTimespan::Infinity());
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

            // Then return the thread to the pool and check if any new tasks has been submitted in that case start work on that
            LocalTask = FTaskManager::Get().ReturnThreadOrRetrieveNextTask(this);
        }
    }

    return 0;
}

void FTaskWorkerThread::Stop()
{
    bIsRunning = false;
    if (Event)
    {
        Event->Trigger();
    }
}


FTaskManager* FTaskManager::GInstance = nullptr;

FTaskManager::FTaskManager()
    : AvailableWorkers()
    , TaskQueue()
    , TaskQueueCS()
    , bIsRunning(false)
{
}

FTaskManager::~FTaskManager()
{
    // Lock in-case we are currently trying to submit from another thread
    SCOPED_LOCK(TaskQueueCS);
    CHECK(TaskQueue.Size() == 0);
}

bool FTaskManager::Initialize()
{
    if (!GInstance)
    {
        GInstance = new FTaskManager();

        int32 NumThreads = CVarNumTaskThreads.GetValue();
        if (NumThreads <= 0)
        {
            NumThreads = -1;
        }

        if (GInstance->CreateWorkers(NumThreads))
        {
            return true;
        }
    }

    return false;
}

void FTaskManager::Release()
{
    if (GInstance)
    {
        GInstance->DestroyWorkers();
        delete GInstance;
    }
}

bool FTaskManager::IsMultithreaded()
{
    if (GInstance)
    {
        return GInstance->Workers.Size() > 0;
    }

    return false;
}

bool FTaskManager::SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority)
{
    CHECK(NewTask != nullptr);

    // We can disable the Async work pool, execute tasks here in these cases
    if (Workers.IsEmpty())
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

    SCOPED_LOCK(TaskQueueCS);

    // No workers available so enqueue it
    if (AvailableWorkers.IsEmpty())
    {
        TaskQueue.Enqueue(NewTask, Priority);
        FPlatformMisc::MemoryBarrier();
        return true;
    }

    // Check if there is an available worker that can work on this task
    FTaskWorkerThread* WorkerThread = AvailableWorkers[0];
    CHECK(WorkerThread != nullptr);
    AvailableWorkers.RemoveAt(0);

    FPlatformMisc::MemoryBarrier();

    WorkerThread->WakeUpAndStartTask(NewTask);
    return true;
}

bool FTaskManager::AbandonTask(IAsyncTask* NewTask)
{
    if (bIsRunning)
    {
        SCOPED_LOCK(TaskQueueCS);

        if (TaskQueue.Remove(NewTask))
        {
            FPlatformMisc::MemoryBarrier();
            return true;
        }
    }

    return false;
}

IAsyncTask* FTaskManager::ReturnThreadOrRetrieveNextTask(FTaskWorkerThread* InThread)
{
    SCOPED_LOCK(TaskQueueCS);

    CHECK(InThread != nullptr);

    IAsyncTask* NewTask = nullptr;
    if (TaskQueue.Dequeue(&NewTask))
    {
        FPlatformMisc::MemoryBarrier();
        return NewTask;
    }
    else
    {
        AvailableWorkers.Emplace(InThread);
        FPlatformMisc::MemoryBarrier();
        return nullptr;
    }
}

bool FTaskManager::CreateWorkers(int32 NumWorkers)
{
    constexpr int32 MinNumWorkers = 2;

    bool bResult = true;
    if (NumWorkers >= MinNumWorkers)
    {
        // Set the number of total workers, but limit to the number or logical processors on the system
        NumWorkers = FMath::Clamp<int32>(NumWorkers, MinNumWorkers, static_cast<int32>(FPlatformThreadMisc::GetNumProcessors()));

        // Startup workers
        for (int32 Index = 0; Index < NumWorkers && bResult; ++Index)
        {
            const FString Name = FString::CreateFormatted("Task Worker[%d]", Index);

            FTaskWorkerThread* NewWorker = new FTaskWorkerThread();
            if (NewWorker->Create(*Name))
            {
                Workers.Emplace(NewWorker);
                AvailableWorkers.Emplace(NewWorker);
            }
            else
            {
                LOG_ERROR("[FTaskManager] Failed to create worker thread");
            
                delete NewWorker;
                bResult = false;
            }
        }
    }
    else
    {
        LOG_INFO("Turning off worker threads since there are not enough worker threads specified. MinNumWorkers=%d NumWorkers=%d", MinNumWorkers, NumWorkers);
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

void FTaskManager::DestroyWorkers()
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

                if (Workers.Size() == AvailableWorkers.Size())
                {
                    break;
                }
            }

            FPlatformThreadMisc::Pause();
        }
    }

    // Wake all the threads up and stop them
    {
        SCOPED_LOCK(TaskQueueCS);

        for (FTaskWorkerThread* CurrentWorker : Workers)
        {
            CurrentWorker->Stop();
            delete CurrentWorker;
        }

        Workers.Clear();
        AvailableWorkers.Clear();
    }
}
