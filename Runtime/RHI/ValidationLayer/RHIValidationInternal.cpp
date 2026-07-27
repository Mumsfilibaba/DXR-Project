#include "Core/Misc/ConsoleManager.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"

static TAutoConsoleVariable<bool> CVarEnableValidationDebugBreak(
    "RHI.EnableValidationDebugBreak",
    "Enables debug-breaks when detecting errors in the custom RHI-validation layer",
    true);

bool RHIValidationInternal::ShouldBreakOnValidationError()
{
    return CVarEnableValidationDebugBreak.GetValue();
}

ERHIType RHIValidationInternal::SafeGetRHIType(FRHIDevice* RealRHI)
{
    return RealRHI ? RealRHI->GetRHIType() : ERHIType::Unknown;
}

bool RHIValidationInternal::IsBufferValidAsCopyDestination(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopyDest() || BufferDesc.IsReadBack();
}

bool RHIValidationInternal::IsBufferValidAsCopySource(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopySource();
}

bool RHIValidationInternal::IsDepthStencilFormat(EFormat Format)
{
    return Format == EFormat::D16_Unorm || Format == EFormat::D24_Unorm_S8_Uint || Format == EFormat::D32_Float;
}

bool RHIValidationInternal::ValidateBufferRange(const TCHAR* Caller, const FRHIBufferDesc& BufferDesc, uint64 Offset, uint64 Size)
{
    if (!RHIValidationHelpers::IsRangeValid(BufferDesc.Size, Offset, Size))
    {
        RHI_VALIDATION_ERROR("%s: non-empty range [Offset=%llu, Size=%llu] exceeds buffer size %llu.", Caller, Offset, Size, BufferDesc.Size);
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateIndirectCountBuffer(const CHAR* Operation, FRHIBuffer* CountBuffer, uint64 CountBufferOffset)
{
    if (!CountBuffer || !CountBuffer->GetDesc().IsIndirectArguments())
    {
        RHI_VALIDATION_ERROR("%s requires a non-null IndirectArguments count buffer.", Operation);
        return false;
    }

    if ((CountBufferOffset % RHIValidationHelpers::IndirectArgumentOffsetAlignment) != 0 ||
        !RHIValidationHelpers::IsIndirectCommandRangeValid(CountBuffer->GetDesc().Size, CountBufferOffset, sizeof(uint32), 1))
    {
        RHI_VALIDATION_ERROR("%s count-buffer range is misaligned or outside the buffer.", Operation);
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateBufferView(const TCHAR* Caller, const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, uint32 FirstElement, uint32 NumElements, EFormat Format)
{
    if (ViewType == EBufferViewType::Unknown)
    {
        RHI_VALIDATION_ERROR("%s: buffer view type cannot be Unknown.", Caller);
        return false;
    }

    uint32 ElementSize = 0;
    switch (ViewType)
    {
        case EBufferViewType::Structured:
        {
            if (BufferDesc.Stride == 0)
            {
                RHI_VALIDATION_ERROR("%s: structured buffer views require a non-zero buffer stride.", Caller);
                return false;
            }

            ElementSize = BufferDesc.Stride;
            break;
        }

        case EBufferViewType::ByteAddress:
        {
            if (Format != EFormat::Unknown)
            {
                RHI_VALIDATION_ERROR("%s: byte-address buffer views must use EFormat::Unknown.", Caller);
                return false;
            }

            ElementSize = sizeof(uint32);
            break;
        }

        case EBufferViewType::Typed:
        {
            if (Format == EFormat::Unknown || IsTypelessFormat(Format))
            {
                RHI_VALIDATION_ERROR("%s: typed buffer views require a concrete typed format.", Caller);
                return false;
            }

            ElementSize = GetByteStrideFromFormat(Format);
            if (ElementSize == 0)
            {
                RHI_VALIDATION_ERROR("%s: format '%s' is not valid for a typed buffer view.", Caller, ToString(Format));
                return false;
            }

            break;
        }

        default:
            return false;
    }

    const uint64 ByteOffset = uint64(FirstElement) * uint64(ElementSize);
    const uint64 ByteSize   = uint64(NumElements) * uint64(ElementSize);

    return ValidateBufferRange(Caller, BufferDesc, ByteOffset, ByteSize);
}

bool RHIValidationInternal::ValidateTextureMip(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, IntVector3& OutExtent)
{
    if (MipLevel >= TextureDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("%s: mip level %u exceeds texture mip count %u.", Caller, MipLevel, TextureDesc.NumMipLevels);
        return false;
    }

    OutExtent.X = Math::Max<int32>(int32(TextureDesc.Extent.X >> MipLevel), 1);
    OutExtent.Y = Math::Max<int32>(int32(TextureDesc.Extent.Y >> MipLevel), 1);
    OutExtent.Z = TextureDesc.Dimension == ETextureDimension::Texture3D ? Math::Max<int32>(int32(TextureDesc.Extent.Z >> MipLevel), 1) : 1;
    return true;
}

bool RHIValidationInternal::ValidateTextureRegion2D(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion2D& Region)
{
    IntVector3 MipExtent;
    if (!ValidateTextureMip(Caller, TextureDesc, MipLevel, MipExtent))
    {
        return false;
    }

    if (Region.Width == 0 || Region.Height == 0)
    {
        RHI_VALIDATION_ERROR("%s: region width and height must be greater than zero.", Caller);
        return false;
    }

    if (Region.PositionX > uint32(MipExtent.X) || Region.Width > uint32(MipExtent.X) - Region.PositionX ||
        Region.PositionY > uint32(MipExtent.Y) || Region.Height > uint32(MipExtent.Y) - Region.PositionY)
    {
        RHI_VALIDATION_ERROR("%s: region exceeds mip %u extent (%u x %u).", Caller, MipLevel, uint32(MipExtent.X), uint32(MipExtent.Y));
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateTextureRegion3D(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion3D& Region)
{
    IntVector3 MipExtent;
    if (!ValidateTextureMip(Caller, TextureDesc, MipLevel, MipExtent))
    {
        return false;
    }

    if (Region.Width == 0 || Region.Height == 0 || Region.Depth == 0)
    {
        RHI_VALIDATION_ERROR("%s: region width, height, and depth must be greater than zero.", Caller);
        return false;
    }

    if (Region.PositionX > uint32(MipExtent.X) || Region.Width > uint32(MipExtent.X) - Region.PositionX ||
        Region.PositionY > uint32(MipExtent.Y) || Region.Height > uint32(MipExtent.Y) - Region.PositionY ||
        Region.PositionZ > uint32(MipExtent.Z) || Region.Depth > uint32(MipExtent.Z) - Region.PositionZ)
    {
        RHI_VALIDATION_ERROR("%s: region exceeds mip %u extent (%u x %u x %u).", Caller, MipLevel, uint32(MipExtent.X), uint32(MipExtent.Y), uint32(MipExtent.Z));
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateTextureSlicesAndMips(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 BaseLayer, uint32 LayerCount,
    uint32 FirstMip, uint32 NumMips, EFormat ViewFormat, EViewDimension ViewDimension)
{
    if (ViewFormat == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("%s: Format cannot be EFormat::Unknown.", Caller);
        return false;
    }

    if (IsTypelessFormat(ViewFormat))
    {
        RHI_VALIDATION_ERROR("%s: Format cannot be a typeless format.", Caller);
        return false;
    }

    if (!IsViewDimensionCompatible(TextureDesc.Dimension, ViewDimension))
    {
        RHI_VALIDATION_ERROR("%s: ViewDimension '%s' is incompatible with texture dimension '%s'.", Caller, ToString(ViewDimension), ToString(TextureDesc.Dimension));
        return false;
    }

    const uint32 MaxLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);
    if (!RHIValidationHelpers::IsSubresourceRangeValid(MaxLayers, BaseLayer, LayerCount))
    {
        RHI_VALIDATION_ERROR("%s: slice range [Base=%u, Count=%u] exceeds texture native layer count (%u).", Caller, BaseLayer, LayerCount, MaxLayers);
        return false;
    }

    if (!RHIValidationHelpers::IsSubresourceRangeValid(TextureDesc.NumMipLevels, FirstMip, NumMips))
    {
        RHI_VALIDATION_ERROR("%s: mip range [First=%u, Count=%u] exceeds texture mip count (%u).", Caller, FirstMip, NumMips, TextureDesc.NumMipLevels);
        return false;
    }

    return true;
}
