#pragma once
#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

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

#pragma clang diagnostic pop
