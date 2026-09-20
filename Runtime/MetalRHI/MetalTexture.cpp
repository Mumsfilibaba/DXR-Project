#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalSwapChain.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static MTLTextureDescriptor* CreateTextureDescriptor(const FRHITextureDesc& Desc)
{
    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType        = MetalRHI::GetMTLTextureType(Desc.Dimension, Desc.IsMultisampled());
    TextureDescriptor.pixelFormat        = MetalRHI::ConvertFormat(Desc.Format);
    TextureDescriptor.usage              = MetalRHI::ConvertTextureFlags(Desc.UsageFlags);
    TextureDescriptor.mipmapLevelCount   = Math::Max(Desc.NumMipLevels, 1u);
    TextureDescriptor.sampleCount        = Math::Max(Desc.NumSamples, 1u);
    TextureDescriptor.storageMode        = MTLStorageModePrivate;
    TextureDescriptor.cpuCacheMode       = MTLCPUCacheModeDefaultCache;
    TextureDescriptor.hazardTrackingMode = MTLHazardTrackingModeDefault;
    TextureDescriptor.width              = static_cast<NSUInteger>(Math::Max(Desc.Extent.X, 1));
    TextureDescriptor.height             = static_cast<NSUInteger>(Math::Max(Desc.Extent.Y, 1));

    TextureDescriptor.allowGPUOptimizedContents = (Desc.IsRenderTarget() || Desc.IsDepthStencil()) ? YES : NO;

    if (Desc.IsTexture3D())
    {
        TextureDescriptor.depth       = static_cast<NSUInteger>(Math::Max(Desc.Extent.Z, 1));
        TextureDescriptor.arrayLength = 1;
    }
    else
    {
        TextureDescriptor.depth       = 1;
        TextureDescriptor.arrayLength = Math::Max(Desc.NumArraySlices, 1u);
    }

    if (IsTypelessFormat(Desc.Format) || (Desc.IsDepthStencil() && Desc.IsShaderResourceTexture()))
    {
        TextureDescriptor.usage |= MTLTextureUsagePixelFormatView;
    }

    if (Desc.IsUnorderedAccessTexture() && (Desc.IsTextureCube() || Desc.IsTextureCubeArray()))
    {
        TextureDescriptor.usage |= MTLTextureUsagePixelFormatView;
    }

    return TextureDescriptor;
}

static FRHIShaderResourceViewDesc CreateDefaultSRVDesc(const FRHITextureDesc& Desc)
{
    const EFormat Format    = Desc.Format;
    const uint8   NumMips   = static_cast<uint8>(Math::Max(Desc.NumMipLevels, 1u));
    const uint16  NumSlices = static_cast<uint16>(Math::Max(Desc.NumArraySlices, 1u));

    switch (Desc.Dimension)
    {
        case ETextureDimension::Texture1D:        return FRHIShaderResourceViewDesc::CreateTexture1D(Format, 0, NumMips);
        case ETextureDimension::Texture1DArray:   return FRHIShaderResourceViewDesc::CreateTexture1DArray(Format, 0, NumMips, 0, NumSlices);
        case ETextureDimension::Texture2D:        return FRHIShaderResourceViewDesc::CreateTexture2D(Format, 0, NumMips);
        case ETextureDimension::Texture2DArray:   return FRHIShaderResourceViewDesc::CreateTexture2DArray(Format, 0, NumMips, 0, NumSlices);
        case ETextureDimension::TextureCube:      return FRHIShaderResourceViewDesc::CreateTextureCube(Format, 0, NumMips);
        case ETextureDimension::TextureCubeArray: return FRHIShaderResourceViewDesc::CreateTextureCubeArray(Format, 0, NumMips, 0, NumSlices);
        case ETextureDimension::Texture3D:        return FRHIShaderResourceViewDesc::CreateTexture3D(Format, 0, NumMips);

        default:
        {
            CHECK(false);
            return FRHIShaderResourceViewDesc();
        }
    }
}

static FRHIUnorderedAccessViewDesc CreateDefaultUAVDesc(const FRHITextureDesc& Desc)
{
    const EFormat Format    = Desc.Format;
    const uint16  NumSlices = static_cast<uint16>(Math::Max(Desc.NumArraySlices, 1u));

    switch (Desc.Dimension)
    {
        case ETextureDimension::Texture1D:      return FRHIUnorderedAccessViewDesc::CreateTexture1D(Format, 0);
        case ETextureDimension::Texture1DArray: return FRHIUnorderedAccessViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
        case ETextureDimension::Texture2D:      return FRHIUnorderedAccessViewDesc::CreateTexture2D(Format, 0);
        case ETextureDimension::Texture2DArray: return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, NumSlices);
        case ETextureDimension::Texture3D:      return FRHIUnorderedAccessViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(Math::Max(Desc.Extent.Z, 1)));

        default:
        {
            CHECK(false);
            return FRHIUnorderedAccessViewDesc();
        }
    }
}

