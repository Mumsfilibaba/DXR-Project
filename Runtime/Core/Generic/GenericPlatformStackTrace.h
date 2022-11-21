#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#define MAX_STACK_DEPTH (128)

struct FStackTraceEntry
{
    enum { MaxNameLength = 512 };

    CHAR FunctionName[MaxNameLength];
    CHAR Filename[MaxNameLength];
    CHAR ModuleName[MaxNameLength];

    uint32 Line;
};

struct CORE_API FGenericPlatformStackTrace
{
    static FORCEINLINE bool InitializeSymbols() { return true; }
    static FORCEINLINE void ReleaseSymbols() { } 

    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount);
    static FORCEINLINE int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth) { return 0; }

    static FORCEINLINE void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry) { }

    static FORCEINLINE TArray<FStackTraceEntry> GetStack(int32 MaxDepth = 128, int32 IgnoreCount = 0)
    { 
        return TArray<FStackTraceEntry>();
    }

    static FORCEINLINE FString GetExecutableFilename()
    {
        return FString();
    }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif