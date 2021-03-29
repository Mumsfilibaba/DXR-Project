#include "WindowsThread.h"

Thread* Thread::Create(ThreadFunction Func)
{
    TRef<WindowsThread> NewThread = DBG_NEW WindowsThread();
    if (!NewThread->Init(Func))
    {
        return nullptr;
    }
    else
    {
        return NewThread.ReleaseOwnership();
    }
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

bool WindowsThread::Init(ThreadFunction InFunc)
{
    hThread = CreateThread(NULL, 0, WindowsThread::ThreadRoutine, (LPVOID)this, 0, &hThreadID);
    if (!hThread)
    {
        LOG_ERROR("[WindowsThread] Failed to create thread");
        return false;
    }
    else
    {
        Func = InFunc;
    }

    return true;
}

void WindowsThread::Wait()
{
    WaitForSingleObject(hThread, INFINITE);
}

DWORD WINAPI WindowsThread::ThreadRoutine(LPVOID ThreadParameter)
{
    WindowsThread* CurrentThread = (WindowsThread*)ThreadParameter;
    if (CurrentThread)
    {
        CurrentThread->Func();
        return 0;
    }
    else
    {
        return -1;
    }
}
