#include "MacThread.h"
#include "MacThreadMisc.h"
#include "Core/Misc/OutputDeviceLogger.h"

FMacThread::FMacThread(FThreadInterface* InRunnable)
    : FGenericThread(InRunnable)
    , Name()
    , Thread()
    , bIsRunning(false)
{ 
}

FMacThread::~FMacThread()
{
    LOG_INFO("Destroying thread %s", Name.GetCString());
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

void FMacThread::WaitForCompletion()
{
    ::pthread_join(Thread, nullptr);
}

void FMacThread::SetName(const FString& InName)
{
    // The name can always be set from the current thread
    const bool bCurrentThreadIsMyself = GetPlatformHandle() == FMacThreadMisc::GetThreadHandle();
    if (bCurrentThreadIsMyself)
    {
        Name = InName;
        ::pthread_setname_np(Name.GetCString());
    }
    else if (!bIsRunning)
    {
        Name = InName;
    }
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
        // Can only set the current thread's name
        if (!CurrentThread->Name.IsEmpty())
        {
            ::pthread_setname_np(CurrentThread->Name.GetCString());
        }

        if (FThreadInterface* Runnable = CurrentThread->GetRunnable())
        {
            if (Runnable->Start())
            {
                Result = Runnable->Run();
            }

            Runnable->Destroy();
        }
    }

    ::pthread_exit(nullptr);
    return reinterpret_cast<void*>(Result);
}
