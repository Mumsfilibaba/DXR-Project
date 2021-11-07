#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/String.h"
#include "Core/Modules/Interface/PlatformLibrary.h"
#include "core/Windows/Windows.h"

class CWindowsLibrary final : public CPlatformLibrary
{
public:
    
    typedef HMODULE PlatformHandle;

    /* Load a dynamic library on the platform */
    static FORCEINLINE PlatformHandle LoadDynamicLib( const char* LibraryName ) 
    { 
        CString CombinedName = LibraryName;
        CombinedName.Append( GetDynamicLibExtension() );
        return LoadLibraryA( CombinedName.CStr() );
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

    /* Retrieve the extension that dynamic libraries use on the platform */
    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dll";
    }

    /* Loads a typed function or variable from with specified name from the specified library */
    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle )
    { 
        return reinterpret_cast<SymbolType>(LoadSymbolAddress( SymbolName, LibraryHandle ));
    }
};

#endif