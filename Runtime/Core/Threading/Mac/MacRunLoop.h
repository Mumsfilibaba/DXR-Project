#pragma once

#if PLATFORM_MACOS
#include "Core/CoreModule.h"

#include <CoreFoundation/CoreFoundation.h>

/* Create the MainThread's RunLoop */
CORE_API bool RegisterMainRunLoop();

/* Destroy the MainThread's RunLoop */
CORE_API void UnregisterMainRunLoop();

/* Perform a call on the MainThread */
CORE_API void MakeMainThreadCall( dispatch_block_t Block, bool WaitUntilFinished );

/* Perform a call on the MainThread and wait for a returnvalue */
template<typename ReturnType>
inline ReturnType MakeMainThreadCallWithReturn( ReturnType (^Block)(void) )
{
	__block ReturnType ReturnValue;
	MakeMainThreadCall(^
	{
		ReturnValue = Block();
	}, true);
	
	return ReturnValue;
}

#endif
