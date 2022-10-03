#pragma once
#include "Core/Core.h"

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

/**
 * Interface for creating the Mac main-thread runloop which enables us to run blocks of code on the
 * main-thread from other threads
 */

/** @brief: Create the MainThread's RunLoop */
CORE_API bool RegisterMainRunLoop();

/** @brief: Perform a call on the MainThread */
CORE_API void ExecuteOnMainThread(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);

/** @brief: Perform a call on the MainThread and wait for a returnvalue */
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
