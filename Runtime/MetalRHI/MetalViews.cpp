#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalTexture.h"

struct FMetalSubresourceRange
{
    uint32 FirstMip   = 0;
    uint32 NumMips    = 1;
    uint32 FirstSlice = 0;
    uint32 NumSlices  = 1;
};

static FMetalSubresourceRange ResolveSRVRange(const FRHIShaderResourceViewDesc& InDesc)
{
    FMetalSubresourceRange Range;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            Range.FirstMip = InDesc.Texture1D.FirstMipLevel;
            Range.NumMips  = InDesc.Texture1D.NumMips;
            break;

        case EViewDimension::Texture1DArray:
            Range.FirstMip   = InDesc.Texture1DArray.FirstMipLevel;
            Range.NumMips    = InDesc.Texture1DArray.NumMips;
            Range.FirstSlice = InDesc.Texture1DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            Range.FirstMip = InDesc.Texture2D.FirstMipLevel;
            Range.NumMips  = InDesc.Texture2D.NumMips;
            break;

        case EViewDimension::Texture2DArray:
            Range.FirstMip   = InDesc.Texture2DArray.FirstMipLevel;
            Range.NumMips    = InDesc.Texture2DArray.NumMips;
            Range.FirstSlice = InDesc.Texture2DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::TextureCube:
            Range.FirstMip  = InDesc.TextureCube.FirstMipLevel;
            Range.NumMips   = InDesc.TextureCube.NumMips;
            Range.NumSlices = RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::TextureCubeArray:
            Range.FirstMip   = InDesc.TextureCubeArray.FirstMipLevel;
            Range.NumMips    = InDesc.TextureCubeArray.NumMips;
            Range.FirstSlice = InDesc.TextureCubeArray.FirstCube * RHI_NUM_CUBE_FACES;
            Range.NumSlices  = InDesc.TextureCubeArray.NumCubes * RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::Texture3D:
            Range.FirstMip = InDesc.Texture3D.FirstMipLevel;
            Range.NumMips  = InDesc.Texture3D.NumMips;
            break;

        default:
            break;
    }

    return Range;
}

static FMetalSubresourceRange ResolveUAVRange(const FRHIUnorderedAccessViewDesc& InDesc)
{
    FMetalSubresourceRange Range;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            Range.FirstMip = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            Range.FirstMip   = InDesc.Texture1DArray.MipLevel;
            Range.FirstSlice = InDesc.Texture1DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            Range.FirstMip = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            Range.FirstMip   = InDesc.Texture2DArray.MipLevel;
            Range.FirstSlice = InDesc.Texture2DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::Texture3D:
            Range.FirstMip = InDesc.Texture3D.MipLevel;
            break;

        default:
            break;
    }

    return Range;
}

static uint32 ResolveBufferViewStride(const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, EFormat ViewFormat)
{
    switch (ViewType)
    {
        case EBufferViewType::ByteAddress: return sizeof(uint32);
        case EBufferViewType::Typed:       return GetByteStrideFromFormat(ViewFormat);
        default:                           return BufferDesc.Stride;
    }
}

static void ResolveRTVMipAndSlice(const FRHIRenderTargetViewDesc& InDesc, uint8& OutMipLevel, uint16& OutArrayIndex)
{
    OutMipLevel   = 0;
    OutArrayIndex = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            OutMipLevel = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            OutMipLevel   = InDesc.Texture1DArray.MipLevel;
            OutArrayIndex = InDesc.Texture1DArray.FirstArraySlice;
            break;

        case EViewDimension::Texture2D:
            OutMipLevel = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            OutMipLevel   = InDesc.Texture2DArray.MipLevel;
            OutArrayIndex = InDesc.Texture2DArray.FirstArraySlice;
            break;

        case EViewDimension::TextureCube:
            OutMipLevel = InDesc.TextureCube.MipLevel;
            break;

        case EViewDimension::TextureCubeArray:
            OutMipLevel   = InDesc.TextureCubeArray.MipLevel;
            OutArrayIndex = InDesc.TextureCubeArray.FirstCube;
            break;

        case EViewDimension::Texture3D:
            OutMipLevel   = InDesc.Texture3D.MipLevel;
            OutArrayIndex = InDesc.Texture3D.FirstWSlice;
            break;

        default:
            break;
    }
}

