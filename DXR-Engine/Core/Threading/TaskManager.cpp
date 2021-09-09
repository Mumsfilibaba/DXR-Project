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
    // Empty for now
}

bool TaskManager::PopTask( Task& OutTask )
{
    TScopedLock<Mutex> Lock( TaskMutex );

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
            TScopedLock<Mutex> Lock( Instance.WakeMutex );
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

    LOG_INFO( "[TaskManager]: Starting '" + std::to_string( ThreadCount ) + "' Workers" );

    // Start so that workers now that they should be running
    IsRunning = true;

    for ( uint32 i = 0; i < ThreadCount; i++ )
    {
        TSharedRef<GenericThread> NewThread = GenericThread::Create( TaskManager::WorkThread );
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
    {
        TScopedLock<Mutex> Lock( TaskMutex );
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

    for ( TSharedRef<GenericThread> Thread : WorkThreads )
    {
        Thread->Wait();
    }

    WorkThreads.Clear();
}

TaskManager& TaskManager::Get()
{
    return Instance;
}
