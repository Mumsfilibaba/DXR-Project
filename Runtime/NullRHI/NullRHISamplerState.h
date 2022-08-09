#pragma once
#include "RHI/RHIResources.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHISamplerState

struct FNullRHISamplerState 
    : public FRHISamplerState
{
    FNullRHISamplerState()  = default;
    ~FNullRHISamplerState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHISamplerState Interface

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
