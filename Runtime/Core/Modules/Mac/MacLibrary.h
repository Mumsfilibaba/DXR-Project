#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/String.h"
#include "Core/Modules/Interface/PlatformLibrary.h"

#include <dlfcn.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

class CMacLibrary final : public CPlatformLibrary
{
public:
    
    typedef void* PlatformHandle;

    /* Load a dynamic library on the platform */
    static FORCEINLINE PlatformHandle LoadDynamicLib( const char* LibraryName ) 
    {
		// TODO: MacOS seems to prefix all libraries with lib*, it would be nice if we could decide if this should happen or not
        CString CombinedName = CString("lib") + LibraryName;
        CombinedName.Append( GetDynamicLibExtension() );

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY;
#else
        const int32 Mode = RTLD_NOW;
#endif
		
		const char* LibraryNameWithExtension = CombinedName.CStr();
        return dlopen( LibraryNameWithExtension, Mode );
    }

    /* Free a dynamic library on the platform */
    static FORCEINLINE void FreeDynamicLib( PlatformHandle LibraryHandle )
    {
        dlclose( LibraryHandle );
    }

    /* Loads a function or variable with specified name from the specified library */
    static FORCEINLINE void* LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) 
    { 
        return dlsym( LibraryHandle, SymbolName );
    }

    /* Retrive the extension that dynamic libraries use on the platform */
    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    /* Loads a typed function or variable from with specified name from the specified library */
    template<typename T>
    static FORCEINLINE T LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) 
    { 
        return reinterpret_cast<T>(LoadSymbolAddress( SymbolName, LibraryHandle ));
    }
};

#endif
