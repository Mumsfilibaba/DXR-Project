#pragma once
#include "Core/Application/Platform/PlatformDebugMisc.h"

class Debug
{
public:
    FORCEINLINE static void DebugBreak()
    {
        PlatformDebugMisc::DebugBreak();
    }

    FORCEINLINE static void OutputDebugString(const std::string& Message)
    {
        PlatformDebugMisc::OutputDebugString(Message);
    }

    FORCEINLINE static bool IsDebuggerPresent()
    {
        return PlatformDebugMisc::IsDebuggerPresent();
    }
};