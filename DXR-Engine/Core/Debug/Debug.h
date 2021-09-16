#pragma once
#include "Core/Application/Platform/PlatformDebugMisc.h"

#ifdef OutputDebugString
#undef OutputDebugString
#endif

class Debug
{
public:
    static FORCEINLINE void DebugBreak()
    {
        PlatformDebugMisc::DebugBreak();
    }

    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
        PlatformDebugMisc::OutputDebugString( Message );
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformDebugMisc::IsDebuggerPresent();
    }
};