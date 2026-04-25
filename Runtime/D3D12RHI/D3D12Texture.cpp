#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12RHI.h"
#include "RHI/RHIStats.h"

FD3D12TextureRHI::FD3D12TextureRHI(FD3D12Device* InDevice, const FRHITextureDesc& InTextureDesc)
    : FD3D12TextureBase(InTextureDesc)
    , FD3D12GenericResource(InDevice)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
}

FD3D12TextureRHI* FD3D12TextureRHI::GetTextureInterface() const
{
    return const_cast<FD3D12TextureRHI*>(this);
}

void* FD3D12TextureRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(ResourceStorage.GetResource());
}

FRHIShaderResourceView* FD3D12TextureRHI::GetShaderResourceView() const
{
    return ShaderResourceView.Get();
}

FRHIUnorderedAccessView* FD3D12TextureRHI::GetUnorderedAccessView() const
{
    return UnorderedAccessView.Get();
}

FRHIRenderTargetView* FD3D12TextureRHI::GetRenderTargetView() const
{
    return RenderTargetView.Get();
}

FRHIDepthStencilView* FD3D12TextureRHI::GetDepthStencilView() const
{
    return DepthStencilView.Get();
}

FRHIDescriptorHandle FD3D12TextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FD3D12TextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FD3D12TextureRHI::~FD3D12TextureRHI()
{
#if D3D12_ENABLE_STATS
    const int64 AllocatedSize = static_cast<int64>(ResourceStorage.GetSize());
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

bool FD3D12TextureRHI::Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension        = ConvertTextureDimension(Desc.Dimension);
    ResourceDesc.Flags            = ConvertTextureFlags(Desc.UsageFlags);
    ResourceDesc.Format           = ConvertFormat(Desc.Format);
    ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.MipLevels        = static_cast<UINT16>(Desc.NumMipLevels);
    ResourceDesc.Alignment        = 0;
    ResourceDesc.Width            = Desc.Extent.X;
    ResourceDesc.Height           = Desc.Extent.Y;
    ResourceDesc.SampleDesc.Count = Desc.NumSamples;
    
    if (Desc.IsTexture3D())
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Desc.Extent.Z);
    }
    else 
    {
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(Desc.NumArraySlices);
    }

    if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
    {
        ResourceDesc.DepthOrArraySize = ResourceDesc.DepthOrArraySize * RHI_NUM_CUBE_FACES;
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

    D3D12_CLEAR_VALUE ClearValue = {};

    const bool bSupportClearValue = Desc.IsRenderTarget() || Desc.IsDepthStencil();
    if (bSupportClearValue)
    {
        ClearValue.Format = (Desc.ClearValue.Format != EFormat::Unknown) ? ConvertFormat(Desc.ClearValue.Format) : ResourceDesc.Format;
        if (Desc.ClearValue.IsDepthStencilValue())
        {
            ClearValue.DepthStencil.Depth   = Desc.ClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = static_cast<uint8>(Desc.ClearValue.AsDepthStencil().Stencil);
        }
        else if (Desc.ClearValue.IsColorValue())
        {
            FMemory::Memcpy(ClearValue.Color, Desc.ClearValue.ColorValue.RGBA, sizeof(float[4]));
        }
    }

    const D3D12_RESOURCE_STATES D3D12DefaultState = DetermineDefaultTextureState(Desc.UsageFlags);
    const bool bAllocated = GetDevice()->GetTextureAllocator()->TryAllocate(
        ResourceDesc, 
        D3D12_RESOURCE_STATE_COMMON, 
        bSupportClearValue ? &ClearValue : nullptr, 
        ResourceStorage);

    if (!bAllocated || ResourceStorage.GetResource() == nullptr)
    {
        return false;
    }

    if (!Desc.IsNoDefaultSRV())
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {};
        ViewDesc.Format                  = D3D12CastShaderResourceFormat(ResourceDesc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (Desc.IsTexture1D())
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
            ViewDesc.Texture1D.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture1D.MostDetailedMip     = 0;
            ViewDesc.Texture1D.ResourceMinLODClamp = 0.0f;
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            ViewDesc.Texture1DArray.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture1DArray.MostDetailedMip     = 0;
            ViewDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture1DArray.ArraySize           = Desc.NumArraySlices;
            ViewDesc.Texture1DArray.FirstArraySlice     = 0;
        }
        else if (Desc.IsTexture2D())
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
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
            ViewDesc.Texture3D.MipLevels           = Desc.NumMipLevels;
            ViewDesc.Texture3D.MostDetailedMip     = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension");
            return false;
        }

        FD3D12ShaderResourceViewRHIRef DefaultSRV = new FD3D12ShaderResourceViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this);
        if (!DefaultSRV->AllocateHandle())
        {
            return false;
        }

        if (!DefaultSRV->CreateView(GetResource(), ViewDesc))
        {
            return false;
        }

        DefaultSRV->RegisterWithResource(this);
        ShaderResourceView = DefaultSRV;
    }

    if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = D3D12CastShaderResourceFormat(ResourceDesc.Format);

        if (Desc.IsTexture1D())
        {
            ViewDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
            ViewDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            ViewDesc.Texture1DArray.MipSlice        = 0;
            ViewDesc.Texture1DArray.FirstArraySlice = 0;
            ViewDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice   = 0;
            ViewDesc.Texture2D.PlaneSlice = 0;
        }
        else if (Desc.IsTexture2DArray())
        {
            ViewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipSlice        = 0;
            ViewDesc.Texture2DArray.PlaneSlice      = 0;
            ViewDesc.Texture2DArray.FirstArraySlice = 0;
            ViewDesc.Texture2DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            ViewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipSlice        = 0;
            ViewDesc.Texture2DArray.PlaneSlice      = 0;
            ViewDesc.Texture2DArray.FirstArraySlice = 0;
            ViewDesc.Texture2DArray.ArraySize       = Desc.NumArraySlices * RHI_NUM_CUBE_FACES;
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
            ViewDesc.Texture3D.MipSlice    = 0;
            ViewDesc.Texture3D.FirstWSlice = 0;
            ViewDesc.Texture3D.WSize       = static_cast<UINT>(Desc.Extent.Z);
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension for default UAV");
            return false;
        }

        FD3D12UnorderedAccessViewRHIRef DefaultUAV = new FD3D12UnorderedAccessViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this);
        if (!DefaultUAV->AllocateHandle())
        {
            return false;
        }

        if (!DefaultUAV->CreateView(nullptr, GetResource(), ViewDesc))
        {
            return false;
        }

        DefaultUAV->RegisterWithResource(this);
        UnorderedAccessView = DefaultUAV;
    }

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = ConvertFormat(Desc.Format);

        if (Desc.IsTexture1D())
        {
            RTVDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
            RTVDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            RTVDesc.Texture1DArray.MipSlice        = 0;
            RTVDesc.Texture1DArray.FirstArraySlice = 0;
            RTVDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            if (!Desc.IsMultisampled())
            {
                RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
                RTVDesc.Texture2D.MipSlice   = 0;
                RTVDesc.Texture2D.PlaneSlice = 0;
            }
            else
            {
                RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            const uint32 ArraySize = Desc.IsTexture2DArray()
                ? Desc.NumArraySlices
                : Desc.NumArraySlices * RHI_NUM_CUBE_FACES;

            if (!Desc.IsMultisampled())
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = 0;
                RTVDesc.Texture2DArray.FirstArraySlice = 0;
                RTVDesc.Texture2DArray.ArraySize       = ArraySize;
                RTVDesc.Texture2DArray.PlaneSlice      = 0;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = 0;
                RTVDesc.Texture2DMSArray.ArraySize       = ArraySize;
            }
        }
        else if (Desc.IsTexture3D())
        {
            RTVDesc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
            RTVDesc.Texture3D.MipSlice    = 0;
            RTVDesc.Texture3D.FirstWSlice = 0;
            RTVDesc.Texture3D.WSize       = static_cast<UINT>(Desc.Extent.Z);
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension for default RTV");
            return false;
        }

        FD3D12RenderTargetViewRHIRef DefaultRTV = new FD3D12RenderTargetViewRHI(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap(), this);
        if (!DefaultRTV->AllocateHandle())
        {
            return false;
        }

        if (!DefaultRTV->CreateView(GetResource(), RTVDesc))
        {
            return false;
        }

        DefaultRTV->RegisterWithResource(this);
        RenderTargetView = DefaultRTV;
    }

    if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
        const EFormat DSVFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;
        DSVDesc.Format = ConvertFormat(DSVFormat);

        if (Desc.IsTexture1D())
        {
            DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
            DSVDesc.Texture1D.MipSlice = 0;
        }
        else if (Desc.IsTexture1DArray())
        {
            DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            DSVDesc.Texture1DArray.MipSlice        = 0;
            DSVDesc.Texture1DArray.FirstArraySlice = 0;
            DSVDesc.Texture1DArray.ArraySize       = Desc.NumArraySlices;
        }
        else if (Desc.IsTexture2D())
        {
            if (!Desc.IsMultisampled())
            {
                DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                DSVDesc.Texture2D.MipSlice = 0;
            }
            else
            {
                DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            const uint32 ArraySize = Desc.IsTexture2DArray()
                ? Desc.NumArraySlices
                : Desc.NumArraySlices * RHI_NUM_CUBE_FACES;

            if (!Desc.IsMultisampled())
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = 0;
                DSVDesc.Texture2DArray.FirstArraySlice = 0;
                DSVDesc.Texture2DArray.ArraySize       = ArraySize;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = 0;
                DSVDesc.Texture2DMSArray.ArraySize       = ArraySize;
            }
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension for default DSV");
            return false;
        }

        FD3D12DepthStencilViewRHIRef DefaultDSV = new FD3D12DepthStencilViewRHI(GetDevice(), GetDevice()->GetDepthStencilOfflineDescriptorHeap(), this);
        if (!DefaultDSV->AllocateHandle())
        {
            return false;
        }

        if (!DefaultDSV->CreateView(GetResource(), DSVDesc))
        {
            return false;
        }

        DefaultDSV->RegisterWithResource(this);
        DepthStencilView = DefaultDSV;
    }

    const bool bHasDefaultState = D3D12DefaultState != D3D12_RESOURCE_STATES(0);
    const ED3D12ResourceStateMode TextureStateMode = bHasDefaultState
        ? ED3D12ResourceStateMode::SingleState
        : ED3D12ResourceStateMode::MultipleStates;

    if (const IRHITextureData* InitialData = InInitialData)
    {
        InCommandContext->StartContext();
        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

        const uint32 NumArraySlices = Desc.IsTexture3D() ? 1 : (Desc.IsTextureCube() || Desc.IsTextureCubeArray() ? 
            Desc.NumArraySlices * RHI_NUM_CUBE_FACES : 
            Desc.NumArraySlices);

        uint32 Width  = Desc.Extent.X;
        uint32 Height = Desc.Extent.Y;
        uint32 Depth  = Desc.IsTexture3D() ? Desc.Extent.Z : 1;

        for (uint32 MipIndex = 0; MipIndex < Desc.NumMipLevels; ++MipIndex)
        {
            void* MipData = InitialData->GetMipData(MipIndex);
            if (!MipData)
            {
                break;
            }

            const uint32 SrcRowPitch   = static_cast<uint32>(InitialData->GetMipRowPitch(MipIndex));
            const int64  SrcSlicePitch = InitialData->GetMipSlicePitch(MipIndex);

            for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ++ArraySlice)
            {
                const uint8* SliceData = reinterpret_cast<const uint8*>(MipData) + ArraySlice * SrcSlicePitch;

                const UINT SubresourceIndex = MipIndex + ArraySlice * Desc.NumMipLevels;

                D3D12_RESOURCE_DESC NativeResourceDesc = GetResource()->GetDesc();
                if ((NativeResourceDesc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
                {
                    NativeResourceDesc.Alignment = 0;
                }

                UINT64 RequiredSize = 0;
                UINT64 RowPitch     = 0;
                UINT32 NumRows      = 0;

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
                InCommandContext->GetDevice()->GetD3D12Device()->GetCopyableFootprints(
                    &NativeResourceDesc, 
                    SubresourceIndex, 
                    1, 
                    0, 
                    &Footprint, 
                    &NumRows, 
                    &RowPitch, 
                    &RequiredSize);

                const uint64 Alignment   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
                const uint64 AlignedSize = Math::AlignUp<uint64>(RequiredSize, Alignment);

                FD3D12ResourceStorage UploadStorage(InCommandContext->GetDevice());
                bool bUploadAllocated = InCommandContext->GetDevice()->GetStagingBufferAllocator()->Allocate(
                    AlignedSize, 
                    Alignment, 
                    UploadStorage) == nullptr;

                if (bUploadAllocated || UploadStorage.GetMappedBaseAddress() == nullptr || UploadStorage.GetResource() == nullptr)
                {
                    D3D12_ERROR_CRITICAL("Upload allocation failed during texture initialization");
                    return false;
                }

                uint8*       WritePtr  = reinterpret_cast<uint8*>(UploadStorage.GetMappedBaseAddress());
                const uint8* SourcePtr = SliceData;

                for (uint64 y = 0; y < NumRows; y++)
                {
                    FMemory::Memcpy(WritePtr, SourcePtr, SrcRowPitch);

                    WritePtr  += Footprint.Footprint.RowPitch;
                    SourcePtr += SrcRowPitch;
                }

                D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
                SourceLocation.pResource                          = UploadStorage.GetResource()->GetD3D12Resource();
                SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                SourceLocation.PlacedFootprint.Offset             = UploadStorage.GetResourceOffset();
                SourceLocation.PlacedFootprint.Footprint.Format   = NativeResourceDesc.Format;
                SourceLocation.PlacedFootprint.Footprint.Width    = Width;
                SourceLocation.PlacedFootprint.Footprint.Height   = Height;
                SourceLocation.PlacedFootprint.Footprint.Depth    = Desc.IsTexture3D() ? Depth : 1;
                SourceLocation.PlacedFootprint.Footprint.RowPitch = Footprint.Footprint.RowPitch;

                D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
                DestLocation.pResource        = GetResource()->GetD3D12Resource();
                DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                DestLocation.SubresourceIndex = SubresourceIndex;

                InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

                InCommandContext->GetCommandList()->CopyTextureRegion(
                    &DestLocation, 
                    0, 
                    0, 
                    0, 
                    &SourceLocation, 
                    nullptr);

                if (Desc.IsTexture3D())
                {
                    break;
                }
            }

            Width  = Math::Max(1u, Width >> 1);
            Height = Math::Max(1u, Height >> 1);
            Depth  = Math::Max(1u, Depth >> 1);
        }

        // NOTE: Transition into InitialAccess
        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::CopyDest, InInitialAccess));
        InCommandContext->FinishContext();
    }
    else if (ResourceStorage.IsPlacedResource() && bSupportClearValue)
    {
        InCommandContext->StartContext();

        if (Desc.IsRenderTarget())
        {
            InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::RenderTarget));
            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

            FD3D12RenderTargetViewRHI* D3D12RenderTargetView = nullptr;
            FRHIRenderTargetViewRef    ClearRenderTargetView;

            if (RenderTargetView)
            {
                D3D12RenderTargetView = RenderTargetView.Get();
            }
            else
            {
                FRHIRenderTargetViewDesc RTVDesc(this);
                ClearRenderTargetView = FD3D12RHI::Get()->CreateRenderTargetView(RTVDesc);
                CHECK(ClearRenderTargetView != nullptr);
                D3D12RenderTargetView = FD3D12RHI::ResourceCast(ClearRenderTargetView.Get());
            }

            const float ClearColor[4] = 
            { 
                ClearValue.Color[0], 
                ClearValue.Color[1],
                ClearValue.Color[2], 
                ClearValue.Color[3]
            };

            InCommandContext->GetCommandList()->ClearRenderTargetView(
                D3D12RenderTargetView->GetOfflineHandle(), 
                ClearColor, 
                0, 
                nullptr);

            if (InInitialAccess != EResourceAccess::RenderTarget)
            {
                InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::RenderTarget, InInitialAccess));
            }
        }
        else if (Desc.IsDepthStencil())
        {
            InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::DepthWrite));
            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

            FD3D12DepthStencilViewRHI* D3D12DepthStencilView = nullptr;
            FRHIDepthStencilViewRef    ClearDepthStencilView;

            if (DepthStencilView)
            {
                D3D12DepthStencilView = DepthStencilView.Get();
            }
            else
            {
                FRHIDepthStencilViewDesc DSVDesc(this);
                ClearDepthStencilView = FD3D12RHI::Get()->CreateDepthStencilView(DSVDesc);
                CHECK(ClearDepthStencilView != nullptr);
                D3D12DepthStencilView = FD3D12RHI::ResourceCast(ClearDepthStencilView.Get());
            }

            const EFormat DepthStencilFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;

            D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
            if (FormatHasStencil(DepthStencilFormat))
            {
                ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            InCommandContext->GetCommandList()->ClearDepthStencilView(
                D3D12DepthStencilView->GetOfflineHandle(), 
                ClearFlags, 
                ClearValue.DepthStencil.Depth, 
                static_cast<uint8>(ClearValue.DepthStencil.Stencil), 
                0, 
                nullptr);

            if (InInitialAccess != EResourceAccess::DepthWrite)
            {
                InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::DepthWrite, InInitialAccess));
            }
        }

        InCommandContext->FinishContext();
    }
    else if (InInitialAccess != EResourceAccess::Common)
    {
        InCommandContext->StartContext();
        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, InInitialAccess));
        InCommandContext->FinishContext();
    }

    GetResource()->SetResourceStateMode(TextureStateMode);

    if (bHasDefaultState)
    {
        GetResource()->SetDefaultState(D3D12DefaultState);
    }

    return true;
}

