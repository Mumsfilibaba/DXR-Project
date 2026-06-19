#include "Core/Tasks/TaskBase.h"
#include "Core/Tasks/TaskEvent.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Platform/PlatformThreadMisc.h"

FGraphTask::FGraphTask(const CHAR* InDebugName, TFunction<void()>&& InBody, ENamedThread::Type InLane, ETaskPriority::Type InPriority)
    : RemainingPrerequisites(0)
    , DebugName(InDebugName)
    , Body(::Move(InBody))
    , Lane(InLane)
    , Priority(InPriority)
    , CompletionEvent(MakeSharedPtr<FTaskEvent>())
{
}

FGraphTask::~FGraphTask() = default;

void FGraphTask::Execute()
{
    // Keep the completion event alive locally. Triggering it may release subsequents, 
    // and the member shared-pointer is destroyed when 'delete this' runs below.
    TSharedPtr<FTaskEvent> LocalEvent = CompletionEvent;
    if (Body)
    {
        TRACE_SCOPE(DebugName);
        Body();
    }

    LocalEvent->Trigger();
    delete this;
}
