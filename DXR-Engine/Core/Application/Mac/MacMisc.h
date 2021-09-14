#pragma once
#include "Core/Application/Generic/GenericMisc.h"
#include "Core/Application/Mac/MacOutputConsole.h"

class CMacMisc : public CGenericMisc
{
public:

    /* Creates an output console if the platform supports it */
    static FORCEINLINE class CGenericOutputConsole* CreateOutputConsole()
    {
        return new CMacOutputConsole();
    }

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __builtin_trap();
    }

    /* Outputs a debug string to the attached debugger */
    static void OutputDebugString( const std::string& Message );

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return false;
    }
};
