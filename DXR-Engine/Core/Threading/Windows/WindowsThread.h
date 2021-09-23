#pragma once
#include "Core/Threading/Generic/GenericThread.h"

class CWindowsThread : public CGenericThread
{
public:

    static FORCEINLINE CWindowsThread* Make( ThreadFunction InFunction )
    {
        return DBG_NEW CWindowsThread( InFunction );
    }

    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    virtual void SetName( const std::string& Name ) override final;

    virtual ThreadID GetID() override final;

private:

    CWindowsThread( ThreadFunction InFunction );
    ~CWindowsThread();

    static DWORD WINAPI ThreadRoutine( LPVOID ThreadParameter );

    HANDLE Thread;
    DWORD  hThreadID;

    ThreadFunction Function;
};