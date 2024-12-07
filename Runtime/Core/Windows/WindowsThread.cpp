#include "Core/Windows/WindowsThread.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformTLS.h"

FGenericThread* FWindowsThread::Create(FRunnable* Runnable, const CHAR* InThreadName, bool bSuspended)
{
    FWindowsThread* NewThread = new FWindowsThread(Runnable, InThreadName, bSuspended);
    return NewThread;
}

FWindowsThread::FWindowsThread(FRunnable* InRunnable, const CHAR* InThreadName, bool bSuspended)
    : FGenericThread(InRunnable, InThreadName)
    , Thread(0)
    , hThreadID(0)
    , bIsSuspended(bSuspended)
{
    DWORD Flags = 0;
    if (bIsSuspended)
    {
        Flags = CREATE_SUSPENDED;
    }

    Thread = ::CreateThread(nullptr, 0, FWindowsThread::ThreadRoutine, reinterpret_cast<void*>(this), Flags, &hThreadID);
    if (!Thread)
    {
        LOG_ERROR("[FWindowsThread] Failed to create thread");
        DEBUG_BREAK();
    }
}

FWindowsThread::~FWindowsThread()
{
    if (Thread)
    {
        ::CloseHandle(Thread);
    }
}

bool FWindowsThread::Start()
{
    CHECK(bIsSuspended);
    CHECK(hThreadID != 0 && Thread != 0);

    DWORD Result = ::ResumeThread(Thread);
    if (Result == DWORD(-1))
    {
        LOG_ERROR("[FWindowsThread] Failed to Start thread");
        return false;
    }

    return true;
}

void FWindowsThread::Kill(bool bWaitUntilCompletion)
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

void FWindowsThread::Suspend()
{
    ::SuspendThread(Thread);
}

void FWindowsThread::Resume()
{
    ::ResumeThread(Thread);
}

void FWindowsThread::WaitForCompletion()
{
    ::WaitForSingleObject(Thread, INFINITE);
}

void* FWindowsThread::GetPlatformHandle()
{
    SIZE_T Handle = static_cast<SIZE_T>(hThreadID);
    return reinterpret_cast<void*>(Handle);
}

DWORD WINAPI FWindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    DWORD Result = DWORD(-1);

    FWindowsThread* CurrentThread = reinterpret_cast<FWindowsThread*>(ThreadParameter);
    if (CurrentThread)
    {
        // Ensure that this thread can be retrieved
        FPlatformTLS::SetTLSValue(FGenericThread::TLSSlot, CurrentThread);

        if (!CurrentThread->ThreadName.IsEmpty())
        {
            FStringWide WideName = CharToWide(CurrentThread->ThreadName);
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
        
        FPlatformTLS::SetTLSValue(FGenericThread::TLSSlot, nullptr);
    }

    return Result;
}