static FRHIRenderTargetViewDesc CreateDefaultRTVDesc(const FRHITextureDesc& Desc)
{
    const EFormat Format    = Desc.Format;
    const uint16  NumSlices = static_cast<uint16>(Math::Max(Desc.NumArraySlices, 1u));

    switch (Desc.Dimension)
    {
        case ETextureDimension::Texture1D:      return FRHIRenderTargetViewDesc::CreateTexture1D(Format, 0);
        case ETextureDimension::Texture1DArray: return FRHIRenderTargetViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
        case ETextureDimension::Texture2D:      return FRHIRenderTargetViewDesc::CreateTexture2D(Format, 0);
        case ETextureDimension::Texture3D:      return FRHIRenderTargetViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(Math::Max(Desc.Extent.Z, 1)));

        default:
        {
            return FRHIRenderTargetViewDesc::CreateTexture2DArray(Format, 0, 0, static_cast<uint16>(RHIDimensionArrayLayers(Desc.Dimension, NumSlices)));
        }
    }
}

static FRHIDepthStencilViewDesc CreateDefaultDSVDesc(const FRHITextureDesc& Desc)
{
    const EFormat Format    = (Desc.ClearValue.Format != EFormat::Unknown) ? Desc.ClearValue.Format : Desc.Format;
    const uint16  NumSlices = static_cast<uint16>(Math::Max(Desc.NumArraySlices, 1u));

    switch (Desc.Dimension)
    {
        case ETextureDimension::Texture1D:      return FRHIDepthStencilViewDesc::CreateTexture1D(Format, 0);
        case ETextureDimension::Texture1DArray: return FRHIDepthStencilViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
        case ETextureDimension::Texture2D:      return FRHIDepthStencilViewDesc::CreateTexture2D(Format, 0);

        default:
        {
            return FRHIDepthStencilViewDesc::CreateTexture2DArray(Format, 0, 0, static_cast<uint16>(RHIDimensionArrayLayers(Desc.Dimension, NumSlices)));
        }
    }
}

FMetalTextureRHI::FMetalTextureRHI(FMetalDevice* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FMetalDeviceChild(InDevice)
    , Texture(nil)
    , ResourceStorage(InDevice)
    , SwapChain(nullptr)
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
}

FMetalTextureRHI::~FMetalTextureRHI()
{
    ResourceStorage.ReleaseResource();
    Texture = nil;
}

void* FMetalTextureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLTexture());
}

FRHIDescriptorHandle FMetalTextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FMetalTextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIShaderResourceView* FMetalTextureRHI::GetShaderResourceView() const
{
    return ShaderResourceView.Get();
}

FRHIUnorderedAccessView* FMetalTextureRHI::GetUnorderedAccessView() const
{
    return UnorderedAccessView.Get();
}

FRHIRenderTargetView* FMetalTextureRHI::GetRenderTargetView() const
{
    return RenderTargetView.Get();
}

FRHIDepthStencilView* FMetalTextureRHI::GetDepthStencilView() const
{
    return DepthStencilView.Get();
}

bool FMetalTextureRHI::Initialize(ERHIResourceState InInitialAccess, const IRHITextureData* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    MTLTextureDescriptor* TextureDescriptor = CreateTextureDescriptor(Desc);
    if (TextureDescriptor.pixelFormat == MTLPixelFormatInvalid)
    {
        METAL_ERROR("Format '%s' has no Metal equivalent", ToString(Desc.Format));
        return false;
    }

    if (!GetDevice()->GetTextureAllocator()->TryAllocate(TextureDescriptor, ResourceStorage))
    {
        METAL_ERROR("Failed to create a %s texture", ToString(Desc.Dimension));
        return false;
    }

    Texture = ResourceStorage.GetTexture();
    if (!Texture)
    {
        METAL_ERROR("Failed to create a %s texture", ToString(Desc.Dimension));
        return false;
    }

    if (InInitialData && !UploadInitialData(InInitialData))
    {
        return false;
    }

    return CreateDefaultViews();
}

