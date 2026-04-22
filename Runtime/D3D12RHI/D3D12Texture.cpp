#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12RHI.h"
#include "RHI/RHIStats.h"

FD3D12TextureRHI::FD3D12TextureRHI(FD3D12Device* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FD3D12GenericResource(InDevice)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetViews()
    , DepthStencilViews()
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

    DestroyDepthStencilViews();
    DestroyRenderTargetViews();
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

    const bool bHasDefaultState = D3D12DefaultState != D3D12_RESOURCE_STATES(0);
    const ED3D12ResourceStateMode TextureStateMode = bHasDefaultState
        ? ED3D12ResourceStateMode::SingleState
        : ED3D12ResourceStateMode::MultipleStates;

    if (const IRHITextureData* InitialData = InInitialData)
    {
        InCommandContext->StartContext();
        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

        const uint32 NumArraySlices = Desc.IsTexture3D() ? 1 : (Desc.IsTextureCube() || Desc.IsTextureCubeArray() ? Desc.NumArraySlices * RHI_NUM_CUBE_FACES : Desc.NumArraySlices);

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
                InCommandContext->GetDevice()->GetD3D12Device()->GetCopyableFootprints(&NativeResourceDesc, SubresourceIndex, 1, 0, &Footprint, &NumRows, &RowPitch, &RequiredSize);

                const uint64 Alignment   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
                const uint64 AlignedSize = Math::AlignUp<uint64>(RequiredSize, Alignment);

                FD3D12ResourceStorage UploadStorage(InCommandContext->GetDevice());
                if (InCommandContext->GetDevice()->GetStagingBufferAllocator()->Allocate(AlignedSize, Alignment, UploadStorage) == nullptr || UploadStorage.GetMappedBaseAddress() == nullptr || UploadStorage.GetResource() == nullptr)
                {
                    D3D12_ERROR_CRITICAL("Upload allocation failed during texture initialization");
                    return false;
                }

                uint8* WritePtr = reinterpret_cast<uint8*>(UploadStorage.GetMappedBaseAddress());
                const uint8* Source = SliceData;

                for (uint64 y = 0; y < NumRows; y++)
                {
                    FMemory::Memcpy(WritePtr, Source, SrcRowPitch);
                    WritePtr += Footprint.Footprint.RowPitch;
                    Source   += SrcRowPitch;
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
                InCommandContext->GetCommandList()->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

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

            FRHIRenderTargetView RTView;
            RTView.Texture        = this;
            RTView.ClearValue     = Desc.ClearValue.IsColorValue() ? Desc.ClearValue.AsColor() : FFloatColor();
            RTView.ArrayIndex     = 0;
            RTView.NumArraySlices = ResourceDesc.DepthOrArraySize;
            RTView.Format         = Desc.Format;
            RTView.MipLevel       = 0;
            RTView.LoadAction     = EAttachmentLoadAction::Clear;
            RTView.StoreAction    = EAttachmentStoreAction::Store;

            FD3D12RenderTargetView* D3D12RTV = GetOrCreateRenderTargetView(RTView);
            CHECK(D3D12RTV != nullptr);

            const float ClearColor[4] = { ClearValue.Color[0], ClearValue.Color[1], ClearValue.Color[2], ClearValue.Color[3] };
            InCommandContext->GetCommandList()->ClearRenderTargetView(D3D12RTV->GetOfflineHandle(), ClearColor, 0, nullptr);

            if (InInitialAccess != EResourceAccess::RenderTarget)
            {
                InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::RenderTarget, InInitialAccess));
            }
        }
        else if (Desc.IsDepthStencil())
        {
            InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::DepthWrite));
            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());

            FRHIDepthStencilView DSView;
            DSView.Texture        = this;
            DSView.ClearValue     = Desc.ClearValue.IsDepthStencilValue() ? Desc.ClearValue.AsDepthStencil() : FDepthStencilValue();
            DSView.ArrayIndex     = 0;
            DSView.NumArraySlices = ResourceDesc.DepthOrArraySize;
            DSView.Format         = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;
            DSView.MipLevel       = 0;
            DSView.LoadAction     = EAttachmentLoadAction::Clear;
            DSView.StoreAction    = EAttachmentStoreAction::Store;

            FD3D12DepthStencilView* D3D12DSV = GetOrCreateDepthStencilView(DSView);
            CHECK(D3D12DSV != nullptr);

            D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
            if (FormatHasStencil(DSView.Format))
            {
                ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            InCommandContext->GetCommandList()->ClearDepthStencilView(D3D12DSV->GetOfflineHandle(), ClearFlags, ClearValue.DepthStencil.Depth, static_cast<uint8>(ClearValue.DepthStencil.Stencil), 0, nullptr);

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

FD3D12RenderTargetView* FD3D12TextureRHI::GetOrCreateRenderTargetView(const FRHIRenderTargetView& RenderTargetView)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_NONE)
    {
        FString DebugName;
        ResourceStorage.GetResource()->GetDebugName(DebugName);
        D3D12_ERROR("Texture '%s' does not allow RenderTargetViews", *DebugName);
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

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.Format = ConvertFormat(RenderTargetView.Format);

    D3D12_ERROR_COND(RTVDesc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for RenderTargetViews");

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            RTVDesc.Texture1DArray.MipSlice        = RenderTargetView.MipLevel;
            RTVDesc.Texture1DArray.ArraySize       = RenderTargetView.NumArraySlices;
            RTVDesc.Texture1DArray.FirstArraySlice = RenderTargetView.ArrayIndex;
        }
        else
        {
            RTVDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
            RTVDesc.Texture1D.MipSlice = RenderTargetView.MipLevel;
        }
    }
    else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
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

    D3D12View->RegisterWithResource(this);

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

