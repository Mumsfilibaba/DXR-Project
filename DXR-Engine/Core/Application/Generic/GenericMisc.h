#pragma once
#include "Core.h"

#ifdef MessageBox
#undef MessageBox
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif


class GenericMisc
{
public:

    /* Takes the title of the messagebox and the message to be displayed */
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
    }

    /* Sends a Exit Message to the application with a certain exitcode */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
    }

    static FORCEINLINE void DebugBreak()
    {
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return false;
    }
};


#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