void FD3D12TextureRHI::SetDebugName(const FString& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

void FD3D12TextureRHI::GetDebugName(FString& OutDebugName) const
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}

FD3D12BackBufferProxyTextureRHI::FD3D12BackBufferProxyTextureRHI(FD3D12SwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc)
    : FD3D12TextureBase(InTextureDesc)
    , SwapChain(InSwapChain)
    , ProxyRenderTargetView(nullptr)
{
}

FD3D12BackBufferProxyTextureRHI::~FD3D12BackBufferProxyTextureRHI()
{
    SwapChain = nullptr;
}

void FD3D12BackBufferProxyTextureRHI::Resize(uint32 InWidth, uint32 InHeight)
{
    Desc.Extent.X = InWidth;
    Desc.Extent.Y = InHeight;
}

void FD3D12BackBufferProxyTextureRHI::SetProxyRenderTargetView(FD3D12BackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView)
{
    if (InProxyRenderTargetView)
    {
        InProxyRenderTargetView->AddRef();
    }

    ProxyRenderTargetView = InProxyRenderTargetView;
}

FD3D12TextureRHI* FD3D12BackBufferProxyTextureRHI::GetTextureInterface() const
{
    return SwapChain ? SwapChain->GetCurrentBackBuffer() : nullptr;
}

