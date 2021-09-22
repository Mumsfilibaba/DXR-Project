#include "WindowsThread.h"

#include "Core/Utilities/StringUtilities.h"

#include <condition_variable>

GenericThread* WindowsThread::Create( ThreadFunction Func )
{
    TSharedRef<WindowsThread> NewThread = DBG_NEW WindowsThread();
    if ( !NewThread->Init( Func ) )
    {
        return nullptr;
    }
    else
    {
        return NewThread.ReleaseOwnership();
    }
}

WindowsThread::WindowsThread()
    : GenericThread()
    , Thread( 0 )
    , hThreadID( 0 )
{
}

WindowsThread::~WindowsThread()
{
    if ( Thread )
    {
        CloseHandle( Thread );
    }
}

bool WindowsThread::Init( ThreadFunction InFunc )
{
    Func = InFunc;

    Thread = CreateThread( NULL, 0, WindowsThread::ThreadRoutine, (LPVOID)this, 0, &hThreadID );
    if ( !Thread )
    {
        LOG_ERROR( "[WindowsThread] Failed to create thread" );
        return false;
    }
    else
    {
        return true;
    }
}

void WindowsThread::Wait()
{
    WaitForSingleObject( Thread, INFINITE );
}

void WindowsThread::SetName( const std::string& Name )
{
    WString WideName = CharToWide( CString( Name.c_str(), Name.length() ) );
    SetThreadDescription( Thread, WideName.CStr() );
}

ThreadID WindowsThread::GetID()
{
    return hThreadID;
}

DWORD WINAPI WindowsThread::ThreadRoutine( LPVOID ThreadParameter )
{
    volatile WindowsThread* CurrentThread = (WindowsThread*)ThreadParameter;
    if ( CurrentThread )
    {
        Assert( CurrentThread->Func != nullptr );

        CurrentThread->Func();
        return 0;
    }

    return (DWORD)-1;
}
