#include "MacThread.h"
#include "MacEvent.h"
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>


FGenericThread* FMacThreadMisc::CreateThread(FThreadInterface* InRunnable)
{
    return new FMacThread(InRunnable);
}

FGenericEvent* FMacThreadMisc::CreateEvent(bool bManualReset)
{
    FMacEventRef NewEvent = new FMacEvent();
    if (!NewEvent->Create(bManualReset))
    {
        return nullptr;
    }

    return NewEvent.ReleaseOwnership();
}
