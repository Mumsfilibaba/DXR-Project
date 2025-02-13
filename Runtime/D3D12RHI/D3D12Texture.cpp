#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Viewport.h"
#include "D3D12RHI/D3D12RHI.h"

FD3D12Texture::FD3D12Texture(FD3D12Device* InDevice, const FRHITextureInfo& InTextureInfo)
    : FRHITexture(InTextureInfo)
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetViews()
    , DepthStencilViews()
{
}

FD3D12Texture::~FD3D12Texture()
{
    DestroyDepthStencilViews();
    DestroyRenderTargetViews();
}

bool FD3D12Texture::Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    D3D12_RESOURCE_DESC ResourceDesc;
    FMemory::Memzero(&ResourceDesc);

    ResourceDesc.Dimension        = ConvertTextureDimension(Info.Dimension);
    ResourceDesc.Flags            = ConvertTextureFlags(Info.UsageFlags);
    ResourceDesc.Format           = ConvertFormat(Info.Format);
    ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.MipLevels        = static_cast<UINT16>(Info.NumMipLevels);
    ResourceDesc.Alignment        = 0;
    ResourceDesc.Width            = Info.Extent.X;
    ResourceDesc.Height           = Info.Extent.Y;
    ResourceDesc.SampleDesc.Count = Info.NumSamples;
    
    if (Info.IsTexture3D())
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Info.Extent.Z);
    }
    else 
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Info.NumArraySlices);
    }

    if (Info.IsTextureCube() || Info.IsTextureCubeArray())
    {
        ResourceDesc.DepthOrArraySize = ResourceDesc.DepthOrArraySize * RHI_NUM_CUBE_FACES;
    }

    if (Info.NumSamples > 1)
    {
        const int32 Quality = GetDevice()->QueryMultisampleQuality(ResourceDesc.Format, Info.NumSamples);
        ResourceDesc.SampleDesc.Quality = Quality - 1;
    }
    else
    {
        ResourceDesc.SampleDesc.Quality = 0;
    }

    // Support tight alignment if the device supports it
    if (GD3D12SupportTightAlignment)
    {
        ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
    }

    D3D12_CLEAR_VALUE ClearValue;

    const bool bSupportClearValue = Info.IsRenderTarget() || Info.IsDepthStencil();
    if (bSupportClearValue)
    {
        FMemory::Memzero(&ClearValue);

        ClearValue.Format = (Info.ClearValue.Format != EFormat::Unknown) ? ConvertFormat(Info.ClearValue.Format) : ResourceDesc.Format;
        if (Info.ClearValue.IsDepthStencilValue())
        {
            ClearValue.DepthStencil.Depth   = Info.ClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = static_cast<uint8>(Info.ClearValue.AsDepthStencil().Stencil);
        }
        else if (Info.ClearValue.IsColorValue())
        {
            FMemory::Memcpy(ClearValue.Color, Info.ClearValue.ColorValue.RGBA, sizeof(float[4]));
        }
    }

    FD3D12ResourceRef NewResource = new FD3D12Resource(GetDevice(), ResourceDesc, D3D12_HEAP_TYPE_DEFAULT);
    if (!NewResource->Initialize(D3D12_RESOURCE_STATE_COMMON, bSupportClearValue ? &ClearValue : nullptr))
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

        ViewDesc.Format                  = D3D12CastShaderResourceFormat(ResourceDesc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (Info.IsTexture2D())
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipLevels           = Info.NumMipLevels;
            ViewDesc.Texture2D.MostDetailedMip     = 0;
            ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2D.PlaneSlice          = 0;
        }
        else if (Info.IsTexture2DArray())
        {
            ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipLevels           = Info.NumMipLevels;
            ViewDesc.Texture2DArray.MostDetailedMip     = 0;
            ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2DArray.PlaneSlice          = 0;
            ViewDesc.Texture2DArray.ArraySize           = Info.NumArraySlices;
            ViewDesc.Texture2DArray.FirstArraySlice     = 0;
        }
        else if (Info.IsTextureCube())
        {
            ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            ViewDesc.TextureCube.MipLevels           = Info.NumMipLevels;
            ViewDesc.TextureCube.MostDetailedMip     = 0;
            ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else if (Info.IsTextureCubeArray())
        {
            ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            ViewDesc.TextureCubeArray.MipLevels           = Info.NumMipLevels;
            ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
            ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
            ViewDesc.TextureCubeArray.NumCubes            = Info.NumArraySlices;
        }
        else if (Info.IsTexture3D())
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture3D.MipLevels           = Info.NumMipLevels;
            ViewDesc.Texture3D.MostDetailedMip     = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension");
            return false;
        }

        FD3D12ShaderResourceViewRef DefaultSRV = new FD3D12ShaderResourceView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this);
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
    const bool bIsTexture2D = Info.IsTexture2D();
    if (bIsTexture2D)
    {
        if (Info.IsUnorderedAccess())
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
            FMemory::Memzero(&ViewDesc);

            // TODO: Handle typeless
            ViewDesc.Format               = D3D12CastShaderResourceFormat(ResourceDesc.Format);
            ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice   = 0;
            ViewDesc.Texture2D.PlaneSlice = 0;

            FD3D12UnorderedAccessViewRef DefaultUAV = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this);
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
        InCommandContext->RHIStartContext();
        InCommandContext->RHITransitionTexture(this, EResourceAccess::Common, EResourceAccess::CopyDest);

        // Transfer all the miplevels
        uint32 Width  = Info.Extent.X;
        uint32 Height = Info.Extent.Y;
        for (uint32 Index = 0; Index < Info.NumMipLevels; ++Index)
        {
            // TODO: This does not feel optimal
            if (IsBlockCompressed(Info.Format) && (!IsBlockCompressedAligned(Width) || !IsBlockCompressedAligned(Height)))
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
            InCommandContext->RHIUpdateTexture2D(this, TextureRegion, Index, Data, static_cast<uint32>(InitialData->GetMipRowPitch(Index)));

            Width  = Width / 2;
            Height = Height / 2;
        }

        // NOTE: Transition into InitialAccess
        InCommandContext->RHITransitionTexture(this, EResourceAccess::CopyDest, InInitialAccess);
        InCommandContext->RHIFinishContext();
    }
    else if (InInitialAccess != EResourceAccess::Common)
    {
        InCommandContext->RHIStartContext();
        InCommandContext->RHITransitionTexture(this, EResourceAccess::Common, InInitialAccess);
        InCommandContext->RHIFinishContext();
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
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_NONE)
    {
        D3D12_ERROR("Texture '%s' does not allow RenderTargetViews", *Resource->GetDebugName());
        return nullptr;
    }

    const DXGI_FORMAT DXGIFormat = ConvertFormat(RenderTargetView.Format);

    FD3D12RenderTargetView* ExistingView = nullptr;
    if (RenderTargetView.NumArraySlices > 1)
    {
        FD3D12HashableTextureView HashableView;
        HashableView.ArrayIndex     = RenderTargetView.ArrayIndex;
        HashableView.NumArraySlices = RenderTargetView.NumArraySlices;
        HashableView.Format         = RenderTargetView.Format;
        HashableView.MipLevel       = RenderTargetView.MipLevel;

        if (FD3D12RenderTargetViewRef* ExistingViewRef = RenderTargetViewMap.Find(HashableView))
        {
            ExistingView = ExistingViewRef->Get();
        }
    }
    else
    {
        const uint32 Subresource = D3D12CalculateSubresource(RenderTargetView.MipLevel, RenderTargetView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);
        if (Subresource < static_cast<uint32>(RenderTargetViews.Size()))
        {
            ExistingView = RenderTargetViews[Subresource].Get();
        }
        else
        {
            RenderTargetViews.Resize(Subresource + 1);
        }
    }

    if (ExistingView)
    {
        D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = ExistingView->GetDesc();
        if (RTVDesc.Format != DXGIFormat)
        {
            D3D12_WARNING("A RenderTargetView for this subresource already exists with another format");
            DEBUG_BREAK();
        }

        return ExistingView;
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
                RTVDesc.Texture2DMSArray.ArraySize       = RenderTargetView.NumArraySlices;
                RTVDesc.Texture2DMSArray.FirstArraySlice = RenderTargetView.ArrayIndex;
            }
            else
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = RenderTargetView.MipLevel;
                RTVDesc.Texture2DArray.ArraySize       = RenderTargetView.NumArraySlices;
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
        RTVDesc.Texture3D.WSize       = RenderTargetView.NumArraySlices;
    }
    else
    {
        D3D12_ERROR("ResourceDimension (=%s) does not support RenderTargetViews", ToString(ResourceDesc.Dimension));
    }

    FD3D12RenderTargetViewRef D3D12View = new FD3D12RenderTargetView(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, RTVDesc))
    {
        return nullptr;
    }

    if (RenderTargetView.NumArraySlices > 1)
    {
        FD3D12HashableTextureView HashableView;
        HashableView.ArrayIndex     = RenderTargetView.ArrayIndex;
        HashableView.NumArraySlices = RenderTargetView.NumArraySlices;
        HashableView.Format         = RenderTargetView.Format;
        HashableView.MipLevel       = RenderTargetView.MipLevel;

        RenderTargetViewMap.Add(HashableView, D3D12View);
    }
    else
    {
        const uint32 Subresource = D3D12CalculateSubresource(RenderTargetView.MipLevel, RenderTargetView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);
        RenderTargetViews[Subresource] = D3D12View;
    }

    return D3D12View.Get();
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
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_NONE)
    {
        D3D12_ERROR("Texture '%s' does not allow DepthStencilViews", *Resource->GetDebugName());
        return nullptr;
    }

    const DXGI_FORMAT DXGIFormat = ConvertFormat(DepthStencilView.Format);

    FD3D12DepthStencilView* ExistingView = nullptr;
    if (DepthStencilView.NumArraySlices > 1)
    {
        FD3D12HashableTextureView HashableView;
        HashableView.ArrayIndex     = DepthStencilView.ArrayIndex;
        HashableView.NumArraySlices = DepthStencilView.NumArraySlices;
        HashableView.Format         = DepthStencilView.Format;
        HashableView.MipLevel       = DepthStencilView.MipLevel;

        if (FD3D12DepthStencilViewRef* ExistingViewRef = DepthStencilViewMap.Find(HashableView))
        {
            ExistingView = ExistingViewRef->Get();
        }
    }
    else
    {
        const uint32 Subresource = D3D12CalculateSubresource(DepthStencilView.MipLevel, DepthStencilView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);
        if (Subresource < static_cast<uint32>(DepthStencilViews.Size()))
        {
            ExistingView = DepthStencilViews[Subresource].Get();
        }
        else
        {
            DepthStencilViews.Resize(Subresource + 1);
        }
    }

    if (ExistingView)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = ExistingView->GetDesc();
        if (DSVDesc.Format != DXGIFormat)
        {
            D3D12_WARNING("A DepthStencilView for this subresource already exists with another format");
            DEBUG_BREAK();
        }

        return ExistingView;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
    FMemory::Memzero(&DSVDesc);

    DSVDesc.Format = ConvertFormat(DepthStencilView.Format);
    if (DSVDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
        D3D12_ERROR("Unallowed format for DepthStencilViews");
        return nullptr;
    }

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.ArraySize       = DepthStencilView.NumArraySlices;
                DSVDesc.Texture2DMSArray.FirstArraySlice = DepthStencilView.ArrayIndex;
            }
            else
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = DepthStencilView.MipLevel;
                DSVDesc.Texture2DArray.ArraySize       = DepthStencilView.NumArraySlices;
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

    FD3D12DepthStencilViewRef D3D12View = new FD3D12DepthStencilView(GetDevice(), GetDevice()->GetDepthStencilOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, DSVDesc))
    {
        return nullptr;
    }

    if (DepthStencilView.NumArraySlices > 1)
    {
        FD3D12HashableTextureView HashableView;
        HashableView.ArrayIndex     = DepthStencilView.ArrayIndex;
        HashableView.NumArraySlices = DepthStencilView.NumArraySlices;
        HashableView.Format         = DepthStencilView.Format;
        HashableView.MipLevel       = DepthStencilView.MipLevel;

        DepthStencilViewMap.Add(HashableView, D3D12View);
    }
    else
    {
        const uint32 Subresource = D3D12CalculateSubresource(DepthStencilView.MipLevel, DepthStencilView.ArrayIndex, 0, ResourceDesc.MipLevels, ResourceDesc.DepthOrArraySize);
        DepthStencilViews[Subresource] = D3D12View;
    }

    return D3D12View.Get();
}

