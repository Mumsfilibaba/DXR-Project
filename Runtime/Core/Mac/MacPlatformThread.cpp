#include "Core/Mac/MacPlatformThread.h"
#include "Core/Mac/MacPlatformThreadMisc.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformTLS.h"

FGenericPlatformThread* FMacPlatformThread::Create(FRunnable* InRunnable, const CHAR* ThreadName, bool bSuspended)
{
    FMacPlatformThread* NewThread = new FMacPlatformThread(InRunnable, ThreadName);
    if (!bSuspended && !NewThread->Start())
    {
        delete NewThread;
        return nullptr;
    }

    return NewThread;
}

FMacPlatformThread::FMacPlatformThread(FRunnable* InRunnable, const CHAR* ThreadName)
    : FGenericPlatformThread(InRunnable, ThreadName)
    , Thread()
    , bIsJoinable(false)
{ 
}

FMacPlatformThread::~FMacPlatformThread()
{
    if (bIsJoinable)
    {
        ::pthread_detach(Thread);
        bIsJoinable = false;
    }
}

bool FMacPlatformThread::Start()
{
    CHECK(!bIsJoinable);

    pthread_attr_t Attributes;
    ::pthread_attr_init(&Attributes);
    ::pthread_attr_set_qos_class_np(&Attributes, QOS_CLASS_USER_INITIATED, 0);

    const int32 Result = ::pthread_create(&Thread, &Attributes, FMacPlatformThread::ThreadRoutine, reinterpret_cast<void*>(this));
    ::pthread_attr_destroy(&Attributes);

    if (Result)
    {
        LOG_ERROR("[FMacPlatformThread] Failed to create thread");
        return false;
    }

    bIsJoinable = true;
    return true;
}

void FMacPlatformThread::Kill(bool bWaitUntilCompletion)
{
    if (Runnable)
    {
        Runnable->Stop();
    }

    if (bWaitUntilCompletion)
    {
        WaitForCompletion();
    }
    else if (bIsJoinable)
    {
        ::pthread_detach(Thread);
        bIsJoinable = false;
    }
}

void FMacPlatformThread::WaitForCompletion()
{
    // Cleared before the join so a racing second call cannot join a handle the first one freed
    if (bIsJoinable)
    {
        bIsJoinable = false;
        ::pthread_join(Thread, nullptr);
    }
}

void* FMacPlatformThread::GetPlatformHandle()
{
    return reinterpret_cast<void*>(Thread);
}

void* FMacPlatformThread::ThreadRoutine(void* ThreadParameter)
{
    int32 Result = int32(-1);

    FMacPlatformThread* CurrentThread = reinterpret_cast<FMacPlatformThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Ensure that this thread can be retrieved
        FPlatformTLS::SetTLSValue(FGenericPlatformThread::TLSSlot, CurrentThread);

        // Thread-name can only be set from the running thread
        if (!CurrentThread->Name.IsEmpty())
        {
            const CHAR* ThreadName = *CurrentThread->Name;
            ::pthread_setname_np(ThreadName);
        }

        if (FRunnable* Runnable = CurrentThread->GetRunnable())
        {
            if (Runnable->Start())
            {
                Result = Runnable->Run();
            }

            Runnable->Destroy();
        }

        FPlatformTLS::SetTLSValue(FGenericPlatformThread::TLSSlot, nullptr);
    }

    ::pthread_exit(nullptr);
    return reinterpret_cast<void*>(Result);
}
