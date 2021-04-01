#pragma once
#include "ThreadSafeInt.h"

#include "Platform/Mutex.h"
#include "Platform/ConditionVariable.h"

#include "Generic/GenericThread.h"

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

    TaskID AddTask(const Task& NewTask);

    void WaitForTask(TaskID Task);
    void WaitForAllTasks();

    void Release();

    static TaskManager& Get();

private:
    TaskManager();
    ~TaskManager();

    bool PopTask(Task& OutTask);

    void KillWorkers();

    static void WorkThread();

private:
    TArray<TRef<GenericThread>> WorkThreads;

    TArray<Task> Tasks;
    Mutex TaskMutex;

    ConditionVariable WakeCondition;
    Mutex WakeMutex;

    ThreadSafeInt32 TaskAdded;
    ThreadSafeInt32 TaskCompleted;

    volatile bool IsRunning;

    static TaskManager Instance;
};