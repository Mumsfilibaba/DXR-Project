#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformLibrary.h"

struct FWindowsPlatformLibrary final : public IPlatformLibrary
{
    static FORCEINLINE void* LoadDynamicLib(const CHAR_T* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(::LoadLibraryW(StringCast<WIDECHAR>(RealName).Get()));
    }

    static FORCEINLINE void* GetLoadedHandle(const CHAR_T* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(::GetModuleHandleW(StringCast<WIDECHAR>(RealName).Get()));
    }

    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle)
    {
        ::FreeLibrary(reinterpret_cast<HMODULE>(LibraryHandle));
    }

    static FORCEINLINE void* LoadSymbol(const CHAR_T* SymbolName, void* LibraryHandle)
    {
        return ::GetProcAddress(reinterpret_cast<HMODULE>(LibraryHandle), StringCast<ANSICHAR>(SymbolName).Get());
    }

    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbol(const CHAR_T* SymbolName, void* LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbol(SymbolName, LibraryHandle));
    }

    static FORCEINLINE const CHAR_T* GetDynamicLibExtension()
    {
        return ".dll";
    }

    static FORCEINLINE FString GetRealName(const CHAR_T* LibraryName)
    {
        return FString(LibraryName) + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const CHAR_T* LibraryName)
    {
        return GetLoadedHandle(LibraryName) != nullptr;
    }
};
