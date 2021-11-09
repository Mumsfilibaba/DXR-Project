#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformThread.h"

class CORE_API CWindowsThread final : public CPlatformThread
{
public:

    static TSharedRef<CWindowsThread> Make( ThreadFunction InFunction )
    {
        return dbg_new CWindowsThread( InFunction );
    }

    static TSharedRef<CWindowsThread> Make( ThreadFunction InFunction, const CString & InName )
    {
        return dbg_new CWindowsThread( InFunction, InName );
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
#endif