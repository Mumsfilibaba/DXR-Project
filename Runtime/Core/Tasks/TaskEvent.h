#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Time/Timespan.h"

class FGenericPlatformEvent;

class CORE_API FTaskEvent
{
public:
    FTaskEvent();
    ~FTaskEvent();

    /** @brief Marks the event complete, releases all subsequents and wakes blocking waiters. */
    void Trigger();
    
    /**
     * @brief Registers a task as waiting on this event.
     * @param Subsequent Task to schedule once this event completes.
     * @return Returns true if the subsequent was registered, false if the event was already complete.
     */
    bool AddSubsequent(FGraphTask* Subsequent);
    
    /**
     * @brief Blocks the calling thread until the event is complete (or the timeout elapses).
     * @param Timeout Maximum time to wait.
     * @return Returns true if the event is complete on return.
     */
    bool WaitUntilComplete(FTimespan Timeout);

    /** @return Returns true if the owning task has finished executing. */
    FORCEINLINE bool IsComplete() const
    {
        return bComplete.Load();
    }

private:
    AtomicBool             bComplete;
    FCriticalSection       SubsequentsCS;
    TArray<FGraphTask*>    Subsequents;
    FGenericPlatformEvent* CompletionEvent;
};
