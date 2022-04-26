#pragma once
#include "Core/Containers/String.h"
#include "Core/Windows/Windows.h"
#include "Core/Modules/Generic/GenericLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsLibrary

class CWindowsLibrary final : public CGenericLibrary
{
public:

    typedef HMODULE PlatformHandle;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericLibrary Interface

    static FORCEINLINE PlatformHandle LoadDynamicLib(const char* LibraryName)
    {
        String RealName = GetRealName(LibraryName);
        return LoadLibraryA(RealName.CStr());
    }

    static FORCEINLINE PlatformHandle GetLoadedHandle(const char* LibraryName)
    {
        String RealName = GetRealName(LibraryName);
        return GetModuleHandleA(RealName.CStr());
    }

    static FORCEINLINE void FreeDynamicLib(PlatformHandle LibraryHandle)
    {
        FreeLibrary(LibraryHandle);
    }

    static FORCEINLINE void* LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle)
    {
        return GetProcAddress(LibraryHandle, SymbolName);
    }

    static FORCEINLINE const char* GetDynamicLibExtension()
    {
        return ".dll";
    }

    static FORCEINLINE String GetRealName(const char* LibraryName)
    {
        return String(LibraryName) + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const char* LibraryName)
    {
        return (GetLoadedHandle(LibraryName) != nullptr);
    }

    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbolAddress(const char* SymbolName, PlatformHandle LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbolAddress(SymbolName, LibraryHandle));
    }
};