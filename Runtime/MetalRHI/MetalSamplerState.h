#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSamplerStateRHI> FMetalSamplerStateRef;

class FMetalSamplerStateRHI : public FRHISamplerState, public FMetalDeviceChild
{
public:
    FMetalSamplerStateRHI(FMetalDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc);
    ~FMetalSamplerStateRHI();

    // FRHISamplerState Interface
    virtual void* GetRHINativeSampler() const override final;
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
    
    bool Initialize();

    id<MTLSamplerState> GetMTLSamplerState() const
    {
        return SamplerState;
    }
    
private:
    id<MTLSamplerState> SamplerState;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
