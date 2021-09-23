#if defined(PLATFORM_WINDOWS)
#include "WindowsThread.h"

#include "Core/Utilities/StringUtilities.h"

CWindowsThread::CWindowsThread( ThreadFunction InFunction )
    : CGenericThread()
    , Function( InFunction )
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

bool CWindowsThread::Start()
{
    Thread = CreateThread( NULL, 0, CWindowsThread::ThreadRoutine, reinterpret_cast<void*>(this), 0, &hThreadID );
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

void CWindowsThread::WaitUntilFinished()
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

        CurrentThread->Function();
        return 0;
    }

    return (DWORD)-1;
}

#endif