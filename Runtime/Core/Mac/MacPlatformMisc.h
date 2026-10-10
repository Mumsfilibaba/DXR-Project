#pragma once
#include "Core/Mac/Mac.h"
#include "Core/PlatformInterface/IPlatformMisc.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct FMacPlatformMisc final : public IPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR_T* Message)
    {
        if (Message && Message[0] != '\0')
        {
            WriteToStdOutput(Message, static_cast<uint32>(strlen(Message)));
        }

        WriteToStdOutput(TEXT("\n"), 1);
    }

    static FORCEINLINE void WriteToStdOutput(const CHAR_T* Text, uint32 Length)
    {
        if (Length == 0)
        {
            return;
        }

        (void)::write(STDOUT_FILENO, Text, Length);
    }

    static bool IsDebuggerPresent();

    static EAssertDialogResult ShowAssertDialog(const CHAR_T* Title, const CHAR_T* Message);

    static void InstallCrashHandler();
    static void PrepareMetalDebugLayerEnvironment(bool bEnableDebugLayer, bool bEnableShaderValidation = false);

    NODISCARD static FGuid CreateGuid();

    static FORCEINLINE void MemoryBarrier() 
    {
        __sync_synchronize();
    }

    static FORCEINLINE bool GetEnvironmentVariable(const CHAR_T* Name, FString& OutValue)
    {
        OutValue.Clear();

        if (!Name || Name[0] == '\0')
        {
            return false;
        }

        const CHAR_T* Value = ::getenv(Name);
        if (!Value)
        {
            return false;
        }

        OutValue.Append(Value);
        return true;
    }

    static FORCEINLINE bool SetEnvironmentVariable(const CHAR_T* Name, const CHAR_T* Value)
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

    static FORCEINLINE int32 GetLastErrorString(FString& OutErrorString)
    {
        const int32 LastError = errno;

        // The XSI strerror_r, which macOS provides, returns an int rather than the GNU variant's pointer
        CHAR_T MessageBuffer[256] = {};
        if (::strerror_r(LastError, MessageBuffer, sizeof(MessageBuffer)) != 0)
        {
            MessageBuffer[0] = 0;
        }

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer);
        return LastError;
    }
};
