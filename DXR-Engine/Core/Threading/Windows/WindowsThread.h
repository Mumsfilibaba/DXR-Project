#pragma once
#include "Core/Threading/Generic/Thread.h"

class WindowsThread : public Thread
{
public:
    WindowsThread();
    ~WindowsThread();

    bool Init(ThreadFunction InFunc);

    virtual void Wait() override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE hThread;
    DWORD  hThreadID;
    ThreadFunction Func;
};