void FD3D12Texture::DestroyRenderTargetViews()
{
    RenderTargetViews.Clear();
    RenderTargetViewMap.Clear();
}

void FD3D12Texture::DestroyDepthStencilViews()
{
    DepthStencilViews.Clear();
    DepthStencilViewMap.Clear();
}

void FD3D12Texture::SetDebugName(const FString& InName)
{
    if (Resource)
    {
        Resource->SetDebugName(InName);
    }
}

FString FD3D12Texture::GetDebugName() const
{
    if (Resource)
    {
        return Resource->GetDebugName();
    }

    return "";
}

FD3D12BackBufferTexture::FD3D12BackBufferTexture(FD3D12Device* InDevice, FD3D12Viewport* InViewport, const FRHITextureInfo& InTextureInfo)
    : FD3D12Texture(InDevice, InTextureInfo)
    , Viewport(InViewport)
{
}

FD3D12BackBufferTexture::~FD3D12BackBufferTexture()
{
    Viewport = nullptr;
}

void FD3D12BackBufferTexture::Resize(uint32 InWidth, uint32 InHeight)
{
    Info.Extent.X = InWidth;
    Info.Extent.Y = InHeight;
}

FD3D12Texture* FD3D12BackBufferTexture::GetCurrentBackBufferTexture() const
{
    return Viewport ? Viewport->GetCurrentBackBuffer() : nullptr;
}