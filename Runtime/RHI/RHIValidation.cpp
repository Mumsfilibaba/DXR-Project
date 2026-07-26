#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHIValidation.h"
#include "RHI/RHIValidationHelpers.h"

#include <cmath>

#define RHI_VALIDATION_ERROR(...) \
    do \
    { \
        LOG_ERROR("[RHI VALIDATION ERROR] " __VA_ARGS__); \
        if (CVarEnableValidationDebugBreak.GetValue()) \
        { \
            DEBUG_BREAK(); \
        } \
    } while (false)

#define RHI_VALIDATION_WARNING(...) \
    do \
    { \
        LOG_WARNING("[RHI VALIDATION WARNING] " __VA_ARGS__); \
    } while (false)

static TAutoConsoleVariable<bool> CVarEnableValidationDebugBreak(
    "RHI.EnableValidationDebugBreak",
    "Enables debug-breaks when detecting errors in the custom RHI-validation layer",
    true);

static ERHIType SafeGetRHIType(FRHIDevice* RealRHI)
{
    return RealRHI ? RealRHI->GetRHIType() : ERHIType::Unknown;
}

static bool CanUseBufferAsCopyDestination(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopyDest() || BufferDesc.IsReadBack();
}

static bool CanUseBufferAsCopySource(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopySource();
}

static bool IsDepthStencilFormat(EFormat Format)
{
    return Format == EFormat::D16_Unorm || Format == EFormat::D24_Unorm_S8_Uint || Format == EFormat::D32_Float;
}

static bool ValidateBufferRange(const TCHAR* Caller, const FRHIBufferDesc& BufferDesc, uint64 Offset, uint64 Size)
{
    if (!RHIValidationHelpers::IsRangeValid(BufferDesc.Size, Offset, Size))
    {
        RHI_VALIDATION_ERROR("%s: non-empty range [Offset=%llu, Size=%llu] exceeds buffer size %llu.", Caller, Offset, Size, BufferDesc.Size);
        return false;
    }

    return true;
}

static bool ValidateBufferView(const TCHAR* Caller, const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, uint32 FirstElement, uint32 NumElements, EFormat Format)
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

static bool ValidateTextureMip(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, IntVector3& OutExtent)
{
    if (MipLevel >= TextureDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("%s: mip level %u exceeds texture mip count %u.", Caller, MipLevel, TextureDesc.NumMipLevels);
        return false;
    }

    OutExtent.X = Math::Max<int32>(int32(TextureDesc.Extent.X >> MipLevel), 1);
    OutExtent.Y = Math::Max<int32>(int32(TextureDesc.Extent.Y >> MipLevel), 1);
    OutExtent.Z = TextureDesc.Dimension == ETextureDimension::Texture3D
        ? Math::Max<int32>(int32(TextureDesc.Extent.Z >> MipLevel), 1)
        : 1;

    return true;
}

static bool ValidateTextureRegion2D(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion2D& Region)
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

static bool ValidateTextureRegion3D(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion3D& Region)
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

static bool ValidateTextureSlicesAndMips(const TCHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 BaseLayer, uint32 LayerCount, uint32 FirstMip, uint32 NumMips, EFormat ViewFormat, EViewDimension ViewDimension)
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

FRHIValidation::FRHIValidation(FRHIDevice* InRealRHI)
    : FRHIDevice()
    , RealRHI(InRealRHI)
{
}

ERHIType FRHIValidation::GetRHIType() const
{
    return SafeGetRHIType(RealRHI);
}

FRHIValidation::~FRHIValidation()
{
    for (auto It = RealContextToValidationContextMap.CreateIterator(); !It.IsEnd(); ++It)
    {
        delete It.GetValue();
    }

    RealContextToValidationContextMap.Clear();

    delete RealRHI;
    RealRHI = nullptr;
}

void FRHIValidation::BeginFrame()
{
    RealRHI->BeginFrame();
}

void FRHIValidation::EndFrame()
{
    RealRHI->EndFrame();
}

FRHITexture* FRHIValidation::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
	// -------------------------------------------------------------------------------------------
	// Basic sanity
	// -------------------------------------------------------------------------------------------

	if (InTextureDesc.Dimension == ETextureDimension::None)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Invalid texture dimension (None). A valid ETextureDimension must be specified.");
		return nullptr;
	}

    if (InTextureDesc.Format == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateTexture: Format cannot be EFormat::Unknown.");
        return nullptr;
    }

	if (InTextureDesc.Extent.X == 0 || InTextureDesc.Extent.Y == 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Invalid texture extent (Width=%u, Height=%u). Both dimensions must be greater than zero.",
            InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
		return nullptr;
	}

	if (InTextureDesc.IsTexture3D())
	{
		if (InTextureDesc.Extent.Z == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: Texture3D requires Depth > 0. (Depth=%u).", InTextureDesc.Extent.Z);
			return nullptr;
		}

		if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: Texture3D must have NumArraySlices == 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}
	}
	else if (InTextureDesc.Extent.Z != 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Non-3D textures must have Depth == 0. (Depth=%u).", InTextureDesc.Extent.Z);
		return nullptr;
	}

	// -------------------------------------------------------------------------------------------
	// Dimension-specific rules
	// -------------------------------------------------------------------------------------------

	switch (InTextureDesc.Dimension)
	{
	case ETextureDimension::Texture1D:
		if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture1D) NumArraySlices must be 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::Texture1DArray:
		if (InTextureDesc.NumArraySlices == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture1DArray) NumArraySlices must be >= 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::Texture2D:
		if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2D) NumArraySlices must be 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::Texture2DArray:
		if (InTextureDesc.NumArraySlices == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2DArray) NumArraySlices must be >= 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::TextureCube:
		if (InTextureDesc.Extent.X != InTextureDesc.Extent.Y)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Faces must be square. (Width=%u, Height=%u).", InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
			return nullptr;
		}
		
        if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) NumArraySlices must be 1. (NumArraySlices=%u). Use TextureCubeArray for arrays.", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::TextureCubeArray:
		if (InTextureDesc.Extent.X != InTextureDesc.Extent.Y)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) Faces must be square. (Width=%u, Height=%u).", InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
			return nullptr;
		}

		if (InTextureDesc.NumArraySlices == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) NumArraySlices must be >= 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::Texture3D:
		break;

	default:
		RHI_VALIDATION_ERROR("CreateTexture: Unsupported ETextureDimension enum value (%s).", ToString(InTextureDesc.Dimension));
		return nullptr;
	}

	// -------------------------------------------------------------------------------------------
	// Device feature support checks
	// -------------------------------------------------------------------------------------------
	if (InTextureDesc.IsTexture3D())
	{
		if (static_cast<uint32>(InTextureDesc.Extent.X) > RHI::MaxTexture3DWidth || static_cast<uint32>(InTextureDesc.Extent.Y) > RHI::MaxTexture3DHeight ||
			static_cast<uint32>(InTextureDesc.Extent.Z) > RHI::MaxTexture3DDepth)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture3D) Extent (%u,%u,%u) exceeds device feature support limit (%u,%u,%u).",
                InTextureDesc.Extent.X, InTextureDesc.Extent.Y, InTextureDesc.Extent.Z,
                RHI::MaxTexture3DWidth, RHI::MaxTexture3DHeight, RHI::MaxTexture3DDepth);
			return nullptr;
		}
	}
	else if (InTextureDesc.IsTextureCube() || InTextureDesc.IsTextureCubeArray())
	{
		if (static_cast<uint32>(InTextureDesc.Extent.X) > RHI::MaxCubeTextureSize || static_cast<uint32>(InTextureDesc.Extent.Y) > RHI::MaxCubeTextureSize)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Face extent (%u,%u) exceeds device feature support limit (%u).", InTextureDesc.Extent.X, InTextureDesc.Extent.Y, RHI::MaxCubeTextureSize);
			return nullptr;
		}

		if (InTextureDesc.IsTextureCubeArray())
		{
			if (InTextureDesc.NumArraySlices > RHI::MaxCubeArrayCount)
			{
				RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) NumArraySlices (%u) exceeds device feature support limit (%u cubes).", InTextureDesc.NumArraySlices, RHI::MaxCubeArrayCount);
				return nullptr;
			}
		}
	}
	else if (InTextureDesc.IsTexture1D() || InTextureDesc.IsTexture1DArray())
    {
        if (static_cast<uint32>(InTextureDesc.Extent.X) > RHI::MaxTexture1DSize)
        {
            RHI_VALIDATION_ERROR("CreateTexture: Texture1D width %u exceeds device limit %u.", InTextureDesc.Extent.X, RHI::MaxTexture1DSize);
            return nullptr;
        }

        if (InTextureDesc.IsTexture1DArray() && InTextureDesc.NumArraySlices > RHI::MaxTexture1DArrayLayers)
        {
            RHI_VALIDATION_ERROR("CreateTexture: Texture1DArray layer count %u exceeds device limit %u.", InTextureDesc.NumArraySlices, RHI::MaxTexture1DArrayLayers);
            return nullptr;
        }
    }
	else
	{
		if (static_cast<uint32>(InTextureDesc.Extent.X) > RHI::MaxTexture2DSize || static_cast<uint32>(InTextureDesc.Extent.Y) > RHI::MaxTexture2DSize)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2D) Extent (%u,%u) exceeds device feature support limit (%u).", InTextureDesc.Extent.X, InTextureDesc.Extent.Y, RHI::MaxTexture2DSize);
			return nullptr;
		}

		if (InTextureDesc.IsTexture2DArray() && InTextureDesc.NumArraySlices > RHI::MaxTexture2DArrayLayers)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2DArray) NumArraySlices (%u) exceeds device feature support limit (%u).", InTextureDesc.NumArraySlices, RHI::MaxTexture2DArrayLayers);
			return nullptr;
		}
	}

	// -------------------------------------------------------------------------------------------
	// MipLevels
	// -------------------------------------------------------------------------------------------

	if (InTextureDesc.NumMipLevels == 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Invalid NumMipLevels (0). A texture must have at least one mip level.");
		return nullptr;
	}

	if (InTextureDesc.IsMultisampled())
	{
		if (InTextureDesc.NumMipLevels != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: Multisampled texture cannot have mip chains. (NumMipLevels=%u, Expected=1).", InTextureDesc.NumMipLevels);
			return nullptr;
		}
	}
	else
	{
		const uint32 MaxPossibleMipLevels = Math::MaxMipLevelsFromExtent(InTextureDesc.Extent.X, InTextureDesc.Extent.Y,
            InTextureDesc.IsTexture3D() ? InTextureDesc.Extent.Z : 1u);

		if (InTextureDesc.NumMipLevels > MaxPossibleMipLevels)
		{
			RHI_VALIDATION_ERROR("CreateTexture: NumMipLevels (%u) exceeds maximum allowed (%u) based on texture extent (%u,%u,%u).",
                InTextureDesc.NumMipLevels, MaxPossibleMipLevels, InTextureDesc.Extent.X, InTextureDesc.Extent.Y,
                InTextureDesc.IsTexture3D() ? InTextureDesc.Extent.Z : 1u);

			return nullptr;
		}
	}

	// -------------------------------------------------------------------------------------------
	// Sample count sanity
	// -------------------------------------------------------------------------------------------

	if (InTextureDesc.NumSamples == 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: NumSamples must be >= 1. (Got 0).");
		return nullptr;
	}

    if (!Math::IsPowerOfTwo(InTextureDesc.NumSamples))
    {
        RHI_VALIDATION_ERROR("CreateTexture: NumSamples must be a power of two. (Got %u).", InTextureDesc.NumSamples);
        return nullptr;
    }

	if ((InTextureDesc.IsTexture3D() || InTextureDesc.IsTexture1D() || InTextureDesc.IsTexture1DArray()) && InTextureDesc.NumSamples > 1)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Texture1D/Texture3D dimensions do not support MSAA. (NumSamples=%u, Expected=1).", InTextureDesc.NumSamples);
		return nullptr;
	}

	// -------------------------------------------------------------------------------------------
	// Usage flag combinations
	// -------------------------------------------------------------------------------------------

	const bool bIsRenderTarget = InTextureDesc.IsRenderTarget();
	const bool bIsDepthStencil = InTextureDesc.IsDepthStencil();
	const bool bIsUAV          = InTextureDesc.IsUnorderedAccessTexture();
	const bool bIsPresentable  = InTextureDesc.IsPresentable();

	if (bIsRenderTarget && bIsDepthStencil)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Texture cannot have both RenderTarget and DepthStencil usage flags set.");
		return nullptr;
	}

	if (bIsDepthStencil && bIsUAV)
	{
		RHI_VALIDATION_ERROR("CreateTexture: DepthStencil textures cannot have UnorderedAccessTexture usage flag set.");
		return nullptr;
	}

    if (bIsDepthStencil && !IsDepthStencilFormat(InTextureDesc.Format) && !IsTypelessFormat(InTextureDesc.Format))
    {
        RHI_VALIDATION_ERROR("CreateTexture: DepthStencil usage requires a depth/stencil or compatible typeless format.");
        return nullptr;
    }

    if (bIsRenderTarget && IsDepthStencilFormat(InTextureDesc.Format))
    {
        RHI_VALIDATION_ERROR("CreateTexture: RenderTarget usage cannot use a depth/stencil format.");
        return nullptr;
    }

    if (bIsUAV && !IsTypelessFormat(InTextureDesc.Format) && !RealRHI->QueryUAVFormatSupport(InTextureDesc.Format))
    {
        RHI_VALIDATION_ERROR("CreateTexture: format '%s' does not support unordered access.", ToString(InTextureDesc.Format));
        return nullptr;
    }

    if (InTextureDesc.IsShadingRateTexture())
    {
        if (InTextureDesc.Dimension != ETextureDimension::Texture2D || InTextureDesc.NumMipLevels != 1 || InTextureDesc.NumSamples != 1)
        {
            RHI_VALIDATION_ERROR("CreateTexture: ShadingRateTexture requires a single-sample Texture2D with one mip.");
            return nullptr;
        }
    }

	if (InTextureDesc.IsMultisampled() && bIsUAV)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Multisampled textures cannot have UnorderedAccessTexture usage flag set.");
		return nullptr;
	}

	if (bIsPresentable)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Presentable textures are retrieved via a SwapChain.");
		return nullptr;
	}

	return RealRHI->CreateTexture(InTextureDesc, InInitialState, InInitialData);
}

