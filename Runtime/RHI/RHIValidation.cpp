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

static TAutoConsoleVariable<bool> CVarEnableValidationDebugBreak(
    "RHI.EnableValidationDebugBreak",
    "Enables debug-breaks when detecting errors in the custom RHI-validation layer",
    true);

static ERHIType SafeGetRHIType(FRHI* RealRHI)
{
    return RealRHI ? RealRHI->GetType() : ERHIType::Unknown;
}

FRHIValidation::FRHIValidation(FRHI* InRealRHI)
    : FRHI(SafeGetRHIType(InRealRHI))
    , RealRHI(InRealRHI)
{
}

FRHIValidation::~FRHIValidation()
{
}

bool FRHIValidation::Initialize()
{
    return RealRHI->Initialize();
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

	if (InTextureDesc.GetWidth() == 0 || InTextureDesc.GetHeight() == 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Invalid texture extent (Width=%u, Height=%u). Both dimensions must be greater than zero.", InTextureDesc.GetWidth(), InTextureDesc.GetHeight());
		return nullptr;
	}

	if (InTextureDesc.IsTexture3D())
	{
		if (InTextureDesc.GetDepth() == 0)
		{
			RHI_VALIDATION_ERROR("CreateTexture: Texture3D requires Depth > 0. (Depth=%u).", InTextureDesc.GetDepth());
			return nullptr;
		}

		if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: Texture3D must have NumArraySlices == 1. (NumArraySlices=%u).", InTextureDesc.NumArraySlices);
			return nullptr;
		}
	}
	else if (InTextureDesc.GetDepth() != 0)
	{
		RHI_VALIDATION_ERROR("CreateTexture: Non-3D textures must have Depth == 0. (Depth=%u).", InTextureDesc.GetDepth());
		return nullptr;
	}

	// -------------------------------------------------------------------------------------------
	// Dimension-specific rules
	// -------------------------------------------------------------------------------------------
	switch (InTextureDesc.Dimension)
	{
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
		if (InTextureDesc.GetWidth() != InTextureDesc.GetHeight())
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Faces must be square. (Width=%u, Height=%u).", InTextureDesc.GetWidth(), InTextureDesc.GetHeight());
			return nullptr;
		}
		
        if (InTextureDesc.NumArraySlices != 1)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) NumArraySlices must be 1. (NumArraySlices=%u). Use TextureCubeArray for arrays.", InTextureDesc.NumArraySlices);
			return nullptr;
		}

		break;

	case ETextureDimension::TextureCubeArray:
		if (InTextureDesc.GetWidth() != InTextureDesc.GetHeight())
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) Faces must be square. (Width=%u, Height=%u).", InTextureDesc.GetWidth(), InTextureDesc.GetHeight());
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
		if (InTextureDesc.GetWidth() > RHIDeviceFeatureSupport::MaxTexture3DWidth || InTextureDesc.GetHeight() > RHIDeviceFeatureSupport::MaxTexture3DHeight ||
			InTextureDesc.GetDepth() > RHIDeviceFeatureSupport::MaxTexture3DDepth)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture3D) Extent (%u,%u,%u) exceeds device feature support limit (%u,%u,%u).", InTextureDesc.GetWidth(), InTextureDesc.GetHeight(),
                InTextureDesc.GetDepth(), RHIDeviceFeatureSupport::MaxTexture3DWidth, RHIDeviceFeatureSupport::MaxTexture3DHeight, RHIDeviceFeatureSupport::MaxTexture3DDepth);
			return nullptr;
		}
	}
	else if (InTextureDesc.IsTextureCube() || InTextureDesc.IsTextureCubeArray())
	{
		if (InTextureDesc.GetWidth() > RHIDeviceFeatureSupport::MaxCubeTextureSize || InTextureDesc.GetHeight() > RHIDeviceFeatureSupport::MaxCubeTextureSize)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (TextureCube) Face extent (%u,%u) exceeds device feature support limit (%u).", InTextureDesc.GetWidth(), InTextureDesc.GetHeight(),
				RHIDeviceFeatureSupport::MaxCubeTextureSize);
			return nullptr;
		}

		if (InTextureDesc.IsTextureCubeArray())
		{
			const uint32 MaxCubeArraySlices = RHIDeviceFeatureSupport::MaxCubeArrayCount * RHI_NUM_CUBE_FACES;
			if (InTextureDesc.NumArraySlices > MaxCubeArraySlices)
			{
				RHI_VALIDATION_ERROR("CreateTexture: (TextureCubeArray) NumArraySlices (%u) exceeds device feature support limit (%u). (Cubes=%u)", 
                    InTextureDesc.NumArraySlices, MaxCubeArraySlices, RHIDeviceFeatureSupport::MaxCubeArrayCount);
				return nullptr;
			}
		}
	}
	else
	{
		if (InTextureDesc.GetWidth() > RHIDeviceFeatureSupport::MaxTexture2DSize || InTextureDesc.GetHeight() > RHIDeviceFeatureSupport::MaxTexture2DSize)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2D) Extent (%u,%u) exceeds device feature support limit (%u).", InTextureDesc.GetWidth(), InTextureDesc.GetHeight(),
                RHIDeviceFeatureSupport::MaxTexture2DSize);
			return nullptr;
		}

		if (InTextureDesc.IsTexture2DArray() && InTextureDesc.NumArraySlices > RHIDeviceFeatureSupport::MaxTexture2DArrayLayers)
		{
			RHI_VALIDATION_ERROR("CreateTexture: (Texture2DArray) NumArraySlices (%u) exceeds device feature support limit (%u).", InTextureDesc.NumArraySlices, 
                RHIDeviceFeatureSupport::MaxTexture2DArrayLayers);
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
		const uint32 MaxPossibleMipLevels = Math::MaxMipLevelsFromExtent(InTextureDesc.GetWidth(), InTextureDesc.GetHeight(), InTextureDesc.IsTexture3D() ? InTextureDesc.GetDepth() : 1u);
		if (InTextureDesc.NumMipLevels > MaxPossibleMipLevels)
		{
			RHI_VALIDATION_ERROR("CreateTexture: NumMipLevels (%u) exceeds maximum allowed (%u) based on texture extent (%u,%u,%u).", InTextureDesc.NumMipLevels, MaxPossibleMipLevels,
				InTextureDesc.GetWidth(), InTextureDesc.GetHeight(), InTextureDesc.IsTexture3D() ? InTextureDesc.GetDepth() : 1u);
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

    if (BufferDesc.Size > RHIDeviceFeatureSupport::MaxBufferSize)
    {
		RHI_VALIDATION_ERROR("CreateBuffer: The buffer size (%llu bytes) exceeds device feature support. (MaxBufferSize=%llu)",
			static_cast<uint64>(BufferDesc.Size), static_cast<uint64>(RHIDeviceFeatureSupport::MaxBufferSize));
        return nullptr;
    }

    // -------------------------------------------------------------------------------------------
    // Memory flags: require exactly one of Default/Dynamic/ReadBack
    // -------------------------------------------------------------------------------------------
    const bool bMemoryDefault  = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::Default);
    const bool bMemoryDynamic  = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::Dynamic);
    const bool bMemoryReadBack = IsEnumFlagSet(BufferDesc.Flags, EBufferFlags::ReadBack);

    const int32 StorageFlagCount = (bMemoryDefault ? 1 : 0) + (bMemoryDynamic ? 1 : 0) + (bMemoryReadBack ? 1 : 0);
    if (StorageFlagCount != 1)
    {
        RHI_VALIDATION_ERROR("CreateBuffer: Exactly one memory flag must be set. The options are Default/Dynamic/ReadBack. (The flags set are Default=%s, Dynamic=%s, ReadBack=%s)",
            bMemoryDefault ? "true" : "false", bMemoryDynamic ? "true" : "false", bMemoryReadBack ? "true" : "false");
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
        // Disallow ReadBack for CB (GPU-read only in practice)
        if (bMemoryReadBack)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: a buffer with ConstantBuffer usage-flag cannot use ReadBack memory. Use Default or Dynamic for GPU-accessible ConstantBuffer.");
            return nullptr;
        }

        if (BufferDesc.Size > RHIDeviceFeatureSupport::MaxConstantBufferSize)
        {
            RHI_VALIDATION_ERROR("CreateBuffer: size (%llu bytes) exceeds device feature support. (MaxConstantBufferSize=%u)", 
                static_cast<uint64>(BufferDesc.Size), RHIDeviceFeatureSupport::MaxConstantBufferSize);
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
        if (BufferDesc.Size > RHIDeviceFeatureSupport::MaxStorageBufferSize)
        {
			RHI_VALIDATION_ERROR("CreateBuffer: %s size exceeds device feature support. (Size=%llu, MaxStorageBufferSize=%llu)",
				bIsUnorderedAccessBuffer ? "UnorderedAccessBuffer" : "ShaderResourceBuffer", static_cast<uint64>(BufferDesc.Size),
				static_cast<uint64>(RHIDeviceFeatureSupport::MaxStorageBufferSize));
            return nullptr;
        }

        if (BufferDesc.Stride > 0)
        {
            // Structured: enforce stride window and size divisibility
            const uint32 MinStride = RHIDeviceFeatureSupport::StructuredBufferMinStride;
            const uint32 MaxStride = RHIDeviceFeatureSupport::StructuredBufferMaxStride;

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
            const uint64 RequiredAlignment = static_cast<uint64>(RHIDeviceFeatureSupport::RawBufferRequiredAlignment);
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

FRHIShaderResourceView* FRHIValidation::CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
{
    if (InDesc.IsBufferSRV())
    {
		if (!InDesc.BufferSRV.Buffer)
		{
			RHI_VALIDATION_ERROR("Buffer cannot be nullptr when creating a ShaderResourceView");
			return nullptr;
		}

		const FRHIBufferDesc& BufferDesc = InDesc.BufferSRV.Buffer->GetDesc();
		if (!BufferDesc.IsShaderResourceBuffer())
		{
			RHI_VALIDATION_ERROR("Buffer must have a the EBufferFlags::ShaderResourceBuffer to used with a ShaderResourceView");
			return nullptr;
		}
    }
    else if (InDesc.IsTextureSRV())
    {
        if (!InDesc.TextureSRV.Texture)
        {
            RHI_VALIDATION_ERROR("Texture cannot be nullptr when creating a ShaderResourceView");
            return nullptr;
        }

        const FRHITextureDesc& TextureDesc = InDesc.TextureSRV.Texture->GetDesc();
        if (!TextureDesc.IsShaderResourceTexture())
        {
            RHI_VALIDATION_ERROR("Texture must have a the ETextureUsageFlags::ShaderResourceTexture to used with a ShaderResourceView");
            return nullptr;
        }

        if (InDesc.TextureSRV.Format == EFormat::Unknown)
        {
            RHI_VALIDATION_ERROR("Format cannot be EFormat::Unknown when creating a ShaderResourceView");
            return nullptr;
        }

        if (IsTypelessFormat(InDesc.TextureSRV.Format))
        {
            RHI_VALIDATION_ERROR("Format cannot be a typeless format when creating a ShaderResourceView");
            return nullptr;
        }

        const uint32 NumArraySlices = InDesc.TextureSRV.FirstArraySlice + InDesc.TextureSRV.NumSlices;
        if (NumArraySlices > TextureDesc.NumArraySlices)
        {
            RHI_VALIDATION_ERROR("Trying to create a ShaderResourceView with '%u' ArraySlices, but texture only contains '%u'", NumArraySlices, TextureDesc.NumArraySlices);
            return nullptr;
        }

        const uint32 NumMipLevels = InDesc.TextureSRV.FirstMipLevel + InDesc.TextureSRV.NumMips;
        if (NumMipLevels > TextureDesc.NumMipLevels)
        {
            RHI_VALIDATION_ERROR("Trying to create a ShaderResourceView with '%u' MipLevels, but texture only contains '%u'", NumMipLevels, TextureDesc.NumMipLevels);
            return nullptr;
        }
    }
    else
    {
		RHI_VALIDATION_ERROR("Invalid type of ShaderResourceView");
		return nullptr;
    }

    return RealRHI->CreateShaderResourceView(InDesc);
}

FRHIUnorderedAccessView* FRHIValidation::CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (InDesc.IsBufferUAV())
    {
	    if (!InDesc.BufferUAV.Buffer)
	    {
		    RHI_VALIDATION_ERROR("Buffer cannot be nullptr when creating a UnorderedAccessView");
		    return nullptr;
	    }

	    const FRHIBufferDesc& BufferDesc = InDesc.BufferUAV.Buffer->GetDesc();
	    if (!BufferDesc.IsUnorderedAccessBuffer())
	    {
		    RHI_VALIDATION_ERROR("Buffer must have a the EBufferFlags::UnorderedAccessBuffer to used with a UnorderedAccessView");
		    return nullptr;
	    }
    }
    else if (InDesc.IsTextureUAV())
    {
		if (!InDesc.TextureUAV.Texture)
		{
			RHI_VALIDATION_ERROR("Texture cannot be nullptr when creating a UnorderedAccessView");
			return nullptr;
		}

		const FRHITextureDesc& TextureDesc = InDesc.TextureUAV.Texture->GetDesc();
		if (!TextureDesc.IsUnorderedAccessTexture())
		{
			RHI_VALIDATION_ERROR("Texture must have a the ETextureUsageFlags::UnorderedAccessTexture to used with a UnorderedAccessView");
			return nullptr;
		}

		if (InDesc.TextureUAV.Format == EFormat::Unknown)
		{
			RHI_VALIDATION_ERROR("Format cannot be EFormat::Unknown when creating a UnorderedAccessView");
			return nullptr;
		}

		if (IsTypelessFormat(InDesc.TextureUAV.Format))
		{
			RHI_VALIDATION_ERROR("Format cannot be a typeless format when creating a UnorderedAccessView");
			return nullptr;
		}

		const uint32 NumArraySlices = InDesc.TextureUAV.FirstArraySlice + InDesc.TextureUAV.NumSlices;
		if (NumArraySlices > TextureDesc.NumArraySlices)
		{
			RHI_VALIDATION_ERROR("Trying to create a UnorderedAccessView with '%u' ArraySlices, but texture only contains '%u'", NumArraySlices, TextureDesc.NumArraySlices);
			return nullptr;
		}

		if (InDesc.TextureUAV.MipLevel >= TextureDesc.NumMipLevels)
		{
			RHI_VALIDATION_ERROR("Trying to create a UnorderedAccessView for MipLevel '%u', but texture only contains '%u'", InDesc.TextureUAV.MipLevel, TextureDesc.NumMipLevels);
			return nullptr;
		}
    }
    else
    {
		RHI_VALIDATION_ERROR("Invalid type of UnorderedAccessView");
		return nullptr;
    }

    return RealRHI->CreateUnorderedAccessView(InDesc);
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

bool FRHIValidation::GetQueryResult(FRHIQuery* Query, uint64& OutResult)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Cannot retrieve Query-result from a nullptr Query");
        return false;
    }

    return RealRHI->GetQueryResult(Query, OutResult);
}

