#pragma once
#include "Mac.h"
#include "Core/Generic/GenericPlatformMisc.h"

struct FMacPlatformMisc final : public FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
        NSLog(@"%s", Message);
    }

    static bool IsDebuggerPresent();

    static FORCEINLINE void MemoryBarrier() 
    {
        _mm_sfence();
    }
};
