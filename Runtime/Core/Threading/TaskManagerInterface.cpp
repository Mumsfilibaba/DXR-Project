#include "ScopedLock.h"
#include "AsyncTaskManager.h"
#include "ThreadManager.h"

#include "Core/Platform/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTaskManager

FTaskManagerInterface FTaskManagerInterface::Instance;

FTaskManagerInterface::FTaskManagerInterface()
    : QueueCS()
    , bIsRunning(false)
{ }

FTaskManagerInterface::~FTaskManagerInterface()
{
    KillWorkers();
}

bool FTaskManagerInterface::PopDispatch(FAsyncTask& OutTask)
{
    TScopedLock<FCriticalSection> Lock(QueueCS);

    if (!Queue.IsEmpty())
    {
        OutTask = Queue.FirstElement();
        Queue.RemoveAt(0);

        return true;
    }
    else
    {
        return false;
    }
}

void FTaskManagerInterface::KillWorkers()
{
    if (bIsRunning && !WorkerThreads.IsEmpty())
    {
        bIsRunning = false;
        WakeCondition.NotifyAll();
    }
}

void FTaskManagerInterface::WorkThread()
{
    LOG_INFO("Starting Work thread: %llu", FPlatformThreadMisc::GetThreadHandle());

    FTaskManagerInterface& AsyncTaskManager = FTaskManagerInterface::Get();
    while (AsyncTaskManager.bIsRunning)
    {
        FAsyncTask CurrentTask;

        if (!Instance.PopDispatch(CurrentTask))
        {
            TScopedLock<FCriticalSection> Lock(Instance.WakeMutex);
            Instance.WakeCondition.Wait(Lock);
        }
        else
        {
            CurrentTask.Delegate.ExecuteIfBound();
            Instance.DispatchCompleted.Increment();
        }
    }

    LOG_INFO("End Work thread: %llu", FPlatformThreadMisc::GetThreadHandle());
}

bool FTaskManagerInterface::Initialize()
{
    const uint32 ThreadCount = NMath::Max<int32>(FPlatformThreadMisc::GetNumProcessors() - 1, 1);
    WorkerThreads.Resize(ThreadCount);

    if (ThreadCount == 1)
    {
        LOG_INFO("[FAsyncTaskManager]: No workers available, tasks will be executing on the main thread");
        WorkerThreads.Clear();
        return true;
    }

    LOG_INFO("[FAsyncTaskManager]: Starting '%u' Workers", ThreadCount);

    // Start so that workers now that they should be running
    bIsRunning = true;

    for (uint32 Thread = 0; Thread < ThreadCount; ++Thread)
    {
        FString ThreadName = FString::CreateFormatted("WorkerThread[%d]", Thread);

        FGenericThreadRef NewThread = FThreadManager::Get().CreateNamedThread(FTaskManagerInterface::WorkThread, ThreadName);
        if (NewThread)
        {
            WorkerThreads[Thread] = NewThread;
            NewThread->Start();
        }
        else
        {
            KillWorkers();
            return false;
        }
    }

    return true;
}

DispatchID FTaskManagerInterface::Dispatch(const FAsyncTask& NewTask)
{
    if (WorkerThreads.IsEmpty())
    {
        // Execute task on main-thread
        FAsyncTask MainThreadTask = NewTask;
        MainThreadTask.Delegate.ExecuteIfBound();

        // Make sure that both fences is incremented
        DispatchID NewTaskID = DispatchAdded.Increment();
        DispatchCompleted.Increment();
        return NewTaskID;
    }

    DispatchID NewTaskID = DispatchAdded.Increment();

    {
        TScopedLock<FCriticalSection> Lock(QueueCS);
        Queue.Emplace(NewTask);
    }

    WakeCondition.NotifyOne();
    return NewTaskID;
}

void FTaskManagerInterface::WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting)
{
    while (DispatchCompleted.Load() < Task)
    {
        if (bUseThisThreadWhileWaiting)
        {
            FAsyncTask CurrentTask;

            if (Instance.PopDispatch(CurrentTask))
            {
                CurrentTask.Delegate.ExecuteIfBound();
                Instance.DispatchCompleted.Increment();
            }
        }

        // TODO: Look into proper yield
        FPlatformThreadMisc::Sleep(0);
    }
}

void FTaskManagerInterface::WaitForAll(bool bUseThisThreadWhileWaiting)
{
    WaitFor(DispatchAdded.Load(), bUseThisThreadWhileWaiting);
}

void FTaskManagerInterface::Release()
{
    KillWorkers();

    for (TSharedRef<FGenericThread> Thread : WorkerThreads)
    {
        Thread->WaitForCompletion(FTimespan::Infinity());
    }

    WorkerThreads.Clear();
}

FTaskManagerInterface& FTaskManagerInterface::Get()
{
    return Instance;
}
