#pragma once
#include "Core/Application/Generic/GenericMisc.h"

#include "Windows.h"

#ifdef MessageBox
#undef MessageBox
#endif

class WindowsMisc : public GenericMisc
{
public:
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
        MessageBoxA( 0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK );
    }

    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
        PostQuitMessage( ExitCode );
    }

    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
        OutputDebugStringA( Message.c_str() );
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return IsDebuggerPresent();
    }
};