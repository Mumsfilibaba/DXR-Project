#include "D3D11RHI/D3D11Texture.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11TextureRHI::FD3D11TextureRHI(FD3D11Device* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FD3D11Resource()
    , FD3D11DeviceChild(InDevice)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
    , DebugName()
{
}

FD3D11TextureRHI::~FD3D11TextureRHI() = default;

bool FD3D11TextureRHI::InitializeSwapChainTexture(const TComPtr<ID3D11Texture2D>& InBackBuffer, EFormat InFormat, uint32 InWidth, uint32 InHeight)
{
    Desc.Format   = InFormat;
    Desc.Extent.X = static_cast<int32>(InWidth);
    Desc.Extent.Y = static_cast<int32>(InHeight);

    Resource     = InBackBuffer;
    CurrentState = ERHIResourceState::Present;

    const DXGI_FORMAT BackBufferFormat = ConvertFormat(Desc.Format);

    if (Desc.IsRenderTarget())
    {
        D3D11_RENDER_TARGET_VIEW_DESC D3D11ViewDesc = {};
        D3D11ViewDesc.Format             = BackBufferFormat;
        D3D11ViewDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
        D3D11ViewDesc.Texture2D.MipSlice = 0;

        if (!RenderTargetView)
        {
            const FRHIRenderTargetViewDesc ViewDesc = FRHIRenderTargetViewDesc::CreateTexture2D(Desc.Format, 0);
            RenderTargetView = new FD3D11RenderTargetViewRHI(GetDevice(), this, ViewDesc);
        }

        if (!RenderTargetView->Initialize(InBackBuffer.Get(), D3D11ViewDesc))
        {
            return false;
        }
    }

    if (Desc.IsUnorderedAccessTexture())
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC D3D11ViewDesc = {};
        D3D11ViewDesc.Format             = BackBufferFormat;
        D3D11ViewDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        D3D11ViewDesc.Texture2D.MipSlice = 0;

        if (!UnorderedAccessView)
        {
            const FRHIUnorderedAccessViewDesc ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(Desc.Format, 0);
            UnorderedAccessView = new FD3D11UnorderedAccessViewRHI(GetDevice(), this, ViewDesc);
        }

        if (!UnorderedAccessView->Initialize(InBackBuffer.Get(), D3D11ViewDesc))
        {
            return false;
        }
    }

    if (Desc.IsShaderResourceTexture())
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC D3D11ViewDesc = {};
        D3D11ViewDesc.Format                    = BackBufferFormat;
        D3D11ViewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        D3D11ViewDesc.Texture2D.MostDetailedMip = 0;
        D3D11ViewDesc.Texture2D.MipLevels       = 1;

        if (!ShaderResourceView)
        {
            const FRHIShaderResourceViewDesc ViewDesc = FRHIShaderResourceViewDesc::CreateTexture2D(Desc.Format, 0, 1);
            ShaderResourceView = new FD3D11ShaderResourceViewRHI(GetDevice(), this, ViewDesc);
        }

        if (!ShaderResourceView->Initialize(InBackBuffer.Get(), D3D11ViewDesc))
        {
            return false;
        }
    }

    return true;
}

void FD3D11TextureRHI::ReleaseSwapChainTexture()
{
    if (RenderTargetView)
    {
        RenderTargetView->ReleaseView();
    }

    if (UnorderedAccessView)
    {
        UnorderedAccessView->ReleaseView();
    }

    if (ShaderResourceView)
    {
        ShaderResourceView->ReleaseView();
    }

    Resource.Reset();
}
