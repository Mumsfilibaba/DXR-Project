#include "RHI/RHIStats.h"
#include "D3D11RHI/D3D11Texture.h"
#include "D3D11RHI/D3D11Device.h"
#include <climits>

static uint64 CalculateTextureSize(const FRHITextureDesc& Desc, DXGI_FORMAT Format)
{
    const uint64 BitsPerPixel = GetBitsPerPixel(Format);
    const bool   bCompressed  = IsFormatCompressed(Format);
    const uint32 NumMips      = Math::Max<uint32>(Desc.NumMipLevels, 1);
    const uint32 NumSlices    = Desc.IsTexture3D() ? 1 : RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);
    const uint32 NumSamples   = Math::Max<uint32>(Desc.NumSamples, 1);

    uint64 Size = 0;
    for (uint32 Mip = 0; Mip < NumMips; ++Mip)
    {
        uint64 Width  = Math::Max<uint32>(static_cast<uint32>(Desc.Extent.X) >> Mip, 1);
        uint64 Height = Math::Max<uint32>(static_cast<uint32>(Desc.Extent.Y) >> Mip, 1);
        uint64 Depth  = Desc.IsTexture3D() ? Math::Max<uint32>(static_cast<uint32>(Desc.Extent.Z) >> Mip, 1) : 1;

        if (bCompressed)
        {
            Width  = Math::AlignUp<uint64>(Width, D3D11_BLOCK_COMPRESSION_BLOCK_SIZE);
            Height = Math::AlignUp<uint64>(Height, D3D11_BLOCK_COMPRESSION_BLOCK_SIZE);
        }

        Size += (Width * Height * Depth * BitsPerPixel) / CHAR_BIT;
    }

    return Size * NumSlices * NumSamples;
}

FD3D11TextureRHI::FD3D11TextureRHI(FD3D11Device* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FD3D11Resource()
    , FD3D11DeviceChild(InDevice)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
}

FD3D11TextureRHI::~FD3D11TextureRHI()
{
#if D3D11_ENABLE_STATS
    const int64 AllocatedSize = static_cast<int64>(AllocationSize);
    if (AllocatedSize > 0)
    {
        if (Desc.IsRenderTarget() || Desc.IsDepthStencil())
        {
            STAT_SUBTRACT(STAT_RHI_RenderTargetMemory, AllocatedSize);
        }
        else
        {
            STAT_SUBTRACT(STAT_RHI_TextureMemory, AllocatedSize);
        }
    }
#endif
}

