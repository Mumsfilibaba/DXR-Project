#include "Core/Math/Math.h"
#include "RHI/RHI.h"
#include "RendererCore/RenderGraph/RenderGraphViews.h"

static FRHITextureSubresourceRange CreateTextureSubresourceRange(uint32 FirstMip, uint32 NumMips, uint32 FirstSlice, uint32 NumSlices)
{
    FRHITextureSubresourceRange Range;
    Range.FirstMipLevel   = FirstMip;
    Range.NumMipLevels    = NumMips;
    Range.FirstArraySlice = FirstSlice;
    Range.NumArraySlices  = NumSlices;
    Range.FirstPlaneSlice = 0;
    Range.NumPlaneSlices  = RHI_ALL_PLANE_SLICES;
    return Range;
}

static uint32 ResolveMipCount(uint32 FirstMip, uint32 NumMips, uint32 TextureMipCount)
{
    if (NumMips == 0 || NumMips >= RHI_ALL_MIP_LEVELS)
    {
        return TextureMipCount > FirstMip ? TextureMipCount - FirstMip : 1u;
    }

    return NumMips;
}

static uint32 ResolveSliceCount(uint32 FirstSlice, uint32 NumSlices, uint32 TextureSliceCount)
{
    if (NumSlices == 0 || NumSlices >= RHI_ALL_ARRAY_SLICES)
    {
        return TextureSliceCount > FirstSlice ? TextureSliceCount - FirstSlice : 1u;
    }

    return NumSlices;
}

static FRHITextureSubresourceRange DeriveSubresourceRange(FRenderGraphTexture* Parent, const FRHIShaderResourceViewDesc& Desc)
{
    if (!Parent)
    {
        return FRHITextureSubresourceRange::All();
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    const uint32           MaxLayers   = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return CreateTextureSubresourceRange(Desc.Texture1D.FirstMipLevel, 
                ResolveMipCount(Desc.Texture1D.FirstMipLevel, Desc.Texture1D.NumMips, TextureDesc.NumMipLevels), 0, 1);

        case EViewDimension::Texture1DArray:
            return CreateTextureSubresourceRange(Desc.Texture1DArray.FirstMipLevel, 
                ResolveMipCount(Desc.Texture1DArray.FirstMipLevel, Desc.Texture1DArray.NumMips, TextureDesc.NumMipLevels), Desc.Texture1DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, MaxLayers));

        case EViewDimension::Texture2D:
            return CreateTextureSubresourceRange(Desc.Texture2D.FirstMipLevel, 
                ResolveMipCount(Desc.Texture2D.FirstMipLevel, Desc.Texture2D.NumMips, TextureDesc.NumMipLevels), 0, 1);

        case EViewDimension::Texture2DArray:
            return CreateTextureSubresourceRange(Desc.Texture2DArray.FirstMipLevel, 
                ResolveMipCount(Desc.Texture2DArray.FirstMipLevel, Desc.Texture2DArray.NumMips, TextureDesc.NumMipLevels), Desc.Texture2DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, MaxLayers));

        case EViewDimension::TextureCube:
            return CreateTextureSubresourceRange(Desc.TextureCube.FirstMipLevel, 
                ResolveMipCount(Desc.TextureCube.FirstMipLevel, Desc.TextureCube.NumMips, TextureDesc.NumMipLevels), 0, 6);

        case EViewDimension::TextureCubeArray:
        {
            const uint32 FirstLayer = Desc.TextureCubeArray.FirstCube * 6u;
            const uint32 NumLayers  = ResolveSliceCount(FirstLayer, Desc.TextureCubeArray.NumCubes * 6u, MaxLayers);
            return CreateTextureSubresourceRange(Desc.TextureCubeArray.FirstMipLevel, 
                ResolveMipCount(Desc.TextureCubeArray.FirstMipLevel, Desc.TextureCubeArray.NumMips, TextureDesc.NumMipLevels), FirstLayer, NumLayers);
        }

        case EViewDimension::Texture3D:
            return CreateTextureSubresourceRange(Desc.Texture3D.FirstMipLevel, 
                ResolveMipCount(Desc.Texture3D.FirstMipLevel, Desc.Texture3D.NumMips, TextureDesc.NumMipLevels), 0, 1);

        default:
            return FRHITextureSubresourceRange::All();
    }
}

