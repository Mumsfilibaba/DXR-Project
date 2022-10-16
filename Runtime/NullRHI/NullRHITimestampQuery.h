#pragma once
#include "RHI/RHITimestampQuery.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct FNullRHITimestampQuery 
    : public FRHITimestampQuery
{
    FNullRHITimestampQuery()  = default;

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final { OutQuery = FRHITimestamp(); }

    virtual uint64 GetFrequency() const override final { return 1; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
