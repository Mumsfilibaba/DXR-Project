#include "TaskManager.h"

#include "Platform/PlatformProcess.h"

#include "ScopedLock.h"

#include <condition_variable>

TaskManager TaskManager::Instance;

TaskManager::TaskManager()
    : TaskMutex()
    , IsRunning( false )
{
}

TaskManager::~TaskManager()
{
    KillWorkers();
}

bool TaskManager::PopTask( Task& OutTask )
{
    TScopedLock<CCriticalSection> Lock( TaskMutex );

    if ( !Tasks.IsEmpty() )
    {
        OutTask = Tasks.FirstElement();
        Tasks.RemoveAt( 0 );

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
    LOG_INFO( "Starting Work thread: " + std::to_string( PlatformProcess::GetThreadID() ) );

    while ( Instance.IsRunning )
    {
        Task CurrentTask;

        if ( !Instance.PopTask( CurrentTask ) )
        {
            TScopedLock<CCriticalSection> Lock( Instance.WakeMutex );
            Instance.WakeCondition.Wait( Lock );
        }
        else
        {
            CurrentTask.Delegate.ExecuteIfBound();
            Instance.TaskCompleted++;
        }
    }

    LOG_INFO( "End Work thread: " + std::to_string( PlatformProcess::GetThreadID() ) );
}

bool TaskManager::Init()
{
    uint32 ThreadCount = NMath::Max<int32>( PlatformProcess::GetNumProcessors() - 1, 1 );
    WorkThreads.Resize( ThreadCount );

    if ( ThreadCount == 1 )
    {
        LOG_INFO( "[TaskManager]: No workers available, tasks will be executing on the main thread" );
        WorkThreads.Clear();
        return true;
    }

    LOG_INFO( "[TaskManager]: Starting '" + std::to_string( ThreadCount ) + "' Workers" );

    // Start so that workers now that they should be running
    IsRunning = true;

    for ( uint32 i = 0; i < ThreadCount; i++ )
    {
        TSharedRef<CGenericThread> NewThread = PlatformThread::Make( TaskManager::WorkThread );
        if ( NewThread )
        {
            WorkThreads[i] = NewThread;
            NewThread->SetName( "WorkerThread " + std::to_string( i ) );
        }
        else
        {
            KillWorkers();
            return false;
        }
    }

    return true;
}

TaskID TaskManager::AddTask( const Task& NewTask )
{
    if ( WorkThreads.IsEmpty() )
    {
        // Execute task on mainthread
        Task MainThreadTask = NewTask;
        MainThreadTask.Delegate.ExecuteIfBound();

        // Make sure that both fences is incremented
        Instance.TaskCompleted++;
        return TaskAdded.Increment();
    }

    {
        TScopedLock<CCriticalSection> Lock( TaskMutex );
        Tasks.Emplace( NewTask );
    }

    ThreadID NewTaskID = TaskAdded.Increment();
    WakeCondition.NotifyOne();
    return NewTaskID;
}

void TaskManager::WaitForTask( TaskID Task )
{
    while ( TaskCompleted.Load() < Task )
    {
        // Look into proper yeild
        PlatformProcess::Sleep( 0 );
    }
}

void TaskManager::WaitForAllTasks()
{
    while ( TaskCompleted.Load() < TaskAdded.Load() )
    {
        // Look into proper yeild
        PlatformProcess::Sleep( 0 );
    }
}

void TaskManager::Release()
{
    KillWorkers();

    for ( TSharedRef<CGenericThread> Thread : WorkThreads )
    {
        Thread->Wait();
    }

    WorkThreads.Clear();
}

TaskManager& TaskManager::Get()
{
    return Instance;
}
