#pragma once
#include "Core.h"

#ifdef MessageBox
#undef MessageBox
#endif

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericMisc
{
public:
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
    }

    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
    }

    static FORCEINLINE void DebugBreak()
    {
    }

    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return false;
    }
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