FD3D12DepthStencilView* FD3D12TextureRHI::GetOrCreateDepthStencilView(const FRHIDepthStencilView& DepthStencilView)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_NONE)
    {
        FString DebugName;
        ResourceStorage.GetResource()->GetDebugName(DebugName);
        D3D12_ERROR("Texture '%s' does not allow DepthStencilViews", *DebugName);
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

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    DSVDesc.Format = ConvertFormat(DepthStencilView.Format);

    if (DSVDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
        D3D12_ERROR("Unallowed format for DepthStencilViews");
        return nullptr;
    }

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            DSVDesc.Texture1DArray.MipSlice        = DepthStencilView.MipLevel;
            DSVDesc.Texture1DArray.ArraySize       = DepthStencilView.NumArraySlices;
            DSVDesc.Texture1DArray.FirstArraySlice = DepthStencilView.ArrayIndex;
        }
        else
        {
            DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
            DSVDesc.Texture1D.MipSlice = DepthStencilView.MipLevel;
        }
    }
    else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
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

    D3D12View->RegisterWithResource(this);

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

void FD3D12TextureRHI::DestroyRenderTargetViews()
{
    RenderTargetViews.Clear();
    RenderTargetViewMap.Clear();
}

void FD3D12TextureRHI::DestroyDepthStencilViews()
{
    DepthStencilViews.Clear();
    DepthStencilViewMap.Clear();
}

void FD3D12TextureRHI::SetDebugName(const FString& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

FString FD3D12TextureRHI::GetDebugName() const
{
    FString DebugName;
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->GetDebugName(DebugName);
    }
    return DebugName;
}


FD3D12BackBufferTexture::FD3D12BackBufferTexture(FD3D12Device* InDevice, FD3D12SwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc)
    : FD3D12TextureRHI(InDevice, InTextureDesc)
    , SwapChain(InSwapChain)
{
}

FD3D12BackBufferTexture::~FD3D12BackBufferTexture()
{
    SwapChain = nullptr;
}

void FD3D12BackBufferTexture::Resize(uint32 InWidth, uint32 InHeight)
{
    Desc.Extent.X = InWidth;
    Desc.Extent.Y = InHeight;
}

FD3D12TextureRHI* FD3D12BackBufferTexture::GetCurrentBackBufferTexture() const
{
    return SwapChain ? SwapChain->GetCurrentBackBuffer() : nullptr;
}
