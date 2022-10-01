#include "AsyncTask.h"
#include "AsyncThreadPool.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTaskBase

FAsyncTaskBase::FAsyncTaskBase()
    : TaskCompleteEvent(nullptr)
    , NumInvokations(0)
{ }

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
        Check(NumInvokations.Load() == 1);
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
        Check(NumInvokations.Load() == 1);
        NumInvokations--;
        FinishAsyncTask();
        return true;
    }

    return false;
}