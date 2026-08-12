#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

#define MAX_STACK_DEPTH (128)

struct FStackTraceEntry
{
    inline static constexpr uint32 MaxNameLength = 512;

    CHAR   FunctionName[MaxNameLength];
    CHAR   Filename[MaxNameLength];
    CHAR   ModuleName[MaxNameLength];
    uint32 Line;
};

struct CORE_API IPlatformStackTrace
{
    /** @return Returns true if the debug symbols needed to resolve addresses were loaded */
    static FORCEINLINE bool InitializeSymbols()
    {
        return true;
    }

    /** @brief Release the debug symbols loaded by InitializeSymbols */
    static FORCEINLINE void ReleaseSymbols()
    {
    }

    /** @return Returns the number of addresses written, dropping the IgnoreCount innermost frames */
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount);

    /** @return Returns the number of return addresses captured for the calling thread, at most MaxDepth */
    static FORCEINLINE int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth)
    {
        return 0;
    }

    /** @brief Resolve a return address into the function, file, line and module it belongs to */
    static FORCEINLINE void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
    {
    }

    /** @return Returns the resolved call-stack of the calling thread, loading and releasing symbols around the capture */
    static TArray<FStackTraceEntry> GetStack(int32 MaxDepth = 128, int32 IgnoreCount = 0);

    /** @return Returns the full path of the running executable */
    static FORCEINLINE String GetExecutableFilename()
    {
        return String();
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
