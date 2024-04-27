#include "GenericThread.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Misc/OutputDeviceLogger.h"

uint32 FGenericThread::TLSSlot = FGenericThread::AllocTLSSlot();

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericThread* FGenericThread::Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended)
{
    return new FGenericThread(Runnable, ThreadName);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FGenericThread* FGenericThread::GetThread()
{
    void* LocalThread = FPlatformTLS::GetTLSValue(TLSSlot);
    return reinterpret_cast<FGenericThread*>(LocalThread);
}

uint32 FGenericThread::AllocTLSSlot()
{
    uint32 SlotIndex = FPlatformTLS::AllocTLSSlot();
    if (SlotIndex == CORE_INVALID_TLS_INDEX)
    {
        LOG_ERROR("Failed to allocate TLS slot");
        DEBUG_BREAK();
    }

    return SlotIndex;
}

FGenericThread::FGenericThread(FRunnable* InRunnable, const CHAR* InThreadName)
    : Runnable(InRunnable)
    , ThreadName(InThreadName)
{
    // Register this thread in the ThreadManager
    FThreadManager::Get().RegisterThread(this);
}

FGenericThread::~FGenericThread()
{
    // Unregister this thread in the ThreadManager
    FThreadManager::Get().UnregisterThread(this);
}
