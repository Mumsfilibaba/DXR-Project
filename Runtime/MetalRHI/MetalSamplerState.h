#pragma once
#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalSamplerState

class CMetalSamplerState : public FRHISamplerState
{
public:

    CMetalSamplerState()  = default;
    ~CMetalSamplerState() = default;
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHISamplerState Interface

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

#pragma clang diagnostic pop
