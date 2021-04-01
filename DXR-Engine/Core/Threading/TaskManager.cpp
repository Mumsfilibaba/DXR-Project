#include "TaskManager.h"

#include "Platform/PlatformProcess.h"

#include "ScopedLock.h"

#include <condition_variable>

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

void TaskManager::KillWorkers()
{
    IsRunning = false;

    WakeCondition.NotifyAll();
}

void TaskManager::WorkThread()
{
    LOG_INFO("Starting Workthread: " + std::to_string(PlatformProcess::GetThreadID()));

    while (Instance.IsRunning)
    {
        Task CurrentTask;

        if (!Instance.PopTask(CurrentTask))
        {
            TScopedLock<Mutex> Lock(Instance.WakeMutex);
            Instance.WakeCondition.Wait(Lock);
        }
        else
        {
            CurrentTask.Delegate();
            Instance.TaskCompleted++;
        }
    }

    LOG_INFO("End Workthread: " + std::to_string(PlatformProcess::GetThreadID()));
}

bool TaskManager::Init()
{
    // NOTE: Maybe change to NumProcessors - 1 -> Test performance
    uint32 ThreadCount = Math::Max<int32>(PlatformProcess::GetNumProcessors() - 1, 1);
    WorkThreads.Resize(ThreadCount);

    LOG_INFO("[TaskManager]: Starting '" + std::to_string(ThreadCount) + "' Workers");

    // Start so that workers now that they should be running
    IsRunning = true;
    
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
            KillWorkers();
            return false;
        }
    }

    return true;
}

TaskID TaskManager::AddTask(const Task& NewTask)
{
    {
        TScopedLock<Mutex> Lock(TaskMutex);
        Tasks.EmplaceBack(NewTask);
    }

    ThreadID NewTaskID = TaskAdded.Increment();

    WakeCondition.NotifyOne();

    return NewTaskID;
}

void TaskManager::WaitForTask(TaskID Task)
{
    while (TaskCompleted.Load() < Task)
    {
        // Look into proper yeild
        PlatformProcess::Sleep(0);
    }
}

void TaskManager::WaitForAllTasks()
{
    while (TaskCompleted.Load() < TaskAdded.Load())
    {
        // Look into proper yeild
        PlatformProcess::Sleep(0);
    }
}

void TaskManager::Release()
{
    KillWorkers();

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
