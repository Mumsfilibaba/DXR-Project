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

struct CORE_API FGenericPlatformStackTrace
{
    static FORCEINLINE bool InitializeSymbols() { return true; }
    static FORCEINLINE void ReleaseSymbols() { } 
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount);
    static FORCEINLINE int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth) { return 0; }
    static FORCEINLINE void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry) { }
    static TArray<FStackTraceEntry> GetStack(int32 MaxDepth = 128, int32 IgnoreCount = 0);

    static FORCEINLINE FString GetExecutableFilename()
    {
        return FString();
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
