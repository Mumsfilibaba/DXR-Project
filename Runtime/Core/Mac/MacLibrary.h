#pragma once
#include "Core/Containers/String.h"
#include "Core/Generic/GenericLibrary.h"

#include <dlfcn.h>

struct CORE_API FMacLibrary final : public FGenericLibrary
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

    static FORCEINLINE const CHAR* GetDynamicLibPrefix()
    {
        return "lib";
    }
    
    static FORCEINLINE const CHAR* GetDynamicLibExtension()
    {
        return ".dylib";
    }

    static FORCEINLINE FString GetRealName(const CHAR* LibraryName) 
    {
        return FString("lib") + LibraryName + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const CHAR* LibraryName)
    { 
        return GetLoadedHandle(LibraryName) != nullptr;
    }

    template<typename T>
    static FORCEINLINE T LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    { 
        return reinterpret_cast<T>(LoadSymbol(SymbolName, LibraryHandle));
    }
};
