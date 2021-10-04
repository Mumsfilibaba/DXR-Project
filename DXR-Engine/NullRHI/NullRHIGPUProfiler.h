#pragma once
#include "CoreRHI/GPUProfiler.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullGPUProfiler : public CGPUProfiler
{
public:
    CNullGPUProfiler() = default;
    ~CNullGPUProfiler() = default;

    virtual void GetTimeQuery( STimeQuery& OutQuery, uint32 Index ) const override final
    {
        OutQuery = STimeQuery();
    }

    virtual uint64 GetFrequency() const override final
    {
        return 1;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
