#include "Core/Platform/PlatformStackTrace.h"

int32 IPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount)
{
    uint64 StaticStackTrace[MAX_STACK_DEPTH];
    
    MaxDepth = Math::Min<int32>(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);
    
    const int32 Depth = FPlatformStackTrace::CaptureStackTrace(StaticStackTrace, MaxDepth);

    int32 DepthResult = 0;
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; ++CurrentDepth)
    {
        StackTrace[DepthResult++] = StaticStackTrace[CurrentDepth];
    }

    return DepthResult;
}

TArray<FStackTraceEntry> IPlatformStackTrace::GetStack(int32 MaxDepth, int32 IgnoreCount)
{
    TArray<FStackTraceEntry> Stack;
    if (!FPlatformStackTrace::InitializeSymbols())
    {
        return Stack;
    }

    // Skip the 2 first (CaptureCallstack functions)
    IgnoreCount += 2;

    uint64 StackTrace[MAX_STACK_DEPTH];
    Memory::Memzero(StackTrace);

    MaxDepth = Math::Min(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);

    const int32 Depth = FPlatformStackTrace::CaptureStackTrace(StackTrace, MaxDepth);
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; CurrentDepth++)
    {
        FStackTraceEntry& NewEntry = Stack.Emplace();
        FPlatformStackTrace::GetStackTraceEntryFromAddress(StackTrace[CurrentDepth], NewEntry);
    }

    FPlatformStackTrace::ReleaseSymbols();
    return Stack;
}

TArray<FStackTraceEntry> IPlatformStackTrace::GetThreadStack(const FThreadStackContext& ThreadContext, int32 MaxDepth)
{
    TArray<FStackTraceEntry> Stack;
    if (!FPlatformStackTrace::InitializeSymbols())
    {
        return Stack;
    }

    // Nothing is skipped the way GetStack skips its own frames, because the walk starts in the 
    // faulting thread rather than in this one, so every frame it produces belongs to the report.
    uint64 StackTrace[MAX_STACK_DEPTH];
    Memory::Memzero(StackTrace);

    MaxDepth = Math::Min(MAX_STACK_DEPTH, MaxDepth);

    const int32 Depth = FPlatformStackTrace::CaptureThreadStackTrace(ThreadContext, StackTrace, MaxDepth);
    for (int32 CurrentDepth = 0; CurrentDepth < Depth; CurrentDepth++)
    {
        FStackTraceEntry& NewEntry = Stack.Emplace();
        FPlatformStackTrace::GetStackTraceEntryFromAddress(StackTrace[CurrentDepth], NewEntry);
    }

    FPlatformStackTrace::ReleaseSymbols();
    return Stack;
}