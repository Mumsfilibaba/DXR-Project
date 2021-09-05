#pragma once
#include "Core/Application/Platform/PlatformMisc.h"

#ifdef OutputDebugString
#undef OutputDebugString
#endif

class Debug
{
public:
    static FORCEINLINE void DebugBreak()
    {
        PlatformMisc::DebugBreak();
    }

    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
        PlatformMisc::OutputDebugString( Message );
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformMisc::IsDebuggerPresent();
    }
};