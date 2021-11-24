#include "MacRunLoop.h"

#include "Core/Containers/Array.h"
#include "Core/Threading/Spinlock.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include <Foundation/Foundation.h>

/* Create definition for CMacRunLoopSource, is not needed outside this compilation unit */
class CMacRunLoopSource
{
public:

    CMacRunLoopSource( CFRunLoopRef InRunLoop, NSString* InRunLoopMode )
        : RunLoop( InRunLoop )
        , RunLoopMode( InRunLoopMode )
        , BlockLock()
        , Blocks()
    {
        CFRunLoopSourceContext SourceContext;
		CMemory::Memzero( &SourceContext );
		
        SourceContext.info     = reinterpret_cast<void*>(this);
        SourceContext.version  = 0;
        SourceContext.perform  = &CMacRunLoopSource::Perform;
		SourceContext.schedule = &CMacRunLoopSource::Schedule;
		SourceContext.cancel   = &CMacRunLoopSource::Cancel;
		
        Source = CFRunLoopSourceCreate( nullptr, 0, &SourceContext );
        CFStringRef RunLoopModeName = (CFStringRef)RunLoopMode;
        CFRunLoopAddSource( RunLoop, Source, RunLoopModeName );
    }
    
    ~CMacRunLoopSource() = default;
    
    void ScheduleBlock( dispatch_block_t Block )
    {
        dispatch_block_t CopyBlock = Block_copy( Block );

        // Add block and signal source to perform next runloop iteration
        {
			TScopedLock Lock( BlockLock );
            Blocks.Push( CopyBlock );
        }
        
        CFRunLoopSourceSignal( Source );
    }
    
    void Execute()
    {
        // Copy blocks
        TArray<dispatch_block_t> CopiedBlocks;
        {
			TScopedLock Lock( BlockLock );
            
            CopiedBlocks = TArray<dispatch_block_t>( Blocks );
            Blocks.Clear();
        }
        
        // Execute all blocks
        for ( dispatch_block_t Block : CopiedBlocks )
        {
            Block();
            Block_release( Block );
        }
    }
    
    void RunInMode( CFRunLoopMode RunMode )
    {
        CFRunLoopRunInMode( RunMode, 0, true );
    }
    
    void WakeUp()
    {
        CFRunLoopWakeUp( RunLoop );
    }
    
private:

	static void Schedule(void* Info, CFRunLoopRef RunLoop, CFStringRef Mode)
	{
		NSLog(@"Schedule");
	}
	
	static void Cancel(void* Info, CFRunLoopRef RunLoop, CFStringRef Mode)
	{
		NSLog(@"Cancel");
	}
	
	static void Perform( void* Info )
	{
		NSLog(@"Perform");
		
		CMacRunLoopSource* RunLoopSource = reinterpret_cast<CMacRunLoopSource*>(Info);
		if ( RunLoopSource )
		{
			RunLoopSource->Execute();
		}
	}
    
    CFRunLoopRef       RunLoop     = nullptr;
    CFRunLoopSourceRef Source      = nullptr;
    NSString*          RunLoopMode = nullptr;
    
	// Blocks that are going to be executed
    CSpinLock BlockLock;
    TArray<dispatch_block_t> Blocks;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

CMacRunLoopSource* GMainThread = nullptr;

bool RegisterMainRunLoop()
{
    CFRunLoopRef MainLoop = CFRunLoopGetMain();
	GMainThread = new CMacRunLoopSource( MainLoop, NSDefaultRunLoopMode );
	
	return true;
}

void UnregisterMainRunLoop()
{
    SafeDelete( GMainThread );
}

void MakeMainThreadCall( dispatch_block_t Block, bool WaitUntilFinished )
{
    dispatch_block_t CopiedBlock = Block_copy( Block );
    
    if ( PlatformThreadMisc::IsMainThread() )
    {
        // If already on mainthread, execute Block here
        CopiedBlock();
    }
    else
    {
        // Otherwise schedule Block on main thread
        Assert( GMainThread != nullptr );

        if ( WaitUntilFinished )
        {
            dispatch_semaphore_t WaitSemaphore = dispatch_semaphore_create( 0 );
			
            dispatch_block_t WaitableBlock = Block_copy(^
            {
                CopiedBlock();
                dispatch_semaphore_signal( WaitSemaphore );
            });
            
			GMainThread->ScheduleBlock( WaitableBlock );
            
			do
			{
				GMainThread->WakeUp();
				GMainThread->RunInMode( (CFStringRef)NSDefaultRunLoopMode );
			} while ( dispatch_semaphore_wait( WaitSemaphore, dispatch_time(0, 100000ull) ) );
			
            Block_release( WaitableBlock );
        }
        else
        {
			GMainThread->ScheduleBlock( CopiedBlock );
			GMainThread->WakeUp();
        }
    }
    
    Block_release( CopiedBlock );
}
