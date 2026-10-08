#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"

typedef TSharedRef<class FD3D11SamplerStateRHI> FD3D11SamplerStateRHIRef;

class FD3D11SamplerStateRHI : public FRHISamplerState, public FD3D11DeviceChild
{
public:
    FD3D11SamplerStateRHI(FD3D11Device* InDevice, const FRHISamplerStateDesc& InSamplerDesc);
    virtual ~FD3D11SamplerStateRHI();

    // FRHISamplerState Interface
    virtual void* GetRHINativeSampler() const override final { return SamplerState.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool Initialize();

    FORCEINLINE ID3D11SamplerState* GetD3D11SamplerState() const
    {
        return SamplerState.Get();
    }

private:
    TComPtr<ID3D11SamplerState> SamplerState;
};
