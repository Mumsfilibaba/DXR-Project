#include "MacRunLoop.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Threading/Spinlock.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/PlatformThreadMisc.h"

#include <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRunLoopSource

@interface FRunLoopSource : NSObject
{
    // Use a NSObject since we want the source to be CFRunLoopSourceContext for lifetime-handling 

@private
	FRunLoopSourceContext* Context;

	CFRunLoopRef ScheduledRunLoop;
	CFStringRef  ScheduledMode;
}

- (id)initWithSource:(FRunLoopSourceContext*)InSource;
- (void)dealloc;

- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;
- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;

- (void)perform;
@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRunLoopSourceContext

class FRunLoopSourceContext
    : public FRefCounted
{
public:
    FRunLoopSourceContext(CFRunLoopRef InRunLoop)
        : RunLoop(InRunLoop)
        , SourceAndModeDictionary(CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks))
        , BlockLock()
        , Blocks()
    {
        CFRetain(RunLoop);

        // Register for the default mode
        RegisterForMode(kCFRunLoopDefaultMode);
    }
    
    ~FRunLoopSourceContext()
    {
        // This dictionary is created so that keys and values are automatically retained and released when added or removed
        CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Destroy, RunLoop);
		CFRelease(SourceAndModeDictionary);
        CFRelease(RunLoop);
    }
    
    void RegisterForMode(CFStringRef InRunLoopMode)
    {
        // Register for a new mode if the source is not already registered
        if(CFDictionaryContainsKey(SourceAndModeDictionary, InRunLoopMode) == false)
        {
            FRunLoopSource* RunLoopSource = [[FRunLoopSource alloc] initWithSource:this];

            CFRunLoopSourceContext SourceContext = CFRunLoopSourceContext();
            SourceContext.info            = reinterpret_cast<void*>(RunLoopSource);
            SourceContext.version         = 0;

            // These functions operate on the Info pointer
            SourceContext.retain          = CFRetain;
            SourceContext.release         = CFRelease;
            SourceContext.copyDescription = CFCopyDescription;
            SourceContext.equal           = CFEqual;
            SourceContext.hash            = CFHash;
            
            SourceContext.perform         = &FRunLoopSourceContext::Perform;
            // SourceContext.schedule        = &FRunLoopSourceContext::Schedule;
			// SourceContext.cancel          = &FRunLoopSourceContext::Cancel;

            Source = CFRunLoopSourceCreate(nullptr, 0, &SourceContext);

            // This dictionary is created so that keys and values are automatically retained and released when added or removed
			CFDictionaryAddValue(SourceDictionary, Mode, Source);

            CFRunLoopAddSource(RunLoop, Source, InRunLoopMode);
			CFRelease(Source);
        }
    }

    void ScheduleBlock(dispatch_block_t Block, NSArray* InModes)
    {
		for (NSString* Mode in InModes)
		{
			RegisterForMode((CFStringRef)Mode);
		}

        dispatch_block_t CopyBlock = Block_copy(Block);

        {
            TScopedLock Lock(BlockLock);
            Blocks.Push(CopyBlock);
        }
        
        CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Signal, Source);
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
    
    void RunInMode(CFStringRef RunMode)
    {
        CFRunLoopRunInMode(RunMode, 0, true);
    }
    
    void WakeUp()
    {
        CFRunLoopWakeUp(RunLoop);
    }
    
private:
    static void Destroy(const void* Key, const void* Value, void* Context)
    {
		CFRunLoopRef RunLoop = static_cast<CFRunLoopRef>(Context);
		if(RunLoop)
		{
            CFStringRef        RunMode = (CFStringRef)Key;
            CFRunLoopSourceRef Source  = (CFRunLoopSourceRef)Value;
			CFRunLoopRemoveSource(RunLoop, Source, RunMode);
		}
    }

	static void Signal(const void* Key, const void* Value, void* Context)
	{
		CFRunLoopSourceRef RunLoopSource = (CFRunLoopSourceRef)Value;
		if(RunLoopSource)
		{
			CFRunLoopSourceSignal(RunLoopSource);
		}
	}

    static void Perform(void* Info)
    {
        FRunLoopSourceContext* RunLoopSource = reinterpret_cast<FRunLoopSourceContext*>(Info);
        if (RunLoopSource)
        {
            RunLoopSource->Execute();
        }
    }
    
    CFRunLoopRef           RunLoop;
    CFMutableDictionaryRef SourceAndModeDictionary;
    
    // Blocks that are going to be executed
    FSpinLock                BlockLock;
    TArray<dispatch_block_t> Blocks;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRunLoopSource

@implementation FRunLoopSource

- (id)initWithSource:(FRunLoopSourceContext*)InSource
{
	id Self = [super init];
	if(Self)
	{
		Check(InSource);
		Source = InSource;
		Source->AddRef();
	}

	return Self;
}

- (void)dealloc
{
	check(Source);
	Source->Release();
	Source = nullptr;
	[super dealloc];
}

- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
	check(!RunLoop);
	check(!Mode);
	RunLoop = InRunLoop;
	Mode = InMode;
}

- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
	if(CFEqual(InRunLoop, RunLoop) && CFEqual(Mode, InMode))
	{
		RunLoop = nullptr;
		Mode = nullptr;
	}
}

- (void)perform
{
	check(Source);
	check(RunLoop);
	check(Mode);
	check(CFEqual(RunLoop, CFRunLoopGetCurrent()));
	CFStringRef CurrentMode = CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent());
	check(CFEqual(CurrentMode, Mode));
	Source->Process(CurrentMode);
	CFRelease(CurrentMode);
}

@end

#pragma clang diagnostic pop

// Main-thread runloop source
FRunLoopSourceContext* GMainThread = nullptr;

bool RegisterMainRunLoop()
{
    CFRunLoopRef MainLoop = CFRunLoopGetMain();
    GMainThread = new FRunLoopSourceContext(MainLoop, NSDefaultRunLoopMode);
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

