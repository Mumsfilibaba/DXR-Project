#pragma once
#include "InterlockedInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 TaskID;

struct Task
{
    TDelegate<void()> Delegate;
};

class TaskManager
{
public:
    bool Init();

    TaskID AddTask( const Task& NewTask );

    void WaitForTask( TaskID Task );
    void WaitForAllTasks();

    void Release();

    static TaskManager& Get();

private:
    TaskManager();
    ~TaskManager();

    bool PopTask( Task& OutTask );

    void KillWorkers();

    static void WorkThread();

private:
    TArray<TSharedRef<CGenericThread>> WorkThreads;

    TArray<Task> Tasks;
    CCriticalSection TaskMutex;

    ConditionVariable WakeCondition;
    CCriticalSection WakeMutex;

    InterlockedInt32 TaskAdded;
    InterlockedInt32 TaskCompleted;

    volatile bool IsRunning;

    static TaskManager Instance;
};