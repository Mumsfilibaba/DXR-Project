#include "D3D12Texture.h"
#include "D3D12Viewport.h"
#include "D3D12RHI.h"

FD3D12Texture::FD3D12Texture(FD3D12Device* InDevice, const FRHITextureDesc& InDesc)
    : FRHITexture(InDesc)
    , FD3D12DeviceChild(InDevice)
    , FD3D12RefCounted()
    , Resource(nullptr)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetViews()
    , DepthStencilViews()
{
}

bool FD3D12Texture::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    D3D12_RESOURCE_DESC ResourceDesc;
    FMemory::Memzero(&ResourceDesc);

    ResourceDesc.Dimension        = ConvertTextureDimension(Desc.Dimension);
    ResourceDesc.Flags            = ConvertTextureFlags(Desc.UsageFlags);
    ResourceDesc.Format           = ConvertFormat(Desc.Format);
    ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.MipLevels        = static_cast<UINT16>(Desc.NumMipLevels);
    ResourceDesc.Alignment        = 0;
    ResourceDesc.Width            = Desc.Extent.x;
    ResourceDesc.Height           = Desc.Extent.y;
    ResourceDesc.SampleDesc.Count = Desc.NumSamples;
    
    if (Desc.IsTexture3D())
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Desc.Extent.z);
    }
    else 
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Desc.NumArraySlices);
    }

    if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
    {
        ResourceDesc.DepthOrArraySize = ResourceDesc.DepthOrArraySize * kRHINumCubeFaces;
    }

    if (Desc.NumSamples > 1)
    {
        const int32 Quality = GetDevice()->QueryMultisampleQuality(ResourceDesc.Format, Desc.NumSamples);
        ResourceDesc.SampleDesc.Quality = Quality - 1;
    }
    else
    {
        ResourceDesc.SampleDesc.Quality = 0;
    }

    D3D12_CLEAR_VALUE* ClearValue = nullptr;
    D3D12_CLEAR_VALUE  D3D12ClearValue;
    if (Desc.IsRenderTarget() || Desc.IsDepthStencil())
    {
        FMemory::Memzero(&D3D12ClearValue);
        ClearValue = &D3D12ClearValue;

        D3D12ClearValue.Format = (Desc.ClearValue.Format != EFormat::Unknown) ? ConvertFormat(Desc.ClearValue.Format) : ResourceDesc.Format;
        if (Desc.ClearValue.IsDepthStencilValue())
        {
            D3D12ClearValue.DepthStencil.Depth   = Desc.ClearValue.AsDepthStencil().Depth;
            D3D12ClearValue.DepthStencil.Stencil = Desc.ClearValue.AsDepthStencil().Stencil;
        }
        else if (Desc.ClearValue.IsColorValue())
        {
            FMemory::Memcpy(D3D12ClearValue.Color, &Desc.ClearValue.ColorValue.r, sizeof(float[4]));
        }
    }

    FD3D12ResourceRef NewResource = new FD3D12Resource(GetDevice(), ResourceDesc, D3D12_HEAP_TYPE_DEFAULT);
    if (!NewResource->Initialize(D3D12_RESOURCE_STATE_COMMON, ClearValue))
    {
        return false;
    }
    else
    {
        Resource = NewResource;
    }

    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
        FMemory::Memzero(&ViewDesc);

        ViewDesc.Format                  = CastShaderResourceFormat(ResourceDesc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (Desc.IsTexture2D())
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture2D.MostDetailedMip     = 0;
            ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2D.PlaneSlice          = 0;
        }
        else if (Desc.IsTexture2DArray())
        {
            ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture2DArray.MostDetailedMip     = 0;
            ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2DArray.PlaneSlice          = 0;
            ViewDesc.Texture2DArray.ArraySize           = Desc.NumArraySlices;
            ViewDesc.Texture2DArray.FirstArraySlice     = 0;
        }
        else if (Desc.IsTextureCube())
        {
            ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            ViewDesc.TextureCube.MipLevels           = Desc.NumMipLevels;
            ViewDesc.TextureCube.MostDetailedMip     = 0;
            ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else if (Desc.IsTextureCubeArray())
        {
            ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            ViewDesc.TextureCubeArray.MipLevels           = Desc.NumMipLevels;
            ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
            ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
            ViewDesc.TextureCubeArray.NumCubes            = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture3D.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture3D.MostDetailedMip     = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension");
            return false;
        }

        FD3D12ShaderResourceViewRef DefaultSRV = new FD3D12ShaderResourceView(GetDevice(), FD3D12RHI::GetRHI()->GetResourceOfflineDescriptorHeap(), this);
        if (!DefaultSRV->AllocateHandle())
        {
            return false;
        }

        if (!DefaultSRV->CreateView(GetD3D12Resource(), ViewDesc))
        {
            return false;
        }

        ShaderResourceView = DefaultSRV;
    }

    // TODO: Fix for other resources than Texture2D
    const bool bIsTexture2D = Desc.IsTexture2D();
    if (bIsTexture2D)
    {
        if (Desc.IsUnorderedAccess())
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
            FMemory::Memzero(&ViewDesc);

            // TODO: Handle typeless
            ViewDesc.Format               = CastShaderResourceFormat(ResourceDesc.Format);
            ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice   = 0;
            ViewDesc.Texture2D.PlaneSlice = 0;

            FD3D12UnorderedAccessViewRef DefaultUAV = new FD3D12UnorderedAccessView(GetDevice(), FD3D12RHI::GetRHI()->GetResourceOfflineDescriptorHeap(), this);
            if (!DefaultUAV->AllocateHandle())
            {
                return false;
            }

            if (!DefaultUAV->CreateView(nullptr, GetD3D12Resource(), ViewDesc))
            {
                return false;
            }

            UnorderedAccessView = DefaultUAV;
        }
    }

    const IRHITextureData* InitialData = InInitialData;
    if (InitialData && bIsTexture2D)
    {
        // TODO: Support other types than texture 2D

        FD3D12CommandContext* Context = FD3D12RHI::GetRHI()->ObtainCommandContext();
        Context->RHIStartContext();
        Context->RHITransitionTexture(this, EResourceAccess::Common, EResourceAccess::CopyDest);

        // Transfer all the miplevels
        uint32 Width  = Desc.Extent.x;
        uint32 Height = Desc.Extent.y;
        for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
        {
            // TODO: This does not feel optimal
            if (IsBlockCompressed(Desc.Format) && (!IsBlockCompressedAligned(Width) || !IsBlockCompressedAligned(Height)))
            {
                break;
            }

            // If there is no data for this miplevel we break
            void* Data = InitialData->GetMipData(Index);
            if (!Data)
            {
                break;
            }

            FTextureRegion2D TextureRegion(Width, Height);
            Context->RHIUpdateTexture2D(this, TextureRegion, Index, Data, static_cast<uint32>(InitialData->GetMipRowPitch(Index)));

            Width  = Width / 2;
            Height = Height / 2;
        }

        // NOTE: Transition into InitialAccess
        Context->RHITransitionTexture(this, EResourceAccess::CopyDest, InInitialAccess);
        Context->RHIFinishContext();
    }
    else if (InInitialAccess != EResourceAccess::Common)
    {
        FD3D12CommandContext* Context = FD3D12RHI::GetRHI()->ObtainCommandContext();
        Context->RHIStartContext();
        Context->RHITransitionTexture(this, EResourceAccess::Common, InInitialAccess);
        Context->RHIFinishContext();
    }

    return true;
}

