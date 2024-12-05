#pragma once
#include "Core/Containers/String.h"
#include "Core/Windows/Windows.h"
#include "Core/Generic/GenericLibrary.h"

struct FWindowsLibrary final : public FGenericLibrary
{
    static FORCEINLINE void* LoadDynamicLib(const CHAR* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(::LoadLibraryA(*RealName));
    }

    static FORCEINLINE void* GetLoadedHandle(const CHAR* LibraryName)
    {
        const FString RealName = GetRealName(LibraryName);
        return reinterpret_cast<void*>(::GetModuleHandleA(*RealName));
    }

    static FORCEINLINE void FreeDynamicLib(void* LibraryHandle)
    {
        ::FreeLibrary(reinterpret_cast<HMODULE>(LibraryHandle));
    }

    static FORCEINLINE void* LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    {
        return ::GetProcAddress(reinterpret_cast<HMODULE>(LibraryHandle), SymbolName);
    }

    template<typename SymbolType>
    static FORCEINLINE SymbolType LoadSymbol(const CHAR* SymbolName, void* LibraryHandle)
    {
        return reinterpret_cast<SymbolType>(LoadSymbol(SymbolName, LibraryHandle));
    }

    static FORCEINLINE const CHAR* GetDynamicLibExtension()
    {
        return ".dll";
    }

    static FORCEINLINE FString GetRealName(const CHAR* LibraryName)
    {
        return FString(LibraryName) + GetDynamicLibExtension();
    }

    static FORCEINLINE bool IsLibraryLoaded(const CHAR* LibraryName)
    {
        return GetLoadedHandle(LibraryName) != nullptr;
    }
};