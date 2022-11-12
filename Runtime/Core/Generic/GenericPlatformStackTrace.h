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

struct FStackTraceEntry
{
    enum { MaxNameLength = 1024 };

    CHAR Name[MaxNameLength];
    CHAR File[MaxNameLength];

    uint32 Line;
};

struct CORE_API FGenericPlatformStackTrace
{
    static FORCEINLINE bool InitializeSymbols() { return true; }

    static FORCEINLINE void ReleaseSymbols() { } 

    static FORCEINLINE TArray<FStackTraceEntry> GetStack(int32 MaxDepth) 
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