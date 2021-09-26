#include "TaskManager.h"

#include "Platform/PlatformThreadMisc.h"

#include "ScopedLock.h"

#include <string>

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
    LOG_INFO( "Starting Work thread: " + std::to_string( PlatformThreadMisc::GetThreadHandle() ) );

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

    LOG_INFO( "End Work thread: " + std::to_string( PlatformThreadMisc::GetThreadHandle() ) );
}

bool TaskManager::Init()
{
    uint32 ThreadCount = NMath::Max<int32>( PlatformThreadMisc::GetNumProcessors() - 1, 1 );
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
		CString ThreadName;
		ThreadName.Format( "WorkerThread[%d]", i );
		
        TSharedRef<CGenericThread> NewThread = PlatformThread::Make( TaskManager::WorkThread, ThreadName );
        if ( NewThread )
        {
            WorkThreads[i] = NewThread;
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

    TaskID NewTaskID = TaskAdded.Increment();
    WakeCondition.NotifyOne();
    return NewTaskID;
}

void TaskManager::WaitForTask( TaskID Task )
{
    while ( TaskCompleted.Load() < Task )
    {
        // Look into proper yeild
        PlatformThreadMisc::Sleep( 0 );
    }
}

void TaskManager::WaitForAllTasks()
{
    while ( TaskCompleted.Load() < TaskAdded.Load() )
    {
        // Look into proper yeild
        PlatformThreadMisc::Sleep( 0 );
    }
}

void TaskManager::Release()
{
    KillWorkers();

    for ( TSharedRef<CGenericThread> Thread : WorkThreads )
    {
        Thread->WaitUntilFinished();
    }

    WorkThreads.Clear();
}

TaskManager& TaskManager::Get()
{
    return Instance;
}
