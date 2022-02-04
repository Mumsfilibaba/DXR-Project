#if PLATFORM_WINDOWS
#include "WindowsThread.h"

#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// WindowsThread

TSharedRef<CWindowsThread> CWindowsThread::Make(ThreadFunction InFunction)
{
    return dbg_new CWindowsThread(InFunction);
}

TSharedRef<CWindowsThread> CWindowsThread::Make(ThreadFunction InFunction, const String & InName)
{
    return dbg_new CWindowsThread(InFunction, InName);
}

CWindowsThread::CWindowsThread(ThreadFunction InFunction)
    : CPlatformThread()
    , Thread(0)
    , hThreadID(0)
    , Name()
    , Function(InFunction)
{
}

CWindowsThread::CWindowsThread(ThreadFunction InFunction, const String& InName)
    : CPlatformThread()
    , Thread(0)
    , hThreadID(0)
    , Name(InName)
    , Function(InFunction)
{
}

CWindowsThread::~CWindowsThread()
{
    if (Thread)
    {
        CloseHandle(Thread);
    }
}

bool CWindowsThread::Start()
{
    Thread = CreateThread(NULL, 0, CWindowsThread::ThreadRoutine, reinterpret_cast<void*>(this), 0, &hThreadID);
    if (!Thread)
    {
        LOG_ERROR("[CWindowsThread] Failed to create thread");
        return false;
    }
    else
    {
        return true;
    }
}

void CWindowsThread::WaitUntilFinished()
{
    WaitForSingleObject(Thread, INFINITE);
}

void CWindowsThread::SetName(const String& InName)
{
    WString WideName = CharToWide(InName);
    SetThreadDescription(Thread, WideName.CStr());

    Name = InName;
}

PlatformThreadHandle CWindowsThread::GetPlatformHandle()
{
    return hThreadID;
}

DWORD WINAPI CWindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    CWindowsThread* CurrentThread = (CWindowsThread*)ThreadParameter;
    if (CurrentThread)
    {
        if (!CurrentThread->Name.IsEmpty())
        {
            WString WideName = CharToWide(CurrentThread->Name);
            SetThreadDescription(CurrentThread->Thread, WideName.CStr());
        }

        Assert(CurrentThread->Function != nullptr);
        CurrentThread->Function();

        return 0;
    }

    return (DWORD)-1;
}

#endif