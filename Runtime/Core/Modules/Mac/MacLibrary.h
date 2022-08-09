#pragma once
#include "Core/Containers/String.h"
#include "Core/Modules/Generic/GenericLibrary.h"

#include <dlfcn.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacLibrary

class FMacLibrary final : public FGenericLibrary
{
public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericLibrary Interface

    static FORCEINLINE void* LoadDynamicLib(const char* LibraryName)
    {
        FString RealName = GetRealName(LibraryName);

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY;
#else
        const int32 Mode = RTLD_NOW;
#endif
		
		const char* LibraryNameWithExtension = RealName.GetCString();
        return dlopen(LibraryNameWithExtension, Mode);
    }

    static FORCEINLINE void* GetLoadedHandle(const char* LibraryName)
    { 
        FString RealName = GetRealName(LibraryName);

#if ENABLE_LIBRARY_LAZY_MODE
        const int32 Mode = RTLD_LAZY | RTLD_NOLOAD;
#else
        const int32 Mode = RTLD_NOW | RTLD_NOLOAD;
#endif
        const char* LibraryNameWithExtension = RealName.GetCString();
        void* Handle = dlopen(LibraryNameWithExtension, Mode);
        
        // Handle is ref-counted so release the new ref-count in order to have parity with windows
        if (Handle)
        {
            dlclose(Handle);
        } 
        
        return Handle;
    }

    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle)
    {
        dlclose(LibraryHandle);
    }

    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, void* LibraryHandle)
    { 
        return dlsym(LibraryHandle, SymbolName);
    }

    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    static FORCEINLINE FString GetRealName(const char* LibraryName) 
    { 
        // TODO: MacOS seems to prefix all libraries with lib*, it would be nice if we could decide if this should happen or not
        return FString("lib") + LibraryName + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName)
    { 
        return (GetLoadedHandle(LibraryName) != nullptr);
    }

    template<typename T>
    static FORCEINLINE T LoadSymbolAddress(const char* SymbolName, void* LibraryHandle)
    { 
        return reinterpret_cast<T>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};
