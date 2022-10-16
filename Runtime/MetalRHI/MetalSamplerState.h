#pragma once
#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalSamplerState 
    : public FRHISamplerState
{
public:
    FMetalSamplerState()  = default;
    ~FMetalSamplerState() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    id<MTLSamplerState> GetMTLSamplerState() const { return SamplerState; }
    
private:
    id<MTLSamplerState> SamplerState = nil;
};

#pragma clang diagnostic pop
