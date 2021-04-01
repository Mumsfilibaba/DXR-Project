#pragma once
#include "Core/Application/Platform/PlatformMisc.h"

#ifdef OutputDebugString
    #undef OutputDebugString
#endif

class Debug
{
public:
    FORCEINLINE static void DebugBreak()
    {
        PlatformMisc::DebugBreak();
    }

    FORCEINLINE static void OutputDebugString(const std::string& Message)
    {
        PlatformMisc::OutputDebugString(Message);
    }

    FORCEINLINE static bool IsDebuggerPresent()
    {
        return PlatformMisc::IsDebuggerPresent();
    }
};