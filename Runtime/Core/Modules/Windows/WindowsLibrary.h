#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/String.h"
#include "Core/Modules/Interface/PlatformLibrary.h"

#include "Windows/Windows.h"

class CWindowsLibrary final : public CPlatformLibrary
{
public:
    
    typedef HANDLE PlatformHandle;

    /* Load a dynamic library on the platform */
    static FORCEINLINE PlatformHandle LoadDynamicLib( const char* LibraryName ) 
    { 
        CString CombinedName = LibraryName;
        CombinedName.Append( GetDynamicLibExtension() );
        return LoadLibrary( LibraryName );
    }

    /* Free a dynamic library on the platform */
    static FORCEINLINE void FreeDynamicLib( PlatformHandle LibraryHandle )
    {
        FreeLibrary( LibraryHandle );
    }

    /* Loads a function or variable with specified name from the specified library */
    static FORCEINLINE void* LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) 
    { 
        return GetProcAddress( LibraryHandle, SymbolName );
    }

    /* Retrive the extension that dynamic libraries use on the platform */
    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dll";
    }

    /* Loads a typed function or variable from with specified name from the specified library */
    template<typename T>
    static FORCEINLINE T LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) 
    { 
        return reinterpret_cast<T>(LoadSymbolAddress( SymbolName, LibraryHandle ));
    }
};

#endif