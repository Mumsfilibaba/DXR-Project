#include "TaskManager.h"

#include "Platform/PlatformThreadMisc.h"

#include "ScopedLock.h"

#include <string>

CTaskManager CTaskManager::Instance;

CTaskManager::CTaskManager()
    : TaskMutex()
    , IsRunning( false )
{
}

CTaskManager::~CTaskManager()
{
    KillWorkers();
}

bool CTaskManager::PopTask( SExecutableTask& OutTask )
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

void CTaskManager::KillWorkers()
{
    IsRunning = false;

    WakeCondition.NotifyAll();
}

void CTaskManager::WorkThread()
{
    LOG_INFO( "Starting Work thread: " + ToString( PlatformThreadMisc::GetThreadHandle() ) );

    while ( Instance.IsRunning )
    {
        SExecutableTask CurrentTask;

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

    LOG_INFO( "End Work thread: " + ToString( PlatformThreadMisc::GetThreadHandle() ) );
}

bool CTaskManager::Init()
{
    uint32 ThreadCount = NMath::Max<int32>( PlatformThreadMisc::GetNumProcessors() - 1, 1 );
    WorkThreads.Resize( ThreadCount );

    if ( ThreadCount == 1 )
    {
        LOG_INFO( "[CTaskManager]: No workers available, tasks will be executing on the main thread" );
        WorkThreads.Clear();
        return true;
    }

    LOG_INFO( "[CTaskManager]: Starting '" + ToString( ThreadCount ) + "' Workers" );

    // Start so that workers now that they should be running
    IsRunning = true;

    for ( uint32 i = 0; i < ThreadCount; i++ )
    {
        CString ThreadName;
        ThreadName.Format( "WorkerThread[%d]", i );

        TSharedRef<CCoreThread> NewThread = PlatformThread::Make( CTaskManager::WorkThread, ThreadName );
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

TaskID CTaskManager::AddTask( const SExecutableTask& NewTask )
{
    if ( WorkThreads.IsEmpty() )
    {
        // Execute task on mainthread
        SExecutableTask MainThreadTask = NewTask;
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

void CTaskManager::WaitForTask( TaskID Task )
{
    while ( TaskCompleted.Load() < Task )
    {
        // Look into proper yeild
        PlatformThreadMisc::Sleep( 0 );
    }
}

void CTaskManager::WaitForAllTasks()
{
    while ( TaskCompleted.Load() < TaskAdded.Load() )
    {
        // Look into proper yeild
        PlatformThreadMisc::Sleep( 0 );
    }
}

void CTaskManager::Release()
{
    KillWorkers();

    for ( TSharedRef<CCoreThread> Thread : WorkThreads )
    {
        Thread->WaitUntilFinished();
    }

    WorkThreads.Clear();
}

CTaskManager& CTaskManager::Get()
{
    return Instance;
}
