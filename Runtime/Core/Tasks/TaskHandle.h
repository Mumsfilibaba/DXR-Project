#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskEvent.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Time/Timespan.h"

class CORE_API FTaskHandle
{
public:
    FTaskHandle() = default;

    explicit FTaskHandle(const TSharedPtr<FTaskEvent>& InEvent)
        : Event(InEvent)
    {
    }

    /** @return Returns true if the handle references a task. */
    FORCEINLINE bool IsValid() const
    {
        return Event.IsValid();
    }

    /** @return Returns true if there is no task or the task has finished executing. */
    bool IsComplete() const;

    /**
     * @brief Blocks the calling thread until the referenced task is complete.
     * @param Timeout Maximum time to wait.
     */
    void Wait(FTimespan Timeout = FTimespan::Infinity()) const;

    /** @return Returns the raw completion event (may be nullptr). */
    FORCEINLINE FTaskEvent* GetEvent() const
    {
        return Event.Get();
    }

    /** @return Returns the shared completion-event pointer. */
    FORCEINLINE const TSharedPtr<FTaskEvent>& GetEventPtr() const
    {
        return Event;
    }

private:
    TSharedPtr<FTaskEvent> Event;
};
