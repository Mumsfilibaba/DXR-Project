#include "Core/Tasks/Tasks.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/TaskWorker.h"
#include "Core/Threading/ThreadManager.h"

FTaskHandle Tasks::LaunchImpl(const CHAR* DebugName, TFunction<void()>&& Body, ENamedThread::Type Thread, ETaskPriority::Type Priority, TArrayView<const FTaskHandle> Prerequisites)
{
    return FTaskGraph::Get().LaunchInternal(DebugName, ::Move(Body), Thread, Priority, Prerequisites);
}

void Tasks::Wait(TArrayView<const FTaskHandle> InTasks, FTimespan Timeout)
{
    for (const FTaskHandle& Handle : InTasks)
    {
        Handle.Wait(Timeout);
    }
}

void Tasks::ProcessMainThreadTasks()
{
    if (FTaskGraph::IsInitialized())
    {
        FTaskGraph::Get().ProcessMainThreadTasks();
    }
}

bool Tasks::IsInMainThread()
{
    return FThreadManager::IsMainThread();
}

bool Tasks::IsInRenderThread()
{
    if (FTaskWorker* Worker = FTaskWorker::GetCurrentWorker())
    {
        if (Worker->GetLane() == ENamedThread::RenderThread)
        {
            return true;
        }
    }

    // With the render thread disabled, render work executes inline on the main thread.
    if (FTaskGraph::IsInitialized() && !FTaskGraph::Get().IsRenderThreadEnabled())
    {
        return FThreadManager::IsMainThread();
    }

    return false;
}

bool Tasks::IsInRHIThread()
{
    if (FTaskWorker* Worker = FTaskWorker::GetCurrentWorker())
    {
        return Worker->GetLane() == ENamedThread::RHIThread;
    }

    return false;
}
