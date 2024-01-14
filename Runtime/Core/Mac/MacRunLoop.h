#pragma once
#include "Core/Core.h"

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

#define APPLICATION_THREAD_ENABLED (1)

/** @brief - Extend NSThread in order to check for the ApplicationThread in a similar way as the MainThread */
@interface NSThread (FApplicationThread)
+(NSThread*) applicationThread;
+(BOOL) isApplicationThread;
-(BOOL) isApplicationThread;
@end

/** @brief - Create a subclass for the ApplicationThread */
@interface FApplicationThread : NSThread
-(id) init;
-(id) initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument;
-(void) main;
-(void) dealloc;
@end

/**
 * Interface for creating the Mac main-thread runloop which enables us to run blocks of code on the
 * main-thread from other threads
 */

/** @brief - Setup and start the ApplicationThread and initialize the Main- and ApplicationThread's RunLoop */
CORE_API bool SetupApplicationThread(id Delegate, SEL ApplicationThreadEntry);

/** @brief - Release and Shutdown the ApplicationThread */
CORE_API void ShutdownApplicationThread();

/** @brief - Run the ApplicationThread's RunLoop and ensure there are no pending events */
CORE_API void PumpMessagesApplicationThread(bool bUntilEmpty);

/** @brief - Perform a call on the MainThread */
CORE_API void ExecuteOnMainThread(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);

/** @brief - Perform a call on the ApplicationThread */
CORE_API void ExecuteOnAppThread(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);

/** @brief - Perform a call on the MainThread and wait for a returnvalue */
template<typename ReturnType>
inline ReturnType ExecuteOnMainThreadAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
{
    __block ReturnType ReturnValue;
    ExecuteOnMainThread(^
    {
        ReturnValue = Block();
    }, WaitMode, true);
    
    return ReturnValue;
}

/** @brief - Perform a call on the ApplicationThread and wait for a returnvalue */
template<typename ReturnType>
inline ReturnType ExecuteOnAppThreadAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
{
    __block ReturnType ReturnValue;
    ExecuteOnMainThread(^
    {
        ReturnValue = Block();
    }, WaitMode, true);
    
    return ReturnValue;
}
