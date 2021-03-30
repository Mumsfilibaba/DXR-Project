#pragma once
#include "Core/Threading/Generic/GenericThread.h"

class WindowsThread : public GenericThread
{
public:
    WindowsThread();
    ~WindowsThread();

    bool Init(ThreadFunction InFunc);

    virtual void Wait() override final;

    virtual void SetName(const std::string& Name) override final;

    virtual ThreadID GetID() override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    ThreadFunction Func;
};