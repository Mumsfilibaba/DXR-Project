#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/String.h"
#include "Core/Modules/Interface/PlatformLibrary.h"
#include "Core/Windows/Windows.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows implementation for loading a dynamic library

class CWindowsLibrary final : public CPlatformLibrary
{
public:

    typedef HMODULE PlatformHandle;

    /**
     * Load a dynamic library on the platform
     *
     * @param LibraryName: Name of the library to load without extension or prefixes
     * @return: A native handle to a dynamic library
     */
    static FORCEINLINE PlatformHandle LoadDynamicLib(const char* LibraryName)
    {
        CString RealName = GetRealName(LibraryName);
        return LoadLibraryA(RealName.CStr());
    }

    /**
     * Retrieves a handle a dynamic handle if the library is already loaded into the application
     *
     * @param LibraryName: Name of the library to load without extension or prefixes
     * @return: A native handle to a dynamic library
     */
    static FORCEINLINE PlatformHandle GetLoadedHandle(const char* LibraryName)
    {
        CString RealName = GetRealName(LibraryName);
        return GetModuleHandleA(RealName.CStr());
    }

    /**
     * Free a dynamic library on the platform
     *
     * @param LibraryHandle: Handle to the dynamic library unload from the application
     */
    static FORCEINLINE void FreeDynamicLib(PlatformHandle LibraryHandle)
    {
        FreeLibrary(LibraryHandle);
    }

    /**
     * Loads a function or variable with specified name from the specified library
     *
     * @param SymbolName: Name of the symbol to load
     * @param LibraryHandle: Handle to the library to load from
     * @return: Returns a pointer to the symbol in the dynamic library
     */
    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle)
    {
        return GetProcAddress(LibraryHandle, SymbolName);
    }

    /**
     * Retrieve the extension that dynamic libraries use on the platform
     *
     * @return: Returns the extension that is used on the platform for dynamic libraries
     */
    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dll";
    }

    /**
     * Retrieve the full name for the library including prefixes and extension
     *
     * @param LibraryName: Name of the library without extension or prefixes
     */
    static FORCEINLINE CString GetRealName(const char* LibraryName)
    {
        return CString(LibraryName) + GetDynamicLibExtension();
    }

    /**
     * Check if the dynamic library is already loaded into the application
     *
     * @param LibraryName: Name of the library without extension or prefixes
     * @return: Returns true if the library is loaded into the application
     */
    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName)
    {
        return (GetLoadedHandle(LibraryName) != nullptr);
    }

    /**
     * Loads a typed function or variable from with specified name from the specified library
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

#endif