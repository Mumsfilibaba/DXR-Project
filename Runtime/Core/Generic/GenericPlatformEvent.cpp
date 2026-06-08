#include "Core/Generic/GenericPlatformEvent.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericPlatformEvent* FGenericPlatformEvent::Create(bool bManualReset)
{
    return new FGenericPlatformEvent();
}

void FGenericPlatformEvent::Recycle(FGenericPlatformEvent* InEvent)
{
    if (InEvent)
    {
        delete InEvent;
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING