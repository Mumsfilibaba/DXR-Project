#include "D3D11RHI/D3D11SamplerState.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11SamplerStateRHI::FD3D11SamplerStateRHI(FD3D11Device* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FD3D11DeviceChild(InDevice)
    , SamplerState(nullptr)
{
}

FD3D11SamplerStateRHI::~FD3D11SamplerStateRHI() = default;

bool FD3D11SamplerStateRHI::Initialize()
{
    if (!GetDevice()->FindOrCreateSamplerState(Desc, SamplerState))
    {
        D3D11_ERROR("[FD3D11SamplerStateRHI]: Failed to create sampler");
        return false;
    }

    return true;
}
