#pragma once
#include "Windows.h"

#include "Core/Generic/GenericPlatformStackTrace.h"

struct CORE_API FWindowsPlatformStackTrace 
    : public FGenericPlatformStackTrace
{
    static bool InitializeSymbols();

    static void ReleaseSymbols();

    static TArray<FStackTraceEntry> GetStack(int32 MaxDepth = 128);

    static FORCEINLINE FString GetSymbolPath()
    {
        FString Path;// = GetExecutableFilename();
        return Path;
    }

    static FORCEINLINE FString GetExecutableFilename()
    {
        CHAR ModulePathName[FStackTraceEntry::MaxNameLength + 1];
        ::GetModuleFileNameA(::GetModuleHandleA(nullptr), ModulePathName, FStackTraceEntry::MaxNameLength);
        return FString(ModulePathName);
    }
};