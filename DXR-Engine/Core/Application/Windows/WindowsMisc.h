#pragma once
#include "Core/Application/Generic/GenericMisc.h"
#include "Core/Application/Windows/WindowsOutputConsole.h"

#include "Windows.h"

#ifdef MessageBox
#undef MessageBox
#endif

class CWindowsMisc : public CGenericMisc
{
public:

    /* Creates a output console if the platform supports it */
    static FORCEINLINE CGenericOutputConsole* CreateOutputConsole()
    {
        return DBG_NEW WindowsOutputConsole();
    }

    /* Shows a message box with desired title and message */
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
        MessageBoxA( 0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK );
    }

    /* Sends a quit message which then is handled in the applications message loop */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
        PostQuitMessage( ExitCode );
    }

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
        OutputDebugStringA( Message.c_str() );
    }

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }
};