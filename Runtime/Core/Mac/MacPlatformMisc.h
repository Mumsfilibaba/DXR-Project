#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Generic/GenericPlatformMisc.h"

#include <unistd.h>

struct FMacPlatformMisc final : public FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
        NSLog(@"%s", Message);
    }

    static FORCEINLINE void WriteToStdOutput(const CHAR* Text, uint32 Length)
    {
        if (Length == 0)
        {
            return;
        }

        (void)::write(STDOUT_FILENO, Text, Length);
    }

    static bool IsDebuggerPresent();

    static FORCEINLINE void MemoryBarrier() 
    {
        __sync_synchronize();
    }
};
