#include "WindowsThread.h"

Thread* Thread::Create()
{
    TRef<WindowsThread> NewThread = DBG_NEW WindowsThread();
    if (NewThread->Init())
    {
        return nullptr;
    }

    return NewThread.ReleaseOwnership();
}

WindowsThread::WindowsThread()
    : Thread()
    , hThread(0)
    , hThreadID(0)
{
}

WindowsThread::~WindowsThread()
{
    if (hThread)
    {
        CloseHandle(hThread);
    }
}

bool WindowsThread::Init()
{
    hThread = CreateThread(NULL, 0, WindowsThread::ThreadRoutine, (LPVOID)this, 0, &hThreadID);
    if (!hThread)
    {
        LOG_ERROR("[WindowsThread] Failed to create thread");
        return false;
    }

    return true;
}

void WindowsThread::Sleep(Timestamp Time)
{
}

DWORD WINAPI WindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    WindowsThread* CurrentThread = (WindowsThread*)ThreadParameter;
    if (CurrentThread)
    {
    }

    return 0;
}
