#include "Core/Platform/PlatformStackTrace.h"

int32 FGenericPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount)
{
    uint64 StaticStackTrace[MAX_STACK_DEPTH];
    
    MaxDepth = NMath::Min<int32>(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);
    
    const int32 Depth = FPlatformStackTrace::CaptureStackTrace(StaticStackTrace, MaxDepth);

    int32 DepthResult = 0;
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; ++CurrentDepth)
    {
        StackTrace[DepthResult++] = StaticStackTrace[CurrentDepth];
    }

    return DepthResult;
}