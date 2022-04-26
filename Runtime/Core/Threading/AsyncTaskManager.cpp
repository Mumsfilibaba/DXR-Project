#include "ScopedLock.h"
#include "AsyncTaskManager.h"

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

bool CAsyncTaskManager::PopDispatch(SAsyncTask& OutTask)
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
    LOG_INFO("Starting Work thread: " + ToString(PlatformThreadMisc::GetThreadHandle()));

    while (Instance.bIsRunning)
    {
        SAsyncTask CurrentTask;

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

    LOG_INFO("End Work thread: " + ToString(PlatformThreadMisc::GetThreadHandle()));
}

bool CAsyncTaskManager::Initialize()
{
    uint32 ThreadCount = NMath::Max<int32>(PlatformThreadMisc::GetNumProcessors() - 1, 1);
    WorkerThreads.Resize(ThreadCount);

    if (ThreadCount == 1)
    {
        LOG_INFO("[CTaskManager]: No workers available, tasks will be executing on the main thread");
        WorkerThreads.Clear();
        return true;
    }

    LOG_INFO("[CTaskManager]: Starting '" + ToString(ThreadCount) + "' Workers");

    // Start so that workers now that they should be running
    bIsRunning = true;

    for (uint32 i = 0; i < ThreadCount; i++)
    {
        String ThreadName;
        ThreadName.Format("WorkerThread[%d]", i);

        TSharedRef<CGenericThread> NewThread = PlatformThread::Make(CAsyncTaskManager::WorkThread, ThreadName);
        if (NewThread)
        {
            WorkerThreads[i] = NewThread;
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

DispatchID CAsyncTaskManager::Dispatch(const SAsyncTask& NewTask)
{
    if (WorkerThreads.IsEmpty())
    {
        // Execute task on main-thread
        SAsyncTask MainThreadTask = NewTask;
        MainThreadTask.Delegate.ExecuteIfBound();

        // Make sure that both fences is incremented
        Instance.DispatchCompleted.Increment();
        return DispatchAdded.Increment();
    }

    {
        TScopedLock<CCriticalSection> Lock(QueueMutex);
        Queue.Emplace(NewTask);
    }

    DispatchID NewTaskID = DispatchAdded.Increment();
    WakeCondition.NotifyOne();
    return NewTaskID;
}

void CAsyncTaskManager::WaitFor(DispatchID Task)
{
    while (DispatchCompleted.Load() < Task)
    {
        // Look into proper yield
        PlatformThreadMisc::Sleep(0);
    }
}

void CAsyncTaskManager::WaitForAll()
{
    while (DispatchCompleted.Load() < DispatchAdded.Load())
    {
        // Look into proper yield
        PlatformThreadMisc::Sleep(0);
    }
}

void CAsyncTaskManager::Release()
{
    KillWorkers();

    for (TSharedRef<CGenericThread> Thread : WorkerThreads)
    {
        Thread->WaitUntilFinished();
    }

    WorkerThreads.Clear();
}

CAsyncTaskManager& CAsyncTaskManager::Get()
{
    return Instance;
}
