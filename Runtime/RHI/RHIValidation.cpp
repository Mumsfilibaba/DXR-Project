#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHIValidation.h"

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
    if (BaseLayer + LayerCount > MaxLayers)
    {
        RHI_VALIDATION_ERROR("%s: slice range [%u, %u) exceeds texture native layer count (%u).", Caller, BaseLayer, BaseLayer + LayerCount, MaxLayers);
        return false;
    }

    if (NumMips == 0 || FirstMip + NumMips > TextureDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("%s: mip range [%u, %u) exceeds texture mip count (%u).", Caller, FirstMip, FirstMip + NumMips, TextureDesc.NumMipLevels);
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
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Faces must be square. (Width=%u, Height=%u).",
                InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
			return nullptr;
		}
		
        if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) NumArraySlices must be 1. (NumArraySlices=%u). Use TextureCubeArray for arrays.",
                InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::TextureCubeArray:
		if (InTextureDesc.Extent.X != InTextureDesc.Extent.Y)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) Faces must be square. (Width=%u, Height=%u).",
                InTextureDesc.Extent.X, InTextureDesc.Extent.Y);
			return nullptr;
		}

		if (InTextureDesc.NumArraySlices == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) NumArraySlices must be >= 1. (NumArraySlices=%u).",
                InTextureDesc.NumArraySlices);
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
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Face extent (%u,%u) exceeds device feature support limit (%u).",
                InTextureDesc.Extent.X, InTextureDesc.Extent.Y, RHI::MaxCubeTextureSize);
			return nullptr;
		}

		if (InTextureDesc.IsTextureCubeArray())
		{
			if (InTextureDesc.NumArraySlices > RHI::MaxCubeArrayCount)
			{
				RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) NumArraySlices (%u) exceeds device feature support limit (%u cubes).",
                    InTextureDesc.NumArraySlices, RHI::MaxCubeArrayCount);
				return nullptr;
			}
		}
	}
	else
	{
		if (static_cast<uint32>(InTextureDesc.Extent.X) > RHI::MaxTexture2DSize || static_cast<uint32>(InTextureDesc.Extent.Y) > RHI::MaxTexture2DSize)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2D) Extent (%u,%u) exceeds device feature support limit (%u).",
                InTextureDesc.Extent.X, InTextureDesc.Extent.Y, RHI::MaxTexture2DSize);
			return nullptr;
		}

		if (InTextureDesc.IsTexture2DArray() && InTextureDesc.NumArraySlices > RHI::MaxTexture2DArrayLayers)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2DArray) NumArraySlices (%u) exceeds device feature support limit (%u).",
                InTextureDesc.NumArraySlices, RHI::MaxTexture2DArrayLayers);
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

	if (InTextureDesc.IsTexture3D() && InTextureDesc.NumSamples > 1)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Texture3D does not support MSAA. (NumSamples=%u, Expected=1).", InTextureDesc.NumSamples);
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

    // -------------------------------------------------------------------------------------------
    // Constant buffer rules
    // -------------------------------------------------------------------------------------------
    
    if (bIsConstantBuffer)
    {
        if (bMemoryReadBack)
        {
            RHI_VALIDATION_ERROR(
                "CreateBuffer: a buffer with ConstantBuffer usage-flag cannot use ReadBack memory. Use Default, Dynamic, or Transient for GPU-accessible ConstantBuffer.");
            return nullptr;
        }

        if (BufferDesc.Size > RHI::MaxConstantBufferSize)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: size (%llu bytes) exceeds device feature support. (MaxConstantBufferSize=%u)", 
                static_cast<uint64>(BufferDesc.Size), RHI::MaxConstantBufferSize);
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
    return RealRHI->CreateSamplerState(InSamplerDesc);
}

FRHISwapChain* FRHIValidation::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    if (!InSwapChainDesc.WindowHandle)
    {
        RHI_VALIDATION_ERROR("Trying to create a viewport with an invalid WindowHandle");
        return nullptr;
    }

    return RealRHI->CreateSwapChain(InSwapChainDesc);
}

FRHISceneAccelerationStructure* FRHIValidation::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    return RealRHI->CreateSceneAccelerationStructure(InSceneDesc);
}

