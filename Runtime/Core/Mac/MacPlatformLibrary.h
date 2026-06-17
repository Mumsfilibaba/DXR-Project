#pragma once
#include "Core/Containers/String.h"
#include "Core/Generic/GenericPlatformLibrary.h"
#include <dlfcn.h>

struct CORE_API FMacPlatformLibrary final : public FGenericPlatformLibrary
{
    static void* LoadDynamicLib(const CHAR* LibraryName);
    static void* GetLoadedHandle(const CHAR* LibraryName);

    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle)
    {
        ::dlclose(LibraryHandle);
    }

    static FORCEINLINE void* LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    { 
        return ::dlsym(LibraryHandle, SymbolName);
    }

    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    { 
        return reinterpret_cast<SymbolType>(LoadSymbol(SymbolName, LibraryHandle));
    }

    static FORCEINLINE const CHAR* GetDynamicLibPrefix()
    {
        return "lib";
    }

    static FORCEINLINE const CHAR* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    static FORCEINLINE String GetRealName(const CHAR* LibraryName) 
    {
        return String("lib") + LibraryName + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const CHAR* LibraryName)
    { 
        return GetLoadedHandle(LibraryName) != nullptr;
    }
};
