#pragma once
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplication.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef OutputDebugString
    #undef OutputDebugString
#endif

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericPlatformMisc

struct FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const FString& Message) { }

    static FORCEINLINE bool IsDebuggerPresent() { return false; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif

