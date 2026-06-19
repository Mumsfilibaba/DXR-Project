#include "Core/Tasks/TaskEvent.h"
#include "Core/Tasks/TaskBase.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/TaskGraphStats.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/PlatformEvent.h"

FTaskEvent::FTaskEvent()
    : bComplete(false)
    , SubsequentsCS()
    , Subsequents()
    , CompletionEvent(nullptr)
{
}

FTaskEvent::~FTaskEvent()
{
    if (CompletionEvent)
    {
        FPlatformEvent::Recycle(CompletionEvent);
        CompletionEvent = nullptr;
    }
}

void FTaskEvent::Trigger()
{
    TArray<FGraphTask*>    LocalSubsequents;
    FGenericPlatformEvent* LocalEvent = nullptr;

    {
        SCOPED_LOCK(SubsequentsCS);

        bComplete.Store(true);
        
        LocalSubsequents = ::Move(Subsequents);

        Subsequents.Clear();
        LocalEvent = CompletionEvent;
    }

    STAT_ADD(STAT_TaskGraph_TasksCompleted, 1);

    // Release every task that was waiting on this one. Each may become ready to run.
    for (FGraphTask* Subsequent : LocalSubsequents)
    {
        FTaskGraph::Get().OnPrerequisiteComplete(Subsequent);
    }

    if (LocalEvent)
    {
        LocalEvent->Trigger();
    }
}

bool FTaskEvent::AddSubsequent(FGraphTask* Subsequent)
{
    SCOPED_LOCK(SubsequentsCS);

    if (bComplete.Load())
    {
        return false;
    }

    Subsequents.Add(Subsequent);
    return true;
}

bool FTaskEvent::WaitUntilComplete(FTimespan Timeout)
{
    if (IsComplete())
    {
        return true;
    }

    FGenericPlatformEvent* LocalEvent = nullptr;
    
    {
        SCOPED_LOCK(SubsequentsCS);

        // Re-check under the lock to close the race with a concurrent Trigger.
        if (bComplete.Load())
        {
            return true;
        }

        if (!CompletionEvent)
        {
            // Manual-reset so that the signal is observed by every waiter and by repeated checks.
            CompletionEvent = FPlatformEvent::Create(true);
        }

        LocalEvent = CompletionEvent;
    }

    if (LocalEvent)
    {
        LocalEvent->Wait(Timeout);
    }

    return IsComplete();
}
