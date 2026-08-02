#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12RHI.h"
#include "RHI/RHIStats.h"

static bool ShouldUseDefaultState(D3D12_RESOURCE_STATES CandidateDefaultState, D3D12_RESOURCE_STATES RequestedInitialState)
{
    return (CandidateDefaultState != D3D12_RESOURCE_STATES(0)) && ((RequestedInitialState & ~CandidateDefaultState) == D3D12_RESOURCE_STATES(0));
}

FD3D12TextureRHI::FD3D12TextureRHI(FD3D12Device* InDevice, const FRHITextureDesc& InTextureDesc)
    : FD3D12TextureBase(InTextureDesc)
    , FD3D12ResourceBase(InDevice)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
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

FD3D12TextureRHI* FD3D12TextureRHI::GetTextureInterface() const
{
    return const_cast<FD3D12TextureRHI*>(this);
}

void* FD3D12TextureRHI::GetRHINativeResource() const
{
    FD3D12Resource* Resource = ResourceStorage.GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
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
    return ShaderResourceView ? ShaderResourceView->GetBindlessHandle() : FRHIDescriptorHandle();
}

FRHIDescriptorHandle FD3D12TextureRHI::GetBindlessUAVHandle() const
{
    return UnorderedAccessView ? UnorderedAccessView->GetBindlessHandle() : FRHIDescriptorHandle();
}

