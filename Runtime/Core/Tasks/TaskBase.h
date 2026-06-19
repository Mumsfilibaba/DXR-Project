#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskTypes.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Threading/Atomic/AtomicInt.h"

class FTaskEvent;

class CORE_API FGraphTask
{
    friend class FTaskGraph; // mutates RemainingPrerequisites during launch/prerequisite resolution

public:
    FGraphTask(const CHAR* InDebugName, TFunction<void()>&& InBody, ENamedThread::Type InLane, ETaskPriority::Type InPriority);
    ~FGraphTask();

    /** @brief Runs the task body, triggers the completion event and destroys the task. */
    void Execute();

    /** @return Returns the shared completion event for this task. */
    FORCEINLINE const TSharedPtr<FTaskEvent>& GetCompletionEvent() const
    {
        return CompletionEvent;
    }

    FORCEINLINE ENamedThread::Type GetLane() const
    {
        return Lane;
    }

    FORCEINLINE ETaskPriority::Type GetPriority() const
    {
        return Priority;
    }

private:
    AtomicInt32            RemainingPrerequisites;
    const CHAR*            DebugName;
    TFunction<void()>      Body;
    ENamedThread::Type     Lane;
    ETaskPriority::Type    Priority;
    TSharedPtr<FTaskEvent> CompletionEvent;
};