static FRHITextureSubresourceRange DeriveSubresourceRange(FRenderGraphTexture* Parent, const FRHIUnorderedAccessViewDesc& Desc)
{
    if (!Parent)
    {
        return FRHITextureSubresourceRange::All();
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    const uint32           MaxLayers   = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return CreateTextureSubresourceRange(Desc.Texture1D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture1DArray:
            return CreateTextureSubresourceRange(Desc.Texture1DArray.MipLevel, 1, Desc.Texture1DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, MaxLayers));

        case EViewDimension::Texture2D:
            return CreateTextureSubresourceRange(Desc.Texture2D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture2DArray:
            return CreateTextureSubresourceRange(Desc.Texture2DArray.MipLevel, 1, Desc.Texture2DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, MaxLayers));

        case EViewDimension::TextureCube:
            return CreateTextureSubresourceRange(Desc.TextureCube.MipLevel, 1, 0, 6);

        case EViewDimension::TextureCubeArray:
        {
            const uint32 FirstLayer = Desc.TextureCubeArray.FirstCube * 6u;
            return CreateTextureSubresourceRange(Desc.TextureCubeArray.MipLevel, 1, FirstLayer, 
                ResolveSliceCount(FirstLayer, Desc.TextureCubeArray.NumCubes * 6u, MaxLayers));
        }

        case EViewDimension::Texture3D:
            return CreateTextureSubresourceRange(Desc.Texture3D.MipLevel, 1, Desc.Texture3D.FirstWSlice, 
                ResolveSliceCount(Desc.Texture3D.FirstWSlice, Desc.Texture3D.WSize, TextureDesc.Extent.Z));

        default:
            return FRHITextureSubresourceRange::All();
    }
}

static FRHITextureSubresourceRange DeriveSubresourceRange(FRenderGraphTexture* Parent, const FRHIRenderTargetViewDesc& Desc)
{
    if (!Parent)
    {
        return FRHITextureSubresourceRange::All();
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    const uint32           MaxLayers   = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return CreateTextureSubresourceRange(Desc.Texture1D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture1DArray:
            return CreateTextureSubresourceRange(Desc.Texture1DArray.MipLevel, 1, Desc.Texture1DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, MaxLayers));

        case EViewDimension::Texture2D:
            return CreateTextureSubresourceRange(Desc.Texture2D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture2DArray:
            return CreateTextureSubresourceRange(Desc.Texture2DArray.MipLevel, 1, Desc.Texture2DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, MaxLayers));

        case EViewDimension::TextureCube:
            return CreateTextureSubresourceRange(Desc.TextureCube.MipLevel, 1, 0, 6);

        case EViewDimension::TextureCubeArray:
        {
            const uint32 FirstLayer = Desc.TextureCubeArray.FirstCube * 6u;
            return CreateTextureSubresourceRange(Desc.TextureCubeArray.MipLevel, 1, FirstLayer, 
                ResolveSliceCount(FirstLayer, Desc.TextureCubeArray.NumCubes * 6u, MaxLayers));
        }

        case EViewDimension::Texture3D:
            return CreateTextureSubresourceRange(Desc.Texture3D.MipLevel, 1, Desc.Texture3D.FirstWSlice, 
                ResolveSliceCount(Desc.Texture3D.FirstWSlice, Desc.Texture3D.WSize, TextureDesc.Extent.Z));

        default:
            return FRHITextureSubresourceRange::All();
    }
}

static FRHITextureSubresourceRange DeriveSubresourceRange(FRenderGraphTexture* Parent, const FRHIDepthStencilViewDesc& Desc)
{
    if (!Parent)
    {
        return FRHITextureSubresourceRange::All();
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    const uint32           MaxLayers   = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return CreateTextureSubresourceRange(Desc.Texture1D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture1DArray:
            return CreateTextureSubresourceRange(Desc.Texture1DArray.MipLevel, 1, Desc.Texture1DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, MaxLayers));

        case EViewDimension::Texture2D:
            return CreateTextureSubresourceRange(Desc.Texture2D.MipLevel, 1, 0, 1);

        case EViewDimension::Texture2DArray:
            return CreateTextureSubresourceRange(Desc.Texture2DArray.MipLevel, 1, Desc.Texture2DArray.FirstArraySlice, 
                ResolveSliceCount(Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, MaxLayers));

        case EViewDimension::TextureCube:
            return CreateTextureSubresourceRange(Desc.TextureCube.MipLevel, 1, 0, 6);

        case EViewDimension::TextureCubeArray:
        {
            const uint32 FirstLayer = Desc.TextureCubeArray.FirstCube * 6u;
            return CreateTextureSubresourceRange(Desc.TextureCubeArray.MipLevel, 1, FirstLayer, 
                ResolveSliceCount(FirstLayer, Desc.TextureCubeArray.NumCubes * 6u, MaxLayers));
        }

        default:
            return FRHITextureSubresourceRange::All();
    }
}