FRHIBuffer* FRHIValidation::CreateBuffer(const FRHIBufferDesc& BufferDesc, EResourceAccess InitialState, const void* InitialData)
{
    // -------------------------------------------------------------------------------------------
    // Basic sanity
    // -------------------------------------------------------------------------------------------

    if (BufferDesc.Size == 0)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: Buffer size must be greater than zero. (parameter 'Size' was 0 bytes)");
        return nullptr;
    }

    if (BufferDesc.Flags == EBufferFlags::None)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: Buffer flags must be specified. (parameter 'Flags' was EBufferFlags::None)");
        return nullptr;
    }

    if (BufferDesc.Size > RHI::MaxBufferSize)
    {
		RHI_VALIDATION_ERROR("CreateBuffer: The buffer size (%llu bytes) exceeds device feature support. (MaxBufferSize=%llu)",
			static_cast<uint64>(BufferDesc.Size), static_cast<uint64>(RHI::MaxBufferSize));
        return nullptr;
    }

    // -------------------------------------------------------------------------------------------
    // Memory flags: require exactly one of Default/Dynamic/ReadBack/Transient
    // -------------------------------------------------------------------------------------------

    const bool bMemoryDefault   = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::Default);
    const bool bMemoryDynamic   = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::Dynamic);
    const bool bMemoryReadBack  = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::ReadBack);
    const bool bMemoryTransient = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::Transient);

    const int32 StorageFlagCount = (bMemoryDefault ? 1 : 0) + (bMemoryDynamic ? 1 : 0) + (bMemoryReadBack ? 1 : 0) + (bMemoryTransient ? 1 : 0);
    if (StorageFlagCount != 1)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: Exactly one memory flag must be set. The options are Default/Dynamic/ReadBack/Transient. (The flags set are Default=%s, Dynamic=%s, ReadBack=%s, Transient=%s)",
            bMemoryDefault ? "true" : "false", bMemoryDynamic ? "true" : "false", bMemoryReadBack ? "true" : "false", bMemoryTransient ? "true" : "false");
        return nullptr;
    }

    const bool bIsConstantBuffer        = BufferDesc.IsConstantBuffer();
    const bool bIsShaderResourceBuffer  = BufferDesc.IsShaderResourceBuffer();
    const bool bIsVertexBuffer          = BufferDesc.IsVertexBuffer();
    const bool bIsIndexBuffer           = BufferDesc.IsIndexBuffer();
    const bool bIsUnorderedAccessBuffer = BufferDesc.IsUnorderedAccessBuffer();
    const bool bIsCopySource            = BufferDesc.IsCopySource();
    const bool bIsCopyDest              = BufferDesc.IsCopyDest();
    const bool bIsStreamOutput          = BufferDesc.IsStreamOutputBuffer();
    const bool bIsAccelerationStructure = BufferDesc.IsAccelerationStructure();

    if (bIsCopyDest && (bMemoryDynamic || bMemoryTransient))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopyDest usage is invalid on Dynamic/Transient upload memory.");
        return nullptr;
    }

    if (bIsCopySource && bMemoryReadBack)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopySource usage is invalid on ReadBack memory.");
        return nullptr;
    }

    if (bIsStreamOutput && bMemoryReadBack)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: StreamOutputBuffer usage is invalid on ReadBack memory.");
        return nullptr;
    }

    if (bIsAccelerationStructure)
    {
        if (!bMemoryDefault || (BufferDesc.Size % RHI::AccelerationStructureBufferAlignment) != 0)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: AccelerationStructure buffers require Default memory and %u-byte size alignment.", RHI::AccelerationStructureBufferAlignment);
            return nullptr;
        }
    }

    if (IsEnumFlagSet(InitialState, EResourceAccess::CopyDest) && !CanUseBufferAsCopyDestination(BufferDesc))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopyDest initial access requires EBufferFlags::CopyDest or ReadBack memory.");
        return nullptr;
    }

    if (IsEnumFlagSet(InitialState, EResourceAccess::CopySource) && !CanUseBufferAsCopySource(BufferDesc))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopySource initial access requires EBufferFlags::CopySource.");
        return nullptr;
    }

    // -------------------------------------------------------------------------------------------
    // Constant buffer rules
    // -------------------------------------------------------------------------------------------
    
    if (bIsConstantBuffer)
    {
        if (bMemoryReadBack)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: a buffer with ConstantBuffer usage-flag cannot use ReadBack memory. Use Default, Dynamic, or Transient for GPU-accessible ConstantBuffer.");
            return nullptr;
        }

        if (BufferDesc.Size > RHI::MaxConstantBufferSize)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: size (%llu bytes) exceeds device feature support. (MaxConstantBufferSize=%u)", static_cast<uint64>(BufferDesc.Size), RHI::MaxConstantBufferSize);
            return nullptr;
        }
    }

    // -------------------------------------------------------------------------------------------
    // Vertex / Index buffer rules
    // -------------------------------------------------------------------------------------------

    if (bIsVertexBuffer || bIsIndexBuffer)
    {
        if (BufferDesc.Stride == 0)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: %s requires a non-zero Stride. (Stride=0)", bIsVertexBuffer ? "VertexBuffer" : "IndexBuffer");
            return nullptr;
        }

        if ((BufferDesc.Size % BufferDesc.Stride) != 0ull)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: %s size must be a multiple of Stride to satisfy device feature support. (Size=%llu, Stride=%u)",
                bIsVertexBuffer ? "VertexBuffer" : "IndexBuffer", static_cast<uint64>(BufferDesc.Size), BufferDesc.Stride);
            return nullptr;
        }

        if (bIsIndexBuffer)
        {
            if (!(BufferDesc.Stride == 2u || BufferDesc.Stride == 4u))
            {
                RHI_VALIDATION_ERROR("CreateBuffer: IndexBuffer stride must be 2 or 4 bytes (uint16 or uint32). (Stride=%u)", BufferDesc.Stride);
                return nullptr;
            }
        }

        if (bMemoryReadBack)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: VertexBuffer/IndexBuffer cannot use ReadBack storage. Use Default or Dynamic memory for GPU pipeline consumption.");
            return nullptr;
        }
    }

    // -------------------------------------------------------------------------------------------
    // SRV/UAV buffer rules
    // -------------------------------------------------------------------------------------------

    if (bIsShaderResourceBuffer || bIsUnorderedAccessBuffer)
    {
        if (BufferDesc.Size > RHI::MaxStorageBufferSize)
        {
			RHI_VALIDATION_ERROR("CreateBuffer: %s size exceeds device feature support. (Size=%llu, MaxStorageBufferSize=%llu)",
				bIsUnorderedAccessBuffer ? "UnorderedAccessBuffer" : "ShaderResourceBuffer", static_cast<uint64>(BufferDesc.Size),
				static_cast<uint64>(RHI::MaxStorageBufferSize));
            return nullptr;
        }

        if (BufferDesc.Stride > 0)
        {
            // Structured: enforce stride window and size divisibility
            const uint32 MinStride = RHI::StructuredBufferMinStride;
            const uint32 MaxStride = RHI::StructuredBufferMaxStride;

            if (BufferDesc.Stride < MinStride || BufferDesc.Stride > MaxStride)
            {
                RHI_VALIDATION_ERROR("CreateBuffer: StructuredBuffer stride is outside device feature support. (Stride=%u, Allowed range: [%u, %u])", 
                    BufferDesc.Stride, MinStride, MaxStride);
                return nullptr;
            }

            if ((BufferDesc.Size % BufferDesc.Stride) != 0ull)
            {
                RHI_VALIDATION_ERROR("CreateBuffer: StructuredBuffer size must be an integer multiple of Stride to satisfy device feature support. (Size=%llu, Stride=%u)",
                    static_cast<uint64>(BufferDesc.Size), BufferDesc.Stride);
                return nullptr;
            }
        }
        else
        {
            const uint64 RequiredAlignment = static_cast<uint64>(RHI::RawBufferRequiredAlignment);
            if ((BufferDesc.Size % RequiredAlignment) != 0ull)
            {
                RHI_VALIDATION_ERROR("CreateBuffer: RWBuffer size must be aligned to satisfy device feature support. (Size=%llu, RequiredAlignment=%llu)",
                    static_cast<uint64>(BufferDesc.Size), static_cast<uint64>(RequiredAlignment));
                return nullptr;
            }
        }

        // ReadBack heaps are not suitable for SRV/UAV usage
        if (bMemoryReadBack)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: %s cannot use ReadBack storage. ReadBack heaps are not GPU-readable/writable.",
                bIsUnorderedAccessBuffer ? "UnorderedAccessBuffer" : "ShaderResourceBuffer");
            return nullptr;
        }
    }

    return RealRHI->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FRHISamplerState* FRHIValidation::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    const auto IsValidMode = [](ESamplerMode Mode)
    {
        return Mode >= ESamplerMode::Wrap && Mode <= ESamplerMode::MirrorOnce;
    };

    if (!IsValidMode(InSamplerDesc.AddressU) || !IsValidMode(InSamplerDesc.AddressV) || !IsValidMode(InSamplerDesc.AddressW) ||
        InSamplerDesc.Filter < ESamplerFilter::MinMagMipPoint || InSamplerDesc.Filter > ESamplerFilter::Comparison_Anisotropic)
    {
        RHI_VALIDATION_ERROR("CreateSamplerState: invalid address mode or filter enum.");
        return nullptr;
    }

    if (InSamplerDesc.IsComparisonSampler() && InSamplerDesc.ComparisonFunc == EComparisonFunc::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateSamplerState: comparison samplers require a valid comparison function.");
        return nullptr;
    }

    const bool bAnisotropic = InSamplerDesc.Filter == ESamplerFilter::Anistrotopic ||
        InSamplerDesc.Filter == ESamplerFilter::Comparison_Anisotropic;
    if (bAnisotropic && (InSamplerDesc.MaxAnisotropy == 0 || InSamplerDesc.MaxAnisotropy > 16))
    {
        RHI_VALIDATION_ERROR("CreateSamplerState: MaxAnisotropy must be in [1, 16] for anisotropic filters.");
        return nullptr;
    }

    if (!std::isfinite(InSamplerDesc.MipLODBias) || !std::isfinite(InSamplerDesc.MinLOD) ||
        !std::isfinite(InSamplerDesc.MaxLOD) || InSamplerDesc.MinLOD > InSamplerDesc.MaxLOD)
    {
        RHI_VALIDATION_ERROR("CreateSamplerState: LOD values must be finite and MinLOD must not exceed MaxLOD.");
        return nullptr;
    }

    return RealRHI->CreateSamplerState(InSamplerDesc);
}

