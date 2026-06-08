#include "Core/Mac/MacPlatformThread.h"
#include "Core/Mac/MacPlatformThreadMisc.h"
#include "Core/Misc/OutputDeviceLogger.h"

FGenericPlatformThread* FMacPlatformThread::Create(FRunnable* InRunnable, const CHAR* ThreadName, bool bSuspended)
{
    FMacPlatformThread* NewThread = new FMacPlatformThread(InRunnable, ThreadName);
    if (!bSuspended)
    {
        NewThread->Start();
    }

    return NewThread;
}

FMacPlatformThread::FMacPlatformThread(FRunnable* InRunnable, const CHAR* ThreadName)
    : FGenericPlatformThread(InRunnable, ThreadName)
    , Thread()
{ 
}

bool FMacPlatformThread::Start()
{
    const int32 Result = ::pthread_create(&Thread, nullptr, FMacPlatformThread::ThreadRoutine, reinterpret_cast<void*>(this));
    if (Result)
    {
        LOG_ERROR("[FMacPlatformThread] Failed to create thread");
        return false;
    }
    else
    {
        return true;
    }
}

void FMacPlatformThread::Kill(bool bWaitUntilCompletion)
{
    if (Runnable)
    {
        Runnable->Stop();
    }

    if (bWaitUntilCompletion)
    {
        ::pthread_join(Thread, nullptr);
    }
}

void FMacPlatformThread::WaitForCompletion()
{
    ::pthread_join(Thread, nullptr);
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
