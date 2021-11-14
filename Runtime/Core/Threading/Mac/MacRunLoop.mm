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
        SourceContext.info    = reinterpret_cast<void*>( this );
        SourceContext.version = 0;
        SourceContext.perform = CMacRunLoopSource::Perform;
		
        Source = CFRunLoopSourceCreate( kCFAllocatorDefault, 0, &SourceContext );
        Assert(Source != nullptr);
        
        CFStringRef RunLoopModeName = (CFStringRef)RunLoopMode;
        CFRunLoopAddSource( RunLoop, Source, RunLoopModeName );
        CFRelease( Source );
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
    
    void RunInMode(CFRunLoopMode RunMode)
    {
        CFRunLoopRunInMode( RunMode, 0, true );
    }
    
    void WakeUp()
    {
        CFRunLoopWakeUp( RunLoop );
    }
    
private:
	
    static void Perform(void* Info)
    {
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

CMacRunLoopSource* CMacMainThread::MainThread = nullptr;

void CMacMainThread::Init()
{
    CFRunLoopRef MainLoop = CFRunLoopGetMain();
    MainThread = new CMacRunLoopSource( MainLoop, NSDefaultRunLoopMode );
}

void CMacMainThread::Release()
{
    SafeDelete( MainThread );
}

void CMacMainThread::Tick()
{
    Assert( MainThread != nullptr );
    MainThread->RunInMode( (CFRunLoopMode)NSDefaultRunLoopMode );
}

void CMacMainThread::MakeCall( dispatch_block_t Block, bool WaitUntilFinished )
{
    dispatch_block_t CopiedBlock = Block_copy( Block );
    
	// Have to be careful here about when things are run, since this function may be called before the PlatformThreadMisc::Init is called
    if ( [NSThread isMainThread] )
    {
        // If already on mainthread, execute Block here
        CopiedBlock();
    }
    else
    {
        // Otherwise schedule Block on main thread
        Assert( MainThread != nullptr );

        if (WaitUntilFinished)
        {
            dispatch_semaphore_t WaitSemaphore = dispatch_semaphore_create( 0 );
			
            dispatch_block_t WaitableBlock = Block_copy(^
            {
                CopiedBlock();
                dispatch_semaphore_signal( WaitSemaphore );
            });
            
            MainThread->ScheduleBlock( WaitableBlock );
            MainThread->RunInMode( (CFStringRef)NSDefaultRunLoopMode );
            
            dispatch_semaphore_wait( WaitSemaphore, DISPATCH_TIME_FOREVER );
            Block_release( WaitableBlock );
        }
        else
        {
            MainThread->ScheduleBlock( CopiedBlock );
            MainThread->WakeUp();
        }
    }
    
    Block_release( CopiedBlock );
}
