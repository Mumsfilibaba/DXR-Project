#pragma once
#include "Core/Core.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericLibrary
{
    /**
     * @brief Load a dynamic library on the platform 
     * @param LibraryName Name of the library to load without extension or prefixes
     * @return A native handle to a dynamic library
     */
    static FORCEINLINE void* LoadDynamicLib(const CHAR* LibraryName) { return nullptr; }

    /**
     * @brief Retrieves a handle a dynamic handle if the library is already loaded into the application
     * @param LibraryName Name of the library to load without extension or prefixes
     * @return A native handle to a dynamic library
     */
    static FORCEINLINE void* GetLoadedHandle(const CHAR* LibraryName) { return nullptr; }

    /**
     * @brief Free a dynamic library on the platform 
     * @param LibraryHandle Handle to the dynamic library unload from the application
     */
    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle) { }

    /**
     * @brief Loads a function or variable with specified name from the specified library 
     * @param SymbolName Name of the symbol to load
     * @param LibraryHandle Handle to the library to load from
     * @return Returns a pointer to the symbol in the dynamic library
     */
    static FORCEINLINE void* LoadSymbol(const CHAR* SymbolName, void* LibraryHandle) { return nullptr; }

    /**
     * @brief Loads a typed function or variable from with specified name from the specified library
     * @param SymbolName Name of the symbol to load
     * @param LibraryHandle Handle to the library to load from
     * @return Returns a pointer to the symbol in the dynamic library
     */
    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbol(SymbolName, LibraryHandle));
    }

    /**
     * @brief Retrieve the extension that dynamic libraries use on the platform 
     * @return Returns the extension that is used on the platform for dynamic libraries
     */
    static FORCEINLINE const CHAR* GetDynamicLibExtension() { return ""; }

    /**
     * @brief Retrieve the full name for the library including prefixes and extension 
     * @param LibraryName Name of the library without extension or prefixes
     */
    static FORCEINLINE FString GetRealName(const CHAR* LibraryName) { return ""; }

    /**
     * @brief Check if the dynamic library is already loaded into the application 
     * @param LibraryName Name of the library without extension or prefixes
     * @return Returns true if the library is loaded into the application
     */
    static FORCEINLINE bool IsLibraryLoaded(const CHAR* LibraryName) { return false; }

};

ENABLE_UNREFERENCED_VARIABLE_WARNING
