#pragma once
#include "Mac.h"
#include "Core/Generic/GenericPlatformStackTrace.h"

struct CORE_API FMacPlatformStackTrace final : public FGenericPlatformStackTrace
{
    using FGenericPlatformStackTrace::GetStack;
    using FGenericPlatformStackTrace::CaptureStackTrace;

    static bool InitializeSymbols();

    static void ReleaseSymbols();

    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth);

    static void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry);
};
