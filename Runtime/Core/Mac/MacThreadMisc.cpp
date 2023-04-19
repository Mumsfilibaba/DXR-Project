#include "MacThread.h"
#include "MacEvent.h"
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>

FGenericEvent* FMacThreadMisc::CreateEvent(bool bManualReset)
{
    FMacEventRef NewEvent = new FMacEvent();
    if (!NewEvent->Create(bManualReset))
    {
        return nullptr;
    }

    return NewEvent.ReleaseOwnership();
}

FGenericThread* FMacThreadMisc::CreateThread(FThreadInterface* InRunnable, bool bSuspended)
{
    FMacThread* NewThread = new FMacThread(InRunnable);
    if (!bSuspended)
    {
        NewThread->Start();
    }

    return NewThread;
}
