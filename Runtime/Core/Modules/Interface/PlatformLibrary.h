#pragma once
#include "Core/Core.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CPlatformLibrary
{
public:

    typedef void* PlatformHandle;

    /* Load a dynamic library on the platform */
    static FORCEINLINE PlatformHandle LoadDynamicLib( const char* LibraryName ) { return nullptr; }

    /* Retrieves a handle a dynamic handle if the library is already loaded into the application */
    static FORCEINLINE PlatformHandle GetLoadedHandle( const char* LibraryName ) { return nullptr; }

    /* Free a dynamic library on the platform */
    static FORCEINLINE void FreeDynamicLib( PlatformHandle LibraryHandle ) {}

    /* Loads a function or variable with specified name from the specified library */
    static FORCEINLINE void* LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) { return nullptr; }

    /* Retrive the extension that dynamic libraries use on the platform */
    static FORCEINLINE const char* GetDynamicLibExtension() { return ""; }

    /* Retrieve the real name for the library including prefixes and extension */
    static FORCEINLINE CString GetRealName( const char* LibraryName ) { return ""; }

    /* Check if the dynamic library is already loaded into the application */
    static FORCEINLINE bool IsLibraryLoaded( const char* LibraryName ) { return false; }

    /* Loads a typed function or variable from with specified name from the specified library */
    template<typename T>
    static FORCEINLINE T LoadSymbolAddress( const char* SymbolName, PlatformHandle LibraryHandle ) 
    { 
        return reinterpret_cast<T>(LoadSymbolAddress( SymbolName, LibraryHandle ));
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
