#include "Core/Platform/PlatformStackTrace.h"

int32 FGenericPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount)
{
    uint64 StaticStackTrace[MAX_STACK_DEPTH];
    
    MaxDepth = FMath::Min<int32>(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);
    
    const int32 Depth = FPlatformStackTrace::CaptureStackTrace(StaticStackTrace, MaxDepth);

    int32 DepthResult = 0;
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; ++CurrentDepth)
    {
        StackTrace[DepthResult++] = StaticStackTrace[CurrentDepth];
    }

    return DepthResult;
}

TArray<FStackTraceEntry> FGenericPlatformStackTrace::GetStack(int32 MaxDepth, int32 IgnoreCount)
{
    TArray<FStackTraceEntry> Stack;
    if (!FPlatformStackTrace::InitializeSymbols())
    {
        return Stack;
    }

    // Skip the 2 first (CaptureCallstack functions)
    IgnoreCount += 2;

    uint64 StackTrace[MAX_STACK_DEPTH];
    FMemory::Memzero(StackTrace);

    // Ensure that static buffer does not overflow
    MaxDepth = FMath::Min(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);

    const int32 Depth = CaptureStackTrace(StackTrace, MaxDepth);
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; CurrentDepth++)
    {
        FStackTraceEntry& NewEntry = Stack.Emplace();
        FPlatformStackTrace::GetStackTraceEntryFromAddress(StackTrace[CurrentDepth], NewEntry);
    }

    FPlatformStackTrace::ReleaseSymbols();
    return Stack;
}