void FRHIValidation::EnqueueResourceDeletion(FRHIResource* Resource)
{
    RealRHI->EnqueueResourceDeletion(Resource);
}

void* FRHIValidation::GetNativeAdapter()
{
    return RealRHI->GetNativeAdapter();
}

void* FRHIValidation::GetNativeDevice()
{
    return RealRHI->GetNativeDevice();
}

void* FRHIValidation::GetNativeDirectCommandQueue()
{
    return RealRHI->GetNativeDirectCommandQueue();
}

void* FRHIValidation::GetNativeComputeCommandQueue()
{
    return RealRHI->GetNativeComputeCommandQueue();
}

void* FRHIValidation::GetNativeCopyCommandQueue()
{
    return RealRHI->GetNativeCopyCommandQueue();
}

bool FRHIValidation::QueryUAVFormatSupport(EFormat Format) const
{
    return RealRHI->QueryUAVFormatSupport(Format);
}

bool FRHIValidation::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    return RealRHI->QueryVideoMemoryInfo(MemoryType, OutMemoryInfo);
}

FString FRHIValidation::GetAdapterName() const
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

void FRHIValidationCommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    if (!RenderTargetView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearRenderTargetView when Texture is nullptr");
        return;
    }

    RealContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void FRHIValidationCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil)
{
    if (!DepthStencilView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearDepthStencilView when Texture is nullptr");
        return;
    }

    RealContext->ClearDepthStencilView(DepthStencilView, Depth, Stencil);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewFloat when UnorderedAccessView is nullptr");
        return;
    }

    RealContext->ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
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
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'", RHI_MAX_RENDER_TARGETS, BeginRenderPassDesc.NumRenderTargets);
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

