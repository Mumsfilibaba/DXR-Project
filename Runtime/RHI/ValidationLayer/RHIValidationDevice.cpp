#include "Core/Math/Math.h"
#include "RHI/ValidationLayer/RHIValidationDevice.h"
#include "RHI/ValidationLayer/RHIValidationCommandContext.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"
#include "RHI/ValidationLayer/RHIValidationShaderBindingTable.h"

using namespace RHIValidationInternal;

static bool ValidateAttachmentSampleCount(const CHAR* Caller, FRHIDevice* Device, const CHAR* Attachment, EFormat Format, uint32 SampleCount)
{
    uint32 SupportedSampleCounts = 0;
    if (!Device->QuerySupportedSampleCounts(Format, SupportedSampleCounts))
    {
        RHI_VALIDATION_ERROR("%s: %s format '%s' cannot be used as an attachment.", Caller, Attachment, ToString(Format));
        return false;
    }

    if (!IsSampleCountSupported(SupportedSampleCounts, SampleCount))
    {
        RHI_VALIDATION_ERROR("%s: sample count %u is unsupported for %s format '%s' (supported mask 0x%X).",
            Caller, SampleCount, Attachment, ToString(Format), SupportedSampleCounts);
        return false;
    }

    return true;
}

static bool ValidateOutputFormatsSampleCount(const CHAR* Caller, FRHIDevice* Device, const FRHIGraphicsPipelineFormats& OutputFormats, uint32 SampleCount)
{
    for (uint32 Index = 0; Index < OutputFormats.NumRenderTargets; ++Index)
    {
        if (!ValidateAttachmentSampleCount(Caller, Device, "render-target", OutputFormats.RenderTargetFormats[Index], SampleCount))
        {
            return false;
        }
    }

    if (OutputFormats.DepthStencilFormat != EFormat::Unknown)
    {
        return ValidateAttachmentSampleCount(Caller, Device, "depth-stencil", OutputFormats.DepthStencilFormat, SampleCount);
    }

    return true;
}

FRHIValidationDevice::FRHIValidationDevice(FRHIDevice* InRealRHI)
    : FRHIDevice()
    , Device(InRealRHI)
{
}

ERHIType FRHIValidationDevice::GetRHIType() const
{
    return SafeGetRHIType(Device);
}

FRHIValidationDevice::~FRHIValidationDevice()
{
    for (auto It = RealContextToValidationContextMap.CreateIterator(); !It.IsEnd(); ++It)
    {
        delete It.GetValue();
    }

    RealContextToValidationContextMap.Clear();

    delete Device;
    Device = nullptr;
}

void FRHIValidationDevice::BeginFrame()
{
    Device->BeginFrame();
}

void FRHIValidationDevice::EndFrame()
{
    Device->EndFrame();
}

FRHITexture* FRHIValidationDevice::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
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

    const bool bIsSamplerFeedback = InTextureDesc.IsSamplerFeedbackTexture();

    // Opaque feedback formats are always UAV-writable but report no conventional format support.
    if (bIsUAV && !bIsSamplerFeedback && !IsTypelessFormat(InTextureDesc.Format) && !Device->QueryUAVFormatSupport(InTextureDesc.Format))
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

    if (bIsSamplerFeedback)
    {
        if (!RHI::bSupportsSamplerFeedback)
        {
            RHI_VALIDATION_ERROR("CreateTexture: sampler feedback is not supported on this backend (see RHI.DumpCaps).");
            return nullptr;
        }

        if (!IsSamplerFeedbackFormat(InTextureDesc.Format))
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedback requires SamplerFeedbackMinMipOpaque or SamplerFeedbackMipRegionUsedOpaque. (Got '%s').",
                ToString(InTextureDesc.Format));
            return nullptr;
        }

        if (InTextureDesc.Dimension != ETextureDimension::Texture2D && InTextureDesc.Dimension != ETextureDimension::Texture2DArray)
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedback requires Texture2D or Texture2DArray. (Got '%s').", ToString(InTextureDesc.Dimension));
            return nullptr;
        }

        if (InTextureDesc.NumSamples != 1)
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedback requires a single-sampled texture. (NumSamples=%u).", InTextureDesc.NumSamples);
            return nullptr;
        }

        // Each mip region dimension must be a power of two, at least 4, and at most half the paired mip-0 extent.
        const IntVector3 MipRegion = InTextureDesc.SamplerFeedbackMipRegion;
        if (MipRegion.X <= 0 || MipRegion.Y <= 0)
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedback requires a SamplerFeedbackMipRegion with positive X and Y. (Got %d,%d).", MipRegion.X, MipRegion.Y);
            return nullptr;
        }

        if (!Math::IsPowerOfTwo(static_cast<uint32>(MipRegion.X)) || !Math::IsPowerOfTwo(static_cast<uint32>(MipRegion.Y)))
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedbackMipRegion dimensions must be powers of two. (Got %d,%d).", MipRegion.X, MipRegion.Y);
            return nullptr;
        }

        if (MipRegion.X < 4 || MipRegion.Y < 4)
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedbackMipRegion dimensions must be >= 4. (Got %d,%d).", MipRegion.X, MipRegion.Y);
            return nullptr;
        }

        if (MipRegion.X > InTextureDesc.Extent.X / 2 || MipRegion.Y > InTextureDesc.Extent.Y / 2)
        {
            RHI_VALIDATION_ERROR("CreateTexture: SamplerFeedbackMipRegion (%d,%d) must not exceed half the paired texture extent (%d,%d).",
                MipRegion.X, MipRegion.Y, InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
            return nullptr;
        }
    }
    else if (IsSamplerFeedbackFormat(InTextureDesc.Format))
    {
        RHI_VALIDATION_ERROR("CreateTexture: format '%s' requires ETextureUsageFlags::SamplerFeedback.", ToString(InTextureDesc.Format));
        return nullptr;
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

	return Device->CreateTexture(InTextureDesc, InInitialState, InInitialData);
}

