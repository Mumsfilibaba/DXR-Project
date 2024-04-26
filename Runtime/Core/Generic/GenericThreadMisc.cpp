#include "GenericThreadMisc.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericEvent* FGenericThreadMisc::CreateEvent(bool bManualReset)
{
    return new FGenericEvent();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