void FRHIValidationCommandContext::SetBlendFactor(const FVector4& Color)
{
    RealContext->SetBlendFactor(Color);
}

void FRHIValidationCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    RealContext->SetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    RealContext->SetIndexBuffer(IndexBuffer, IndexFormat);
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

void FRHIValidationCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceView when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
}

void FRHIValidationCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceViews when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceViews(Shader, InShaderResourceViews, ParameterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessView when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessViews when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessViews(Shader, InUnorderedAccessViews, ParameterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffer when Shader is nullptr");
        return;
    }

    RealContext->SetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffers when Shader is nullptr");
        return;
    }

    RealContext->SetConstantBuffers(Shader, InConstantBuffers, ParameterIndex);
}

void FRHIValidationCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerState when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerState(Shader, SamplerState, ParameterIndex);
}

void FRHIValidationCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerStates when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerStates(Shader, InSamplerStates, ParameterIndex);
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

void FRHIValidationCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc)
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

    RealContext->CopyTextureRegion(Dst, Src, InCopyDesc);
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

void FRHIValidationCommandContext::TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionTexture when Texture is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionTexture when inside a render-pass");
		return;
	}

    RealContext->TransitionTexture(Texture, TextureTransition);
}

void FRHIValidationCommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionBuffer when Buffer is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionBuffer when inside a render-pass");
		return;
	}

    RealContext->TransitionBuffer(Buffer, BeforeState, AfterState);
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

