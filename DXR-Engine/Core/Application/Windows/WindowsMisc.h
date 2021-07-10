#pragma once
#include "Core/Application/Generic/GenericMisc.h"

#include "Windows.h"

#ifdef MessageBox
#undef MessageBox
#endif

class WindowsMisc : public GenericMisc
{
public:
    FORCEINLINE static void MessageBox( const std::string& Title, const std::string& Message )
    {
        MessageBoxA( 0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK );
    }

    FORCEINLINE static void RequestExit( int32 ExitCode )
    {
        PostQuitMessage( ExitCode );
    }

    FORCEINLINE static void DebugBreak()
    {
        __debugbreak();
    }

    FORCEINLINE static void OutputDebugString( const std::string& Message )
    {
        OutputDebugStringA( Message.c_str() );
    }

    FORCEINLINE static bool IsDebuggerPresent()
    {
        return IsDebuggerPresent();
    }
};