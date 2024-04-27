#pragma once
#include "MetalDeviceChild.h"
#include "MetalRefCounted.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSamplerState> FMetalSamplerStateRef;

class FMetalSamplerState : public FRHISamplerState, public FMetalDeviceChild
{
public:
    FMetalSamplerState(FMetalDeviceContext* InDeviceContext, const FRHISamplerStateInfo& InSamplerInfo);
    ~FMetalSamplerState();

    bool Initialize();

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    id<MTLSamplerState> GetMTLSamplerState() const
    {
        return SamplerState;
    }
    
private:
    id<MTLSamplerState> SamplerState;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