bool FD3D12TextureRHI::Initialize(FD3D12CommandContext* InCommandContext, ERHIResourceState InInitialAccess, const IRHITextureData* InInitialData)
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
        ResourceDesc.DepthOrArraySize = static_cast<UINT16>(RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices));
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
            Memory::Memcpy(ClearValue.Color, Desc.ClearValue.ColorValue.RGBA, sizeof(float[4]));
        }
    }

    const D3D12_RESOURCE_STATES CandidateDefaultState = DetermineDefaultTextureState(Desc.UsageFlags);
    const D3D12_RESOURCE_STATES RequestedInitialState = ConvertResourceState(InInitialAccess);

    const bool bIsManual        = (ConvertResourceStateMode(Desc.TrackingMode) == ED3D12ResourceStateMode::ManualState);
    const bool bHasDefaultState = !bIsManual && ShouldUseDefaultState(CandidateDefaultState, RequestedInitialState);
    const D3D12_RESOURCE_STATES D3D12DefaultState = bHasDefaultState ? CandidateDefaultState : D3D12_RESOURCE_STATES(0);

    D3D12_RESOURCE_STATES D3D12CreateState;
    if (InInitialData != nullptr)
    {
        D3D12CreateState = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    else if (bHasDefaultState)
    {
        D3D12CreateState = D3D12DefaultState;
    }
    else
    {
        D3D12CreateState = RequestedInitialState;
    }

    const bool bAllocated = GetDevice()->GetTextureAllocator()->TryAllocate(
        ResourceDesc, 
        D3D12CreateState, 
        bSupportClearValue ? &ClearValue : nullptr, 
        ResourceStorage);

    if (!bAllocated || ResourceStorage.GetResource() == nullptr)
    {
        return false;
    }

    const ED3D12ResourceStateMode ResolvedStateMode = bIsManual ? ED3D12ResourceStateMode::ManualState
        : (bHasDefaultState ? ED3D12ResourceStateMode::SingleState : ED3D12ResourceStateMode::MultipleStates);

    Desc.TrackingMode = ERHIResourceStateTrackingMode::Tracked;
    GetResource()->SetResourceStateMode(ED3D12ResourceStateMode::MultipleStates);

    if (bHasDefaultState)
    {
        GetResource()->SetDefaultState(D3D12DefaultState);
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
            D3D12_ERROR("Unsupported resource dimension for default SRV");
            CHECK(false);
            return false;
        }

        FD3D12ShaderResourceViewRHIRef DefaultSRV = new FD3D12ShaderResourceViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this, GetDefaultShaderResourceViewDescForTexture(Desc));
        if (!DefaultSRV->Initialize(GetResource(), ViewDesc))
        {
            return false;
        }

        DefaultSRV->RegisterWithResource(this);
        ShaderResourceView = DefaultSRV;
    }

    if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = D3D12CastUnorderedAccessFormat(ResourceDesc.Format);

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
            ViewDesc.Texture2DArray.ArraySize       = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);
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
            CHECK(false);
            return false;
        }

        FD3D12UnorderedAccessViewRHIRef DefaultUAV = new FD3D12UnorderedAccessViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this, GetDefaultUnorderedAccessViewDescForTexture(Desc));
        if (!DefaultUAV->Initialize(nullptr, GetResource(), ViewDesc))
        {
            return false;
        }

        DefaultUAV->RegisterWithResource(this);
        UnorderedAccessView = DefaultUAV;
    }

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = D3D12CastRenderTargetFormat(ResourceDesc.Format);

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
            const uint32 ArraySize = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

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
            CHECK(false);
            return false;
        }

        FD3D12RenderTargetViewRHIRef DefaultRTV = new FD3D12RenderTargetViewRHI(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap(), this, GetDefaultRenderTargetViewDescForTexture(Desc));
        if (!DefaultRTV->Initialize(GetResource(), RTVDesc))
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
        DSVDesc.Format = D3D12CastDepthStencilFormat(ConvertFormat(DSVFormat));

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
            const uint32 ArraySize = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

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
            CHECK(false);
            return false;
        }

        FD3D12DepthStencilViewRHIRef DefaultDSV = new FD3D12DepthStencilViewRHI(GetDevice(), GetDevice()->GetDepthStencilOfflineDescriptorHeap(), this, GetDefaultDepthStencilViewDescForTexture(Desc));
        if (!DefaultDSV->Initialize(GetResource(), DSVDesc))
        {
            return false;
        }

        DefaultDSV->RegisterWithResource(this);
        DepthStencilView = DefaultDSV;
    }

    if (ResourceStorage.GetResource()->IsPlacedResource())
    {
        InCommandContext->StartContext();

        InCommandContext->AliasingBarrier(GetResource());
        InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

        const bool bWillBeInitialized = (InInitialData != nullptr) || bSupportClearValue;
        const bool bDiscardable       = (D3D12CreateState & (D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_DEPTH_WRITE)) != 0;

        if (!bWillBeInitialized && bDiscardable)
        {
            InCommandContext->GetCommandList()->DiscardResource(GetResource()->GetD3D12Resource(), nullptr);
        }

        InCommandContext->FinishContext();
    }

    if (const IRHITextureData* InitialData = InInitialData)
    {
        // Resource was created in COPY_DEST; no COMMON->COPY_DEST fixup needed.
        InCommandContext->StartContext();

        CHECK(IsTextureCube(Desc.Dimension) || !Desc.IsTexture3D() || Desc.NumArraySlices == 1);

        const uint32 NumArraySlices = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

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
            #if D3D12_USE_TIGHT_ALIGNMENT
                if ((NativeResourceDesc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
                {
                    NativeResourceDesc.Alignment = 0;
                }
            #endif

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
                    Memory::Memcpy(WritePtr, SourcePtr, SrcRowPitch);

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

        const ERHIResourceState RestingAccess = (bHasDefaultState && Desc.IsShaderResourceTexture())
            ? ERHIResourceState::ShaderResource
            : InInitialAccess;

        if (RestingAccess != ERHIResourceState::CopyDest)
        {
            InCommandContext->TransitionBarrier(MakeArrayView({ FRHITransitionBarrierDesc::CreateTexture(this, ERHIResourceState::CopyDest, RestingAccess) }));
        }

        InCommandContext->FinishContext();
    }
    else if (ResourceStorage.IsPlacedResource() && bSupportClearValue)
    {
        InCommandContext->StartContext();

        if (Desc.IsRenderTarget())
        {
            if (InInitialAccess != ERHIResourceState::RenderTarget)
            {
                InCommandContext->TransitionBarrier(MakeArrayView({ FRHITransitionBarrierDesc::CreateTexture(this, InInitialAccess, ERHIResourceState::RenderTarget) }));
            }

            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

            FD3D12RenderTargetViewRHI* D3D12RenderTargetView = nullptr;
            FRHIRenderTargetViewRef    ClearRenderTargetView;

            if (RenderTargetView)
            {
                D3D12RenderTargetView = RenderTargetView.Get();
            }
            else
            {
                D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
                RTVDesc.Format = D3D12CastRenderTargetFormat(ResourceDesc.Format);

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
                    const uint32 ArraySize = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

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
                    D3D12_ERROR("Unsupported resource dimension for Clear RTV");
                    return false;
                }

                FD3D12RenderTargetViewRHIRef NewRTV = new FD3D12RenderTargetViewRHI(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap(), this, GetDefaultRenderTargetViewDescForTexture(Desc));
                if (!NewRTV->Initialize(GetResource(), RTVDesc))
                {
                    D3D12_ERROR("Clear: Failed to create temporary RTV");
                    return false;
                }

                ClearRenderTargetView = NewRTV;
                D3D12RenderTargetView = NewRTV.Get();
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

            if (InInitialAccess != ERHIResourceState::RenderTarget)
            {
                InCommandContext->TransitionBarrier(MakeArrayView({ FRHITransitionBarrierDesc::CreateTexture(this, ERHIResourceState::RenderTarget, InInitialAccess) }));
            }
        }
        else if (Desc.IsDepthStencil())
        {
            if (InInitialAccess != ERHIResourceState::DepthWrite)
            {
                InCommandContext->TransitionBarrier(MakeArrayView({ FRHITransitionBarrierDesc::CreateTexture(this, InInitialAccess, ERHIResourceState::DepthWrite) }));
            }

            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

            FD3D12DepthStencilViewRHI* D3D12DepthStencilView = nullptr;
            FRHIDepthStencilViewRef    ClearDepthStencilView;

            if (DepthStencilView)
            {
                D3D12DepthStencilView = DepthStencilView.Get();
            }
            else
            {
                const EFormat DSVViewFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;

                D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
                DSVDesc.Format = D3D12CastDepthStencilFormat(ConvertFormat(DSVViewFormat));

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
                    const uint32 ArraySize = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);

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
                    D3D12_ERROR("Unsupported resource dimension for Clear DSV");
                    return false;
                }

                FD3D12DepthStencilViewRHIRef NewDSV = new FD3D12DepthStencilViewRHI(GetDevice(), GetDevice()->GetDepthStencilOfflineDescriptorHeap(), this, GetDefaultDepthStencilViewDescForTexture(Desc));
                if (!NewDSV->Initialize(GetResource(), DSVDesc))
                {
                    D3D12_ERROR("Clear: Failed to create temporary DSV");
                    return false;
                }

                ClearDepthStencilView = NewDSV;
                D3D12DepthStencilView = NewDSV.Get();
            }

            const EFormat DepthStencilFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;

            D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
            if (IsStencilFormat(DepthStencilFormat))
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

            if (InInitialAccess != ERHIResourceState::DepthWrite)
            {
                InCommandContext->TransitionBarrier(MakeArrayView({ FRHITransitionBarrierDesc::CreateTexture(this, ERHIResourceState::DepthWrite, InInitialAccess) }));
            }
        }

        InCommandContext->FinishContext();
    }

    if (ResolvedStateMode != ED3D12ResourceStateMode::MultipleStates)
    {
        Desc.TrackingMode = ConvertResourceStateMode(ResolvedStateMode);
        GetResource()->SetResourceStateMode(ResolvedStateMode);
    }

    ResourceStorage.FinalizeAllocation();
    return true;
}

void FD3D12TextureRHI::SetDebugName(const String& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

void FD3D12TextureRHI::GetDebugName(String& OutDebugName) const
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
