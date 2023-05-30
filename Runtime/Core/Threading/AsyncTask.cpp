#include "AsyncTask.h"
#include "AsyncThreadPool.h"

FAsyncTaskBase::FAsyncTaskBase()
    : TaskCompleteEvent(nullptr)
    , NumInvokations(0)
{
}

FAsyncTaskBase::~FAsyncTaskBase()
{
    DestroyWaitEvent();
}

void FAsyncTaskBase::DoAsyncWork()
{
    DoWork();
    FinishAsyncTask();
}

void FAsyncTaskBase::Abandon()
{
    if (TryAbandon())
    {
        CHECK(NumInvokations.Load() == 1);
        NumInvokations--;
    }
    else
    {
        DoWork();
    }

    FinishAsyncTask();
}

bool FAsyncTaskBase::Launch(EQueuePriority Priority, bool bAsync)
{
    FPlatformMisc::MemoryBarrier();

    CHECK(NumInvokations.Load() == 0);
    NumInvokations++;

    if (bAsync)
    {
        if (!TaskCompleteEvent)
        {
            TaskCompleteEvent = FPlatformThreadMisc::CreateEvent(true);
            if (!TaskCompleteEvent)
            {
                return false;
            }
        }

        CHECK(NumInvokations.Load() == 1);
        
        // Reset the event
        TaskCompleteEvent->Reset();

        // Submit this task
        FAsyncThreadPool::Get().SubmitTask(this, Priority);
    }
    else
    {
        DestroyWaitEvent();
        DoWork();
    }

    return true;
}

bool FAsyncTaskBase::Cancel()
{
    if (FAsyncThreadPool::Get().AbandonTask(this))
    {
        CHECK(NumInvokations.Load() == 1);
        NumInvokations--;
        FinishAsyncTask();
        return true;
    }

    return false;
}