FRHIBuffer* FRHIValidationDevice::CreateBuffer(const FRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const void* InitialData)
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
    const bool bIsIndirectArguments     = BufferDesc.IsIndirectArguments();

    if (bIsIndirectArguments && BufferDesc.Size < sizeof(uint32))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: IndirectArguments buffers must contain at least one uint32.");
        return nullptr;
    }

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

    if (IsEnumFlagSet(InitialState, ERHIResourceState::CopyDest) && !IsBufferValidAsCopyDestination(BufferDesc))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopyDest initial access requires EBufferFlags::CopyDest or ReadBack memory.");
        return nullptr;
    }

    if (IsEnumFlagSet(InitialState, ERHIResourceState::CopySource) && !IsBufferValidAsCopySource(BufferDesc))
    {
        RHI_VALIDATION_ERROR("CreateBuffer: CopySource initial access requires EBufferFlags::CopySource.");
        return nullptr;
    }

    if (IsEnumFlagSet(InitialState, ERHIResourceState::IndirectArgument) && !bIsIndirectArguments)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: IndirectArgument initial access requires EBufferFlags::IndirectArguments.");
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

    return Device->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FRHISamplerState* FRHIValidationDevice::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
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

    if (Math::IsNaN(InSamplerDesc.MipLODBias) || Math::IsInfinity(InSamplerDesc.MipLODBias) ||
        Math::IsNaN(InSamplerDesc.MinLOD) || Math::IsInfinity(InSamplerDesc.MinLOD) ||
        Math::IsNaN(InSamplerDesc.MaxLOD) || Math::IsInfinity(InSamplerDesc.MaxLOD) ||
        InSamplerDesc.MinLOD > InSamplerDesc.MaxLOD)
    {
        RHI_VALIDATION_ERROR("CreateSamplerState: LOD values must be finite and MinLOD must not exceed MaxLOD.");
        return nullptr;
    }

    return Device->CreateSamplerState(InSamplerDesc);
}

FRHISwapChain* FRHIValidationDevice::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
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

    return Device->CreateSwapChain(InSwapChainDesc);
}

FRHISceneAccelerationStructure* FRHIValidationDevice::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
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

    return Device->CreateSceneAccelerationStructure(InSceneDesc);
}

FRHIGeometryAccelerationStructure* FRHIValidationDevice::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure: ray tracing is unsupported.");
        return nullptr;
    }

    if (InGeometryDesc.GeometryType != ERayTracingGeometryType::ProceduralAABBs)
    {
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
    }

    if (InGeometryDesc.IsClusteredGeometry() && !RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateGeometryAccelerationStructure: clustered geometry is unsupported.");
        return nullptr;
    }

    return Device->CreateGeometryAccelerationStructure(InGeometryDesc);
}

FRHIShaderResourceView* FRHIValidationDevice::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
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

    return Device->CreateShaderResourceView(InResource, InDesc);
}

