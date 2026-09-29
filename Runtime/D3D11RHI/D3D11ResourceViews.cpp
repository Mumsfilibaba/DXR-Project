#include "D3D11RHI/D3D11ResourceViews.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11ShaderResourceViewRHI::FD3D11ShaderResourceViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
{
}

FD3D11ShaderResourceViewRHI::~FD3D11ShaderResourceViewRHI() = default;

bool FD3D11ShaderResourceViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateShaderResourceView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11ShaderResourceViewRHI]: FAILED to create ShaderResourceView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11UnorderedAccessViewRHI::FD3D11UnorderedAccessViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FRHIUnorderedAccessView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
{
}

FD3D11UnorderedAccessViewRHI::~FD3D11UnorderedAccessViewRHI() = default;

bool FD3D11UnorderedAccessViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateUnorderedAccessView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11UnorderedAccessViewRHI]: FAILED to create UnorderedAccessView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11RenderTargetViewRHI::FD3D11RenderTargetViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
    : FRHIRenderTargetView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
{
}

FD3D11RenderTargetViewRHI::~FD3D11RenderTargetViewRHI() = default;

bool FD3D11RenderTargetViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_RENDER_TARGET_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateRenderTargetView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11RenderTargetViewRHI]: FAILED to create RenderTargetView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11DepthStencilViewRHI::FD3D11DepthStencilViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
    : FRHIDepthStencilView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , D3D11Desc()
    , View(nullptr)
{
}

FD3D11DepthStencilViewRHI::~FD3D11DepthStencilViewRHI() = default;

bool FD3D11DepthStencilViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateDepthStencilView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11DepthStencilViewRHI]: FAILED to create DepthStencilView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    D3D11Desc = InDesc;
    return true;
}
