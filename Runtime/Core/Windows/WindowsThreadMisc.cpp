#include "WindowsThread.h"
#include "WindowsEvent.h"
#include "WindowsThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThreadMisc

FGenericThread* FWindowsThreadMisc::CreateThread(const FThreadFunction& InFunction)
{
    return dbg_new FWindowsThread(InFunction);
}

FGenericThread* FWindowsThreadMisc::CreateNamedThread(const FThreadFunction& InFunction, const FString& InName)
{
    return dbg_new FWindowsThread(InFunction, InName);
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