#include "Core/Tasks/TaskHandle.h"
#include "Core/Tasks/TaskGraph.h"

bool FTaskHandle::IsComplete() const
{
    return !Event.IsValid() || Event->IsComplete();
}

void FTaskHandle::Wait(FTimespan Timeout) const
{
    if (Event.IsValid())
    {
        FTaskGraph::Get().WaitForEvent(Event.Get(), Timeout);
    }
}
