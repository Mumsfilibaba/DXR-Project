#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Generic/GenericPlatformStackTrace.h"

#include <mach-o/dyld.h>
#include <sys/syslimits.h>

struct CORE_API FMacPlatformStackTrace final : public FGenericPlatformStackTrace
{
    using FGenericPlatformStackTrace::GetStack;
    using FGenericPlatformStackTrace::CaptureStackTrace;

    static bool InitializeSymbols();
    static void ReleaseSymbols();
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth);
    static void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry);

    static FORCEINLINE String GetExecutableFilename()
    {
        CHAR   PathBuffer[PATH_MAX] = {};
        uint32 BufferSize           = sizeof(PathBuffer);

        if (::_NSGetExecutablePath(PathBuffer, &BufferSize) != 0)
        {
            return String();
        }

        return String(PathBuffer);
    }
};
