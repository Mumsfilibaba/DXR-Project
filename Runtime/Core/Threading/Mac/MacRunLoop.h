#pragma once

#if PLATFORM_MACOS
#include "Core/Core.h"

#include <CoreFoundation/CoreFoundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Interface for creating the Mac main-thread runloop which enables us to run blocks of code on the 
*  main-thread from other threads
*/

/** Create the MainThread's RunLoop */
CORE_API bool RegisterMainRunLoop();

/** Destroy the MainThread's RunLoop */
CORE_API void UnregisterMainRunLoop();

/** Perform a call on the MainThread */
CORE_API void MakeMainThreadCall(dispatch_block_t Block, bool WaitUntilFinished);

/** Perform a call on the MainThread and wait for a returnvalue */
template<typename ReturnType>
inline ReturnType MakeMainThreadCallWithReturn(ReturnType (^Block)(void))
{
	__block ReturnType ReturnValue;
	MakeMainThreadCall(^
	{
		ReturnValue = Block();
	}, true);
	
	return ReturnValue;
}

#endif
