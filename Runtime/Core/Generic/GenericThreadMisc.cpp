#include "GenericThreadMisc.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericEvent* FGenericThreadMisc::CreateEvent(bool bManualReset)
{
    return new FGenericEvent();
}

FGenericThread* FGenericThreadMisc::CreateThread(FThreadInterface* Runnable, bool bSuspended)
{
    return new FGenericThread(Runnable);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