static uint64 GetBufferElementSize(const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, EFormat Format)
{
    switch (ViewType)
    {
        case EBufferViewType::Structured:
            return BufferDesc.Stride;

        case EBufferViewType::ByteAddress:
            return 4;

        case EBufferViewType::Typed:
            return GetByteStrideFromFormat(Format);

        default:
            return BufferDesc.Stride > 0 ? BufferDesc.Stride : 1;
    }
}

static FBufferRegion DeriveBufferRange(FRenderGraphBuffer* Parent, const FRHIShaderResourceViewDesc& Desc)
{
    if (!Parent || Desc.ViewDimension != EViewDimension::Buffer)
    {
        return FBufferRegion::Whole();
    }

    const FRHIBufferDesc& BufferDesc   = Parent->GetDesc().BufferDesc;
    const uint64          ElementSize  = GetBufferElementSize(BufferDesc, Desc.Buffer.Type, Desc.Buffer.Format);
    const uint64          ByteOffset   = static_cast<uint64>(Desc.Buffer.FirstElement) * ElementSize;
    const uint64          ByteSize     = static_cast<uint64>(Desc.Buffer.NumElements) * ElementSize;
    return FBufferRegion(ByteOffset, ByteSize);
}

static FBufferRegion DeriveBufferRange(FRenderGraphBuffer* Parent, const FRHIUnorderedAccessViewDesc& Desc)
{
    if (!Parent || Desc.ViewDimension != EViewDimension::Buffer)
    {
        return FBufferRegion::Whole();
    }

    const FRHIBufferDesc& BufferDesc   = Parent->GetDesc().BufferDesc;
    const uint64          ElementSize  = GetBufferElementSize(BufferDesc, Desc.Buffer.Type, Desc.Buffer.Format);
    const uint64          ByteOffset   = static_cast<uint64>(Desc.Buffer.FirstElement) * ElementSize;
    const uint64          ByteSize     = static_cast<uint64>(Desc.Buffer.NumElements) * ElementSize;
    return FBufferRegion(ByteOffset, ByteSize);
}

static bool RangesOverlap(uint32 AFirst, uint32 ANum, uint32 BFirst, uint32 BNum)
{
    if (ANum == 0 || ANum >= RHI_ALL_MIP_LEVELS || BNum == 0 || BNum >= RHI_ALL_MIP_LEVELS)
    {
        return true;
    }

    const uint32 AEnd = AFirst + ANum;
    const uint32 BEnd = BFirst + BNum;
    return AFirst < BEnd && BFirst < AEnd;
}

static FRHITextureSubresourceRange NormalizeSubresourceRange(FRenderGraphTexture* Parent, FRHITextureSubresourceRange Range)
{
    if (!Parent || Range.IsAllSubresources())
    {
        return Range;
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    const uint32           MaxLayers   = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    const bool bCoversAllMips = Range.FirstMipLevel == 0 && 
        (Range.NumMipLevels >= TextureDesc.NumMipLevels || Range.NumMipLevels == RHI_ALL_MIP_LEVELS);
    const bool bCoversAllSlices = Range.FirstArraySlice == 0 && 
        (Range.NumArraySlices >= MaxLayers || Range.NumArraySlices == RHI_ALL_ARRAY_SLICES);
    const bool bCoversAllPlanes = Range.FirstPlaneSlice == 0 && 
        (Range.NumPlaneSlices >= RHI_ALL_PLANE_SLICES || Range.NumPlaneSlices == 0);

    if (bCoversAllMips && bCoversAllSlices && bCoversAllPlanes)
    {
        return FRHITextureSubresourceRange::All();
    }

    return Range;
}

static FBufferRegion NormalizeBufferRange(FRenderGraphBuffer* Parent, FBufferRegion Range)
{
    if (!Parent || (Range.Offset == 0 && Range.Size == RHI_WHOLE_SIZE))
    {
        return Range;
    }

    if (Range.Offset == 0 && Range.Size >= Parent->GetDesc().BufferDesc.Size)
    {
        return FBufferRegion::Whole();
    }

    return Range;
}

FRHIShaderResourceViewDesc RenderGraphDefaultViewDescs::ShaderResourceForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint8   NumMips   = static_cast<uint8>(TextureDesc.NumMipLevels);
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCube(Format, 0, NumMips);
    }

    if (TextureDesc.IsTextureCubeArray())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCubeArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture3D(Format, 0, NumMips);
    }

    return FRHIShaderResourceViewDesc{};
}

