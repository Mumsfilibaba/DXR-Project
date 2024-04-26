#pragma once
#include "Windows.h"
#include "Core/Generic/GenericPlatformStackTrace.h"

struct CORE_API FWindowsPlatformStackTrace final : public FGenericPlatformStackTrace
{
    using FGenericPlatformStackTrace::GetStack;
    using FGenericPlatformStackTrace::CaptureStackTrace;

    static bool InitializeSymbols();
    static void ReleaseSymbols();
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth);
    static void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry);

    static FORCEINLINE FString GetSymbolPath()
    {
        // TODO: Assumed to be the same as the executable path
        const FString Path = GetExecutableFilename();
        return Path;
    }

    static FORCEINLINE FString GetExecutableFilename()
    {
        CHAR ModulePathName[FStackTraceEntry::MaxNameLength + 1];
        ::GetModuleFileNameA(::GetModuleHandleA(nullptr), ModulePathName, FStackTraceEntry::MaxNameLength);
        return FString(ModulePathName);
    }
};