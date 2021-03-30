#pragma once
#include "Platform/Mutex.h"
#include "Platform/ConditionVariable.h"

#include "Generic/GenericThread.h"

#include "Core/Delegates/Delegate.h"

struct Task
{
    TDelegate<void()> Delegate;
};

class TaskManager
{
public:
    bool Init();

    void AddTask(const Task& NewTask);

    void Release();

    static TaskManager& Get();

private:
    TaskManager();
    ~TaskManager();

    bool PopTask(Task& OutTask);

    static void WorkThread();

private:
    TArray<TRef<GenericThread>> WorkThreads;

    TArray<Task> Tasks;
    Mutex TaskMutex;

    Mutex WakeMutex;
    ConditionVariable WakeCondition;

    volatile bool IsRunning;

    static TaskManager Instance;
};