void FRHIValidationCommandContext::EnableResourceStateTracking(FRHITexture* Texture, EResourceAccess InitialState)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call EnableResourceStateTracking when Texture is nullptr");
        return;
    }

    RealContext->EnableResourceStateTracking(Texture, InitialState);
}

void FRHIValidationCommandContext::DisableResourceStateTracking(FRHITexture* Texture, EResourceAccess TargetState)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call DisableResourceStateTracking when Texture is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DisableResourceStateTracking when inside a render-pass");
        return;
    }

    RealContext->DisableResourceStateTracking(Texture, TargetState);
}

void FRHIValidationCommandContext::EnableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess InitialState)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call EnableResourceStateTracking when Buffer is nullptr");
        return;
    }

    RealContext->EnableResourceStateTracking(Buffer, InitialState);
}

void FRHIValidationCommandContext::DisableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess TargetState)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call DisableResourceStateTracking when Buffer is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DisableResourceStateTracking when inside a render-pass");
        return;
    }

    RealContext->DisableResourceStateTracking(Buffer, TargetState);
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

void FRHIValidationCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResizeSwapChain when SwapChain is nullptr");
        return;
    }

    RealContext->ResizeSwapChain(SwapChain, Width, Height);
}

void FRHIValidationCommandContext::ClearState()
{
    RealContext->ClearState();
}

void FRHIValidationCommandContext::Flush()
{
    RealContext->Flush();
}

void FRHIValidationCommandContext::InsertMarker(const FStringView& Message)
{
    RealContext->InsertMarker(Message);
}

void FRHIValidationCommandContext::BeginExternalCapture()
{
    RealContext->BeginExternalCapture();
}

void FRHIValidationCommandContext::EndExternalCapture()
{
    RealContext->EndExternalCapture();
}

void* FRHIValidationCommandContext::GetNativeCommandList()
{
    return RealContext->GetNativeCommandList();
}