FRHIUnorderedAccessView* FRHIValidationDevice::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
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

    if (InDesc.IsSamplerFeedbackUAV())
    {
        RHI_VALIDATION_ERROR("CreateUnorderedAccessView: sampler feedback views must be created with CreateSamplerFeedbackUnorderedAccessView");
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

        if (InDesc.Buffer.Type == EBufferViewType::Typed && !Device->QueryUAVFormatSupport(InDesc.Buffer.Format))
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

        if (!Device->QueryUAVFormatSupport(ViewFormat))
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

    return Device->CreateUnorderedAccessView(InResource, InDesc);
}

FRHIUnorderedAccessView* FRHIValidationDevice::CreateSamplerFeedbackUnorderedAccessView(FRHITexture* InFeedbackTexture, FRHITexture* InTargetedTexture)
{
    if (!RHI::bSupportsSamplerFeedback)
    {
        RHI_VALIDATION_ERROR("CreateSamplerFeedbackUnorderedAccessView: sampler feedback is not supported on this backend (see RHI.DumpCaps).");
        return nullptr;
    }

    if (!InFeedbackTexture || !InFeedbackTexture->GetDesc().IsSamplerFeedbackTexture())
    {
        RHI_VALIDATION_ERROR("CreateSamplerFeedbackUnorderedAccessView: feedback texture must have ETextureUsageFlags::SamplerFeedback");
        return nullptr;
    }

    // A null paired texture is legal; shader writes through the view become no-ops.
    if (InTargetedTexture)
    {
        const FRHITextureDesc& FeedbackDesc = InFeedbackTexture->GetDesc();
        const FRHITextureDesc& TargetedDesc = InTargetedTexture->GetDesc();

        if (FeedbackDesc.Extent.X != TargetedDesc.Extent.X || FeedbackDesc.Extent.Y != TargetedDesc.Extent.Y ||
            FeedbackDesc.NumMipLevels != TargetedDesc.NumMipLevels || FeedbackDesc.NumArraySlices != TargetedDesc.NumArraySlices)
        {
            RHI_VALIDATION_ERROR("CreateSamplerFeedbackUnorderedAccessView: feedback map must match the paired texture's extent, mip count and array size");
            return nullptr;
        }

        if (TargetedDesc.IsMultisampled())
        {
            RHI_VALIDATION_ERROR("CreateSamplerFeedbackUnorderedAccessView: the paired texture must be single-sampled");
            return nullptr;
        }
    }

    return Device->CreateSamplerFeedbackUnorderedAccessView(InFeedbackTexture, InTargetedTexture);
}

FRHIRenderTargetView* FRHIValidationDevice::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
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

    return Device->CreateRenderTargetView(InResource, InDesc);
}

FRHIDepthStencilView* FRHIValidationDevice::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
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

    return Device->CreateDepthStencilView(InResource, InDesc);
}

FRHIComputeShader* FRHIValidationDevice::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateComputeShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateComputeShader(ShaderCode);
}

FRHIVertexShader* FRHIValidationDevice::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateVertexShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateVertexShader(ShaderCode);
}

FRHIHullShader* FRHIValidationDevice::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateHullShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateHullShader(ShaderCode);
}

FRHIDomainShader* FRHIValidationDevice::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateDomainShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateDomainShader(ShaderCode);
}

FRHIGeometryShader* FRHIValidationDevice::CreateGeometryShader(const TArray<uint8>& ShaderCode)
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

    return Device->CreateGeometryShader(ShaderCode);
}

FRHIMeshShader* FRHIValidationDevice::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateMeshShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateMeshShader(ShaderCode);
}

FRHIAmplificationShader* FRHIValidationDevice::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateAmplificationShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreateAmplificationShader(ShaderCode);
}

FRHIPixelShader* FRHIValidationDevice::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    if (ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreatePixelShader: shader bytecode cannot be empty.");
        return nullptr;
    }

    return Device->CreatePixelShader(ShaderCode);
}

FRHIRayGenShader* FRHIValidationDevice::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayGenShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayGenShader(ShaderCode);
}

FRHIRayAnyHitShader* FRHIValidationDevice::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayAnyHitShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayAnyHitShader(ShaderCode);
}

FRHIRayClosestHitShader* FRHIValidationDevice::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayClosestHitShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayClosestHitShader(ShaderCode);
}

FRHIRayMissShader* FRHIValidationDevice::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayMissShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayMissShader(ShaderCode);
}

FRHIRayIntersectionShader* FRHIValidationDevice::CreateRayIntersectionShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayIntersectionShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayIntersectionShader(ShaderCode);
}

