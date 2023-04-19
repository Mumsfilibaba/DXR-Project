#include "WindowsThread.h"
#include "WindowsEvent.h"
#include "WindowsThreadMisc.h"

FGenericEvent* FWindowsThreadMisc::CreateEvent(bool bManualReset)
{
    FWindowsEventRef NewEvent = new FWindowsEvent();
    if (!NewEvent->Create(bManualReset))
    {
        return nullptr;
    }
    
    return NewEvent.ReleaseOwnership();
}

FGenericThread* FWindowsThreadMisc::CreateThread(FThreadInterface* Runnable, bool bSuspended)
{
    FWindowsThread* NewThread = new FWindowsThread(Runnable, bSuspended);
    return NewThread;
}