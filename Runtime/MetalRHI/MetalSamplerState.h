#pragma once
#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalSamplerState

class FMetalSamplerState : public FRHISamplerState
{
public:

    FMetalSamplerState()  = default;
    ~FMetalSamplerState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHISamplerState Interface

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

#pragma clang diagnostic pop
