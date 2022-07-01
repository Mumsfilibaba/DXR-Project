#include "ScopedLock.h"
#include "AsyncTaskManager.h"
#include "ThreadManager.h"

#include "Platform/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CAsyncTaskManager

CAsyncTaskManager CAsyncTaskManager::Instance;

CAsyncTaskManager::CAsyncTaskManager()
    : QueueMutex()
    , bIsRunning(false)
{ }

CAsyncTaskManager::~CAsyncTaskManager()
{
    KillWorkers();
}

bool CAsyncTaskManager::PopDispatch(CAsyncTask& OutTask)
{
    TScopedLock<CCriticalSection> Lock(QueueMutex);

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

void CAsyncTaskManager::KillWorkers()
{
    bIsRunning = false;

    WakeCondition.NotifyAll();
}

void CAsyncTaskManager::WorkThread()
{
    LOG_INFO("Starting Work thread: %llu", PlatformThreadMisc::GetThreadHandle());

    CAsyncTaskManager& AsyncTaskManager = CAsyncTaskManager::Get();
    while (AsyncTaskManager.bIsRunning)
    {
        CAsyncTask CurrentTask;

        if (!Instance.PopDispatch(CurrentTask))
        {
            TScopedLock<CCriticalSection> Lock(Instance.WakeMutex);
            Instance.WakeCondition.Wait(Lock);
        }
        else
        {
            CurrentTask.Delegate.ExecuteIfBound();
            Instance.DispatchCompleted.Increment();
        }
    }

    LOG_INFO("End Work thread: %llu", PlatformThreadMisc::GetThreadHandle());
}

bool CAsyncTaskManager::Initialize()
{
    const uint32 ThreadCount = NMath::Max<int32>(PlatformThreadMisc::GetNumProcessors() - 1, 1);
    WorkerThreads.Resize(ThreadCount);

    if (ThreadCount == 1)
    {
        LOG_INFO("[CAsyncTaskManager]: No workers available, tasks will be executing on the main thread");
        WorkerThreads.Clear();
        return true;
    }

    LOG_INFO("[CAsyncTaskManager]: Starting '%u' Workers", ThreadCount);

    // Start so that workers now that they should be running
    bIsRunning = true;

    for (uint32 Thread = 0; Thread < ThreadCount; ++Thread)
    {
        FString ThreadName = FString::CreateFormated("WorkerThread[%d]", Thread);

        GenericThreadRef NewThread = CThreadManager::Get().CreateNamedThread(CAsyncTaskManager::WorkThread, ThreadName);
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

DispatchID CAsyncTaskManager::Dispatch(const CAsyncTask& NewTask)
{
    if (WorkerThreads.IsEmpty())
    {
        // Execute task on main-thread
        CAsyncTask MainThreadTask = NewTask;
        MainThreadTask.Delegate.ExecuteIfBound();

        // Make sure that both fences is incremented
        DispatchID NewTaskID = DispatchAdded.Increment();
        DispatchCompleted.Increment();
        return NewTaskID;
    }

    DispatchID NewTaskID = DispatchAdded.Increment();

    {
        TScopedLock<CCriticalSection> Lock(QueueMutex);
        Queue.Emplace(NewTask);
    }

    WakeCondition.NotifyOne();
    return NewTaskID;
}

void CAsyncTaskManager::WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting)
{
    while (DispatchCompleted.Load() < Task)
    {
        if (bUseThisThreadWhileWaiting)
        {
            CAsyncTask CurrentTask;

            if (Instance.PopDispatch(CurrentTask))
            {
                CurrentTask.Delegate.ExecuteIfBound();
                Instance.DispatchCompleted.Increment();
            }
        }

        // TODO: Look into proper yield
        PlatformThreadMisc::Sleep(0);
    }
}

void CAsyncTaskManager::WaitForAll(bool bUseThisThreadWhileWaiting)
{
    WaitFor(DispatchAdded.Load(), bUseThisThreadWhileWaiting);
}

void CAsyncTaskManager::Release()
{
    KillWorkers();

    for (TSharedRef<CGenericThread> Thread : WorkerThreads)
    {
        Thread->WaitForCompletion(kWaitForThreadInfinity);
    }

    WorkerThreads.Clear();
}

CAsyncTaskManager& CAsyncTaskManager::Get()
{
    return Instance;
}
