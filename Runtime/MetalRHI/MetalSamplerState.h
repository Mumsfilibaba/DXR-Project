#pragma once
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalSamplerState : public FRHISamplerState
{
public:
    FMetalSamplerState(const FRHISamplerStateDesc& InDesc)
        : FRHISamplerState(InDesc)
    {
    }
    
    ~FMetalSamplerState() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    id<MTLSamplerState> GetMTLSamplerState() const { return SamplerState; }
    
private:
    id<MTLSamplerState> SamplerState = nil;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
