#include "ScopedLock.h"
#include "DispatchQueue.h"

#include "Platform/PlatformThreadMisc.h"

CDispatchQueue CDispatchQueue::Instance;

CDispatchQueue::CDispatchQueue()
    : QueueMutex()
    , bIsRunning(false)
{
}

CDispatchQueue::~CDispatchQueue()
{
    KillWorkers();
}

bool CDispatchQueue::PopDispatch(SDispatch& OutTask)
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

void CDispatchQueue::KillWorkers()
{
    bIsRunning = false;

    WakeCondition.NotifyAll();
}

void CDispatchQueue::WorkThread()
{
    LOG_INFO("Starting Work thread: " + ToString(PlatformThreadMisc::GetThreadHandle()));

    while (Instance.bIsRunning)
    {
        SDispatch CurrentTask;

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

bool CDispatchQueue::Init()
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
        CString ThreadName;
        ThreadName.Format("WorkerThread[%d]", i);

        TSharedRef<CPlatformThread> NewThread = PlatformThread::Make(CDispatchQueue::WorkThread, ThreadName);
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

DispatchID CDispatchQueue::Dispatch(const SDispatch& NewTask)
{
    if (WorkerThreads.IsEmpty())
    {
        // Execute task on main-thread
        SDispatch MainThreadTask = NewTask;
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

void CDispatchQueue::WaitFor(DispatchID Task)
{
    while (DispatchCompleted.Load() < Task)
    {
        // Look into proper yield
        PlatformThreadMisc::Sleep(0);
    }
}

void CDispatchQueue::WaitForAll()
{
    while (DispatchCompleted.Load() < DispatchAdded.Load())
    {
        // Look into proper yield
        PlatformThreadMisc::Sleep(0);
    }
}

void CDispatchQueue::Release()
{
    KillWorkers();

    for (TSharedRef<CPlatformThread> Thread : WorkerThreads)
    {
        Thread->WaitUntilFinished();
    }

    WorkerThreads.Clear();
}

CDispatchQueue& CDispatchQueue::Get()
{
    return Instance;
}
