#pragma once
#include "Core/Platform/PlatformMisc.h"

#ifdef OutputDebugString
     #undef OutputDebugString
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDebug - Class for easy access to debugging functions 

struct FDebug
{
    static FORCEINLINE void OutputDebugString(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message);
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return FPlatformMisc::IsDebuggerPresent();
    }
};
