#pragma once
#include "Core/Platform/PlatformMisc.h"

#ifdef OutputDebugString
     #undef OutputDebugString
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for easy access to debugging functions 

class CDebug
{
public:
    static FORCEINLINE void DebugBreak()
    {
        PlatformMisc::DebugBreak();
    }

    static FORCEINLINE void OutputDebugString(const FString& Message)
    {
        PlatformMisc::OutputDebugString(Message);
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformMisc::IsDebuggerPresent();
    }
};