FD3D12RenderTargetView* FD3D12Texture::GetOrCreateRenderTargetView(const FRHIRenderTargetView& RenderTargetView)
{
    FD3D12Resource* D3D12Resource = GetD3D12Resource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    D3D12_ERROR_COND(ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, "Texture '%s' does not allow RenderTargetViews", Resource->GetName().GetCString());

    const uint32 Subresource = D3D12CalcSubresource(RenderTargetView.MipLevel, RenderTargetView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);

    const DXGI_FORMAT DXGIFormat = ConvertFormat(RenderTargetView.Format);
    if (Subresource < static_cast<uint32>(RenderTargetViews.Size()))
    {
        if (FD3D12RenderTargetView* ExistingView = RenderTargetViews[Subresource].Get())
        {
            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = ExistingView->GetDesc();
            D3D12_WARNING_COND(RTVDesc.Format == DXGIFormat, "A RenderTargetView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        RenderTargetViews.Resize(Subresource + 1);
    }

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
    FMemory::Memzero(&RTVDesc);

    RTVDesc.Format = ConvertFormat(RenderTargetView.Format);
    D3D12_ERROR_COND(RTVDesc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for RenderTargetViews");

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                RTVDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.ArraySize       = 1;
                RTVDesc.Texture2DMSArray.FirstArraySlice = RenderTargetView.ArrayIndex;
            }
            else
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = RenderTargetView.MipLevel;
                RTVDesc.Texture2DArray.ArraySize       = 1;
                RTVDesc.Texture2DArray.FirstArraySlice = RenderTargetView.ArrayIndex;
                RTVDesc.Texture2DArray.PlaneSlice      = 0;
            }
        }
        else 
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
                RTVDesc.Texture2D.MipSlice   = RenderTargetView.MipLevel;
                RTVDesc.Texture2D.PlaneSlice = 0;
            }
        }
    }
    else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        RTVDesc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        RTVDesc.Texture3D.MipSlice    = RenderTargetView.MipLevel;
        RTVDesc.Texture3D.FirstWSlice = RenderTargetView.ArrayIndex;
        RTVDesc.Texture3D.WSize       = 1;
    }
    else
    {
        D3D12_ERROR("ResourceDimension (=%s) does not support RenderTargetViews", ToString(ResourceDesc.Dimension));
    }

    FD3D12RenderTargetViewRef D3D12View = new FD3D12RenderTargetView(GetDevice(), FD3D12RHI::GetRHI()->GetRenderTargetOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, RTVDesc))
    {
        return nullptr;
    }
    else
    {
        RenderTargetViews[Subresource] = D3D12View;
        return D3D12View.Get();
    }
}

