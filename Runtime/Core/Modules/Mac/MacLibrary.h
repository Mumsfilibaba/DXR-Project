#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/String.h"
#include "Core/Modules/Interface/PlatformLibrary.h"

#include <dlfcn.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac implementation for loading a dynamic library

class CMacLibrary final : public CPlatformLibrary
{
public:
    
    typedef void* PlatformHandle;

    /**
     * Load a dynamic library on the platform 
     * 
     * @param LibraryName: Name of the library to load without extension or prefixes
     * @return: A native handle to a dynamic library
     */
    static FORCEINLINE PlatformHandle LoadDynamicLib(const char* LibraryName) 
    {
        CString RealName = GetRealName(LibraryName);

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY;
#else
        const int32 Mode = RTLD_NOW;
#endif
		
		const char* LibraryNameWithExtension = RealName.CStr();
        return dlopen(LibraryNameWithExtension, Mode);
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

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY | RTLD_NOLOAD;
#else
        const int32 Mode = RTLD_NOW | RTLD_NOLOAD;
#endif
        const char* LibraryNameWithExtension = RealName.CStr();
        PlatformHandle Handle = dlopen(LibraryNameWithExtension, Mode);
        
        // Handle is ref-counted so release the new ref-count in order to have parity with windows
        if (Handle)
        {
            dlclose(Handle);
        } 
        
        return Handle;
    }

    /**
     * Free a dynamic library on the platform 
     * 
     * @param LibraryHandle: Handle to the dynamic library unload from the application
     */
    static FORCEINLINE void FreeDynamicLib(PlatformHandle LibraryHandle)
    {
        dlclose(LibraryHandle);
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
        return dlsym(LibraryHandle, SymbolName);
    }

    /**
     * Retrieve the extension that dynamic libraries use on the platform 
     * 
     * @return: Returns the extension that is used on the platform for dynamic libraries
     */
    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    /**
     * Retrieve the full name for the library including prefixes and extension 
     * 
     * @param LibraryName: Name of the library without extension or prefixes
     */
    static FORCEINLINE CString GetRealName(const char* LibraryName) 
    { 
        // TODO: MacOS seems to prefix all libraries with lib*, it would be nice if we could decide if this should happen or not
        return CString("lib") + LibraryName + GetDynamicLibExtension();
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
    template<typename T>
    static FORCEINLINE T LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle) 
    { 
        return reinterpret_cast<T>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};

#endif
