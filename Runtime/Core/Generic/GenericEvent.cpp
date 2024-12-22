#include "Core/Generic/GenericEvent.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericEvent* FGenericEvent::Create(bool bManualReset)
{
    return new FGenericEvent();
}

void FGenericEvent::Recycle(FGenericEvent* InEvent)
{
    if (InEvent)
    {
        delete InEvent;
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING