#pragma once
#include "Core/Threading/Generic/Thread.h"

class WindowsThread : public Thread
{
public:
    WindowsThread();
    ~WindowsThread();

    bool Init();

    virtual void Sleep(Timestamp Time) override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE hThread;
    DWORD  hThreadID;
};