FRHISwapChain* FRHIValidation::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    if (!InSwapChainDesc.WindowHandle)
    {
        RHI_VALIDATION_ERROR("Trying to create a viewport with an invalid WindowHandle");
        return nullptr;
    }

    if (InSwapChainDesc.Usage == ESwapChainUsageFlags::None)
    {
        RHI_VALIDATION_ERROR("CreateSwapChain: Usage cannot be ESwapChainUsageFlags::None.");
        return nullptr;
    }

    return RealRHI->CreateSwapChain(InSwapChainDesc);
}

FRHISceneAccelerationStructure* FRHIValidation::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateSceneAccelerationStructure: ray tracing is unsupported.");
        return nullptr;
    }

    if (InSceneDesc.IsPartitioned() && !RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateSceneAccelerationStructure: partitioned scenes are unsupported.");
        return nullptr;
    }

    for (const FRHIGeometryAccelerationStructureInstance& Instance : InSceneDesc.Instances)
    {
        if (!Instance.Geometry)
        {
            RHI_VALIDATION_ERROR("CreateSceneAccelerationStructure: instances cannot reference nullptr geometry.");
            return nullptr;
        }
    }

    return RealRHI->CreateSceneAccelerationStructure(InSceneDesc);
}

FRHIGeometryAccelerationStructure* FRHIValidation::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure: ray tracing is unsupported.");
        return nullptr;
    }

    if (!InGeometryDesc.VertexBuffer || InGeometryDesc.NumVertices == 0 ||
        !InGeometryDesc.VertexBuffer->GetDesc().IsVertexBuffer())
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure requires a vertex buffer and non-zero vertex count.");
        return nullptr;
    }

    if (InGeometryDesc.NumIndices > 0 &&
        (!InGeometryDesc.IndexBuffer || !InGeometryDesc.IndexBuffer->GetDesc().IsIndexBuffer() ||
         InGeometryDesc.IndexFormat == EIndexFormat::Unknown))
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure indexed geometry requires an index buffer and valid index format.");
        return nullptr;
    }

    if (InGeometryDesc.IsClusteredGeometry() && !RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure: clustered geometry is unsupported.");
        return nullptr;
    }

    return RealRHI->CreateGeometryAccelerationStructure(InGeometryDesc);
}

FRHIShaderResourceView* FRHIValidation::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        RHI_VALIDATION_ERROR("CreateShaderResourceView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InDesc.ViewDimension == EViewDimension::None)
    {
        RHI_VALIDATION_ERROR("CreateShaderResourceView: ViewDimension must be set explicitly");
        return nullptr;
    }

    if (InDesc.IsBufferSRV())
    {
        if (InResource->GetResourceType() != ERHIResourceType::Buffer)
        {
            RHI_VALIDATION_ERROR("CreateShaderResourceView: buffer view requires an FRHIBuffer resource (got %s)", ToString(InResource->GetResourceType()));
            return nullptr;
        }

        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(InResource);
        if (!Buffer->GetDesc().IsShaderResourceBuffer())
        {
            RHI_VALIDATION_ERROR("CreateShaderResourceView: buffer must have EBufferFlags::ShaderResourceBuffer");
            return nullptr;
        }

        if (!ValidateBufferView("CreateShaderResourceView", Buffer->GetDesc(), InDesc.Buffer.Type,
            InDesc.Buffer.FirstElement, InDesc.Buffer.NumElements, InDesc.Buffer.Format))
        {
            return nullptr;
        }
    }
    else if (InDesc.IsTextureSRV())
    {
        if (InResource->GetResourceType() != ERHIResourceType::Texture)
        {
            RHI_VALIDATION_ERROR("CreateShaderResourceView: texture view requires an FRHITexture resource (got %s)", ToString(InResource->GetResourceType()));
            return nullptr;
        }

        FRHITexture* Texture = static_cast<FRHITexture*>(InResource);
        const FRHITextureDesc& TextureDesc = Texture->GetDesc();
        if (!TextureDesc.IsShaderResourceTexture())
        {
            RHI_VALIDATION_ERROR("CreateShaderResourceView: texture must have ETextureUsageFlags::ShaderResourceTexture");
            return nullptr;
        }

        EFormat ViewFormat   = EFormat::Unknown;
        uint32  BaseLayer    = 0;
        uint32  LayerCount   = 1;
        uint32  FirstMip     = 0;
        uint32  NumMips      = 1;

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
                ViewFormat = InDesc.Texture1D.Format;
                FirstMip   = InDesc.Texture1D.FirstMipLevel;
                NumMips    = InDesc.Texture1D.NumMips;
                break;

            case EViewDimension::Texture1DArray:
                ViewFormat = InDesc.Texture1DArray.Format;
                FirstMip   = InDesc.Texture1DArray.FirstMipLevel;
                NumMips    = InDesc.Texture1DArray.NumMips;
                BaseLayer  = InDesc.Texture1DArray.FirstArraySlice;
                LayerCount = InDesc.Texture1DArray.NumSlices;
                break;

            case EViewDimension::Texture2D:
                ViewFormat = InDesc.Texture2D.Format;
                FirstMip   = InDesc.Texture2D.FirstMipLevel;
                NumMips    = InDesc.Texture2D.NumMips;
                break;

            case EViewDimension::Texture2DArray:
                ViewFormat = InDesc.Texture2DArray.Format;
                FirstMip   = InDesc.Texture2DArray.FirstMipLevel;
                NumMips    = InDesc.Texture2DArray.NumMips;
                BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
                LayerCount = InDesc.Texture2DArray.NumSlices;
                break;

            case EViewDimension::TextureCube:
                ViewFormat = InDesc.TextureCube.Format;
                FirstMip   = InDesc.TextureCube.FirstMipLevel;
                NumMips    = InDesc.TextureCube.NumMips;
                BaseLayer  = 0;
                LayerCount = RHI_NUM_CUBE_FACES;
                break;

            case EViewDimension::TextureCubeArray:
                ViewFormat = InDesc.TextureCubeArray.Format;
                FirstMip   = InDesc.TextureCubeArray.FirstMipLevel;
                NumMips    = InDesc.TextureCubeArray.NumMips;
                BaseLayer  = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.FirstCube);
                LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.NumCubes);
                break;

            case EViewDimension::Texture3D:
                ViewFormat = InDesc.Texture3D.Format;
                FirstMip   = InDesc.Texture3D.FirstMipLevel;
                NumMips    = InDesc.Texture3D.NumMips;
                break;

            default:
                break;
        }

        if (!ValidateTextureSlicesAndMips("CreateShaderResourceView", TextureDesc, BaseLayer, LayerCount, FirstMip, NumMips, ViewFormat, InDesc.ViewDimension))
        {
            return nullptr;
        }
    }
    else if (InDesc.IsAccelerationStructureSRV())
    {
        if (InResource->GetResourceType() != ERHIResourceType::SceneAccelerationStructure)
        {
            RHI_VALIDATION_ERROR("CreateShaderResourceView: AccelerationStructure view requires an FRHISceneAccelerationStructure resource (got %s)",
                ToString(InResource->GetResourceType()));
            return nullptr;
        }
    }
    else
    {
        RHI_VALIDATION_ERROR("CreateShaderResourceView: Invalid ViewDimension");
        return nullptr;
    }

    return RealRHI->CreateShaderResourceView(InResource, InDesc);
}

FRHIUnorderedAccessView* FRHIValidation::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        RHI_VALIDATION_ERROR("CreateUnorderedAccessView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InDesc.ViewDimension == EViewDimension::None)
    {
        RHI_VALIDATION_ERROR("CreateUnorderedAccessView: ViewDimension must be set explicitly");
        return nullptr;
    }

    if (InDesc.IsBufferUAV())
    {
        if (InResource->GetResourceType() != ERHIResourceType::Buffer)
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: buffer view requires an FRHIBuffer resource (got %s)", ToString(InResource->GetResourceType()));
            return nullptr;
        }

        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(InResource);
        if (!Buffer->GetDesc().IsUnorderedAccessBuffer())
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: buffer must have EBufferFlags::UnorderedAccessBuffer");
            return nullptr;
        }

        if (!ValidateBufferView("CreateUnorderedAccessView", Buffer->GetDesc(), InDesc.Buffer.Type,
            InDesc.Buffer.FirstElement, InDesc.Buffer.NumElements, InDesc.Buffer.Format))
        {
            return nullptr;
        }

        if (InDesc.Buffer.Type == EBufferViewType::Typed && !RealRHI->QueryUAVFormatSupport(InDesc.Buffer.Format))
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: typed buffer format '%s' does not support UAV access.", ToString(InDesc.Buffer.Format));
            return nullptr;
        }
    }
    else if (InDesc.IsTextureUAV())
    {
        if (InResource->GetResourceType() != ERHIResourceType::Texture)
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: texture view requires an FRHITexture resource (got %s)", ToString(InResource->GetResourceType()));
            return nullptr;
        }

        FRHITexture* Texture = static_cast<FRHITexture*>(InResource);

        const FRHITextureDesc& TextureDesc = Texture->GetDesc();
        if (!TextureDesc.IsUnorderedAccessTexture())
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: texture must have ETextureUsageFlags::UnorderedAccessTexture");
            return nullptr;
        }

        EFormat ViewFormat = EFormat::Unknown;
        uint32  BaseLayer  = 0;
        uint32  LayerCount = 1;
        uint32  MipLevel   = 0;

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
                ViewFormat = InDesc.Texture1D.Format;
                MipLevel   = InDesc.Texture1D.MipLevel;
                break;

            case EViewDimension::Texture1DArray:
                ViewFormat = InDesc.Texture1DArray.Format;
                MipLevel   = InDesc.Texture1DArray.MipLevel;
                BaseLayer  = InDesc.Texture1DArray.FirstArraySlice;
                LayerCount = InDesc.Texture1DArray.NumSlices;
                break;

            case EViewDimension::Texture2D:
                ViewFormat = InDesc.Texture2D.Format;
                MipLevel   = InDesc.Texture2D.MipLevel;
                break;

            case EViewDimension::Texture2DArray:
                ViewFormat = InDesc.Texture2DArray.Format;
                MipLevel   = InDesc.Texture2DArray.MipLevel;
                BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
                LayerCount = InDesc.Texture2DArray.NumSlices;
                break;

            case EViewDimension::TextureCube:
                ViewFormat = InDesc.TextureCube.Format;
                MipLevel   = InDesc.TextureCube.MipLevel;
                BaseLayer  = 0;
                LayerCount = RHI_NUM_CUBE_FACES;
                break;

            case EViewDimension::TextureCubeArray:
                ViewFormat = InDesc.TextureCubeArray.Format;
                MipLevel   = InDesc.TextureCubeArray.MipLevel;
                BaseLayer  = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.FirstCube);
                LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.NumCubes);
                break;
            
            case EViewDimension::Texture3D:
                ViewFormat = InDesc.Texture3D.Format;
                MipLevel   = InDesc.Texture3D.MipLevel;
                break;
            
            default:
                break;
        }

        if (!ValidateTextureSlicesAndMips("CreateUnorderedAccessView", TextureDesc, BaseLayer, LayerCount, MipLevel, 1u, ViewFormat, InDesc.ViewDimension))
        {
            return nullptr;
        }

        if (!RealRHI->QueryUAVFormatSupport(ViewFormat))
        {
            RHI_VALIDATION_ERROR("CreateUnorderedAccessView: texture format '%s' does not support UAV access.", ToString(ViewFormat));
            return nullptr;
        }

        if (InDesc.ViewDimension == EViewDimension::Texture3D)
        {
            IntVector3 MipExtent;
            if (!ValidateTextureMip("CreateUnorderedAccessView", TextureDesc, MipLevel, MipExtent))
            {
                return nullptr;
            }

            if (InDesc.Texture3D.WSize == 0 ||
                InDesc.Texture3D.FirstWSlice > uint32(MipExtent.Z) ||
                InDesc.Texture3D.WSize > uint32(MipExtent.Z) - InDesc.Texture3D.FirstWSlice)
            {
                RHI_VALIDATION_ERROR("CreateUnorderedAccessView: Texture3D W-slice range [First=%u, Count=%u] exceeds mip depth %u.",
                    InDesc.Texture3D.FirstWSlice, InDesc.Texture3D.WSize, uint32(MipExtent.Z));
                return nullptr;
            }
        }
    }
    else
    {
        RHI_VALIDATION_ERROR("CreateUnorderedAccessView: Invalid ViewDimension");
        return nullptr;
    }

    return RealRHI->CreateUnorderedAccessView(InResource, InDesc);
}