FRHIRayCallableShader* FRHIValidationDevice::CreateRayCallableShader(const TArray<uint8>& ShaderCode)
{
    if (!RHI::bSupportsRayTracing || ShaderCode.IsEmpty())
    {
        RHI_VALIDATION_ERROR("CreateRayCallableShader requires ray-tracing support and non-empty bytecode.");
        return nullptr;
    }

    return Device->CreateRayCallableShader(ShaderCode);
}

FRHIDepthStencilState* FRHIValidationDevice::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return Device->CreateDepthStencilState(InDesc);
}

FRHIRasterizerState* FRHIValidationDevice::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return Device->CreateRasterizerState(InDesc);
}

FRHIBlendState* FRHIValidationDevice::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return Device->CreateBlendState(InDesc);
}

FRHIInputLayout* FRHIValidationDevice::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return Device->CreateInputLayout(InInputElements);
}

FRHIGraphicsPipelineState* FRHIValidationDevice::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
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

    if (!ValidateOutputFormatsSampleCount("CreateGraphicsPipelineState", Device, InDesc.RasterizerOutputFormats, InDesc.MultiSampleState.SampleCount))
    {
        return nullptr;
    }

    if (InDesc.MultiSampleState.SampleMask == 0)
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: SampleMask of 0 discards every sample.");
        return nullptr;
    }

    if (InDesc.RasterizerState && InDesc.RasterizerState->GetDesc().ForcedSampleCount != 0 &&
        InDesc.RasterizerOutputFormats.DepthStencilFormat != EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateGraphicsPipelineState: ForcedSampleCount requires no depth-stencil format.");
        return nullptr;
    }

    return Device->CreateGraphicsPipelineState(InDesc);
}

FRHIComputePipelineState* FRHIValidationDevice::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    if (!InDesc.Shader)
    {
        RHI_VALIDATION_ERROR("CreateComputePipelineState: compute shader is required.");
        return nullptr;
    }

    return Device->CreateComputePipelineState(InDesc);
}

FRHIMeshletPipelineState* FRHIValidationDevice::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
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

    if (!ValidateOutputFormatsSampleCount("CreateMeshletPipelineState", Device, InDesc.RasterizerOutputFormats, InDesc.MultiSampleState.SampleCount))
    {
        return nullptr;
    }

    if (InDesc.MultiSampleState.SampleMask == 0)
    {
        RHI_VALIDATION_ERROR("CreateMeshletPipelineState: SampleMask of 0 discards every sample.");
        return nullptr;
    }

    if (InDesc.RasterizerState && InDesc.RasterizerState->GetDesc().ForcedSampleCount != 0 &&
        InDesc.RasterizerOutputFormats.DepthStencilFormat != EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateMeshletPipelineState: ForcedSampleCount requires no depth-stencil format.");
        return nullptr;
    }

    return Device->CreateMeshletPipelineState(InDesc);
}

FRHIRayTracingPipelineState* FRHIValidationDevice::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
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

    for (FRHIRayCallableShader* Shader : InDesc.CallableShaders)
    {
        if (!Shader)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: CallableShaders cannot contain nullptr.");
            return nullptr;
        }
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : InDesc.HitGroups)
    {
        uint32 NumIntersectionShaders = 0;
        for (FRHIRayTracingShader* Shader : HitGroup.Shaders)
        {
            if (!Shader)
            {
                RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: HitGroup '%s' cannot contain nullptr shaders.", *HitGroup.Name);
                return nullptr;
            }

            if (Shader->GetShaderStage() == EShaderStage::RayIntersection)
            {
                ++NumIntersectionShaders;
            }
        }

        const bool bIsProcedural = (HitGroup.Type == ERayTracingHitGroupType::Procedural);
        if (bIsProcedural && NumIntersectionShaders != 1)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: procedural HitGroup '%s' requires exactly one intersection shader (found %u).", *HitGroup.Name, NumIntersectionShaders);
            return nullptr;
        }
        else if (!bIsProcedural && NumIntersectionShaders != 0)
        {
            RHI_VALIDATION_ERROR("CreateRayTracingPipelineState: triangle HitGroup '%s' cannot contain an intersection shader.", *HitGroup.Name);
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

    return Device->CreateRayTracingPipelineState(InDesc);
}

FRHIClusterAccelerationStructure* FRHIValidationDevice::CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc)
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

    return Device->CreateClusterAccelerationStructure(InDesc);
}