static void ResolveDSVMipAndSlice(const FRHIDepthStencilViewDesc& InDesc, uint8& OutMipLevel, uint16& OutArrayIndex)
{
    OutMipLevel   = 0;
    OutArrayIndex = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            OutMipLevel = InDesc.Texture1D.MipLevel;
            break;
        
        case EViewDimension::Texture1DArray:
            OutMipLevel   = InDesc.Texture1DArray.MipLevel;
            OutArrayIndex = InDesc.Texture1DArray.FirstArraySlice;
            break;

        case EViewDimension::Texture2D:
            OutMipLevel = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            OutMipLevel   = InDesc.Texture2DArray.MipLevel;
            OutArrayIndex = InDesc.Texture2DArray.FirstArraySlice;
            break;
        
        case EViewDimension::TextureCube:
            OutMipLevel = InDesc.TextureCube.MipLevel;
            break;
        
        case EViewDimension::TextureCubeArray: 
            OutMipLevel   = InDesc.TextureCubeArray.MipLevel; 
            OutArrayIndex = InDesc.TextureCubeArray.FirstCube;
            break;
        
        default: 
            break;
    }
}

FMetalView::FMetalView(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , TextureView(nil)
    , BufferView(nil)
    , BufferOffset(0)
    , BufferSize(0)
{
}

FMetalView::~FMetalView()
{
    if (TextureView)
    {
        FMetalDeviceRHI::DeferDeletion(TextureView);
        [TextureView release];
        TextureView = nil;
    }

    if (BufferView)
    {
        FMetalDeviceRHI::DeferDeletion(BufferView);
        [BufferView release];
        BufferView = nil;
    }
}

bool FMetalView::InitializeTextureView(FRHITexture* InTexture, EFormat InFormat, EViewDimension InViewDimension, 
    uint32 InFirstMip, uint32 InNumMips, uint32 InFirstSlice, uint32 InNumSlices)
{
    FMetalTextureRHI* MetalTexture = GetMetalTexture(InTexture);
    if (!MetalTexture)
    {
        METAL_ERROR("Cannot create a texture view without a texture");
        return false;
    }

    id<MTLTexture> ParentTexture = MetalTexture->GetMTLTexture();
    if (!ParentTexture)
    {
        METAL_ERROR("Cannot create a texture view of an unallocated texture");
        return false;
    }

    const FRHITextureDesc& TextureDesc = MetalTexture->GetDesc();

    const MTLPixelFormat ViewFormat = MetalRHI::ConvertFormat(InFormat);
    const MTLTextureType ViewType   = MetalRHI::GetMTLTextureType(InViewDimension, TextureDesc.IsMultisampled());

    if (ViewFormat == MTLPixelFormatInvalid || ViewType == MTLTextureType(-1))
    {
        METAL_ERROR("Texture view has an unsupported format or dimension");
        return false;
    }

    const uint32 ParentNumMips   = Math::Max(TextureDesc.NumMipLevels, 1u);
    const uint32 ParentNumSlices = RHIDimensionArrayLayers(TextureDesc.Dimension, Math::Max(TextureDesc.NumArraySlices, 1u));
    const uint32 NumMips         = (InNumMips   == 0) ? (ParentNumMips   - InFirstMip)   : InNumMips;
    const uint32 NumSlices       = (InNumSlices == 0) ? (ParentNumSlices - InFirstSlice) : InNumSlices;

    if ((InFirstMip + NumMips) > ParentNumMips || (InFirstSlice + NumSlices) > ParentNumSlices)
    {
        METAL_ERROR("Texture view range lies outside the texture");
        return false;
    }

    const bool bCoversWholeTexture = ViewFormat == ParentTexture.pixelFormat 
        && ViewType == ParentTexture.textureType
        && InFirstMip == 0 && NumMips == ParentNumMips 
        && InFirstSlice == 0 && NumSlices == ParentNumSlices;

    if (bCoversWholeTexture)
    {
        TextureView = [ParentTexture retain];
        return true;
    }

    TextureView = [ParentTexture newTextureViewWithPixelFormat:ViewFormat
                                                   textureType:ViewType
                                                        levels:NSMakeRange(InFirstMip, NumMips)
                                                        slices:NSMakeRange(InFirstSlice, NumSlices)];
    if (!TextureView)
    {
        METAL_ERROR("Failed to create a texture view");
        return false;
    }

    return true;
}

bool FMetalView::InitializeBufferView(FRHIBuffer* InBuffer, uint64 InOffset, uint64 InSize)
{
    FMetalBufferRHI* MetalBuffer = GetMetalBuffer(InBuffer);
    if (!MetalBuffer)
    {
        METAL_ERROR("Cannot create a buffer view without a buffer");
        return false;
    }

    id<MTLBuffer> ParentBuffer = MetalBuffer->GetMTLBuffer();
    if (!ParentBuffer)
    {
        METAL_ERROR("Cannot create a buffer view of an unallocated buffer");
        return false;
    }

    if ((InOffset + InSize) > ParentBuffer.length)
    {
        METAL_ERROR("Buffer view range lies outside the buffer");
        return false;
    }

    BufferView   = [ParentBuffer retain];
    BufferOffset = InOffset;
    BufferSize   = InSize;
    return true;
}

FMetalShaderResourceViewRHI::FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalShaderResourceViewRHI::~FMetalShaderResourceViewRHI() = default;

bool FMetalShaderResourceViewRHI::Initialize()
{
    if (Desc.IsAccelerationStructureSRV())
    {
        return true;
    }

    if (Desc.IsBufferSRV())
    {
        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(GetResource());
        if (!Buffer)
        {
            return false;
        }

        const uint32 Stride = ResolveBufferViewStride(Buffer->GetDesc(), Desc.Buffer.Type, Desc.Buffer.Format);
        return InitializeBufferView(Buffer, uint64(Desc.Buffer.FirstElement) * Stride, uint64(Desc.Buffer.NumElements) * Stride);
    }

    const FMetalSubresourceRange Range = ResolveSRVRange(Desc);
    return InitializeTextureView(static_cast<FRHITexture*>(GetResource()), Desc.GetFormat(),
        Desc.ViewDimension, Range.FirstMip, Range.NumMips, Range.FirstSlice, Range.NumSlices);
}

FMetalUnorderedAccessViewRHI::FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FRHIUnorderedAccessView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalUnorderedAccessViewRHI::~FMetalUnorderedAccessViewRHI() = default;

bool FMetalUnorderedAccessViewRHI::Initialize()
{
    if (Desc.IsBufferUAV())
    {
        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(GetResource());
        if (!Buffer)
        {
            return false;
        }

        const uint32 Stride = ResolveBufferViewStride(Buffer->GetDesc(), Desc.Buffer.Type, Desc.Buffer.Format);
        return InitializeBufferView(Buffer, uint64(Desc.Buffer.FirstElement) * Stride, uint64(Desc.Buffer.NumElements) * Stride);
    }

    if (IsCubeViewDimension(Desc.ViewDimension))
    {
        METAL_ERROR("Metal has no writable cube textures");
        return false;
    }

    if (!MetalRHI::MetalFormatSupportsShaderWrite(MetalRHI::ConvertFormat(Desc.GetFormat()), GMetalReadWriteTextureTier))
    {
        METAL_ERROR("Format '%s' cannot be written from a shader on this device", ToString(Desc.GetFormat()));
        return false;
    }

    const FMetalSubresourceRange Range = ResolveUAVRange(Desc);
    return InitializeTextureView(static_cast<FRHITexture*>(GetResource()), Desc.GetFormat(),
        Desc.ViewDimension, Range.FirstMip, Range.NumMips, Range.FirstSlice, Range.NumSlices);
}

FMetalRenderTargetViewRHI::FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc)
    : FRHIRenderTargetView(InTexture, InDesc)
    , FMetalView(InDevice)
    , MipLevel(0)
    , ArrayIndex(0)
{
    ResolveRTVMipAndSlice(InDesc, MipLevel, ArrayIndex);
}