bool FD3D11TextureRHI::Initialize(ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
{
    const DXGI_FORMAT Format = ConvertFormat(Desc.Format);
    if (Format == DXGI_FORMAT_UNKNOWN)
    {
        D3D11_ERROR("[FD3D11TextureRHI]: Format '%s' has no D3D11 equivalent", ToString(Desc.Format));
        return false;
    }

    if (Desc.IsMultisampled())
    {
        uint32 Quality = 0;
        if (!GetDevice()->QueryMultisampleQuality(Format, Desc.NumSamples, Quality))
        {
            D3D11_ERROR("[FD3D11TextureRHI]: SampleCount '%u' is not supported for format '%s'", Desc.NumSamples, ToString(Desc.Format));
            return false;
        }
    }

    const bool        bIsSampledDepth = Desc.IsDepthStencil() && Desc.IsShaderResourceTexture();
    const DXGI_FORMAT ResourceFormat  = bIsSampledDepth ? D3D11CastTypelessDepthFormat(Format) : Format;

    const uint32 NumMips   = Math::Max<uint32>(Desc.NumMipLevels, 1);
    const uint32 NumSlices = Desc.IsTexture3D() ? 1 : RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

    TArray<D3D11_SUBRESOURCE_DATA> InitialData;
    
    bool bHasCompleteData = InInitialData != nullptr;
    if (InInitialData)
    {
        InitialData.Reserve(static_cast<int32>(NumSlices * NumMips));
        for (uint32 Slice = 0; Slice < NumSlices; ++Slice)
        {
            for (uint32 Mip = 0; Mip < NumMips; ++Mip)
            {
                const uint8* MipData    = reinterpret_cast<const uint8*>(InInitialData->GetMipData(Mip));
                const int64  SlicePitch = InInitialData->GetMipSlicePitch(Mip);

                D3D11_SUBRESOURCE_DATA& SubresourceData = InitialData.Emplace();
                SubresourceData.pSysMem          = MipData ? MipData + (Slice * SlicePitch) : nullptr;
                SubresourceData.SysMemPitch      = static_cast<UINT>(InInitialData->GetMipRowPitch(Mip));
                SubresourceData.SysMemSlicePitch = static_cast<UINT>(SlicePitch);

                bHasCompleteData &= MipData != nullptr;
            }
        }
    }

    if (!CreateResource(ResourceFormat, bHasCompleteData ? InitialData.Data() : nullptr))
    {
        return false;
    }

    if (InInitialData && !bHasCompleteData)
    {
        ID3D11DeviceContext* D3D11Context = GetDevice()->GetD3D11Context();
        for (uint32 Slice = 0; Slice < NumSlices; ++Slice)
        {
            for (uint32 Mip = 0; Mip < NumMips; ++Mip)
            {
                const D3D11_SUBRESOURCE_DATA& SubresourceData = InitialData[Slice * NumMips + Mip];
                if (!SubresourceData.pSysMem)
                {
                    break;
                }

                const UINT Subresource = D3D11CalcSubresource(Mip, Slice, NumMips);
                D3D11Context->UpdateSubresource(Resource.Get(), Subresource, nullptr, SubresourceData.pSysMem, SubresourceData.SysMemPitch, SubresourceData.SysMemSlicePitch);
            }
        }
    }

    if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = D3D11CastShaderResourceFormat(ResourceFormat);

        if (Desc.IsTexture1D())
        {
            ViewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE1D;
            ViewDesc.Texture1D.MipLevels       = NumMips;
            ViewDesc.Texture1D.MostDetailedMip = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
            ViewDesc.Texture1DArray.MipLevels       = NumMips;
            ViewDesc.Texture1DArray.MostDetailedMip = 0;
            ViewDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
            ViewDesc.Texture1DArray.FirstArraySlice = 0;
        }
        else if (Desc.IsTexture2D())
        {
            if (!Desc.IsMultisampled())
            {
                ViewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                ViewDesc.Texture2D.MipLevels       = NumMips;
                ViewDesc.Texture2D.MostDetailedMip = 0;
            }
            else
            {
                ViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Desc.IsTexture2DArray())
        {
            if (!Desc.IsMultisampled())
            {
                ViewDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                ViewDesc.Texture2DArray.MipLevels       = NumMips;
                ViewDesc.Texture2DArray.MostDetailedMip = 0;
                ViewDesc.Texture2DArray.ArraySize       = Desc.NumArraySlices;
                ViewDesc.Texture2DArray.FirstArraySlice = 0;
            }
            else
            {
                ViewDesc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                ViewDesc.Texture2DMSArray.ArraySize       = Desc.NumArraySlices;
                ViewDesc.Texture2DMSArray.FirstArraySlice = 0;
            }
        }
        else if (Desc.IsTextureCube())
        {
            ViewDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
            ViewDesc.TextureCube.MipLevels       = NumMips;
            ViewDesc.TextureCube.MostDetailedMip = 0;
        }
        else if (Desc.IsTextureCubeArray())
        {
            ViewDesc.ViewDimension                     = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            ViewDesc.TextureCubeArray.MipLevels        = NumMips;
            ViewDesc.TextureCubeArray.MostDetailedMip  = 0;
            ViewDesc.TextureCubeArray.First2DArrayFace = 0;
            ViewDesc.TextureCubeArray.NumCubes         = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
            ViewDesc.Texture3D.MipLevels       = NumMips;
            ViewDesc.Texture3D.MostDetailedMip = 0;
        }
        else
        {
            D3D11_ERROR("Unsupported resource dimension for default SRV");
            CHECK(false);
            return false;
        }

        FD3D11ShaderResourceViewRHIRef DefaultSRV = new FD3D11ShaderResourceViewRHI(GetDevice(), this, GetDefaultShaderResourceViewDescForTexture(Desc));
        if (!DefaultSRV->Initialize(Resource.Get(), ViewDesc))
        {
            return false;
        }

        ShaderResourceView = DefaultSRV;
    }

    if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = D3D11CastUnorderedAccessFormat(ResourceFormat);

        if (Desc.IsTexture1D())
        {
            ViewDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE1D;
            ViewDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
            ViewDesc.Texture1DArray.MipSlice        = 0;
            ViewDesc.Texture1DArray.FirstArraySlice = 0;
            ViewDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice = 0;
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            ViewDesc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipSlice        = 0;
            ViewDesc.Texture2DArray.FirstArraySlice = 0;
            ViewDesc.Texture2DArray.ArraySize       = NumSlices;
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
            ViewDesc.Texture3D.MipSlice    = 0;
            ViewDesc.Texture3D.FirstWSlice = 0;
            ViewDesc.Texture3D.WSize       = static_cast<UINT>(Desc.Extent.Z);
        }
        else
        {
            D3D11_ERROR("Unsupported resource dimension for default UAV");
            CHECK(false);
            return false;
        }

        FD3D11UnorderedAccessViewRHIRef DefaultUAV = new FD3D11UnorderedAccessViewRHI(GetDevice(), this, GetDefaultUnorderedAccessViewDescForTexture(Desc));
        if (!DefaultUAV->Initialize(Resource.Get(), ViewDesc))
        {
            return false;
        }

        UnorderedAccessView = DefaultUAV;
    }

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = D3D11CastRenderTargetFormat(ResourceFormat);

        if (Desc.IsTexture1D())
        {
            RTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE1D;
            RTVDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
            RTVDesc.Texture1DArray.MipSlice        = 0;
            RTVDesc.Texture1DArray.FirstArraySlice = 0;
            RTVDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            if (!Desc.IsMultisampled())
            {
                RTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
                RTVDesc.Texture2D.MipSlice = 0;
            }
            else
            {
                RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            if (!Desc.IsMultisampled())
            {
                RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = 0;
                RTVDesc.Texture2DArray.FirstArraySlice = 0;
                RTVDesc.Texture2DArray.ArraySize       = NumSlices;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = 0;
                RTVDesc.Texture2DMSArray.ArraySize       = NumSlices;
            }
        }
        else if (Desc.IsTexture3D())
        {
            RTVDesc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
            RTVDesc.Texture3D.MipSlice    = 0;
            RTVDesc.Texture3D.FirstWSlice = 0;
            RTVDesc.Texture3D.WSize       = static_cast<UINT>(Desc.Extent.Z);
        }
        else
        {
            D3D11_ERROR("Unsupported resource dimension for default RTV");
            CHECK(false);
            return false;
        }

        FD3D11RenderTargetViewRHIRef DefaultRTV = new FD3D11RenderTargetViewRHI(GetDevice(), this, GetDefaultRenderTargetViewDescForTexture(Desc));
        if (!DefaultRTV->Initialize(Resource.Get(), RTVDesc))
        {
            return false;
        }

        RenderTargetView = DefaultRTV;
    }

    if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
        const EFormat DSVFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;
        DSVDesc.Format = D3D11CastDepthStencilFormat(ConvertFormat(DSVFormat));

        if (Desc.IsTexture1D())
        {
            DSVDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE1D;
            DSVDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
            DSVDesc.Texture1DArray.MipSlice        = 0;
            DSVDesc.Texture1DArray.FirstArraySlice = 0;
            DSVDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            if (!Desc.IsMultisampled())
            {
                DSVDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                DSVDesc.Texture2D.MipSlice = 0;
            }
            else
            {
                DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            if (!Desc.IsMultisampled())
            {
                DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = 0;
                DSVDesc.Texture2DArray.FirstArraySlice = 0;
                DSVDesc.Texture2DArray.ArraySize       = NumSlices;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = 0;
                DSVDesc.Texture2DMSArray.ArraySize       = NumSlices;
            }
        }
        else
        {
            D3D11_ERROR("Unsupported resource dimension for default DSV");
            CHECK(false);
            return false;
        }

        FD3D11DepthStencilViewRHIRef DefaultDSV = new FD3D11DepthStencilViewRHI(GetDevice(), this, GetDefaultDepthStencilViewDescForTexture(Desc));
        if (!DefaultDSV->Initialize(Resource.Get(), DSVDesc))
        {
            return false;
        }

        DepthStencilView = DefaultDSV;
    }

    CurrentState = InInitialState;
    return true;
}

