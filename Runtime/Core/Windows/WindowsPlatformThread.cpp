#include "Core/Windows/WindowsPlatformThread.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformTLS.h"

FGenericPlatformThread* FWindowsPlatformThread::Create(FRunnable* Runnable, const CHAR* InThreadName, bool bSuspended)
{
    FWindowsPlatformThread* NewThread = new FWindowsPlatformThread(Runnable, InThreadName, bSuspended);
    return NewThread;
}

FWindowsPlatformThread::FWindowsPlatformThread(FRunnable* InRunnable, const CHAR* InThreadName, bool bSuspended)
    : FGenericPlatformThread(InRunnable, InThreadName)
    , Thread(0)
    , hThreadID(0)
    , bIsSuspended(bSuspended)
{
    DWORD Flags = 0;
    if (bIsSuspended)
    {
        Flags = CREATE_SUSPENDED;
    }

    Thread = ::CreateThread(nullptr, 0, FWindowsPlatformThread::ThreadRoutine, reinterpret_cast<void*>(this), Flags, &hThreadID);
    if (!Thread)
    {
        LOG_ERROR("[FWindowsPlatformThread] Failed to create thread");
        DEBUG_BREAK();
    }
}

FWindowsPlatformThread::~FWindowsPlatformThread()
{
    if (Thread)
    {
        ::CloseHandle(Thread);
    }
}

bool FWindowsPlatformThread::Start()
{
    CHECK(bIsSuspended);
    CHECK(hThreadID != 0 && Thread != 0);

    DWORD Result = ::ResumeThread(Thread);
    if (Result == DWORD(-1))
    {
        LOG_ERROR("[FWindowsPlatformThread] Failed to Start thread");
        return false;
    }

    return true;
}

void FWindowsPlatformThread::Kill(bool bWaitUntilCompletion)
{
    if (Runnable)
    {
        Runnable->Stop();
    }

    if (bWaitUntilCompletion)
    {
        ::WaitForSingleObject(Thread, INFINITE);
    }

    ::CloseHandle(Thread);
    Thread = 0;
}

void FWindowsPlatformThread::Suspend()
{
    ::SuspendThread(Thread);
}

void FWindowsPlatformThread::Resume()
{
    ::ResumeThread(Thread);
}

void FWindowsPlatformThread::WaitForCompletion()
{
    ::WaitForSingleObject(Thread, INFINITE);
}

void* FWindowsPlatformThread::GetPlatformHandle()
{
    SIZE_T Handle = static_cast<SIZE_T>(hThreadID);
    return reinterpret_cast<void*>(Handle);
}

DWORD WINAPI FWindowsPlatformThread::ThreadRoutine(LPVOID ThreadParameter)
{
    DWORD Result = DWORD(-1);

    FWindowsPlatformThread* CurrentThread = reinterpret_cast<FWindowsPlatformThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Ensure that this thread can be retrieved
        FPlatformTLS::SetTLSValue(FGenericPlatformThread::TLSSlot, CurrentThread);

        if (!CurrentThread->Name.IsEmpty())
        {
            StringWide WideName = CharToWide(CurrentThread->Name);
            ::SetThreadDescription(CurrentThread->Thread, *WideName);
        }

        if (FRunnable* Runnable = CurrentThread->Runnable)
        {
            if (Runnable->Start())
            {
                Result = DWORD(Runnable->Run());
            }

            Runnable->Destroy();
        }
        
        FPlatformTLS::SetTLSValue(FGenericPlatformThread::TLSSlot, nullptr);
    }

    return Result;
}
