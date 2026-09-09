#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformStackTrace.h"

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

    static FORCEINLINE String GetSymbolPath()
    {
        // TODO: Assumed to be the same as the executable path
        const String Path = GetExecutableFilename();
        return Path;
    }

    static FORCEINLINE String GetExecutableFilename()
    {
        CHAR ModulePathName[FStackTraceEntry::MaxNameLength + 1];
        ::GetModuleFileNameA(::GetModuleHandleA(nullptr), ModulePathName, FStackTraceEntry::MaxNameLength);
        return String(ModulePathName);
    }
};
