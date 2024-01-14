#include "WindowsThread.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Utilities/StringUtilities.h"

FWindowsThread::FWindowsThread(FThreadInterface* InRunnable, bool bSuspended)
    : FGenericThread(InRunnable)
    , Thread(0)
    , hThreadID(0)
    , bIsSuspended(bSuspended)
    , Name()
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

void FWindowsThread::WaitForCompletion()
{
    ::WaitForSingleObject(Thread, INFINITE);
}

void FWindowsThread::SetName(const FString& InName)
{
    FStringWide WideName = CharToWide(InName);
    ::SetThreadDescription(Thread, WideName.GetCString());
    Name = InName;
}

void* FWindowsThread::GetPlatformHandle()
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(hThreadID));
}

DWORD WINAPI FWindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    FWindowsThread* CurrentThread = reinterpret_cast<FWindowsThread*>(ThreadParameter);
    if (CurrentThread)
    {
        if (!CurrentThread->Name.IsEmpty())
        {
            FStringWide WideName = CharToWide(CurrentThread->Name);
            ::SetThreadDescription(CurrentThread->Thread, WideName.GetCString());
        }

        DWORD Result = DWORD(-1);
        if (FThreadInterface* Runnable = CurrentThread->Runnable)
        {
            if (Runnable->Start())
            {
                Result = DWORD(Runnable->Run());
            }

            Runnable->Destroy();
            return Result;
        }
    }

    return DWORD(-1);
}