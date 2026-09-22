#pragma once
#include "Core/Mac/Mac.h"
#include "Core/PlatformInterface/IPlatformMisc.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct FMacPlatformMisc final : public IPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
        if (Message && Message[0] != '\0')
        {
            WriteToStdOutput(Message, static_cast<uint32>(strlen(Message)));
        }

        WriteToStdOutput("\n", 1);
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

    static void InstallCrashHandler();
    static void PrepareMetalDebugLayerEnvironment(bool bEnableDebugLayer);

    static FORCEINLINE void MemoryBarrier() 
    {
        __sync_synchronize();
    }

    static FORCEINLINE bool GetEnvironmentVariable(const CHAR* Name, String& OutValue)
    {
        OutValue.Clear();

        if (!Name || Name[0] == '\0')
        {
            return false;
        }

        const CHAR* Value = ::getenv(Name);
        if (!Value)
        {
            return false;
        }

        OutValue.Append(Value);
        return true;
    }

    static FORCEINLINE bool SetEnvironmentVariable(const CHAR* Name, const CHAR* Value)
    {
        if (!Name || Name[0] == '\0')
        {
            return false;
        }

        if (!Value)
        {
            return ::unsetenv(Name) == 0;
        }

        return ::setenv(Name, Value, 1) == 0;
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
