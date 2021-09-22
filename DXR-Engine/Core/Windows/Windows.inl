#pragma once

#if defined(PLATFORM_WINDOWS)

// Included here since MSVC still cannot find the log-macro when right above this file (Retarded compiler?)
#include "Core/Application/Log.h"

/* Helper for retrieving a function from DLL */

template<typename T>
inline T GetTypedProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
    T Func = reinterpret_cast<T>(GetProcAddress( hModule, lpProcName ));
    if ( !Func )
    {
        const char* ProcName = lpProcName;
        LOG_ERROR( "Failed to load " + std::string( ProcName ) );
    }

    return Func;
}

#endif