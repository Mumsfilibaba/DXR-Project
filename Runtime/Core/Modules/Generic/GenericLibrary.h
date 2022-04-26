#pragma once
#include "Core/Core.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericLibrary

class CGenericLibrary
{
public:

    typedef void* PlatformHandle;

    /**
     * @brief: Load a dynamic library on the platform 
     * 
     * @param LibraryName: Name of the library to load without extension or prefixes
     * @return: A native handle to a dynamic library
     */
    static FORCEINLINE PlatformHandle LoadDynamicLib(const char* LibraryName) { return nullptr; }

    /**
     * @brief: Retrieves a handle a dynamic handle if the library is already loaded into the application
     *
     * @param LibraryName: Name of the library to load without extension or prefixes
     * @return: A native handle to a dynamic library
     */
    static FORCEINLINE PlatformHandle GetLoadedHandle(const char* LibraryName) { return nullptr; }

    /**
     * @brief: Free a dynamic library on the platform 
     * 
     * @param LibraryHandle: Handle to the dynamic library unload from the application
     */
    static FORCEINLINE void FreeDynamicLib(PlatformHandle LibraryHandle) { }

    /**
     * @brief: Loads a function or variable with specified name from the specified library 
     * 
     * @param SymbolName: Name of the symbol to load
     * @param LibraryHandle: Handle to the library to load from
     * @return: Returns a pointer to the symbol in the dynamic library
     */
    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle) { return nullptr; }

    /**
     * @brief: Retrieve the extension that dynamic libraries use on the platform 
     * 
     * @return: Returns the extension that is used on the platform for dynamic libraries
     */
    static FORCEINLINE const char* GetDynamicLibExtension() { return ""; }

    /**
     * @brief: Retrieve the full name for the library including prefixes and extension 
     * 
     * @param LibraryName: Name of the library without extension or prefixes
     */
    static FORCEINLINE String GetRealName(const char* LibraryName) { return ""; }

    /**
     * @brief: Check if the dynamic library is already loaded into the application 
     * 
     * @param LibraryName: Name of the library without extension or prefixes
     * @return: Returns true if the library is loaded into the application
     */
    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName) { return false; }

    /**
     * @brief: Loads a typed function or variable from with specified name from the specified library
     * 
     * @param SymbolName: Name of the symbol to load
     * @param LibraryHandle: Handle to the library to load from
     * @return: Returns a pointer to the symbol in the dynamic library
     */
    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