FRHIUnorderedAccessViewDesc RenderGraphDefaultViewDescs::UnorderedAccessForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIUnorderedAccessViewDesc{};
}

FRHIRenderTargetViewDesc RenderGraphDefaultViewDescs::RenderTargetForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIRenderTargetViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIRenderTargetViewDesc{};
}

FRHIDepthStencilViewDesc RenderGraphDefaultViewDescs::DepthStencilForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.ClearValue.Format != EFormat::Unknown ? TextureDesc.ClearValue.Format : TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIDepthStencilViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    return FRHIDepthStencilViewDesc{};
}

FRHIShaderResourceViewDesc RenderGraphDefaultViewDescs::ShaderResourceForBuffer(const FRHIBufferDesc& BufferDesc)
{
    const uint32 NumElements = BufferDesc.Stride > 0 ? static_cast<uint32>(BufferDesc.Size / BufferDesc.Stride) : 0;
    return FRHIShaderResourceViewDesc::CreateBuffer(0, NumElements, EBufferViewType::Structured);
}

FRHIUnorderedAccessViewDesc RenderGraphDefaultViewDescs::UnorderedAccessForBuffer(const FRHIBufferDesc& BufferDesc)
{
    const uint32 NumElements = BufferDesc.Stride > 0 ? static_cast<uint32>(BufferDesc.Size / BufferDesc.Stride) : 0;
    return FRHIUnorderedAccessViewDesc::CreateBuffer(0, NumElements, EBufferViewType::Structured);
}

bool RenderGraphViewRanges::Overlap(const FRHITextureSubresourceRange& A, const FRHITextureSubresourceRange& B)
{
    if (A.IsAllSubresources() || B.IsAllSubresources())
    {
        return true;
    }

    return RangesOverlap(A.FirstMipLevel, A.NumMipLevels, B.FirstMipLevel, B.NumMipLevels)
        && RangesOverlap(A.FirstArraySlice, A.NumArraySlices, B.FirstArraySlice, B.NumArraySlices)
        && RangesOverlap(A.FirstPlaneSlice, A.NumPlaneSlices, B.FirstPlaneSlice, B.NumPlaneSlices);
}

FRHITextureSubresourceRange RenderGraphViewRanges::SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIShaderResourceViewDesc& Desc)
{
    return NormalizeSubresourceRange(Parent, DeriveSubresourceRange(Parent, Desc));
}

FRHITextureSubresourceRange RenderGraphViewRanges::SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIUnorderedAccessViewDesc& Desc)
{
    return NormalizeSubresourceRange(Parent, DeriveSubresourceRange(Parent, Desc));
}

FRHITextureSubresourceRange RenderGraphViewRanges::SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIRenderTargetViewDesc& Desc)
{
    return NormalizeSubresourceRange(Parent, DeriveSubresourceRange(Parent, Desc));
}

FRHITextureSubresourceRange RenderGraphViewRanges::SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIDepthStencilViewDesc& Desc)
{
    return NormalizeSubresourceRange(Parent, DeriveSubresourceRange(Parent, Desc));
}

FBufferRegion RenderGraphViewRanges::BufferRangeFor(FRenderGraphBuffer* Parent, const FRHIShaderResourceViewDesc& Desc)
{
    return NormalizeBufferRange(Parent, DeriveBufferRange(Parent, Desc));
}

FBufferRegion RenderGraphViewRanges::BufferRangeFor(FRenderGraphBuffer* Parent, const FRHIUnorderedAccessViewDesc& Desc)
{
    return NormalizeBufferRange(Parent, DeriveBufferRange(Parent, Desc));
}
