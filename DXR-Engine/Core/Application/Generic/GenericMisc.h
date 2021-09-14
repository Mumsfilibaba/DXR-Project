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

// TODO: Remove
#include <string>

class CGenericMisc
{
public:

    /* Creates an output console if the platform supports it */
    static FORCEINLINE class CGenericOutputConsole* CreateOutputConsole()
    {
        return nullptr;
    }

    /* Takes the title of the messagebox and the message to be displayed */
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
    }

    /* Sends a Exit Message to the application with a certain exitcode */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
    }

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
    }

    /* Checks weather or not the application is running inside a debugger */
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
