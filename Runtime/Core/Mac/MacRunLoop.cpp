#include "MacRunLoop.h"

#include "Core/Containers/Array.h"
#include "Core/Threading/Spinlock.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/PlatformThreadMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacRunLoopSource

class FMacRunLoopSource
{
public:
    FMacRunLoopSource(CFRunLoopRef InRunLoop, NSString* InRunLoopMode)
        : RunLoop(InRunLoop)
        , RunLoopMode(InRunLoopMode)
        , BlockLock()
        , Blocks()
    {
        CFRetain(RunLoop);
        
        CFRunLoopSourceContext SourceContext = CFRunLoopSourceContext();
        SourceContext.info     = reinterpret_cast<void*>(this);
        SourceContext.version  = 0;
        SourceContext.perform  = &FMacRunLoopSource::Perform;
        
        Source = CFRunLoopSourceCreate(nullptr, 0, &SourceContext);
        CFRunLoopAddSource(RunLoop, Source, (CFStringRef)RunLoopMode);
    }
    
    ~FMacRunLoopSource()
    {
        CFRunLoopRemoveSource(RunLoop, Source, (CFStringRef)RunLoopMode);
        CFRelease(Source);
        
        CFRelease(RunLoop);
    }
    
    void ScheduleBlock(dispatch_block_t Block)
    {
        dispatch_block_t CopyBlock = Block_copy(Block);

        {
            TScopedLock Lock(BlockLock);
            Blocks.Push(CopyBlock);
        }
        
        CFRunLoopSourceSignal(Source);
    }
    
    void Execute()
    {
        // Copy blocks
        TArray<dispatch_block_t> CopiedBlocks;
        {
            TScopedLock Lock(BlockLock);
            CopiedBlocks.Swap(Blocks);
        }
        
        // Execute all blocks
        for (dispatch_block_t Block : CopiedBlocks)
        {
            Block();
            Block_release(Block);
        }
    }
    
    void RunInMode(CFRunLoopMode RunMode)
    {
        CFRunLoopRunInMode(RunMode, 0, true);
    }
    
    void WakeUp()
    {
        CFRunLoopWakeUp(RunLoop);
    }
    
private:
    static void Perform(void* Info)
    {
        FMacRunLoopSource* RunLoopSource = reinterpret_cast<FMacRunLoopSource*>(Info);
        if (RunLoopSource)
        {
            RunLoopSource->Execute();
        }
    }
    
    CFRunLoopRef       RunLoop     = nullptr;
    CFRunLoopSourceRef Source      = nullptr;
    NSString*          RunLoopMode = nullptr;
    
    // Blocks that are going to be executed
    FSpinLock                BlockLock;
    TArray<dispatch_block_t> Blocks;
};

// Main-thread runloop source
FMacRunLoopSource* GMainThread = nullptr;

bool RegisterMainRunLoop()
{
    CFRunLoopRef MainLoop = CFRunLoopGetMain();
    GMainThread = new FMacRunLoopSource(MainLoop, NSDefaultRunLoopMode);
    
    return true;
}

void UnregisterMainRunLoop()
{
    SAFE_DELETE(GMainThread);
}

void MakeMainThreadCall(dispatch_block_t Block, bool bWaitUntilFinished)
{
    dispatch_block_t CopiedBlock = Block_copy(Block);
    
    if (FPlatformThreadMisc::IsMainThread())
    {
        // If already on mainthread, execute Block here
        CopiedBlock();
    }
    else
    {
        // Otherwise schedule Block on main thread
        Check(GMainThread != nullptr);

        if (bWaitUntilFinished)
        {
            __block dispatch_semaphore_t WaitSemaphore = dispatch_semaphore_create(0);
            dispatch_block_t WaitableBlock = Block_copy(^
            {
                CopiedBlock();
                dispatch_semaphore_signal(WaitSemaphore);
            });
            
            GMainThread->ScheduleBlock(WaitableBlock);
            
            do
            {
                GMainThread->WakeUp();
                GMainThread->RunInMode((CFStringRef)NSDefaultRunLoopMode);
            } while (dispatch_semaphore_wait(WaitSemaphore, dispatch_time(0, 10000ull)));
            
            Block_release(WaitableBlock);
            dispatch_release(WaitSemaphore);
        }
        else
        {
            GMainThread->ScheduleBlock(CopiedBlock);
            GMainThread->WakeUp();
        }
    }
    
    Block_release(CopiedBlock);
}
