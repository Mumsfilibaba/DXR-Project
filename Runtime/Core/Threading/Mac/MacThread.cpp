#include "MacThread.h"
#include "MacThreadMisc.h"

#include "Core/Logging/Logger.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThread

FMacThread::FMacThread(const FThreadFunction& InFunction)
    : FGenericThread(InFunction)
    , Name()
    , Thread()
	, ThreadExitCode(-1)
    , bIsRunning(false)
{ }

FMacThread::FMacThread(const FThreadFunction& InFunction, const FString& InName)
    : FGenericThread(InFunction)
    , Name(InName)
    , Thread()
    , ThreadExitCode(-1)
    , bIsRunning(false)
{ }

bool FMacThread::Start()
{
    const auto Result = pthread_create(&Thread, nullptr, FMacThread::ThreadRoutine, reinterpret_cast<void*>(this));
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

int32 FMacThread::WaitForCompletion(uint64 TimeoutInMs)
{
	UNREFERENCED_VARIABLE(TimeoutInMs);
	
    // TODO: Investigate timeout
    const auto Result = pthread_join(Thread, nullptr);
    return Result ? ThreadExitCode : int32(-1);
}

void FMacThread::SetName(const FString& InName)
{
    // The name can always be set from the current thread
    const bool bCurrentThreadIsMyself = GetPlatformHandle() == FMacThreadMisc::GetThreadHandle();
    if (bCurrentThreadIsMyself)
    {
        Name = InName;
        pthread_setname_np(Name.GetCString());
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
    FMacThread* CurrentThread = reinterpret_cast<FMacThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Can only set the current thread's name
        if (!CurrentThread->Name.IsEmpty())
        {
            pthread_setname_np(CurrentThread->Name.GetCString());
        }

        Check(CurrentThread->Function);
        CurrentThread->Function();

		CurrentThread->ThreadExitCode = 0;
    }

    pthread_exit(nullptr);
    return nullptr;
}
