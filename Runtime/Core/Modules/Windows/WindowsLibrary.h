#pragma once
#include "Core/Containers/String.h"
#include "Core/Windows/Windows.h"
#include "Core/Modules/Generic/GenericLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsLibrary

class CWindowsLibrary final : public CGenericLibrary
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericLibrary Interface

    static FORCEINLINE void* LoadDynamicLib(const char* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(LoadLibraryA(RealName.CStr()));
    }

    static FORCEINLINE void* GetLoadedHandle(const char* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(GetModuleHandleA(RealName.CStr()));
    }

    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle)
    {
        FreeLibrary(reinterpret_cast<HMODULE>(LibraryHandle));
    }

    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, void* LibraryHandle)
    {
        return GetProcAddress(reinterpret_cast<HMODULE>(LibraryHandle), SymbolName);
    }

    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dll";
    }

    static FORCEINLINE FString GetRealName(const char* LibraryName)
    {
        return FString(LibraryName) + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName)
    {
        return (GetLoadedHandle(LibraryName) != nullptr);
    }

    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbolAddress(const char* SymbolName, void* LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};