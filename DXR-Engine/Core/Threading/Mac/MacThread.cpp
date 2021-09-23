#if defined(PLATFORM_MACOS)
#include "MacThread.h"
#include "MacThreadMisc.h"

CMacThread::CMacThread( ThreadFunction InFunction )
    : CGenericThread()
    , Thread()
	, Function( InFunction )
	, Name()
	, IsRunning( false )
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

void CMacThread::SetName( const std::string& InName )
{
	// The name can always be set from the current thread
	const bool CurrentThreadIsMyself = GetPlatformHandle() == CMacThreadMisc::GetThreadHandle();
	if ( CurrentThreadIsMyself )
	{
		Name = InName;
		pthread_setname_np( Name.c_str() );
	}
	else if ( !IsRunning )
	{
		Name = InName;
	}
}

PlatformThreadHandle CMacThread::GetPlatformHandle()
{
    return reinterpret_cast<PlatformThreadHandle>(Thread);
}

void* CMacThread::ThreadRoutine( void* ThreadParameter )
{
    CMacThread* CurrentThread = reinterpret_cast<CMacThread*>(ThreadParameter);
    if ( CurrentThread )
    {
		// Can only set the current thread's name
		pthread_setname_np( CurrentThread->Name.c_str() );
		
        Assert( CurrentThread->Function != nullptr );
        CurrentThread->Function();
    }

	pthread_exit(NULL);
    return nullptr;
}
#endif
