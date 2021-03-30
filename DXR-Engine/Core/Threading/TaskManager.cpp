#include "TaskManager.h"

#include "Platform/PlatformProcess.h"

#include "TScopedLock.h"

TaskManager TaskManager::Instance;

TaskManager::TaskManager()
    : TaskMutex()
    , IsRunning(false)
{
}

TaskManager::~TaskManager()
{
    // Empty for now
}

bool TaskManager::PopTask(Task& OutTask)
{
    TScopedLock<Mutex> Lock(TaskMutex);
    if (!Tasks.IsEmpty())
    {
        OutTask = Tasks.Front();
        Tasks.Erase(Tasks.Begin());
        return true;
    }
    else
    {
        return false;
    }
}

void TaskManager::WorkThread()
{
    LOG_INFO("Starting Workthread: " + std::to_string(PlatformProcess::GetThreadID()));

    while (Instance.IsRunning)
    {
        Task CurrentTask;
        if (Instance.PopTask(CurrentTask))
        {
            CurrentTask.Delegate();
        }
        else
        {
            Instance.WakeCondition.Wait(Instance.WakeMutex);
        }
    }

    LOG_INFO("End Workthread: " + std::to_string(PlatformProcess::GetThreadID()));
}

bool TaskManager::Init()
{
    // NOTE: Maybe change to NumProcessors - 1 -> Test performance
    uint32 ThreadCount = Math::Max<uint32>(PlatformProcess::GetNumProcessors(), 1);
    WorkThreads.Resize(ThreadCount);

    for (uint32 i = 0; i < ThreadCount; i++)
    {
        TRef<GenericThread> NewThread = GenericThread::Create(TaskManager::WorkThread);
        if (NewThread)
        {
            WorkThreads[i] = NewThread;
            NewThread->SetName("WorkerThread " + std::to_string(i));
        }
        else
        {
            return false;
        }
    }

    IsRunning = true;

    return true;
}

void TaskManager::AddTask(const Task& NewTask)
{
    TScopedLock<Mutex> Lock(TaskMutex);
    Tasks.EmplaceBack(NewTask);

    WakeCondition.NotifyOne();
}

void TaskManager::Release()
{
    IsRunning = false;

    WakeCondition.NotifyAll();

    for (TRef<GenericThread> Thread : WorkThreads)
    {
        Thread->Wait();
    }

    WorkThreads.Clear();
}

TaskManager& TaskManager::Get()
{
    return Instance;
}