FMetalRenderTargetViewRHI::~FMetalRenderTargetViewRHI() = default;

bool FMetalRenderTargetViewRHI::Initialize()
{
    FMetalTextureRHI* MetalTexture = GetMetalTexture(static_cast<FRHITexture*>(GetResource()));
    if (!MetalTexture)
    {
        return false;
    }

    if (Desc.GetFormat() == MetalTexture->GetDesc().Format)
    {
        return true;
    }

    return InitializeTextureView(MetalTexture, Desc.GetFormat(), Desc.ViewDimension, MipLevel, 1, ArrayIndex, 1);
}

FMetalDepthStencilViewRHI::FMetalDepthStencilViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc)
    : FRHIDepthStencilView(InTexture, InDesc)
    , FMetalView(InDevice)
    , MipLevel(0)
    , ArrayIndex(0)
    , Flags(InDesc.Flags)
{
    ResolveDSVMipAndSlice(InDesc, MipLevel, ArrayIndex);
}

FMetalDepthStencilViewRHI::~FMetalDepthStencilViewRHI() = default;

bool FMetalDepthStencilViewRHI::Initialize()
{
    FMetalTextureRHI* MetalTexture = GetMetalTexture(static_cast<FRHITexture*>(GetResource()));
    if (!MetalTexture)
    {
        return false;
    }

    if (Desc.GetFormat() == MetalTexture->GetDesc().Format)
    {
        return true;
    }

    return InitializeTextureView(MetalTexture, Desc.GetFormat(), Desc.ViewDimension, MipLevel, 1, ArrayIndex, 1);
}

void* FMetalShaderResourceViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalShaderResourceViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalUnorderedAccessViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalRenderTargetViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

void* FMetalDepthStencilViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}
