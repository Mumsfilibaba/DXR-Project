#pragma once
#include "Core/Containers/String.h"
#include "Core/Modules/Generic/GenericLibrary.h"

#include <dlfcn.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacLibrary

class CMacLibrary final : public CGenericLibrary
{
public:
    
    typedef void* PlatformHandle;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericLibrary Interface

    static FORCEINLINE PlatformHandle LoadDynamicLib(const char* LibraryName) 
    {
        String RealName = GetRealName(LibraryName);

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY;
#else
        const int32 Mode = RTLD_NOW;
#endif
		
		const char* LibraryNameWithExtension = RealName.CStr();
        return dlopen(LibraryNameWithExtension, Mode);
    }

    static FORCEINLINE PlatformHandle GetLoadedHandle(const char* LibraryName) 
    { 
        String RealName = GetRealName(LibraryName);

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

    static FORCEINLINE void FreeDynamicLib(PlatformHandle LibraryHandle)
    {
        dlclose(LibraryHandle);
    }

    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle) 
    { 
        return dlsym(LibraryHandle, SymbolName);
    }

    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    static FORCEINLINE String GetRealName(const char* LibraryName) 
    { 
        // TODO: MacOS seems to prefix all libraries with lib*, it would be nice if we could decide if this should happen or not
        return String("lib") + LibraryName + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName)
    { 
        return (GetLoadedHandle(LibraryName) != nullptr);
    }

    template<typename T>
    static FORCEINLINE T LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle) 
    { 
        return reinterpret_cast<T>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};