FRHIRenderTargetView* FRHIValidation::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource)
    {
        RHI_VALIDATION_ERROR("CreateRenderTargetView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        RHI_VALIDATION_ERROR("CreateRenderTargetView: requires an FRHITexture resource (got %s)", ToString(InResource->GetResourceType()));
        return nullptr;
    }

    if (InDesc.ViewDimension == EViewDimension::None || !IsTextureViewDimension(InDesc.ViewDimension))
    {
        RHI_VALIDATION_ERROR("CreateRenderTargetView: ViewDimension must be a valid texture dimension");
        return nullptr;
    }

    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);

    const FRHITextureDesc& TextureDesc = Texture->GetDesc();
    if (!TextureDesc.IsRenderTarget())
    {
        RHI_VALIDATION_ERROR("CreateRenderTargetView: texture must have ETextureUsageFlags::RenderTarget");
        return nullptr;
    }

    EFormat ViewFormat = EFormat::Unknown;
    uint32  BaseLayer  = 0;
    uint32  LayerCount = 1;
    uint32  MipLevel   = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            ViewFormat = InDesc.Texture1D.Format;
            MipLevel   = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            ViewFormat = InDesc.Texture1DArray.Format;
            MipLevel   = InDesc.Texture1DArray.MipLevel;
            BaseLayer  = InDesc.Texture1DArray.FirstArraySlice;
            LayerCount = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            ViewFormat = InDesc.Texture2D.Format;
            MipLevel   = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            ViewFormat = InDesc.Texture2DArray.Format;
            MipLevel   = InDesc.Texture2DArray.MipLevel;
            BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
            LayerCount = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::TextureCube:
            ViewFormat = InDesc.TextureCube.Format;
            MipLevel   = InDesc.TextureCube.MipLevel;
            BaseLayer  = 0; LayerCount = RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::TextureCubeArray:
            ViewFormat = InDesc.TextureCubeArray.Format;
            MipLevel   = InDesc.TextureCubeArray.MipLevel;
            BaseLayer  = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.FirstCube);
            LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.NumCubes);
            break;
        
        case EViewDimension::Texture3D:
            ViewFormat = InDesc.Texture3D.Format;
            MipLevel   = InDesc.Texture3D.MipLevel;
            break;
            
        default:
            break;
    }

    if (!ValidateTextureSlicesAndMips("CreateRenderTargetView", TextureDesc, BaseLayer, LayerCount, MipLevel, 1u, ViewFormat, InDesc.ViewDimension))
    {
        return nullptr;
    }

    if (IsDepthStencilFormat(ViewFormat))
    {
        RHI_VALIDATION_ERROR("CreateRenderTargetView: depth/stencil formats are invalid for render-target views.");
        return nullptr;
    }

    if (InDesc.ViewDimension == EViewDimension::Texture3D)
    {
        IntVector3 MipExtent;
        if (!ValidateTextureMip("CreateRenderTargetView", TextureDesc, MipLevel, MipExtent))
        {
            return nullptr;
        }

        if (InDesc.Texture3D.WSize == 0 ||
            InDesc.Texture3D.FirstWSlice > uint32(MipExtent.Z) ||
            InDesc.Texture3D.WSize > uint32(MipExtent.Z) - InDesc.Texture3D.FirstWSlice)
        {
            RHI_VALIDATION_ERROR("CreateRenderTargetView: Texture3D W-slice range [First=%u, Count=%u] exceeds mip depth %u.",
                InDesc.Texture3D.FirstWSlice, InDesc.Texture3D.WSize, uint32(MipExtent.Z));
            return nullptr;
        }
    }

    return RealRHI->CreateRenderTargetView(InResource, InDesc);
}

FRHIDepthStencilView* FRHIValidation::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource)
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: requires an FRHITexture resource (got %s)", ToString(InResource->GetResourceType()));
        return nullptr;
    }

    if (InDesc.ViewDimension == EViewDimension::None || InDesc.ViewDimension == EViewDimension::Texture3D || !IsTextureViewDimension(InDesc.ViewDimension))
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: ViewDimension must be a valid non-3D texture dimension");
        return nullptr;
    }

    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);
    const FRHITextureDesc& TextureDesc = Texture->GetDesc();
    if (!TextureDesc.IsDepthStencil())
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: texture must have ETextureUsageFlags::DepthStencil");
        return nullptr;
    }

    EFormat ViewFormat = EFormat::Unknown;
    uint32  BaseLayer  = 0;
    uint32  LayerCount = 1;
    uint32  MipLevel   = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            ViewFormat = InDesc.Texture1D.Format;
            MipLevel   = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            ViewFormat = InDesc.Texture1DArray.Format;
            MipLevel   = InDesc.Texture1DArray.MipLevel;
            BaseLayer  = InDesc.Texture1DArray.FirstArraySlice;
            LayerCount = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            ViewFormat = InDesc.Texture2D.Format;
            MipLevel   = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            ViewFormat = InDesc.Texture2DArray.Format;
            MipLevel   = InDesc.Texture2DArray.MipLevel;
            BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
            LayerCount = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::TextureCube:
            ViewFormat = InDesc.TextureCube.Format;
            MipLevel   = InDesc.TextureCube.MipLevel;
            BaseLayer  = 0;
            LayerCount = RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::TextureCubeArray:
            ViewFormat = InDesc.TextureCubeArray.Format;
            MipLevel   = InDesc.TextureCubeArray.MipLevel;
            BaseLayer  = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.FirstCube);
            LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, InDesc.TextureCubeArray.NumCubes);
            break;
            
        default:
            break;
    }

    if (!ValidateTextureSlicesAndMips("CreateDepthStencilView", TextureDesc, BaseLayer, LayerCount, MipLevel, 1u, ViewFormat, InDesc.ViewDimension))
    {
        return nullptr;
    }

    if (!IsDepthStencilFormat(ViewFormat))
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: view format '%s' is not a depth/stencil format.", ToString(ViewFormat));
        return nullptr;
    }

    if (IsEnumFlagSet(InDesc.Flags, EDepthStencilViewFlags::ReadOnlyStencil) && !IsStencilFormat(TextureDesc.Format))
    {
        RHI_VALIDATION_WARNING("CreateDepthStencilView: EDepthStencilViewFlags::ReadOnlyStencil set on a depth-only-format resource ('%s'); the stencil flag will be ignored by the backend.", ToString(TextureDesc.Format));
    }

    return RealRHI->CreateDepthStencilView(InResource, InDesc);
}

FRHIComputeShader* FRHIValidation::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateComputeShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateComputeShader(ShaderCode);
}

FRHIVertexShader* FRHIValidation::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateVertexShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateVertexShader(ShaderCode);
}

FRHIHullShader* FRHIValidation::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateHullShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateHullShader(ShaderCode);
}

FRHIDomainShader* FRHIValidation::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateDomainShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateDomainShader(ShaderCode);
}

FRHIGeometryShader* FRHIValidation::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsGeometryShaders)
    {
        RHI_VALIDATION_ERROR("CreateGeometryShader: geometry shaders are not supported by this device.");
        return nullptr;
    }

    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateGeometryShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateGeometryShader(ShaderCode);
}

FRHIMeshShader* FRHIValidation::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateMeshShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateMeshShader(ShaderCode);
}

FRHIAmplificationShader* FRHIValidation::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateAmplificationShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreateAmplificationShader(ShaderCode);
}

FRHIPixelShader* FRHIValidation::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreatePixelShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return RealRHI->CreatePixelShader(ShaderCode);
}

FRHIRayGenShader* FRHIValidation::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayGenShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return RealRHI->CreateRayGenShader(ShaderCode);
}

FRHIRayAnyHitShader* FRHIValidation::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayAnyHitShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return RealRHI->CreateRayAnyHitShader(ShaderCode);
}

FRHIRayClosestHitShader* FRHIValidation::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayClosestHitShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return RealRHI->CreateRayClosestHitShader(ShaderCode);
}

FRHIRayMissShader* FRHIValidation::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayMissShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return RealRHI->CreateRayMissShader(ShaderCode);
}

FRHIDepthStencilState* FRHIValidation::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return RealRHI->CreateDepthStencilState(InDesc);
}

FRHIRasterizerState* FRHIValidation::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return RealRHI->CreateRasterizerState(InDesc);
}

FRHIBlendState* FRHIValidation::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return RealRHI->CreateBlendState(InDesc);
}

FRHIInputLayout* FRHIValidation::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return RealRHI->CreateInputLayout(InInputElements);
}

FRHIGraphicsPipelineState* FRHIValidation::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    if (!InDesc.VertexShader)
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: VertexShader is required.");
        return nullptr;
    }

    if ((InDesc.HullShader == nullptr) != (InDesc.DomainShader == nullptr))
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: HullShader and DomainShader must be provided together.");
        return nullptr;
    }

    if (InDesc.GeometryShader && !RHI::bSupportsGeometryShaders)
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: geometry shaders are unsupported.");
        return nullptr;
    }

    if (InDesc.RasterizerOutputFormats.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: render-target count exceeds RHI_MAX_RENDER_TARGETS.");
        return nullptr;
    }

    for (uint32 Index = 0; Index < InDesc.RasterizerOutputFormats.NumRenderTargets; ++Index)
    {
        if (InDesc.RasterizerOutputFormats.RenderTargetFormats[Index] == EFormat::Unknown)
        {
            RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: render-target format %u cannot be Unknown.", Index);
            return nullptr;
        }
    }

    if (InDesc.MultiSampleState.SampleCount == 0 || !Math::IsPowerOfTwo(InDesc.MultiSampleState.SampleCount))
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: sample count must be a non-zero power of two.");
        return nullptr;
    }

    return RealRHI->CreateGraphicsPipelineState(InDesc);
}

FRHIComputePipelineState* FRHIValidation::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    if (!InDesc.Shader)
    {
        RHI_VALIDATION_ERROR("CreateComputePipelineState: compute shader is required.");
        return nullptr;
    }

    return RealRHI->CreateComputePipelineState(InDesc);
}

FRHIMeshletPipelineState* FRHIValidation::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
{
    if (!InDesc.MeshShader)
    {
        RHI_VALIDATION_ERROR("CreateMeshletPipelineState: mesh shader is required.");
        return nullptr;
    }

    if (InDesc.RasterizerOutputFormats.NumRenderTargets > RHI_MAX_RENDER_TARGETS ||
        InDesc.MultiSampleState.SampleCount == 0 ||
        !Math::IsPowerOfTwo(InDesc.MultiSampleState.SampleCount))
    {
        RHI_VALIDATION_ERROR("CreateMeshletPipelineState: invalid render-target count or sample count.");
        return nullptr;
    }

    return RealRHI->CreateMeshletPipelineState(InDesc);
}

FRHIRayTracingPipelineState* FRHIValidation::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: ray tracing is unsupported.");
        return nullptr;
    }

    if (!InDesc.BasePipeline && InDesc.RayGenShaders.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: at least one ray-generation shader is required.");
        return nullptr;
    }

    if (InDesc.MaxRecursionDepth == 0 || InDesc.MaxRecursionDepth > RHI::RayTracingMaxRecursionDepth)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: recursion depth %u exceeds valid range [1, %u].",
            InDesc.MaxRecursionDepth, RHI::RayTracingMaxRecursionDepth);
        return nullptr;
    }

    for (FRHIRayGenShader* Shader : InDesc.RayGenShaders)
    {
        if (!Shader)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: RayGenShaders cannot contain nullptr.");
            return nullptr;
        }
    }

    for (FRHIRayMissShader* Shader : InDesc.MissShaders)
    {
        if (!Shader)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: MissShaders cannot contain nullptr.");
            return nullptr;
        }
    }

    if (IsEnumFlagSet(InDesc.Flags, ERayTracingPipelineFlags::AllowInlineRayTracing) && !RHI::bSupportsInlineRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: AllowInlineRayTracing requested but RHI::bSupportsInlineRayTracing is false on this backend.");
        return nullptr;
    }

    if (IsEnumFlagSet(InDesc.Flags, ERayTracingPipelineFlags::AllowOpacityMicromap) && !RHI::bSupportsOpacityMicromap)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: AllowOpacityMicromap requested but RHI::bSupportsOpacityMicromap is false on this backend.");
        return nullptr;
    }

    if (IsEnumFlagSet(InDesc.Flags, ERayTracingPipelineFlags::AllowShaderExecutionReordering) && !RHI::bSupportsShaderExecutionReordering)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: AllowShaderExecutionReordering requested but RHI::bSupportsShaderExecutionReordering is false on this backend.");
        return nullptr;
    }

    if (IsEnumFlagSet(InDesc.Flags, ERayTracingPipelineFlags::AllowClusteredGeometry) && !RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: AllowClusteredGeometry requested but RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure is false on this backend.");
        return nullptr;
    }

    if (InDesc.BasePipeline != nullptr)
    {
        if (!RHI::bSupportsRayTracingPipelineAdditions)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: BasePipeline set (incremental additions) but RHI::bSupportsRayTracingPipelineAdditions is false on this backend (see RHI.DumpRayTracingCaps).");
            return nullptr;
        }
        else if (!IsEnumFlagSet(InDesc.BasePipeline->GetRayTracingPipelineFlags(), ERayTracingPipelineFlags::AllowStateObjectAdditions))
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: BasePipeline was not created with ERayTracingPipelineFlags::AllowStateObjectAdditions and cannot be extended.");
            return nullptr;
        }
    }

    return RealRHI->CreateRayTracingPipelineState(InDesc);
}

