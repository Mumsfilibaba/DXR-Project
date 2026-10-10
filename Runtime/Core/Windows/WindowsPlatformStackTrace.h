#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformStackTrace.h"
#include "Core/Containers/StringConversion.h"

struct CORE_API FWindowsPlatformStackTrace final : public IPlatformStackTrace
{
    using IPlatformStackTrace::GetStack;
    using IPlatformStackTrace::GetThreadStack;
    using IPlatformStackTrace::CaptureStackTrace;

    static bool InitializeSymbols();
    static void ReleaseSymbols();
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth);
    static int32 CaptureThreadStackTrace(const FThreadStackContext& ThreadContext, uint64* StackTrace, int32 MaxDepth);
    static void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry);

    static FORCEINLINE FString GetSymbolPath()
    {
        // TODO: Assumed to be the same as the executable path
        const FString Path = GetExecutableFilename();
        return Path;
    }

    static FORCEINLINE FString GetExecutableFilename()
    {
        WIDECHAR ModulePathName[FStackTraceEntry::MaxNameLength + 1] = {};
        ::GetModuleFileNameW(::GetModuleHandleW(nullptr), ModulePathName, FStackTraceEntry::MaxNameLength);
        return StringCast<CHAR_T>(ModulePathName).ToString();
    }
};
