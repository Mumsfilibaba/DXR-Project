#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Generic/GenericPlatformMisc.h"

#include <errno.h>
#include <string.h>
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

    static EAssertDialogResult ShowAssertDialog(const CHAR* Title, const CHAR* Message);

    static FORCEINLINE void MemoryBarrier() 
    {
        __sync_synchronize();
    }

    static FORCEINLINE int32 GetLastErrorString(String& OutErrorString)
    {
        const int32 LastError = errno;

        // The XSI strerror_r, which macOS provides, returns an int rather than the GNU variant's pointer
        CHAR MessageBuffer[256] = {};
        if (::strerror_r(LastError, MessageBuffer, sizeof(MessageBuffer)) != 0)
        {
            MessageBuffer[0] = 0;
        }

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer);
        return LastError;
    }
};