bool FMetalTextureRHI::UploadInitialData(const IRHITextureData* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    if (Desc.IsMultisampled())
    {
        METAL_ERROR("A multisampled texture cannot be given initial data");
        return false;
    }

    const bool   bIsTexture1D   = Desc.IsTexture1D() || Desc.IsTexture1DArray();
    const bool   bIsTexture3D   = Desc.IsTexture3D();
    const uint32 NumMipLevels   = Math::Max(Desc.NumMipLevels, 1u);
    const uint32 NumArraySlices = RHIDimensionArrayLayers(Desc.Dimension, Math::Max(Desc.NumArraySlices, 1u));
    const uint32 BaseDepth      = bIsTexture3D ? static_cast<uint32>(Math::Max(Desc.Extent.Z, 1)) : 1u;

    uint64 StagingSize = 0;
    for (uint32 MipIndex = 0, MipDepth = BaseDepth; MipIndex < NumMipLevels; ++MipIndex, MipDepth = Math::Max(MipDepth / 2, 1u))
    {
        if (!InInitialData->GetMipData(MipIndex))
        {
            break;
        }

        const uint64 SubresourceSize = static_cast<uint64>(InInitialData->GetMipSlicePitch(MipIndex)) * MipDepth;
        StagingSize += Math::AlignUp<uint64>(SubresourceSize, TEXTURE_UPLOAD_ALIGNMENT) * NumArraySlices;
    }

    if (StagingSize == 0)
    {
        return true;
    }

    FMetalUploadBatch UploadBatch(GetDevice());
    if (!UploadBatch.IsValid())
    {
        return false;
    }

    FMetalResourceStorage StagingStorage(GetDevice());
    if (!UploadBatch.CreateStagingBuffer(StagingSize, StagingStorage))
    {
        return false;
    }

    uint8* StagingContents = static_cast<uint8*>(StagingStorage.GetMappedBaseAddress());
    uint64 StagingOffset   = 0;

    uint32 Width  = static_cast<uint32>(Math::Max(Desc.Extent.X, 1));
    uint32 Height = static_cast<uint32>(Math::Max(Desc.Extent.Y, 1));
    uint32 Depth  = BaseDepth;

    for (uint32 MipIndex = 0; MipIndex < NumMipLevels; ++MipIndex)
    {
        const uint8* MipData = reinterpret_cast<const uint8*>(InInitialData->GetMipData(MipIndex));
        if (!MipData)
        {
            break;
        }

        const uint64 RowPitch        = static_cast<uint64>(InInitialData->GetMipRowPitch(MipIndex));
        const uint64 SlicePitch      = static_cast<uint64>(InInitialData->GetMipSlicePitch(MipIndex));
        const uint64 SubresourceSize = SlicePitch * Depth;

        for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ++ArraySlice)
        {
            Memory::Memcpy(StagingContents + StagingOffset, MipData + (ArraySlice * SubresourceSize), SubresourceSize);

            [UploadBatch.GetBlitEncoder() copyFromBuffer:StagingStorage.GetBuffer()
                                           sourceOffset:StagingStorage.GetResourceOffset() + StagingOffset
                                      sourceBytesPerRow:(bIsTexture1D ? 0 : RowPitch)
                                    sourceBytesPerImage:(bIsTexture3D ? SlicePitch : 0)
                                             sourceSize:MTLSizeMake(Width, Height, Depth)
                                              toTexture:Texture
                                       destinationSlice:ArraySlice
                                       destinationLevel:MipIndex
                                      destinationOrigin:MTLOriginMake(0, 0, 0)];

            StagingOffset += Math::AlignUp<uint64>(SubresourceSize, TEXTURE_UPLOAD_ALIGNMENT);
        }

        Width  = Math::Max(Width / 2, 1u);
        Height = Math::Max(Height / 2, 1u);
        Depth  = Math::Max(Depth / 2, 1u);
    }

    UploadBatch.Submit();
    return true;
}

bool FMetalTextureRHI::CreateDefaultViews()
{
    if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
    {
        ShaderResourceView = new FMetalShaderResourceViewRHI(GetDevice(), this, CreateDefaultSRVDesc(Desc));
        if (!ShaderResourceView->Initialize())
        {
            return false;
        }
    }

    if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV() && !IsTextureCube(Desc.Dimension))
    {
        UnorderedAccessView = new FMetalUnorderedAccessViewRHI(GetDevice(), this, CreateDefaultUAVDesc(Desc));
        if (!UnorderedAccessView->Initialize())
        {
            return false;
        }
    }

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        RenderTargetView = new FMetalRenderTargetViewRHI(GetDevice(), this, CreateDefaultRTVDesc(Desc));
        if (!RenderTargetView->Initialize())
        {
            return false;
        }
    }

    if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
    {
        DepthStencilView = new FMetalDepthStencilViewRHI(GetDevice(), this, CreateDefaultDSVDesc(Desc));
        if (!DepthStencilView->Initialize())
        {
            return false;
        }
    }

    return true;
}

void FMetalTextureRHI::SetDebugName(const String& InName)
{
    @autoreleasepool
    {
        id<MTLTexture> TextureHandle = GetMTLTexture();
        if (TextureHandle)
        {
            TextureHandle.label = InName.GetNSString();
        }
    }
}

void FMetalTextureRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();

    @autoreleasepool
    {
        id<MTLTexture> TextureHandle = GetMTLTexture();
        if (TextureHandle)
        {
            OutDebugName = String(TextureHandle.label);
        }
    }
}

id<MTLTexture> FMetalTextureRHI::GetMTLTexture() const
{
    if (SwapChain)
    {   
        return SwapChain->GetDrawableTexture();
    }
    else
    {
        return Texture;
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
