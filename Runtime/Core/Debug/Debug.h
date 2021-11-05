#pragma once
#include "CoreApplication/Platform/PlatformDebugMisc.h"

#ifdef OutputDebugString
#undef OutputDebugString
#endif

class CDebug
{
public:
    static FORCEINLINE void DebugBreak()
    {
        PlatformDebugMisc::DebugBreak();
    }

    static FORCEINLINE void OutputDebugString( const CString& Message )
    {
        PlatformDebugMisc::OutputDebugString( Message );
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformDebugMisc::IsDebuggerPresent();
    }
};