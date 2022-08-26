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
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    id<MTLSamplerState> GetMTLSamplerState() const { return SamplerState; }
    
private:
    id<MTLSamplerState> SamplerState = nil;
};

#pragma clang diagnostic pop
