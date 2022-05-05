#include "MacThread.h"
#include "MacThreadMisc.h"

#include "Core/Logging/Log.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacThread

CMacThread::CMacThread(const TFunction<void()>& InFunction)
    : CGenericThread(InFunction)
    , Name()
    , Thread()
	, ThreadExitCode(-1)
    , bIsRunning(false)
{ }

CMacThread::CMacThread(const TFunction<void()>& InFunction, const String& InName)
    : CGenericThread(InFunction)
    , Name(InName)
    , Thread()
    , ThreadExitCode(-1)
    , bIsRunning(false)
{ }

bool CMacThread::Start()
{
    const auto Result = pthread_create(&Thread, nullptr, CMacThread::ThreadRoutine, reinterpret_cast<void*>(this));
    if (Result)
    {
        LOG_ERROR("[CMacThread] Failed to create thread");
        return false;
    }
    else
    {
        return true;
    }
}

int32 CMacThread::WaitForCompletion(uint64 TimeoutInMs)
{
    // TODO: Investigate timeout
    const auto Result = pthread_join(Thread, nullptr);
    return Result ? ThreadExitCode : int32(-1);
}

void CMacThread::SetName(const String& InName)
{
    // The name can always be set from the current thread
    const bool bCurrentThreadIsMyself = GetPlatformHandle() == CMacThreadMisc::GetThreadHandle();
    if (bCurrentThreadIsMyself)
    {
        Name = InName;
        pthread_setname_np(Name.CStr());
    }
    else if (!bIsRunning)
    {
        Name = InName;
    }
}

void* CMacThread::GetPlatformHandle()
{
    return reinterpret_cast<void*>(Thread);
}

void* CMacThread::ThreadRoutine(void* ThreadParameter)
{
    CMacThread* CurrentThread = reinterpret_cast<CMacThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Can only set the current thread's name
        if (!CurrentThread->Name.IsEmpty())
        {
            pthread_setname_np(CurrentThread->Name.CStr());
        }

        Check(CurrentThread->Function);
        CurrentThread->Function();

		CurrentThread->ThreadExitCode = 0;
    }

    pthread_exit(nullptr);
    return nullptr;
}
