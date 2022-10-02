#include "AsyncThreadPool.h"
#include "AsyncTask.h"

#include "Core/Threading/ScopedLock.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncThreadPool

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
    Check(NewTask != nullptr);
    CurrentTask = NewTask;
    Check(WorkEvent != nullptr);
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
        if (CurrentTask)
        {
            CurrentTask->DoAsyncWork();
            CurrentTask = FAsyncThreadPool::Get().ReturnThreadOrRetrieveNextTask(this);
        }
        else
        {
            WorkEvent->Wait(FTimespan::Infinity());
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncThreadPool

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
    Check(TaskQueue.GetSize() == 0);
}

bool FAsyncThreadPool::Initialize(int32 NumThreads)
{
    if (!GInstance)
    {
        GInstance = dbg_new FAsyncThreadPool();
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

FAsyncThreadPool& FAsyncThreadPool::Get()
{
    Check(GInstance != nullptr);
    return *GInstance;
}

bool FAsyncThreadPool::SubmitTask(IAsyncTask* NewTask, EQueuePriority Priority)
{
    SCOPED_LOCK(TaskQueueCS);

    Check(NewTask != nullptr);

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

    if (InThread)
    {
        IAsyncTask* NewTask = nullptr;
        if (TaskQueue.Dequeue(&NewTask))
        {
            return NewTask;
        }
        
        AvailableWorkers.Emplace(InThread);
    }

    return nullptr;
}

bool FAsyncThreadPool::CreateWorkers(int32 NumWorkers)
{
    bool bResult = true;
    for (int32 Index = 0; Index < NumWorkers && bResult; ++Index)
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
