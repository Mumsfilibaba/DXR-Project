#pragma once
#include "InterlockedInt.h"

#include "Platform/CriticalSection.h"
#include "Platform/ConditionVariable.h"
#include "Platform/PlatformThread.h"

#include "Core/Delegates/Delegate.h"

typedef int64 TaskID;

struct SExecutableTask
{
    DECLARE_DELEGATE( CTaskDelegate );
    CTaskDelegate Delegate;
};

class CTaskManager
{
public:
    bool Init();

    TaskID AddTask( const SExecutableTask& NewTask );

    void WaitForTask( TaskID Task );
    void WaitForAllTasks();

    void Release();

    static CTaskManager& Get();

private:
    CTaskManager();
    ~CTaskManager();

    bool PopTask( SExecutableTask& OutTask );

    void KillWorkers();

    static void WorkThread();

private:
    TArray<TSharedRef<CCoreThread>> WorkThreads;

    TArray<SExecutableTask> Tasks;
    CCriticalSection TaskMutex;

    CConditionVariable WakeCondition;
    CCriticalSection WakeMutex;

    InterlockedInt32 TaskAdded;
    InterlockedInt32 TaskCompleted;

    volatile bool IsRunning;

    static CTaskManager Instance;
};