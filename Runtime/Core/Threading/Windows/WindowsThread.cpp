#include "WindowsThread.h"

#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThread

FWindowsThread::FWindowsThread(const TFunction<void()>& InFunction)
    : FGenericThread(InFunction)
    , Thread(0)
    , hThreadID(0)
    , Name()
{ }

FWindowsThread::FWindowsThread(const TFunction<void()>& InFunction, const FString& InName)
    : FGenericThread(InFunction)
    , Thread(0)
    , hThreadID(0)
    , Name(InName)
{ }

FWindowsThread::~FWindowsThread()
{
    if (Thread)
    {
        CloseHandle(Thread);
    }
}

bool FWindowsThread::Start()
{
    Thread = CreateThread(nullptr, 0, FWindowsThread::ThreadRoutine, reinterpret_cast<void*>(this), 0, &hThreadID);
    if (!Thread)
    {
        LOG_ERROR("[FWindowsThread] Failed to create thread");
        return false;
    }
    else
    {
        return true;
    }
}

int32 FWindowsThread::WaitForCompletion(uint64 TimeoutInMs)
{
    DWORD ThreadExitCode = 0;
    WaitForSingleObject(Thread, DWORD(TimeoutInMs));

    const BOOL Result = GetExitCodeThread(Thread, &ThreadExitCode);
    return Result ? int32(ThreadExitCode) : int32(-1);
}

void FWindowsThread::SetName(const FString& InName)
{
    FWString WideName = CharToWide(InName);
    SetThreadDescription(Thread, WideName.CStr());
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
            FWString WideName = CharToWide(CurrentThread->Name);
            SetThreadDescription(CurrentThread->Thread, WideName.CStr());
        }

        Check(CurrentThread->Function);
        CurrentThread->Function();
        return 0;
    }

    return DWORD(-1);
}