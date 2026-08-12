#include "Core/PlatformInterface/IPlatformEvent.h"
#include "Core/PlatformInterface/PlatformEventPool.h"

IPlatformEvent* IPlatformEvent::Create(bool bManualReset)
{
    return FPlatformEventPool::Get().Acquire(bManualReset);
}

void IPlatformEvent::Recycle(IPlatformEvent* InEvent)
{
    FPlatformEventPool::Get().Release(InEvent);
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

IPlatformEvent* IPlatformEvent::CreateUnpooled(bool bManualReset)
{
    return nullptr;
}

void IPlatformEvent::DestroyUnpooled(IPlatformEvent* InEvent)
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
