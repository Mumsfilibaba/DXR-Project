#pragma once
#include "Core/Threading/Generic/GenericThread.h"

class CWindowsThread : public CGenericThread
{
public:

    static FORCEINLINE CWindowsThread* Make( ThreadFunction InFunction )
    {
        return DBG_NEW CWindowsThread( InFunction );
    }

    static FORCEINLINE CWindowsThread* Make( ThreadFunction InFunction, const CString& InName )
	{
		return new CWindowsThread( InFunction, InName );
	}

    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    virtual void SetName( const CString& Name ) override final;

    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:

    CWindowsThread( ThreadFunction InFunction );
    CWindowsThread( ThreadFunction InFunction, const CString& InName );
    ~CWindowsThread();

    static DWORD WINAPI ThreadRoutine( LPVOID ThreadParameter );

    HANDLE Thread;
    DWORD  hThreadID;

    CString Name;

    ThreadFunction Function;
};