#include "ScopedLock.h"
#include "TaskManager.h"
#include "ThreadManager.h"

#include "Core/Platform/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTaskManager

FTaskManager::FTaskManager()
    : QueueCS()
    , bIsRunning(false)
{ }

FTaskManager::~FTaskManager()
{
    KillWorkers();
}

bool FTaskManager::PopDispatch(FAsyncTask& OutTask)
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

void FTaskManager::KillWorkers()
{
    if (bIsRunning && !WorkerThreads.IsEmpty())
    {
        bIsRunning = false;
        WakeCondition.NotifyAll();
    }
}

void FTaskManager::WorkThread()
{
    LOG_INFO("Starting Work thread: %llu", FPlatformThreadMisc::GetThreadHandle());

    FTaskManager& TaskManager = static_cast<FTaskManager&>(FTaskManagerInterface::Get());
    while (TaskManager.bIsRunning)
    {
        FAsyncTask CurrentTask;
        if (!TaskManager.PopDispatch(CurrentTask))
        {
            TScopedLock<FCriticalSection> Lock(TaskManager.WakeMutex);
            TaskManager.WakeCondition.Wait(Lock);
        }
        else
        {
            CurrentTask.Delegate.ExecuteIfBound();
            TaskManager.DispatchCompleted.Increment();
        }
    }

    LOG_INFO("End Work thread: %llu", FPlatformThreadMisc::GetThreadHandle());
}

bool FTaskManager::Initialize()
{
    const uint32 ThreadCount = NMath::Max<int32>(FPlatformThreadMisc::GetNumProcessors() - 1, 1);
    WorkerThreads.Resize(ThreadCount);

    if (ThreadCount == 1)
    {
        LOG_INFO("[FTaskManager]: No workers available, tasks will be executing on the main thread");
        WorkerThreads.Clear();
        return true;
    }

    LOG_INFO("[FTaskManager]: Starting '%u' Workers", ThreadCount);

    // Start so that workers now that they should be running
    bIsRunning = true;

    for (uint32 Thread = 0; Thread < ThreadCount; ++Thread)
    {
        FString ThreadName = FString::CreateFormatted("WorkerThread[%d]", Thread);
        if (FGenericThreadRef NewThread = FThreadManager::Get().CreateNamedThread(FTaskManager::WorkThread, ThreadName))
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

DispatchID FTaskManager::Dispatch(const FAsyncTask& NewTask)
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

    WakeCondition.NotifyAll();
    return NewTaskID;
}

void FTaskManager::WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting)
{
    while (DispatchCompleted.Load() < Task)
    {
        if (bUseThisThreadWhileWaiting)
        {
            FAsyncTask CurrentTask;
            if (PopDispatch(CurrentTask))
            {
                CurrentTask.Delegate.ExecuteIfBound();
                DispatchCompleted.Increment();
            }
        }

        // TODO: Look into proper yield
        FPlatformThreadMisc::Sleep(0);
    }
}

void FTaskManager::WaitForAll(bool bUseThisThreadWhileWaiting)
{
    WaitFor(DispatchAdded.Load(), bUseThisThreadWhileWaiting);
}

void FTaskManager::Release()
{
    KillWorkers();

    for (TSharedRef<FGenericThread> Thread : WorkerThreads)
    {
        Thread->WaitForCompletion(FTimespan::Infinity());
    }

    WorkerThreads.Clear();
}
