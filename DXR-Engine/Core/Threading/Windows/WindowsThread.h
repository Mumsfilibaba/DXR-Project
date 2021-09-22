#pragma once
#include "Core/Threading/Generic/GenericThread.h"

class CWindowsThread : public CGenericThread
{
public:

    static CGenericThread* Make( ThreadFunction InFunction );

    bool Init( ThreadFunction InFunc );

    virtual void Wait() override final;

    virtual void SetName( const std::string& Name ) override final;

    virtual ThreadID GetID() override final;

private:

    CWindowsThread();
    ~CWindowsThread();

    static DWORD WINAPI ThreadRoutine( LPVOID ThreadParameter );

    HANDLE Thread;
    DWORD  hThreadID;

    ThreadFunction Func;
};