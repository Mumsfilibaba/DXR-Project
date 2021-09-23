#if defined(PLATFORM_MACOS)
#include "MacThread.h"

CMacThread::CMacThread( ThreadFunction InFunction )
    : CGenericThread()
    , Function( InFunction )
    , Thread()
{
}

CMacThread::~CMacThread()
{
	// Empty for now
}

bool CMacThread::Start()
{
	int Result = pthread_create(&Thread, NULL, CMacThread::ThreadRoutine, reinterpret_cast<void*>(this));
    if ( Result )
    {
        LOG_ERROR( "[CMacThread] Failed to create thread" );
        return false;
    }
    else
    {
        return true;
    }
}

void CMacThread::WaitUntilFinished()
{
	pthread_join(Thread, NULL);
}

void CMacThread::SetName( const std::string& Name )
{
	UNREFERENCED_VARIABLE( Name );
	// Empty for now
}

PlatformThreadHandle CMacThread::GetPlatformHandle()
{
    return reinterpret_cast<PlatformThreadHandle>(Thread);
}

void* CMacThread::ThreadRoutine( void* ThreadParameter )
{
    volatile CMacThread* CurrentThread = (CMacThread*)ThreadParameter;
    if ( CurrentThread )
    {
        Assert( CurrentThread->Function != nullptr );
        CurrentThread->Function();
    }

	pthread_exit(NULL);
    return nullptr;
}
#endif
