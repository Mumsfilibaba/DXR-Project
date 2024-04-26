#include "MacThread.h"
#include "MacThreadMisc.h"
#include "Core/Misc/OutputDeviceLogger.h"

FGenericThread* FMacThread::Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended)
{
    FMacThread* NewThread = new FMacThread(InRunnable, ThreadName);
    if (!bSuspended)
    {
        NewThread->Start();
    }

    return NewThread;
}

FMacThread::FMacThread(FRunnable* InRunnable, const CHAR* ThreadName)
    : FGenericThread(InRunnable, ThreadName)
    , Thread()
    , bIsRunning(false)
{ 
}

bool FMacThread::Start()
{
    const int32 Result = ::pthread_create(&Thread, nullptr, FMacThread::ThreadRoutine, reinterpret_cast<void*>(this));
    if (Result)
    {
        LOG_ERROR("[FMacThread] Failed to create thread");
        return false;
    }
    else
    {
        return true;
    }
}

void FMacThread::Kill(bool bWaitUntilCompletion)
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

void FMacThread::WaitForCompletion()
{
    ::pthread_join(Thread, nullptr);
}

void* FMacThread::GetPlatformHandle()
{
    return reinterpret_cast<void*>(Thread);
}

void* FMacThread::ThreadRoutine(void* ThreadParameter)
{
    int32 Result = int32(-1);

    FMacThread* CurrentThread = reinterpret_cast<FMacThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Ensure that this thread can be retrieved
        FPlatformTLS::SetTLSValue(FGenericThread::TLSSlot, CurrentThread);

        // ThreadName can only be set from the running thread
        if (!CurrentThread->ThreadName.IsEmpty())
        {
            const CHAR* ThreadName = CurrentThread->ThreadName.GetCString();
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

        FPlatformTLS::SetTLSValue(FGenericThread::TLSSlot, nullptr);
    }

    ::pthread_exit(nullptr);
    return reinterpret_cast<void*>(Result);
}
