#include "WindowsThread.h"

#include "Core/Utilities/StringUtilities.h"

#include <condition_variable>

CGenericThread* CWindowsThread::Make( ThreadFunction Func )
{
    TSharedRef<CWindowsThread> NewThread = DBG_NEW CWindowsThread();
    if ( !NewThread->Init( Func ) )
    {
        return nullptr;
    }
    else
    {
        return NewThread.ReleaseOwnership();
    }
}

CWindowsThread::CWindowsThread()
    : CGenericThread()
    , Thread( 0 )
    , hThreadID( 0 )
{
}

CWindowsThread::~CWindowsThread()
{
    if ( Thread )
    {
        CloseHandle( Thread );
    }
}

bool CWindowsThread::Init( ThreadFunction InFunc )
{
    Func = InFunc;

    Thread = CreateThread( NULL, 0, CWindowsThread::ThreadRoutine, (LPVOID)this, 0, &hThreadID );
    if ( !Thread )
    {
        LOG_ERROR( "[CWindowsThread] Failed to create thread" );
        return false;
    }
    else
    {
        return true;
    }
}

void CWindowsThread::Wait()
{
    WaitForSingleObject( Thread, INFINITE );
}

void CWindowsThread::SetName( const std::string& Name )
{
    WString WideName = CharToWide( CString( Name.c_str(), Name.length() ) );
    SetThreadDescription( Thread, WideName.CStr() );
}

ThreadID CWindowsThread::GetID()
{
    return hThreadID;
}

DWORD WINAPI CWindowsThread::ThreadRoutine( LPVOID ThreadParameter )
{
    volatile CWindowsThread* CurrentThread = (CWindowsThread*)ThreadParameter;
    if ( CurrentThread )
    {
        Assert( CurrentThread->Func != nullptr );

        CurrentThread->Func();
        return 0;
    }

    return (DWORD)-1;
}
