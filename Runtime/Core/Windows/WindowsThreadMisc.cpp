#include "WindowsThread.h"
#include "WindowsEvent.h"
#include "WindowsThreadMisc.h"

FGenericThread* FWindowsThreadMisc::CreateThread(FThreadInterface* Runnable)
{
    return new FWindowsThread(Runnable);
}

FGenericEvent* FWindowsThreadMisc::CreateEvent(bool bManualReset)
{
    FWindowsEventRef NewEvent = new FWindowsEvent();
    if (!NewEvent->Create(bManualReset))
    {
        return nullptr;
    }
    
    return NewEvent.ReleaseOwnership();
}