bool FD3D11TextureRHI::CreateResource(DXGI_FORMAT ResourceFormat, const D3D11_SUBRESOURCE_DATA* InitialData)
{
    ID3D11Device* D3D11Device = GetDevice()->GetD3D11Device();

    const UINT BindFlags = ConvertTextureBindFlags(Desc.UsageFlags);
    const UINT NumMips   = Math::Max<uint32>(Desc.NumMipLevels, 1);

    HRESULT Result = E_FAIL;
    if (Desc.IsTexture1D() || Desc.IsTexture1DArray())
    {
        D3D11_TEXTURE1D_DESC TextureDesc = {};
        TextureDesc.Width     = Desc.Extent.X;
        TextureDesc.MipLevels = NumMips;
        TextureDesc.ArraySize = Math::Max<uint32>(Desc.NumArraySlices, 1);
        TextureDesc.Format    = ResourceFormat;
        TextureDesc.Usage     = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags = BindFlags;

        TComPtr<ID3D11Texture1D> NewTexture;
        Result = D3D11Device->CreateTexture1D(&TextureDesc, InitialData, &NewTexture);
        Resource = NewTexture;
    }
    else if (Desc.IsTexture3D())
    {
        D3D11_TEXTURE3D_DESC TextureDesc = {};
        TextureDesc.Width     = Desc.Extent.X;
        TextureDesc.Height    = Desc.Extent.Y;
        TextureDesc.Depth     = Desc.Extent.Z;
        TextureDesc.MipLevels = NumMips;
        TextureDesc.Format    = ResourceFormat;
        TextureDesc.Usage     = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags = BindFlags;

        TComPtr<ID3D11Texture3D> NewTexture;
        Result = D3D11Device->CreateTexture3D(&TextureDesc, InitialData, &NewTexture);
        Resource = NewTexture;
    }
    else
    {
        D3D11_TEXTURE2D_DESC TextureDesc = {};
        TextureDesc.Width              = Desc.Extent.X;
        TextureDesc.Height             = Desc.Extent.Y;
        TextureDesc.MipLevels          = NumMips;
        TextureDesc.ArraySize          = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);
        TextureDesc.Format             = ResourceFormat;
        TextureDesc.SampleDesc.Count   = Math::Max<uint32>(Desc.NumSamples, 1);
        TextureDesc.SampleDesc.Quality = 0;
        TextureDesc.Usage              = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags          = BindFlags;
        TextureDesc.MiscFlags          = IsTextureCube(Desc.Dimension) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

        TComPtr<ID3D11Texture2D> NewTexture;
        Result = D3D11Device->CreateTexture2D(&TextureDesc, InitialData, &NewTexture);
        Resource = NewTexture;
    }

    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11TextureRHI]: FAILED to create %ux%u texture with format '%s' (0x%08X)", Desc.Extent.X, Desc.Extent.Y, ToString(Desc.Format), static_cast<uint32>(Result));
        return false;
    }

    SetAllocation(D3D11_USAGE_DEFAULT, CalculateTextureSize(Desc, ResourceFormat));
    return true;
}

void FD3D11TextureRHI::SetDebugName(const String& InName)
{
    D3D11SetDebugName(Resource.Get(), InName);
}

void FD3D11TextureRHI::GetDebugName(String& OutDebugName) const
{
    D3D11GetDebugName(Resource.Get(), OutDebugName);
}

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
