#pragma once
#include "Core/Generic/GenericPlatformMisc.h"

#include <sys/sysctl.h>


struct FMacPlatformMisc final 
    : public FGenericPlatformMisc
{
    static void OutputDebugString(const FString& Message);

    static FORCEINLINE bool IsDebuggerPresent()
    {
        // See: https://developer.apple.com/library/archive/qa/qa1361/_index.html for original implementation
        int32 Mib[4]
        {
            CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()
        };

        struct kinfo_proc Info;
        Info.kp_proc.p_flag = 0;

        size_t Size = sizeof(Info);
        const int32 Junk = sysctl(Mib, sizeof(Mib) / sizeof(*Mib), &Info, &Size, nullptr, 0);
        CHECK(Junk == 0);

        return ((Info.kp_proc.p_flag & P_TRACED) != 0);
    }
};
