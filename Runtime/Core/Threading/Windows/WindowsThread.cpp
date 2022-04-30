#include "WindowsThread.h"

#include "Core/Logging/Log.h"
#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsThread

CWindowsThread* CWindowsThread::CreateWindowsThread(const TFunction<void()>& InFunction)
{
    return dbg_new CWindowsThread(InFunction);
}

CWindowsThread* CWindowsThread::CreateWindowsThread(const TFunction<void()>& InFunction, const String & InName)
{
    return dbg_new CWindowsThread(InFunction, InName);
}

CWindowsThread::CWindowsThread(const TFunction<void()>& InFunction)
    : CGenericThread(InFunction)
    , Thread(0)
    , hThreadID(0)
    , Name()
{ }

CWindowsThread::CWindowsThread(const TFunction<void()>& InFunction, const String& InName)
    : CGenericThread(InFunction)
    , Thread(0)
    , hThreadID(0)
    , Name(InName)
{ }

CWindowsThread::~CWindowsThread()
{
    if (Thread)
    {
        CloseHandle(Thread);
    }
}

bool CWindowsThread::Start()
{
    Thread = CreateThread(nullptr, 0, CWindowsThread::ThreadRoutine, reinterpret_cast<void*>(this), 0, &hThreadID);
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

void CWindowsThread::WaitUntilFinished(uint64 TimeoutInMilliseconds)
{
    WaitForSingleObject(Thread, DWORD(TimeoutInMilliseconds));
}

void CWindowsThread::SetName(const String& InName)
{
    WString WideName = CharToWide(InName);
    SetThreadDescription(Thread, WideName.CStr());

    Name = InName;
}

void* CWindowsThread::GetPlatformHandle()
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(hThreadID));
}

DWORD WINAPI CWindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    CWindowsThread* CurrentThread = reinterpret_cast<CWindowsThread*>(ThreadParameter);
    if (CurrentThread)
    {
        if (!CurrentThread->Name.IsEmpty())
        {
            WString WideName = CharToWide(CurrentThread->Name);
            SetThreadDescription(CurrentThread->Thread, WideName.CStr());
        }

        Assert(CurrentThread->Function);
        CurrentThread->Function();

        return 0;
    }

    return DWORD(-1);
}