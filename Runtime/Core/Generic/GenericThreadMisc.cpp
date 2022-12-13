#include "GenericThreadMisc.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING


FGenericThread* FGenericThreadMisc::CreateThread(FThreadInterface* Runnable)
{
    return new FGenericThread(Runnable);
}

FGenericEvent* FGenericThreadMisc::CreateEvent(bool bManualReset)
{
    return new FGenericEvent();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
