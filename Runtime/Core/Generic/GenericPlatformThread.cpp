#include "Core/Generic/GenericPlatformThread.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Misc/OutputDeviceLogger.h"

uint32 FGenericPlatformThread::TLSSlot = FGenericPlatformThread::AllocTLSSlot();

DISABLE_UNREFERENCED_VARIABLE_WARNING

FGenericPlatformThread* FGenericPlatformThread::Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended)
{
    return new FGenericPlatformThread(Runnable, ThreadName);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FGenericPlatformThread* FGenericPlatformThread::GetThread()
{
    void* LocalThread = FPlatformTLS::GetTLSValue(TLSSlot);
    return reinterpret_cast<FGenericPlatformThread*>(LocalThread);
}

uint32 FGenericPlatformThread::AllocTLSSlot()
{
    uint32 SlotIndex = FPlatformTLS::AllocTLSSlot();
    if (SlotIndex == CORE_INVALID_TLS_INDEX)
    {
        LOG_ERROR("Failed to allocate TLS slot");
        DEBUG_BREAK();
    }

    return SlotIndex;
}

FGenericPlatformThread::FGenericPlatformThread(FRunnable* InRunnable, const CHAR* InThreadName)
    : Runnable(InRunnable)
    , Name(InThreadName)
{
    FThreadManager::Get().RegisterThread(this);
}

FGenericPlatformThread::~FGenericPlatformThread()
{
    FThreadManager::Get().UnregisterThread(this);
}
