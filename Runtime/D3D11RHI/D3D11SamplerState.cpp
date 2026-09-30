#include "D3D11RHI/D3D11SamplerState.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11SamplerStateRHI::FD3D11SamplerStateRHI(FD3D11Device* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FD3D11DeviceChild(InDevice)
    , SamplerState(nullptr)
{
}

FD3D11SamplerStateRHI::~FD3D11SamplerStateRHI() = default;

bool FD3D11SamplerStateRHI::CreateSampler(const D3D11_SAMPLER_DESC& InDesc)
{
    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateSamplerState(&InDesc, &SamplerState);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11SamplerStateRHI]: FAILED to create SamplerState (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}
