#include "WindowsThread.h"
#include "WindowsEvent.h"
#include "WindowsThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThreadMisc

FGenericThread* FWindowsThreadMisc::CreateThread(FThreadInterface* Runnable)
{
    return dbg_new FWindowsThread(Runnable);
}

FGenericEvent* FWindowsThreadMisc::CreateEvent(bool bManualReset)
{
    FWindowsEventRef NewEvent = dbg_new FWindowsEvent();
    if (!NewEvent->Create(bManualReset))
    {
        return nullptr;
    }
    
    return NewEvent.ReleaseOwnership();
}