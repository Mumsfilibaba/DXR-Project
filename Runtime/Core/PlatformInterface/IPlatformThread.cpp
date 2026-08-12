#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Misc/OutputDeviceLogger.h"

uint32 IPlatformThread::TLSSlot = IPlatformThread::AllocTLSSlot();

IPlatformThread* IPlatformThread::GetThread()
{
    void* LocalThread = FPlatformTLS::GetTLSValue(TLSSlot);
    return reinterpret_cast<IPlatformThread*>(LocalThread);
}

uint32 IPlatformThread::AllocTLSSlot()
{
    uint32 SlotIndex = FPlatformTLS::AllocTLSSlot();
    if (SlotIndex == CORE_INVALID_TLS_INDEX)
    {
        LOG_ERROR("Failed to allocate TLS slot");
        DEBUG_BREAK();
    }

    return SlotIndex;
}
