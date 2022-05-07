#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalSamplerState

class CMetalSamplerState : public CRHISamplerState
{
public:

    CMetalSamplerState()  = default;
    ~CMetalSamplerState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHISamplerState Interface

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