FRHIGeometryAccelerationStructure* FRHIValidation::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
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
                LayerCount = Math::Max<uint16>(InDesc.Texture1DArray.NumSlices, 1u);
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
                LayerCount = Math::Max<uint16>(InDesc.Texture2DArray.NumSlices, 1u);
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
                LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, Math::Max<uint16>(InDesc.TextureCubeArray.NumCubes, 1u));
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
                LayerCount = Math::Max<uint16>(InDesc.Texture1DArray.NumSlices, 1u);
                break;

            case EViewDimension::Texture2D:
                ViewFormat = InDesc.Texture2D.Format;
                MipLevel   = InDesc.Texture2D.MipLevel;
                break;

            case EViewDimension::Texture2DArray:
                ViewFormat = InDesc.Texture2DArray.Format;
                MipLevel   = InDesc.Texture2DArray.MipLevel;
                BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
                LayerCount = Math::Max<uint16>(InDesc.Texture2DArray.NumSlices, 1u);
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
                LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, Math::Max<uint16>(InDesc.TextureCubeArray.NumCubes, 1u));
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
            LayerCount = Math::Max<uint16>(InDesc.Texture1DArray.NumSlices, 1u);
            break;

        case EViewDimension::Texture2D:
            ViewFormat = InDesc.Texture2D.Format;
            MipLevel   = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            ViewFormat = InDesc.Texture2DArray.Format;
            MipLevel   = InDesc.Texture2DArray.MipLevel;
            BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
            LayerCount = Math::Max<uint16>(InDesc.Texture2DArray.NumSlices, 1u);
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
            LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, Math::Max<uint16>(InDesc.TextureCubeArray.NumCubes, 1u));
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
            LayerCount = Math::Max<uint16>(InDesc.Texture1DArray.NumSlices, 1u);
            break;

        case EViewDimension::Texture2D:
            ViewFormat = InDesc.Texture2D.Format;
            MipLevel   = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            ViewFormat = InDesc.Texture2DArray.Format;
            MipLevel   = InDesc.Texture2DArray.MipLevel;
            BaseLayer  = InDesc.Texture2DArray.FirstArraySlice;
            LayerCount = Math::Max<uint16>(InDesc.Texture2DArray.NumSlices, 1u);
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
            LayerCount = RHICubesToArrayLayers(TextureDesc.Dimension, Math::Max<uint16>(InDesc.TextureCubeArray.NumCubes, 1u));
            break;
            
        default:
            break;
    }

    if (ViewFormat == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: Format cannot be EFormat::Unknown");
        return nullptr;
    }

    if (!IsViewDimensionCompatible(TextureDesc.Dimension, InDesc.ViewDimension))
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: ViewDimension '%s' is incompatible with texture dimension '%s'", ToString(InDesc.ViewDimension), ToString(TextureDesc.Dimension));
        return nullptr;
    }

    const uint32 MaxLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);
    if (BaseLayer + LayerCount > MaxLayers)
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: slice range [%u, %u) exceeds texture native layer count (%u).", BaseLayer, BaseLayer + LayerCount, MaxLayers);
        return nullptr;
    }

    if (MipLevel >= TextureDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("CreateDepthStencilView: MipLevel '%u' exceeds texture mip count (%u).", MipLevel, TextureDesc.NumMipLevels);
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
    return RealRHI->CreateComputeShader(ShaderCode);
}

FRHIVertexShader* FRHIValidation::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateVertexShader(ShaderCode);
}

FRHIHullShader* FRHIValidation::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateHullShader(ShaderCode);
}

FRHIDomainShader* FRHIValidation::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateDomainShader(ShaderCode);
}

FRHIGeometryShader* FRHIValidation::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateGeometryShader(ShaderCode);
}

FRHIMeshShader* FRHIValidation::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateMeshShader(ShaderCode);
}

FRHIAmplificationShader* FRHIValidation::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateAmplificationShader(ShaderCode);
}

FRHIPixelShader* FRHIValidation::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreatePixelShader(ShaderCode);
}

FRHIRayGenShader* FRHIValidation::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayGenShader(ShaderCode);
}

FRHIRayAnyHitShader* FRHIValidation::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayAnyHitShader(ShaderCode);
}

FRHIRayClosestHitShader* FRHIValidation::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayClosestHitShader(ShaderCode);
}

FRHIRayMissShader* FRHIValidation::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
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
    return RealRHI->CreateGraphicsPipelineState(InDesc);
}

FRHIComputePipelineState* FRHIValidation::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return RealRHI->CreateComputePipelineState(InDesc);
}

FRHIRayTracingPipelineState* FRHIValidation::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    return RealRHI->CreateRayTracingPipelineState(InDesc);
}

FRHIQuery* FRHIValidation::CreateQuery(EQueryType InQueryType)
{
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

    if (FRHIValidationCommandContext** ExistingValidationContextContext = RealContextToValidationContextMap.Find(RealContext))
    {
        return *ExistingValidationContextContext;
    }
    else
    {
        FRHIValidationCommandContext* NewValitationContext = new FRHIValidationCommandContext(RealContext);
        return RealContextToValidationContextMap.Add(RealContext, NewValitationContext);
    }
}