FRHIClusterAccelerationStructure* FRHIValidation::CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc)
{
    if (!RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateClusterAccelerationStructure: clusters are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    if (InDesc.ClusterLimits.MaxTrianglesPerCluster > RHI::RayTracingMaxTrianglesPerCluster)
    {
        RHI_VALIDATION_ERROR("CreateClusterAccelerationStructure: MaxTrianglesPerCluster (%u) exceeds device limit (%u).",
            InDesc.ClusterLimits.MaxTrianglesPerCluster, RHI::RayTracingMaxTrianglesPerCluster);
        return nullptr;
    }

    if (InDesc.ClusterLimits.MaxVerticesPerCluster > RHI::RayTracingMaxVerticesPerCluster)
    {
        RHI_VALIDATION_ERROR("CreateClusterAccelerationStructure: MaxVerticesPerCluster (%u) exceeds device limit (%u).",
            InDesc.ClusterLimits.MaxVerticesPerCluster, RHI::RayTracingMaxVerticesPerCluster);
        return nullptr;
    }

    return RealRHI->CreateClusterAccelerationStructure(InDesc);
}

FRHIClusterTemplate* FRHIValidation::CreateClusterTemplate(const FRHIClusterTemplateDesc& InDesc)
{
    if (!RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateClusterTemplate: clusters are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    return RealRHI->CreateClusterTemplate(InDesc);
}

FRHIPartitionedSceneAccelerationStructure* FRHIValidation::CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs)
{
    if (!RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreatePartitionedSceneAccelerationStructure: partitioned scenes are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    if (RHI::RayTracingMaxPartitionedInstanceCount != 0 && InInputs.MaxInstanceCount > RHI::RayTracingMaxPartitionedInstanceCount)
    {
        RHI_VALIDATION_ERROR("CreatePartitionedSceneAccelerationStructure: MaxInstanceCount (%u) exceeds device limit (%u).",
            InInputs.MaxInstanceCount, RHI::RayTracingMaxPartitionedInstanceCount);
        return nullptr;
    }

    return RealRHI->CreatePartitionedSceneAccelerationStructure(InInputs);
}

FRHIOpacityMicromap* FRHIValidation::CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc)
{
    if (!RHI::bSupportsOpacityMicromap)
    {
        RHI_VALIDATION_ERROR("CreateOpacityMicromap: opacity micromaps are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    return RealRHI->CreateOpacityMicromap(InDesc);
}

FRHIShaderBindingTable* FRHIValidation::CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateShaderBindingTable: ray tracing is unsupported.");
        return nullptr;
    }

    if (!InDesc.Pipeline)
    {
        RHI_VALIDATION_ERROR("CreateShaderBindingTable: Pipeline cannot be nullptr (the record stride + local layout are reflected from it).");
        return nullptr;
    }

    if (InDesc.NumRayGenerationShaders == 0 ||
        InDesc.NumRayGenerationShaders > InDesc.Pipeline->GetNumExportNames(ERayTracingShaderRecordKind::RayGeneration) ||
        InDesc.NumMissShaders > InDesc.Pipeline->GetNumExportNames(ERayTracingShaderRecordKind::Miss) ||
        InDesc.NumCallableShaders > InDesc.Pipeline->GetNumExportNames(ERayTracingShaderRecordKind::Callable))
    {
        RHI_VALIDATION_ERROR("CreateShaderBindingTable: record counts exceed pipeline exports or ray-generation count is zero.");
        return nullptr;
    }

    if (!RHI::bSupportsShaderBindingTableDescriptors)
    {
        RHI_VALIDATION_WARNING("CreateShaderBindingTable: this backend's local records may only hold buffers; texture/typed-view/sampler local records are rejected at record-update time. Bind those globally instead.");
    }

    return RealRHI->CreateShaderBindingTable(InDesc);
}

void FRHIValidation::GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs& InInputs, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo)
{
    if (!RHI::bSupportsIndirectAccelerationStructureOperations)
    {
        RHI_VALIDATION_ERROR("GetRayTracingAccelerationStructureOperationPrebuildInfo: indirect AS operations are not supported on this backend (see RHI.DumpRayTracingCaps).");
        OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo();
        return;
    }

    RealRHI->GetRayTracingAccelerationStructureOperationPrebuildInfo(InInputs, OutInfo);
}

FRHIRayTracingShaderIdentifier FRHIValidation::GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName)
{
    if (!InPipeline)
    {
        RHI_VALIDATION_ERROR("GetRayTracingShaderIdentifier: Pipeline cannot be nullptr.");
        return FRHIRayTracingShaderIdentifier();
    }

    return RealRHI->GetRayTracingShaderIdentifier(InPipeline, InExportName);
}

bool FRHIValidation::IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader)
{
    if (RealRHI->GetRHIType() != InHeader.RHIType)
    {
        RHI_VALIDATION_WARNING("IsAccelerationStructureSerializationHeaderValid: serialized blob was produced on a different RHI backend; it will be rejected.");
        return false;
    }

    return RealRHI->IsAccelerationStructureSerializationHeaderValid(InHeader);
}

FRHIQuery* FRHIValidation::CreateQuery(EQueryType InQueryType)
{
    if (InQueryType == EQueryType::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateQuery: query type cannot be Unknown.");
        return nullptr;
    }

    if (InQueryType == EQueryType::Timestamp && !RHI::bSupportsTimestampQueries)
    {
        RHI_VALIDATION_ERROR("CreateQuery: timestamp queries are unsupported.");
        return nullptr;
    }

    if (InQueryType == EQueryType::PipelineStatistics && !RHI::bSupportsPipelineStatisticsQueries)
    {
        RHI_VALIDATION_ERROR("CreateQuery: pipeline-statistics queries are unsupported.");
        return nullptr;
    }

    return RealRHI->CreateQuery(InQueryType);
}


FRHIFence* FRHIValidation::CreateFence()
{
    return RealRHI->CreateFence();
}

IRHICommandContext* FRHIValidation::ObtainCommandContext()
{
    IRHICommandContext* RealContext = RealRHI->ObtainCommandContext();
    if (!RealContext)
    {
        return nullptr;
    }

    if (FRHIValidationCommandContext** ExistingValidationContext = RealContextToValidationContextMap.Find(RealContext))
    {
        return *ExistingValidationContext;
    }
    else
    {
        FRHIValidationCommandContext* NewValidationContext = new FRHIValidationCommandContext(RealContext);
        return RealContextToValidationContextMap.Add(RealContext, NewValidationContext);
    }
}

bool FRHIValidation::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Cannot retrieve Query-result from a nullptr Query");
        return false;
    }

    if (Query->GetType() == EQueryType::PipelineStatistics)
    {
        RHI_VALIDATION_ERROR("GetQueryResult cannot be used with a PipelineStatistics query.");
        return false;
    }

    return RealRHI->GetQueryResult(Query, OutResult, Mode);
}

bool FRHIValidation::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Cannot retrieve PipelineStatistics-result from a nullptr Query");
        return false;
    }

    if (Query->GetType() != EQueryType::PipelineStatistics)
    {
        RHI_VALIDATION_ERROR("Query is not a PipelineStatistics query");
        return false;
    }

    return RealRHI->GetPipelineStatisticsResult(Query, OutResult, Mode);
}

void FRHIValidation::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (!Resource)
    {
        RHI_VALIDATION_ERROR("EnqueueResourceDeletion: Resource cannot be nullptr.");
        return;
    }

    RealRHI->EnqueueResourceDeletion(Resource);
}

void* FRHIValidation::GetRHINativeAdapter()
{
    return RealRHI->GetRHINativeAdapter();
}

void* FRHIValidation::GetRHINativeDevice()
{
    return RealRHI->GetRHINativeDevice();
}

void* FRHIValidation::GetRHINativeDirectCommandQueue()
{
    return RealRHI->GetRHINativeDirectCommandQueue();
}

void* FRHIValidation::GetRHINativeComputeCommandQueue()
{
    return RealRHI->GetRHINativeComputeCommandQueue();
}

void* FRHIValidation::GetRHINativeCopyCommandQueue()
{
    return RealRHI->GetRHINativeCopyCommandQueue();
}

bool FRHIValidation::QueryUAVFormatSupport(EFormat Format) const
{
    return RealRHI->QueryUAVFormatSupport(Format);
}

bool FRHIValidation::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    return RealRHI->QueryVideoMemoryInfo(MemoryType, OutMemoryInfo);
}

String FRHIValidation::GetAdapterName() const
{
    return RealRHI->GetAdapterName();
}

FRHIValidationCommandContext::FRHIValidationCommandContext(IRHICommandContext* InRealContext)
    : IRHICommandContext()
    , RealContext(InRealContext)
    , ContextPhase(ECommandContextPhase::Finished)
{
}

FRHIValidationCommandContext::~FRHIValidationCommandContext()
{
}

bool FRHIValidationCommandContext::ValidateRecordingPhase(const CHAR* Caller) const
{
    if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("%s requires an active recording context. Call StartContext first.", Caller);
        return false;
    }

    return true;
}

void FRHIValidationCommandContext::BeginFrame()
{
    RealContext->BeginFrame();
}

void FRHIValidationCommandContext::EndFrame()
{
    RealContext->EndFrame();
}

void FRHIValidationCommandContext::StartContext()
{
    if (ContextPhase >= ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Invalid to call StartContext when FinishContext has not been called in-between");
        return;
    }

    RealContext->StartContext();

    ContextPhase            = ECommandContextPhase::Recording;
    GraphicsPipelineState   = nullptr;
    ComputePipelineState    = nullptr;
    MeshletPipelineState    = nullptr;
    RayTracingPipelineState = nullptr;

    ActiveQueries.Clear();
}

void FRHIValidationCommandContext::FinishContext()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext when inside a renderpass");
        return;
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext before a call to StartContext");
        return;
    }

    if (!ActiveQueries.IsEmpty())
    {
        RHI_VALIDATION_ERROR("FinishContext cannot be called while queries are active.");
        return;
    }

    RealContext->FinishContext();
    ContextPhase = ECommandContextPhase::Finished;
}

void FRHIValidationCommandContext::BeginQuery(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("BeginQuery"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("RHIBeginQuery does not support a Query of type Timestamp");
        return;
    }

    if (ActiveQueries.Contains(Query))
    {
        RHI_VALIDATION_ERROR("BeginQuery cannot begin a query that is already active.");
        return;
    }

    RealContext->BeginQuery(Query);
    ActiveQueries.Add(Query);
}

void FRHIValidationCommandContext::EndQuery(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("EndQuery"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("EndQuery does not support a Query of type Timestamp");
        return;
    }

    if (!ActiveQueries.Contains(Query))
    {
        RHI_VALIDATION_ERROR("EndQuery requires a matching BeginQuery.");
        return;
    }

    RealContext->EndQuery(Query);
    ActiveQueries.Remove(Query);
}

void FRHIValidationCommandContext::QueryTimestamp(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("QueryTimestamp"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call QueryTimestamp when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType != EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("QueryTimestamp only support a Query of type Timestamp");
        return;
    }

    RealContext->QueryTimestamp(Query);
}

void FRHIValidationCommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    if (!RenderTargetView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearRenderTargetView when RenderTargetView is nullptr");
        return;
    }

    RealContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void FRHIValidationCommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, const uint8 Stencil)
{
    if (!DepthStencilView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearDepthStencilView when DepthStencilView is nullptr");
        return;
    }

    RealContext->ClearDepthStencilView(DepthStencilView, Depth, Stencil);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewFloat when UnorderedAccessView is nullptr");
        return;
    }

    RealContext->ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewUint when UnorderedAccessView is nullptr");
        return;
    }

    RealContext->ClearUnorderedAccessViewUint(UnorderedAccessView, Values);
}

void FRHIValidationCommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling EndRenderPass");
        return;
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling StartContext");
        return;
    }

    if (BeginRenderPassDesc.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'",
            RHI_MAX_RENDER_TARGETS, BeginRenderPassDesc.NumRenderTargets);
        return;
    }

    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        if (!BeginRenderPassDesc.RenderTargets[Index].View)
        {
            RHI_VALIDATION_ERROR("BeginRenderPass: render-target attachment %u is nullptr.", Index);
            return;
        }
    }

    if (BeginRenderPassDesc.ShadingRateTexture && !BeginRenderPassDesc.ShadingRateTexture->GetDesc().IsShadingRateTexture())
    {
        RHI_VALIDATION_ERROR("BeginRenderPass: shading-rate texture lacks ShadingRateTexture usage.");
        return;
    }

    RealContext->BeginRenderPass(BeginRenderPassDesc);
    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FRHIValidationCommandContext::EndRenderPass()
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndRenderPass before calling RHIBeginRenderPass");
        return;
    }

    RealContext->EndRenderPass();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    RealContext->SetViewport(ViewportRegion);
}

void FRHIValidationCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    RealContext->SetScissorRect(ScissorRegion);
}

void FRHIValidationCommandContext::SetBlendFactor(const Vector4& Color)
{
    RealContext->SetBlendFactor(Color);
}

void FRHIValidationCommandContext::SetStencilRef(uint32 StencilRef)
{
    RealContext->SetStencilRef(StencilRef);
}

void FRHIValidationCommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
    if (!RHI::bSupportsDynamicDepthBias)
    {
        RHI_VALIDATION_ERROR("SetDepthBias called but dynamic depth bias is not supported on this device");
        return;
    }

    RealContext->SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FRHIValidationCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    if (!ValidateRecordingPhase("SetVertexBuffers"))
    {
        return;
    }

    const uint32 NumVertexBuffers = uint32(InVertexBuffers.Size());
    if (BufferSlot > RHI_MAX_VERTEX_BUFFERS || NumVertexBuffers > RHI_MAX_VERTEX_BUFFERS - BufferSlot)
    {
        RHI_VALIDATION_ERROR("SetVertexBuffers exceeds RHI_MAX_VERTEX_BUFFERS.");
        return;
    }

    for (FRHIBuffer* Buffer : InVertexBuffers)
    {
        if (!Buffer || !Buffer->GetDesc().IsVertexBuffer())
        {
            RHI_VALIDATION_ERROR("SetVertexBuffers requires non-null buffers with EBufferFlags::VertexBuffer.");
            return;
        }
    }

    RealContext->SetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    if (!ValidateRecordingPhase("SetIndexBuffer"))
    {
        return;
    }

    if (!IndexBuffer || !IndexBuffer->GetDesc().IsIndexBuffer() || IndexFormat == EIndexFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("SetIndexBuffer requires an index buffer and a valid index format.");
        return;
    }

    RealContext->SetIndexBuffer(IndexBuffer, IndexFormat);
}

void FRHIValidationCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    if (!ValidateRecordingPhase("SetStreamOutputTargets"))
    {
        return;
    }

    if (!Buffers.IsEmpty() && !Offsets)
    {
        RHI_VALIDATION_ERROR("SetStreamOutputTargets requires offsets for non-empty buffer bindings.");
        return;
    }

    for (FRHIBuffer* const Buffer : Buffers)
    {
        if (Buffer && !Buffer->GetDesc().IsStreamOutputBuffer())
        {
            String DebugName;
            Buffer->GetDebugName(DebugName);
            RHI_VALIDATION_ERROR("SetStreamOutputTargets: Buffer '%s' does not have StreamOutputBuffer flag", *DebugName);
            return;
        }
    }

    RealContext->SetStreamOutputTargets(Buffers, Offsets);
}

void FRHIValidationCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetGraphicsPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetGraphicsPipelineState requires a non-null pipeline.");
        return;
    }

    GraphicsPipelineState = PipelineState;
    RealContext->SetGraphicsPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetComputePipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetComputePipelineState requires a non-null pipeline.");
        return;
    }

    ComputePipelineState = PipelineState;
    RealContext->SetComputePipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetRayTracingPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetRayTracingPipelineState: PipelineState cannot be nullptr.");
        return;
    }

    RayTracingPipelineState = PipelineState;
    RealContext->SetRayTracingPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetMeshletPipelineState(FRHIMeshletPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetMeshletPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetMeshletPipelineState requires a non-null pipeline.");
        return;
    }

    MeshletPipelineState = PipelineState;
    RealContext->SetMeshletPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderConstants when Shader is nullptr");
        return;
    }

    if (!ShaderConstants || NumShaderConstants == 0 || NumShaderConstants > RHI_MAX_SHADER_CONSTANTS)
    {
        RHI_VALIDATION_ERROR("SetShaderConstants requires data and 1..RHI_MAX_SHADER_CONSTANTS constants.");
        return;
    }

    RealContext->SetShaderConstants(Shader, ShaderConstants, NumShaderConstants);
}

void FRHIValidationCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceView when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceView(Shader, ShaderResourceView, RegisterIndex);
}

void FRHIValidationCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceViews when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceViews(Shader, InShaderResourceViews, RegisterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessView when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessView(Shader, UnorderedAccessView, RegisterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessViews when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessViews(Shader, InUnorderedAccessViews, RegisterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffer when Shader is nullptr");
        return;
    }

    if (ConstantBuffer && !ConstantBuffer->GetDesc().IsConstantBuffer())
    {
        RHI_VALIDATION_ERROR("SetConstantBuffer requires EBufferFlags::ConstantBuffer.");
        return;
    }

    RealContext->SetConstantBuffer(Shader, ConstantBuffer, RegisterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffers when Shader is nullptr");
        return;
    }

    for (FRHIBuffer* Buffer : InConstantBuffers)
    {
        if (Buffer && !Buffer->GetDesc().IsConstantBuffer())
        {
            RHI_VALIDATION_ERROR("SetConstantBuffers requires EBufferFlags::ConstantBuffer.");
            return;
        }
    }

    RealContext->SetConstantBuffers(Shader, InConstantBuffers, RegisterIndex);
}

void FRHIValidationCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerState when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerState(Shader, SamplerState, RegisterIndex);
}

void FRHIValidationCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerStates when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerStates(Shader, InSamplerStates, RegisterIndex);
}

void FRHIValidationCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!ValidateRecordingPhase("UpdateBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when SrcData is nullptr");
        return;
    }

    if (Dst->GetDesc().IsDefault() && !Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("UpdateBuffer on Default memory requires EBufferFlags::CopyDest");
        return;
    }

    if (!ValidateBufferRange("UpdateBuffer", Dst->GetDesc(), BufferRegion.Offset, BufferRegion.Size))
    {
        return;
    }

    RealContext->UpdateBuffer(Dst, BufferRegion, SrcData);
}

void FRHIValidationCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    if (!ValidateRecordingPhase("UpdateTexture2D"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when SrcData is nullptr");
        return;
    }

    if (!ValidateTextureRegion2D("UpdateTexture2D", Dst->GetDesc(), MipLevel, TextureRegion))
    {
        return;
    }

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Dst->GetDesc().Format);
    if (BytesPerPixel != 0 && SrcRowPitch < TextureRegion.Width * BytesPerPixel)
    {
        RHI_VALIDATION_ERROR("UpdateTexture2D: source row pitch %u is smaller than the required %u bytes.", SrcRowPitch, TextureRegion.Width * BytesPerPixel);
        return;
    }

    RealContext->UpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
}

void FRHIValidationCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    if (!ValidateRecordingPhase("UpdateTexture3D"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture3D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture3D when SrcData is nullptr");
        return;
    }

    if (Dst->GetDesc().Dimension != ETextureDimension::Texture3D)
    {
        RHI_VALIDATION_ERROR("UpdateTexture3D requires a Texture3D destination.");
        return;
    }

    if (!ValidateTextureRegion3D("UpdateTexture3D", Dst->GetDesc(), MipLevel, TextureRegion))
    {
        return;
    }

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Dst->GetDesc().Format);
    if (BytesPerPixel != 0)
    {
        const uint64 MinRowPitch   = uint64(TextureRegion.Width) * BytesPerPixel;
        const uint64 MinDepthPitch = uint64(SrcRowPitch) * TextureRegion.Height;

        if (SrcRowPitch < MinRowPitch || SrcDepthPitch < MinDepthPitch)
        {
            RHI_VALIDATION_ERROR("UpdateTexture3D: source pitches are too small (Row=%u, Depth=%u, RequiredRow=%llu, RequiredDepth=%llu).",
                SrcRowPitch, SrcDepthPitch, MinRowPitch, MinDepthPitch);
            return;
        }
    }

    RealContext->UpdateTexture3D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
}

void FRHIValidationCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!ValidateRecordingPhase("ResolveTexture"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Src is nullptr");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();
    if (SrcDesc.NumSamples <= 1 || DstDesc.NumSamples != 1 ||
        SrcDesc.Format != DstDesc.Format ||
        SrcDesc.Dimension != DstDesc.Dimension ||
        SrcDesc.Extent != DstDesc.Extent ||
        SrcDesc.NumArraySlices != DstDesc.NumArraySlices)
    {
        RHI_VALIDATION_ERROR("ResolveTexture requires a multisampled source and matching single-sample destination.");
        return;
    }

    RealContext->ResolveTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    if (!ValidateRecordingPhase("CopyBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Src is nullptr");
        return;
    }

    if (!CanUseBufferAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (!CanUseBufferAsCopySource(Src->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyBuffer source requires EBufferFlags::CopySource");
        return;
    }

    if (!ValidateBufferRange("CopyBuffer source", Src->GetDesc(), CopyDesc.SrcOffset, CopyDesc.Size) ||
        !ValidateBufferRange("CopyBuffer destination", Dst->GetDesc(), CopyDesc.DstOffset, CopyDesc.Size))
    {
        return;
    }

    if (Dst == Src)
    {
        if (RHIValidationHelpers::DoRangesOverlap(
            CopyDesc.SrcOffset, CopyDesc.Size, CopyDesc.DstOffset, CopyDesc.Size))
        {
            RHI_VALIDATION_ERROR("CopyBuffer source and destination ranges overlap on the same buffer.");
            return;
        }
    }

    RealContext->CopyBuffer(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!ValidateRecordingPhase("CopyTexture"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Src is nullptr");
        return;
    }

    if (!Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("CopyTexture destination must be created with ETextureUsageFlags::CopyDest");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTexture source must be created with ETextureUsageFlags::CopySource");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();
    if (DstDesc.Dimension != SrcDesc.Dimension ||
        DstDesc.Format != SrcDesc.Format ||
        DstDesc.Extent != SrcDesc.Extent ||
        DstDesc.NumArraySlices != SrcDesc.NumArraySlices ||
        DstDesc.NumMipLevels != SrcDesc.NumMipLevels ||
        DstDesc.NumSamples != SrcDesc.NumSamples)
    {
        RHI_VALIDATION_ERROR("CopyTexture requires matching source and destination dimensions, format, extent, slices, mips, and sample count.");
        return;
    }

    RealContext->CopyTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    if (!ValidateRecordingPhase("CopyTextureRegion"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Src is nullptr");
        return;
    }

    if (!Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion destination must be created with ETextureUsageFlags::CopyDest");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion source must be created with ETextureUsageFlags::CopySource");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();

    if (DstDesc.Dimension != SrcDesc.Dimension)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion requires matching source and destination dimensions.");
        return;
    }

    if (CopyDesc.NumArraySlices == 0 || CopyDesc.NumMipLevels == 0 ||
        CopyDesc.Size.X <= 0 || CopyDesc.Size.Y <= 0 || CopyDesc.Size.Z < 0 ||
        (SrcDesc.Dimension == ETextureDimension::Texture3D && CopyDesc.Size.Z == 0) ||
        CopyDesc.SrcPosition.X < 0 || CopyDesc.SrcPosition.Y < 0 || CopyDesc.SrcPosition.Z < 0 ||
        CopyDesc.DstPosition.X < 0 || CopyDesc.DstPosition.Y < 0 || CopyDesc.DstPosition.Z < 0)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion requires positive XY size/counts, valid depth, and non-negative positions.");
        return;
    }

    const uint32 SrcLayers = RHIDimensionArrayLayers(SrcDesc.Dimension, SrcDesc.NumArraySlices);
    const uint32 DstLayers = RHIDimensionArrayLayers(DstDesc.Dimension, DstDesc.NumArraySlices);

    if (CopyDesc.SrcArraySlice > SrcLayers || CopyDesc.NumArraySlices > SrcLayers - CopyDesc.SrcArraySlice ||
        CopyDesc.DstArraySlice > DstLayers || CopyDesc.NumArraySlices > DstLayers - CopyDesc.DstArraySlice ||
        CopyDesc.SrcMipSlice > SrcDesc.NumMipLevels || CopyDesc.NumMipLevels > SrcDesc.NumMipLevels - CopyDesc.SrcMipSlice ||
        CopyDesc.DstMipSlice > DstDesc.NumMipLevels || CopyDesc.NumMipLevels > DstDesc.NumMipLevels - CopyDesc.DstMipSlice)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion slice or mip range exceeds the source/destination resource.");
        return;
    }

    for (uint32 MipOffset = 0; MipOffset < CopyDesc.NumMipLevels; ++MipOffset)
    {
        IntVector3 SrcExtent;
        IntVector3 DstExtent;
        if (!ValidateTextureMip("CopyTextureRegion source", SrcDesc, CopyDesc.SrcMipSlice + MipOffset, SrcExtent) ||
            !ValidateTextureMip("CopyTextureRegion destination", DstDesc, CopyDesc.DstMipSlice + MipOffset, DstExtent))
        {
            return;
        }

        const uint32 SrcX   = uint32(CopyDesc.SrcPosition.X) >> MipOffset;
        const uint32 SrcY   = uint32(CopyDesc.SrcPosition.Y) >> MipOffset;
        const uint32 SrcZ   = uint32(CopyDesc.SrcPosition.Z) >> MipOffset;
        const uint32 DstX   = uint32(CopyDesc.DstPosition.X) >> MipOffset;
        const uint32 DstY   = uint32(CopyDesc.DstPosition.Y) >> MipOffset;
        const uint32 DstZ   = uint32(CopyDesc.DstPosition.Z) >> MipOffset;
        const uint32 Width  = Math::Max(uint32(CopyDesc.Size.X) >> MipOffset, 1u);
        const uint32 Height = Math::Max(uint32(CopyDesc.Size.Y) >> MipOffset, 1u);
        const uint32 Depth  = Math::Max(uint32(CopyDesc.Size.Z) >> MipOffset, 1u);

        if (SrcX > uint32(SrcExtent.X) || Width > uint32(SrcExtent.X) - SrcX ||
            SrcY > uint32(SrcExtent.Y) || Height > uint32(SrcExtent.Y) - SrcY ||
            SrcZ > uint32(SrcExtent.Z) || Depth > uint32(SrcExtent.Z) - SrcZ ||
            DstX > uint32(DstExtent.X) || Width > uint32(DstExtent.X) - DstX ||
            DstY > uint32(DstExtent.Y) || Height > uint32(DstExtent.Y) - DstY ||
            DstZ > uint32(DstExtent.Z) || Depth > uint32(DstExtent.Z) - DstZ)
        {
            RHI_VALIDATION_ERROR("CopyTextureRegion region exceeds source or destination mip extent.");
            return;
        }
    }

    RealContext->CopyTextureRegion(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    if (!ValidateRecordingPhase("CopyTextureRegionToBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegionToBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegionToBuffer when Src is nullptr");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer source texture must be created with ETextureUsageFlags::CopySource");
        return;
    }

    if (!CanUseBufferAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (!ValidateTextureRegion2D("CopyTextureRegionToBuffer", Src->GetDesc(), SrcMipLevel, SrcRegion) ||
        DstOffset >= Dst->GetDesc().Size)
    {
        if (DstOffset >= Dst->GetDesc().Size)
        {
            RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer destination offset exceeds buffer size.");
        }
        return;
    }

    RealContext->CopyTextureRegionToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel);
}

void FRHIValidationCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    if (!ValidateRecordingPhase("CopyTextureSubresourceToBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureSubresourceToBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureSubresourceToBuffer when Src is nullptr");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer source texture must be created with ETextureUsageFlags::CopySource");
        return;
    }

    if (!CanUseBufferAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    const uint32 SrcLayers = RHIDimensionArrayLayers(Src->GetDesc().Dimension, Src->GetDesc().NumArraySlices);
    if (!ValidateTextureRegion3D("CopyTextureSubresourceToBuffer", Src->GetDesc(), SrcMipLevel, SrcRegion) ||
        SrcArraySlice >= SrcLayers || DstOffset >= Dst->GetDesc().Size)
    {
        if (SrcArraySlice >= SrcLayers)
        {
            RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer source array slice exceeds texture layer count.");
        }
        else if (DstOffset >= Dst->GetDesc().Size)
        {
            RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer destination offset exceeds buffer size.");
        }

        return;
    }

    RealContext->CopyTextureSubresourceToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel, SrcArraySlice);
}

void FRHIValidationCommandContext::WriteFence(FRHIFence* Fence)
{
    if (!ValidateRecordingPhase("WriteFence"))
    {
        return;
    }

    if (!Fence)
    {
        RHI_VALIDATION_ERROR("Invalid to call WriteFence when Fence is nullptr");
        return;
    }

    RealContext->WriteFence(Fence);
}

void FRHIValidationCommandContext::DiscardContents(FRHITexture* Texture)
{
    if (!ValidateRecordingPhase("DiscardContents"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call DiscardContents when Texture is nullptr");
        return;
    }

    RealContext->DiscardContents(Texture);
}

void FRHIValidationCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildSceneAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildSceneAccelerationStructure when RayTracingScene is nullptr");
        return;
    }

    if (BuildDesc.NumInstances > 0 && !BuildDesc.Instances)
    {
        RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: Instances is nullptr for a non-zero instance count.");
        return;
    }

    if (BuildDesc.bUpdate && !IsEnumFlagSet(RayTracingScene->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))
    {
        RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: update requested without AllowUpdate.");
        return;
    }

    RealContext->BuildSceneAccelerationStructure(RayTracingScene, BuildDesc);
}

void FRHIValidationCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildGeometryAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!RayTracingGeometry)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildGeometryAccelerationStructure when RayTracingGeometry is nullptr");
        return;
    }

    if (!BuildDesc.VertexBuffer || BuildDesc.NumVertices == 0 || !BuildDesc.VertexBuffer->GetDesc().IsVertexBuffer())
    {
        RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure requires a vertex buffer and non-zero vertex count.");
        return;
    }

    if (BuildDesc.NumIndices > 0 &&
        (!BuildDesc.IndexBuffer || !BuildDesc.IndexBuffer->GetDesc().IsIndexBuffer() || BuildDesc.IndexFormat == EIndexFormat::Unknown))
    {
        RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure indexed geometry requires an index buffer and valid format.");
        return;
    }

    if (BuildDesc.bUpdate && !IsEnumFlagSet(RayTracingGeometry->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))
    {
        RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: update requested without AllowUpdate.");
        return;
    }

    RealContext->BuildGeometryAccelerationStructure(RayTracingGeometry, BuildDesc);
}

void FRHIValidationCommandContext::SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    if (!ValidateRecordingPhase("SetHitRecordLocalShaderBindings"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: ShaderBindingTable cannot be nullptr.");
        return;
    }

    if (NumBindings > 0 && !Bindings)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: Bindings is nullptr but NumBindings=%u.", NumBindings);
        return;
    }

    const FRHIShaderBindingTableDesc& TableDesc = ShaderBindingTable->GetDesc();
    uint32 RecordCount = 0;
    switch (RecordKind)
    {
        case ERayTracingShaderRecordKind::RayGeneration:
            RecordCount = TableDesc.NumRayGenerationShaders;
            break;

        case ERayTracingShaderRecordKind::Miss:
            RecordCount = TableDesc.NumMissShaders;
            break;

        case ERayTracingShaderRecordKind::Callable:
            RecordCount = TableDesc.NumCallableShaders;
            break;

        case ERayTracingShaderRecordKind::HitGroup:
            RecordCount = TableDesc.NumHitGroupRecords;
            break;

        default:
            RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: invalid record kind.");
            return;
    }

    if (RecordIndex >= RecordCount)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: RecordIndex %u exceeds record count %u.", RecordIndex, RecordCount);
        return;
    }

    if (!RHI::bSupportsShaderBindingTableDescriptors)
    {
        for (uint32 i = 0; i < NumBindings; ++i)
        {
            if (Bindings[i].Type != ERayTracingLocalBindingType::ConstantBuffer)
            {
                RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: this backend's records may only hold buffers; texture/typed-view/sampler local records are not allowed (record %u, kind %s). Use the global bindless heap.", RecordIndex, ToString(RecordKind));
                return;
            }
        }
    }

    RealContext->SetHitRecordLocalShaderBindings(ShaderBindingTable, RecordKind, RecordIndex, Bindings, NumBindings);
}

void FRHIValidationCommandContext::BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ValidateRecordingPhase("BuildShaderBindingTable"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("BuildShaderBindingTable: ShaderBindingTable cannot be nullptr.");
        return;
    }

    RealContext->BuildShaderBindingTable(ShaderBindingTable);
}

void FRHIValidationCommandContext::ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ValidateRecordingPhase("ResetShaderBindingTable"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("ResetShaderBindingTable: ShaderBindingTable cannot be nullptr.");
        return;
    }

    RealContext->ResetShaderBindingTable(ShaderBindingTable);
}

void FRHIValidationCommandContext::DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth)
{
    if (ContextPhase != ECommandContextPhase::Recording || !RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("DispatchRays requires ray-tracing support and a recording context outside a render pass.");
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("DispatchRays: ShaderBindingTable cannot be nullptr.");
        return;
    }

    if (!RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRays requires SetRayTracingPipelineState first.");
        return;
    }

    if (Width == 0 || Height == 0 || Depth == 0)
    {
        RHI_VALIDATION_ERROR("DispatchRays: dispatch dimensions must be non-zero (%u, %u, %u).", Width, Height, Depth);
        return;
    }

    RealContext->DispatchRays(ShaderBindingTable, Width, Height, Depth);
}

void FRHIValidationCommandContext::DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    if (ContextPhase != ECommandContextPhase::Recording || !RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect requires a ray-tracing pipeline and a recording context outside a render pass.");
        return;
    }

    if (!RHI::bSupportsIndirectRayDispatch)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: indirect ray dispatch is not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (!ShaderBindingTable || !ArgumentBuffer)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: ShaderBindingTable and ArgumentBuffer must both be non-null.");
        return;
    }

    if ((ArgumentBufferOffset % sizeof(uint64)) != 0 || ArgumentBufferOffset >= ArgumentBuffer->GetDesc().Size)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect argument offset must be 8-byte aligned and inside the argument buffer.");
        return;
    }

    RealContext->DispatchRaysIndirect(ShaderBindingTable, ArgumentBuffer, ArgumentBufferOffset);
}

void FRHIValidationCommandContext::BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildOpacityMicromap"))
    {
        return;
    }

    if (!RHI::bSupportsOpacityMicromap)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: opacity micromaps are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (!OpacityMicromap)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: OpacityMicromap cannot be nullptr.");
        return;
    }

    if (!BuildDesc.OMMDescriptorBuffer)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: BuildDesc.OMMDescriptorBuffer cannot be nullptr.");
        return;
    }

    if (BuildDesc.NumOpacityMicromaps == 0)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: BuildDesc.NumOpacityMicromaps must be non-zero.");
        return;
    }

    if (!BuildDesc.HistogramEntries.IsEmpty())
    {
        uint64 HistogramTotal = 0;
        for (const FRHIOpacityMicromapHistogramEntry& Entry : BuildDesc.HistogramEntries)
        {
            HistogramTotal += Entry.Count;
        }

        if (HistogramTotal != BuildDesc.NumOpacityMicromaps)
        {
            RHI_VALIDATION_ERROR("BuildOpacityMicromap: histogram entry counts must sum to BuildDesc.NumOpacityMicromaps.");
            return;
        }
    }

    RealContext->BuildOpacityMicromap(OpacityMicromap, BuildDesc);
}