FD3D12DepthStencilView* FD3D12Texture::GetOrCreateDepthStencilView(const FRHIDepthStencilView& DepthStencilView)
{
    FD3D12Resource* D3D12Resource = GetD3D12Resource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    D3D12_ERROR_COND(ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, "Texture '%s' does not allow DepthStencilViews", Resource->GetName().GetCString());

    const uint32 Subresource = D3D12CalcSubresource(DepthStencilView.MipLevel, DepthStencilView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);

    const DXGI_FORMAT DXGIFormat = ConvertFormat(DepthStencilView.Format);
    if (Subresource < static_cast<uint32>(DepthStencilViews.Size()))
    {
        if (FD3D12DepthStencilView* ExistingView = DepthStencilViews[Subresource].Get())
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = ExistingView->GetDesc();
            D3D12_WARNING_COND(DSVDesc.Format == DXGIFormat, "A DepthStencilView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        DepthStencilViews.Resize(Subresource + 1);
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
    FMemory::Memzero(&DSVDesc);

    DSVDesc.Format = ConvertFormat(DepthStencilView.Format);
    D3D12_ERROR_COND(DSVDesc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for DepthStencilViews");

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.ArraySize       = 1;
                DSVDesc.Texture2DMSArray.FirstArraySlice = DepthStencilView.ArrayIndex;
            }
            else
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = DepthStencilView.MipLevel;
                DSVDesc.Texture2DArray.ArraySize       = 1;
                DSVDesc.Texture2DArray.FirstArraySlice = DepthStencilView.ArrayIndex;
            }
        }
        else 
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                DSVDesc.Texture2D.MipSlice = DepthStencilView.MipLevel;
            }
        }
    }
    else
    {
        D3D12_ERROR("ResourceDimension (=%s) does not support DepthStencilViews", ToString(ResourceDesc.Dimension));
    }

    FD3D12DepthStencilViewRef D3D12View = new FD3D12DepthStencilView(GetDevice(), FD3D12RHI::GetRHI()->GetDepthStencilOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, DSVDesc))
    {
        return nullptr;
    }
    else
    {
        DepthStencilViews[Subresource] = D3D12View;
        return D3D12View.Get();
    }
}

void FD3D12Texture::SetName(const FString& InName)
{
    if (Resource)
    {
        Resource->SetName(InName);
    }
}

FString FD3D12Texture::GetName() const
{
    if (Resource)
    {
        return Resource->GetName();
    }

    return "";
}


FD3D12BackBufferTexture::FD3D12BackBufferTexture(FD3D12Device* InDevice, FD3D12Viewport* InViewport, const FRHITextureDesc& InDesc)
    : FD3D12Texture(InDevice, InDesc)
    , Viewport(InViewport)
{
}

FD3D12BackBufferTexture::~FD3D12BackBufferTexture()
{
    Viewport = nullptr;
}

void FD3D12BackBufferTexture::Resize(uint32 InWidth, uint32 InHeight)
{
    Desc.Extent.x = InWidth;
    Desc.Extent.y = InHeight;
}

FD3D12Texture* FD3D12BackBufferTexture::GetCurrentBackBufferTexture()
{
    return Viewport ? Viewport->GetCurrentBackBuffer() : nullptr;
}