void* FD3D12BackBufferProxyTextureRHI::GetRHINativeHandle() const
{
    if (FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        return CurrentBackBuffer->GetRHINativeHandle();
    }
    else
    {
        return nullptr;
    }
}

FRHIShaderResourceView* FD3D12BackBufferProxyTextureRHI::GetShaderResourceView() const
{
    return nullptr;
}

FRHIUnorderedAccessView* FD3D12BackBufferProxyTextureRHI::GetUnorderedAccessView() const
{
    return nullptr;
}

FRHIRenderTargetView* FD3D12BackBufferProxyTextureRHI::GetRenderTargetView() const
{
    return ProxyRenderTargetView.Get();
}

FRHIDepthStencilView* FD3D12BackBufferProxyTextureRHI::GetDepthStencilView() const
{
    return nullptr;
}

FRHIDescriptorHandle FD3D12BackBufferProxyTextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FD3D12BackBufferProxyTextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

void FD3D12BackBufferProxyTextureRHI::SetDebugName(const FString& InName)
{
    if (!SwapChain)
    {
        return;
    }

    const uint32 NumBackBuffers = SwapChain->GetBackBufferCount();
    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        if (FD3D12TextureRHI* BackBuffer = SwapChain->GetBackBufferAtIndex(Index))
        {
            BackBuffer->SetDebugName(InName);
        }
    }
}

void FD3D12BackBufferProxyTextureRHI::GetDebugName(FString& OutDebugName) const
{
    if (FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        CurrentBackBuffer->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}