void FRHIValidationCommandContext::ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations)
{
    if (!ValidateRecordingPhase("ExecuteIndirectRayTracingAccelerationStructureOperations"))
    {
        return;
    }

    if (!RHI::bSupportsIndirectAccelerationStructureOperations)
    {
        RHI_VALIDATION_ERROR("ExecuteIndirectRayTracingAccelerationStructureOperations: indirect AS operations are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (NumOperations > 0 && !Operations)
    {
        RHI_VALIDATION_ERROR("ExecuteIndirectRayTracingAccelerationStructureOperations: Operations is nullptr but NumOperations=%u.", NumOperations);
        return;
    }

    RealContext->ExecuteIndirectRayTracingAccelerationStructureOperations(Operations, NumOperations);
}

void FRHIValidationCommandContext::WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources)
{
    if (!ValidateRecordingPhase("WriteAccelerationStructurePostBuildInfo") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: ray tracing is unsupported.");
        }

        return;
    }

    if (!DstBuffer)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: DstBuffer cannot be nullptr.");
        return;
    }

    if (NumSources == 0 || !Sources)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: at least one source acceleration structure is required.");
        return;
    }

    if (DstOffset >= DstBuffer->GetDesc().Size)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: destination offset exceeds buffer size.");
        return;
    }

    if (InfoType == EAccelerationStructurePostBuildInfoType::CompactedSize)
    {
        for (uint32 Index = 0; Index < NumSources; ++Index)
        {
            if (!Sources[Index])
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: source %u cannot be nullptr.", Index);
                return;
            }

            if (!IsEnumFlagSet(Sources[Index]->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: CompactedSize requires source %u to be built with AllowCompaction.", Index);
                return;
            }
        }
    }
    else
    {
        for (uint32 Index = 0; Index < NumSources; ++Index)
        {
            if (!Sources[Index])
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: source %u cannot be nullptr.", Index);
                return;
            }
        }
    }

    if (InfoType == EAccelerationStructurePostBuildInfoType::ToolsVisualization && !RHI::bSupportsToolsVisualization)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: ToolsVisualization requires RHI::bSupportsToolsVisualization.");
        return;
    }

    RealContext->WriteAccelerationStructurePostBuildInfo(DstBuffer, DstOffset, InfoType, Sources, NumSources);
}

void FRHIValidationCommandContext::CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode)
{
    if (!ValidateRecordingPhase("CopyAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: ray tracing is unsupported.");
        }

        return;
    }

    if (!Destination || !Source)
    {
        RHI_VALIDATION_ERROR("CopyAccelerationStructure: Destination and Source must both be non-null.");
        return;
    }

    if (CopyMode == EAccelerationStructureCopyMode::Compact)
    {
        if (!IsEnumFlagSet(Source->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: Compact copy requires the source to be built with AllowCompaction.");
            return;
        }

        if (Destination == Source)
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: Compact copy requires a destination distinct from the source. Use CompactAccelerationStructure for in-place compaction.");
            return;
        }
    }

    if (CopyMode == EAccelerationStructureCopyMode::ToolsVisualizationDecode && !RHI::bSupportsToolsVisualization)
    {
        RHI_VALIDATION_ERROR("CopyAccelerationStructure: ToolsVisualizationDecode requires RHI::bSupportsToolsVisualization.");
        return;
    }

    RealContext->CopyAccelerationStructure(Destination, Source, CopyMode);
}

void FRHIValidationCommandContext::CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes)
{
    if (!ValidateRecordingPhase("CompactAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("CompactAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!AccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: AccelerationStructure must be non-null.");
        return;
    }

    if (!IsEnumFlagSet(AccelerationStructure->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: requires the structure to be built with AllowCompaction.");
        return;
    }

    if (CompactedSizeInBytes == 0)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: compacted size must be non-zero.");
        return;
    }

    if (CompactedSizeInBytes == 0)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: CompactedSizeInBytes must be non-zero (from a CompactedSize post-build query).");
        return;
    }

    RealContext->CompactAccelerationStructure(AccelerationStructure, CompactedSizeInBytes);
}

void FRHIValidationCommandContext::SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset)
{
    if (!ValidateRecordingPhase("SerializeAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("SerializeAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!DstBuffer || !Source)
    {
        RHI_VALIDATION_ERROR("SerializeAccelerationStructure: DstBuffer and Source must both be non-null.");
        return;
    }

    RealContext->SerializeAccelerationStructure(Source, DstBuffer, DstOffset);
}

void FRHIValidationCommandContext::DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset)
{
    if (!ValidateRecordingPhase("DeserializeAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("DeserializeAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!Destination || !SourceBuffer)
    {
        RHI_VALIDATION_ERROR("DeserializeAccelerationStructure: Destination and SourceBuffer must both be non-null.");
        return;
    }

    RealContext->DeserializeAccelerationStructure(Destination, SourceBuffer, SourceOffset);
}

void FRHIValidationCommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    if (!ValidateRecordingPhase("TransitionTextureState"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionTextureState when Texture is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionTextureState when inside a render-pass");
		return;
	}

    {
        const ETextureUsageFlags Usage = Texture->GetDesc().UsageFlags;
        const bool bCopyDest = IsEnumFlagSet(TextureTransition.AfterState, EResourceAccess::CopyDest) ||
            IsEnumFlagSet(TextureTransition.BeforeState, EResourceAccess::CopyDest);
        const bool bCopySource = IsEnumFlagSet(TextureTransition.AfterState, EResourceAccess::CopySource) ||
            IsEnumFlagSet(TextureTransition.BeforeState, EResourceAccess::CopySource);
        
        if (bCopyDest && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopyDest))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopyDest requires ETextureUsageFlags::CopyDest");
            return;
        }

        if (bCopySource && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopySource))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopySource requires ETextureUsageFlags::CopySource");
            return;
        }
    }

    RealContext->TransitionTextureState(Texture, TextureTransition);
}

void FRHIValidationCommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    if (!ValidateRecordingPhase("TransitionBufferState"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionBufferState when Buffer is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionBufferState when inside a render-pass");
		return;
	}

    const FRHIBufferDesc& BufferDesc = Buffer->GetDesc();

    const bool bUsesCopyDest   = IsEnumFlagSet(BeforeState, EResourceAccess::CopyDest) || IsEnumFlagSet(AfterState, EResourceAccess::CopyDest);
    const bool bUsesCopySource = IsEnumFlagSet(BeforeState, EResourceAccess::CopySource) || IsEnumFlagSet(AfterState, EResourceAccess::CopySource);

    if (bUsesCopyDest && !CanUseBufferAsCopyDestination(BufferDesc))
    {
        RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopyDest requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (bUsesCopySource && !CanUseBufferAsCopySource(BufferDesc))
    {
        RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopySource requires EBufferFlags::CopySource");
        return;
    }

    RealContext->TransitionBufferState(Buffer, BeforeState, AfterState);
}

void FRHIValidationCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    if (!ValidateRecordingPhase("RequireTextureState"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireTextureState when Texture is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireTextureState when inside a render-pass");
        return;
    }

    {
        const ETextureUsageFlags Usage = Texture->GetDesc().UsageFlags;
        if (IsEnumFlagSet(RequiredState.State, EResourceAccess::CopyDest) && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopyDest))
        {
            RHI_VALIDATION_ERROR("Requiring a texture in CopyDest requires ETextureUsageFlags::CopyDest");
            return;
        }
        
        if (IsEnumFlagSet(RequiredState.State, EResourceAccess::CopySource) && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopySource))
        {
            RHI_VALIDATION_ERROR("Requiring a texture in CopySource requires ETextureUsageFlags::CopySource");
            return;
        }
    }

    const FRHITextureDesc& TextureDesc = Texture->GetDesc();
    const uint32 NumLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    if ((RequiredState.MipLevel != RHI_ALL_MIP_LEVELS && RequiredState.MipLevel >= TextureDesc.NumMipLevels) ||
        (RequiredState.ArraySlice != RHI_ALL_ARRAY_SLICES && RequiredState.ArraySlice >= NumLayers))
    {
        RHI_VALIDATION_ERROR("RequireTextureState mip level or array slice exceeds the texture.");
        return;
    }

    RealContext->RequireTextureState(Texture, RequiredState);
}

void FRHIValidationCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
    if (!ValidateRecordingPhase("RequireBufferState"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireBufferState when Buffer is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireBufferState when inside a render-pass");
        return;
    }

    if (IsEnumFlagSet(RequiredState, EResourceAccess::CopyDest) && !CanUseBufferAsCopyDestination(Buffer->GetDesc()))
    {
        RHI_VALIDATION_ERROR("Requiring a buffer in CopyDest requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (IsEnumFlagSet(RequiredState, EResourceAccess::CopySource) && !CanUseBufferAsCopySource(Buffer->GetDesc()))
    {
        RHI_VALIDATION_ERROR("Requiring a buffer in CopySource requires EBufferFlags::CopySource");
        return;
    }

    RealContext->RequireBufferState(Buffer, RequiredState);
}

void FRHIValidationCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    if (!ValidateRecordingPhase("UnorderedAccessTextureBarrier"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessTextureBarrier when Texture is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessTextureBarrier when inside a render-pass");
		return;
	}

    if (!Texture->GetDesc().IsUnorderedAccessTexture())
    {
        RHI_VALIDATION_ERROR("UnorderedAccessTextureBarrier requires UnorderedAccessTexture usage.");
        return;
    }

    RealContext->UnorderedAccessTextureBarrier(Texture);
}

void FRHIValidationCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    if (!ValidateRecordingPhase("UnorderedAccessBufferBarrier"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBufferBarrier when Buffer is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBufferBarrier when inside a render-pass");
		return;
	}

    if (!Buffer->GetDesc().IsUnorderedAccessBuffer())
    {
        RHI_VALIDATION_ERROR("UnorderedAccessBufferBarrier requires UnorderedAccessBuffer usage.");
        return;
    }

    RealContext->UnorderedAccessBufferBarrier(Buffer);
}

void FRHIValidationCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call Draw before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || VertexCount == 0)
    {
        RHI_VALIDATION_ERROR("Draw requires a graphics pipeline and non-zero vertex count.");
        return;
    }

    RealContext->Draw(VertexCount, StartVertexLocation);
}

void FRHIValidationCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexed before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || IndexCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawIndexed requires a graphics pipeline and non-zero index count.");
        return;
    }

    RealContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRHIValidationCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawInstanced before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || VertexCountPerInstance == 0 || InstanceCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawInstanced requires a graphics pipeline and non-zero vertex/instance counts.");
        return;
    }

    RealContext->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexedInstanced before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || IndexCountPerInstance == 0 || InstanceCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawIndexedInstanced requires a graphics pipeline and non-zero index/instance counts.");
        return;
    }

    RealContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    if (ContextPhase != ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Dispatch requires a recording context outside a render pass.");
        return;
    }

    if (!ComputePipelineState || (WorkGroupsX == 0 && WorkGroupsY == 0 && WorkGroupsZ == 0))
    {
        RHI_VALIDATION_ERROR("Dispatch requires a compute pipeline and at least one non-zero work-group dimension.");
        return;
    }

    RealContext->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FRHIValidationCommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("DispatchMesh requires an active render pass.");
        return;
    }

    if (!MeshletPipelineState || (ThreadGroupCountX == 0 && ThreadGroupCountY == 0 && ThreadGroupCountZ == 0))
    {
        RHI_VALIDATION_ERROR("DispatchMesh requires a meshlet pipeline and at least one non-zero group dimension.");
        return;
    }

    RealContext->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FRHIValidationCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call PresentSwapChain when SwapChain is nullptr");
        return;
    }

    RealContext->PresentSwapChain(SwapChain, bVerticalSync);
}

void FRHIValidationCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResizeSwapChain when SwapChain is nullptr");
        return;
    }

    RealContext->ResizeSwapChain(SwapChain, Width, Height, Format, ColorSpace);
}

void FRHIValidationCommandContext::ClearState()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearState when inside a render-pass");
        return;
    }

    if (!ActiveQueries.IsEmpty())
    {
        RHI_VALIDATION_ERROR("ClearState cannot be called while queries are active.");
        return;
    }

    RealContext->ClearState();

    GraphicsPipelineState   = nullptr;
    ComputePipelineState    = nullptr;
    MeshletPipelineState    = nullptr;
    RayTracingPipelineState = nullptr;
}

void FRHIValidationCommandContext::Flush()
{
    RealContext->Flush();
}

void FRHIValidationCommandContext::PushEvent(const StringView& Name)
{
    RealContext->PushEvent(Name);
}

void FRHIValidationCommandContext::PopEvent()
{
    RealContext->PopEvent();
}

void* FRHIValidationCommandContext::GetRHINativeCommandList()
{
    return RealContext->GetRHINativeCommandList();
}
