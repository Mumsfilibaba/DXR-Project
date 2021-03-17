#pragma once
#include "Application/Platform/PlatformDebugMisc.h"

class Debug
{
public:
    static FORCEINLINE void DebugBreak()
    {
        PlatformDebugMisc::DebugBreak();
    }

    static FORCEINLINE void OutputDebugString(const std::string& Message)
    {
        PlatformDebugMisc::OutputDebugString(Message);
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformDebugMisc::IsDebuggerPresent();
    }
};