#pragma once
#include "Core/Core.h"

#include <CoreFoundation/CoreFoundation.h>

/**
 * Interface for creating the Mac main-thread runloop which enables us to run blocks of code on the
 * main-thread from other threads
 */

/** @brief: Create the MainThread's RunLoop */
CORE_API bool RegisterMainRunLoop();

/** @brief: Perform a call on the MainThread */
CORE_API void MakeMainThreadCall(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);

/** @brief: Perform a call on the MainThread and wait for a returnvalue */
template<typename ReturnType>
inline ReturnType MakeMainThreadCallWithReturn(ReturnType (^Block)(void))
{
	__block ReturnType ReturnValue;
	MakeMainThreadCall(^
	{
		ReturnValue = Block();
	}, NSDefaultRunLoopMode, true);
	
	return ReturnValue;
}