FRHIClusterTemplate* FRHIValidationDevice::CreateClusterTemplate(const FRHIClusterTemplateDesc& InDesc)
{
    if (!RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CreateClusterTemplate: clusters are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    return Device->CreateClusterTemplate(InDesc);
}

FRHIPartitionedSceneAccelerationStructure* FRHIValidationDevice::CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs)
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

    return Device->CreatePartitionedSceneAccelerationStructure(InInputs);
}

FRHIOpacityMicromap* FRHIValidationDevice::CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc)
{
    if (!RHI::bSupportsOpacityMicromap)
    {
        RHI_VALIDATION_ERROR("CreateOpacityMicromap: opacity micromaps are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return nullptr;
    }

    return Device->CreateOpacityMicromap(InDesc);
}

FRHIShaderBindingTable* FRHIValidationDevice::CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc)
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

    FRHIShaderBindingTable* ShaderBindingTable = Device->CreateShaderBindingTable(InDesc);
    if (!ShaderBindingTable)
    {
        return nullptr;
    }

    return new FRHIValidationShaderBindingTable(ShaderBindingTable);
}

void FRHIValidationDevice::GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs& InInputs, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo)
{
    if (!RHI::bSupportsIndirectAccelerationStructureOperations)
    {
        RHI_VALIDATION_ERROR("GetRayTracingAccelerationStructureOperationPrebuildInfo: indirect AS operations are not supported on this backend (see RHI.DumpRayTracingCaps).");
        OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo();
        return;
    }

    Device->GetRayTracingAccelerationStructureOperationPrebuildInfo(InInputs, OutInfo);
}

FRHIRayTracingShaderIdentifier FRHIValidationDevice::GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName)
{
    if (!InPipeline)
    {
        RHI_VALIDATION_ERROR("GetRayTracingShaderIdentifier: Pipeline cannot be nullptr.");
        return FRHIRayTracingShaderIdentifier();
    }

    return Device->GetRayTracingShaderIdentifier(InPipeline, InExportName);
}

bool FRHIValidationDevice::IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader)
{
    if (Device->GetRHIType() != InHeader.RHIType)
    {
        RHI_VALIDATION_WARNING("IsAccelerationStructureSerializationHeaderValid: serialized blob was produced on a different RHI backend; it will be rejected.");
        return false;
    }

    return Device->IsAccelerationStructureSerializationHeaderValid(InHeader);
}

FRHIQuery* FRHIValidationDevice::CreateQuery(EQueryType InQueryType)
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

    return Device->CreateQuery(InQueryType);
}


FRHIFence* FRHIValidationDevice::CreateFence()
{
    return Device->CreateFence();
}

IRHICommandContext* FRHIValidationDevice::ObtainCommandContext()
{
    IRHICommandContext* RealContext = Device->ObtainCommandContext();
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

bool FRHIValidationDevice::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
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

    return Device->GetQueryResult(Query, OutResult, Mode);
}

bool FRHIValidationDevice::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
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

    return Device->GetPipelineStatisticsResult(Query, OutResult, Mode);
}

void FRHIValidationDevice::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (!Resource)
    {
        RHI_VALIDATION_ERROR("EnqueueResourceDeletion: Resource cannot be nullptr.");
        return;
    }

    Device->EnqueueResourceDeletion(Resource);
}

void* FRHIValidationDevice::GetRHINativeAdapter()
{
    return Device->GetRHINativeAdapter();
}

void* FRHIValidationDevice::GetRHINativeDevice()
{
    return Device->GetRHINativeDevice();
}

void* FRHIValidationDevice::GetRHINativeDirectCommandQueue()
{
    return Device->GetRHINativeDirectCommandQueue();
}

void* FRHIValidationDevice::GetRHINativeComputeCommandQueue()
{
    return Device->GetRHINativeComputeCommandQueue();
}

void* FRHIValidationDevice::GetRHINativeCopyCommandQueue()
{
    return Device->GetRHINativeCopyCommandQueue();
}

bool FRHIValidationDevice::QueryUAVFormatSupport(EFormat Format) const
{
    return Device->QueryUAVFormatSupport(Format);
}

bool FRHIValidationDevice::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    return Device->QuerySupportedSampleCounts(Format, OutSampleCounts);
}

bool FRHIValidationDevice::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    return Device->QueryVideoMemoryInfo(MemoryType, OutMemoryInfo);
}

String FRHIValidationDevice::GetAdapterName() const
{
    return Device->GetAdapterName();
}