bool FRHIValidation::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Cannot retrieve Query-result from a nullptr Query");
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
    }

    RealContext->StartContext();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::FinishContext()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext when inside a renderpass");
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext before a call to StartContext");
    }

    RealContext->FinishContext();
    ContextPhase = ECommandContextPhase::Finished;
}

void FRHIValidationCommandContext::BeginQuery(FRHIQuery* Query)
{
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

    RealContext->BeginQuery(Query);
}

void FRHIValidationCommandContext::EndQuery(FRHIQuery* Query)
{
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

    RealContext->EndQuery(Query);
}

void FRHIValidationCommandContext::QueryTimestamp(FRHIQuery* Query)
{
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
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling StartContext");
    }

    if (BeginRenderPassDesc.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'",
            RHI_MAX_RENDER_TARGETS, BeginRenderPassDesc.NumRenderTargets);
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
    }

    RealContext->SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FRHIValidationCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    RealContext->SetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    RealContext->SetIndexBuffer(IndexBuffer, IndexFormat);
}

void FRHIValidationCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    for (FRHIBuffer* const Buffer : Buffers)
    {
        if (Buffer && !Buffer->GetDesc().IsStreamOutputBuffer())
        {
            RHI_VALIDATION_ERROR("SetStreamOutputTargets: Buffer '%s' does not have StreamOutputBuffer flag", "");
        }
    }

    RealContext->SetStreamOutputTargets(Buffers, Offsets);
}

void FRHIValidationCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    RealContext->SetGraphicsPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    RealContext->SetComputePipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderConstants when Shader is nullptr");
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

    RealContext->SetConstantBuffer(Shader, ConstantBuffer, RegisterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffers when Shader is nullptr");
        return;
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

    RealContext->UpdateBuffer(Dst, BufferRegion, SrcData);
}

void FRHIValidationCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
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

    RealContext->UpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
}

void FRHIValidationCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
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

    RealContext->UpdateTexture3D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
}

void FRHIValidationCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
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

    RealContext->ResolveTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
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

    RealContext->CopyBuffer(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
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

    RealContext->CopyTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
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

    RealContext->CopyTextureRegion(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
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

    RealContext->CopyTextureRegionToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel);
}

void FRHIValidationCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
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

    RealContext->CopyTextureSubresourceToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel, SrcArraySlice);
}

void FRHIValidationCommandContext::WriteFence(FRHIFence* Fence)
{
    if (!Fence)
    {
        RHI_VALIDATION_ERROR("Invalid to call WriteFence when Fence is nullptr");
        return;
    }

    RealContext->WriteFence(Fence);
}

void FRHIValidationCommandContext::DiscardContents(FRHITexture* Texture)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call DiscardContents when Texture is nullptr");
        return;
    }

    RealContext->DiscardContents(Texture);
}

void FRHIValidationCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildSceneAccelerationStructure when RayTracingScene is nullptr");
        return;
    }

    RealContext->BuildSceneAccelerationStructure(RayTracingScene, BuildDesc);
}

void FRHIValidationCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    if (!RayTracingGeometry)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildGeometryAccelerationStructure when RayTracingGeometry is nullptr");
        return;
    }

    RealContext->BuildGeometryAccelerationStructure(RayTracingGeometry, BuildDesc);
}

void FRHIValidationCommandContext::SetRayTracingBindings(FRHISceneAccelerationStructure* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetRayTracingBindings when RayTracingScene is nullptr");
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetRayTracingBindings when PipelineState is nullptr");
        return;
    }

    RealContext->SetRayTracingBindings(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
}

void FRHIValidationCommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
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

    RealContext->TransitionTextureState(Texture, TextureTransition);
}

void FRHIValidationCommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
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

    RealContext->TransitionBufferState(Buffer, BeforeState, AfterState);
}

void FRHIValidationCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
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

    RealContext->RequireTextureState(Texture, RequiredState);
}

void FRHIValidationCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
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

    RealContext->RequireBufferState(Buffer, RequiredState);
}

void FRHIValidationCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
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

    RealContext->UnorderedAccessTextureBarrier(Texture);
}

void FRHIValidationCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
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

    RealContext->UnorderedAccessBufferBarrier(Buffer);
}

void FRHIValidationCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call Draw before entering a render-pass");
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

    RealContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRHIValidationCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawInstanced before entering a render-pass");
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

    RealContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    RealContext->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FRHIValidationCommandContext::DispatchRays(FRHISceneAccelerationStructure* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    RealContext->DispatchRays(Scene, PipelineState, Width, Height, Depth);
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
    RealContext->ClearState();
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
