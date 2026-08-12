#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

enum class ETextureUsageFlags : uint16
{
    None = 0,

    /** RenderTargetView */
    RenderTarget = FLAG(1),

    /** DepthStencilView */
    DepthStencil = FLAG(2),

    /** UnorderedAccessView */
    UnorderedAccessTexture = FLAG(3),

    /** ShaderResourceView */
    ShaderResourceTexture = FLAG(4),

    /** Indicates that the texture is going to be used as a shading rate texture */
    ShadingRateTexture = FLAG(5),

    /** Indicates that the texture is a BackBuffer resource */
    Presentable = FLAG(6),

    /** Do not create a default ShaderResourceView at texture creation time */
    NoDefaultSRV = FLAG(7),

    /** Do not create a default UnorderedAccessView at texture creation time */
    NoDefaultUAV = FLAG(8),

    /** Do not create a default RenderTargetView at texture creation time */
    NoDefaultRTV = FLAG(9),

    /** Do not create a default DepthStencilView at texture creation time */
    NoDefaultDSV = FLAG(10),

    /** Texture rests as a copy source (staging/upload) */
    CopySource = FLAG(11),

    /** Texture rests as a copy destination (readback/staging) */
    CopyDest = FLAG(12),

    /** Depth/stencil texture may be rendered with custom sample positions */
    SamplePositionsCompatible = FLAG(13),

    /** Opaque sampler feedback map paired with a sampled texture */
    SamplerFeedback = FLAG(14),
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

enum class ETextureDimension : uint8
{
    None = 0,
    
    Texture1D        = 1,
    Texture1DArray   = 2,
    Texture2D        = 3,
    Texture2DArray   = 4,
    TextureCube      = 5,
    TextureCubeArray = 6,
    Texture3D        = 7,
};

NODISCARD constexpr const CHAR* ToString(ETextureDimension TextureDimension)
{
	switch (TextureDimension)
	{
	case ETextureDimension::Texture1D:        return "Texture1D";
	case ETextureDimension::Texture1DArray:   return "Texture1DArray";
	case ETextureDimension::Texture2D:        return "Texture2D";
	case ETextureDimension::Texture2DArray:   return "Texture2DArray";
	case ETextureDimension::TextureCube:      return "TextureCube";
	case ETextureDimension::TextureCubeArray: return "TextureCubeArray";
	case ETextureDimension::Texture3D:        return "Texture3D";

	default: return "Unknown";
	}
}

NODISCARD constexpr bool IsTextureCube(ETextureDimension Dimension)
{
    return Dimension == ETextureDimension::TextureCube || Dimension == ETextureDimension::TextureCubeArray;
}

NODISCARD inline uint32 RHICubesToArrayLayers(ETextureDimension Dimension, uint32 NumCubes)
{
    CHECK(IsTextureCube(Dimension));
    return NumCubes * RHI_NUM_CUBE_FACES;
}

NODISCARD inline uint32 RHIArrayLayersToCubes(ETextureDimension Dimension, uint32 NumLayers)
{
    CHECK(IsTextureCube(Dimension));
    CHECK((NumLayers % RHI_NUM_CUBE_FACES) == 0);
    return NumLayers / RHI_NUM_CUBE_FACES;
}

NODISCARD inline uint32 RHIDimensionArrayLayers(ETextureDimension Dimension, uint32 NumArraySlices)
{
    CHECK(Dimension != ETextureDimension::None);
    
    if (Dimension == ETextureDimension::Texture3D)
    {
        return 1;
    }

    return IsTextureCube(Dimension) ? NumArraySlices * RHI_NUM_CUBE_FACES : NumArraySlices;
}

struct IRHITextureData
{
    virtual ~IRHITextureData() = default;

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const = 0;
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const = 0;
    virtual void* GetMipData(uint32 MipLevel = 0)       const = 0;
};

struct FRHITextureDesc
{
    NODISCARD static FRHITextureDesc CreateTexture1D(
        EFormat                       InFormat,
        uint32                        InWidth,
        uint32                        InNumMipLevels,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::Texture1D, InFormat, IntVector3(InWidth, 1, 0), 1, InNumMipLevels, 1, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateTexture1DArray(
        EFormat                       InFormat,
        uint32                        InWidth,
        uint32                        InArraySlices,
        uint32                        InNumMipLevels,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::Texture1DArray, InFormat, IntVector3(InWidth, 1, 0), InArraySlices, InNumMipLevels, 1, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateTexture2D(
        EFormat                       InFormat,
        uint32                        InWidth,
        uint32                        InHeight,
        uint32                        InNumMipLevels,
        uint32                        InNumSamples,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::Texture2D, InFormat, IntVector3(InWidth, InHeight, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateTexture2DArray(
        EFormat                       InFormat,
        uint32                        InWidth,
        uint32                        InHeight,
        uint32                        InArraySlices,
        uint32                        InNumMipLevels,
        uint32                        InNumSamples,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::Texture2DArray, InFormat, IntVector3(InWidth, InHeight, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateSamplerFeedbackTexture2D(
        EFormat                       InFormat,
        uint32                        InPairedWidth,
        uint32                        InPairedHeight,
        uint32                        InPairedMipLevels,
        IntVector3                    InMipRegion,
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        const ETextureUsageFlags UsageFlags =
            ETextureUsageFlags::SamplerFeedback | 
            ETextureUsageFlags::UnorderedAccessTexture |
            ETextureUsageFlags::NoDefaultSRV | 
            ETextureUsageFlags::NoDefaultUAV |
            ETextureUsageFlags::NoDefaultRTV | 
            ETextureUsageFlags::NoDefaultDSV;

        FRHITextureDesc Desc(ETextureDimension::Texture2D, InFormat, IntVector3(InPairedWidth, InPairedHeight, 0), 1, InPairedMipLevels, 1, UsageFlags, FClearValue(), InTrackingMode);
        Desc.SamplerFeedbackMipRegion = InMipRegion;

        return Desc;
    }

    NODISCARD static FRHITextureDesc CreateSamplerFeedbackTexture2DArray(
        EFormat                       InFormat,
        uint32                        InPairedWidth,
        uint32                        InPairedHeight,
        uint32                        InPairedArraySlices,
        uint32                        InPairedMipLevels,
        IntVector3                    InMipRegion,
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        const ETextureUsageFlags UsageFlags =
            ETextureUsageFlags::SamplerFeedback | 
            ETextureUsageFlags::UnorderedAccessTexture |
            ETextureUsageFlags::NoDefaultSRV | 
            ETextureUsageFlags::NoDefaultUAV |
            ETextureUsageFlags::NoDefaultRTV | 
            ETextureUsageFlags::NoDefaultDSV;

        FRHITextureDesc Desc(ETextureDimension::Texture2DArray, InFormat, IntVector3(InPairedWidth, InPairedHeight, 0), InPairedArraySlices, InPairedMipLevels, 1, UsageFlags, FClearValue(), InTrackingMode);
        Desc.SamplerFeedbackMipRegion = InMipRegion;

        return Desc;
    }

    NODISCARD static FRHITextureDesc CreateTextureCube(
        EFormat                       InFormat,
        uint32                        InExtent,
        uint32                        InNumMipLevels,
        uint32                        InNumSamples,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::TextureCube, InFormat, IntVector3(InExtent, InExtent, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateTextureCubeArray(
        EFormat                       InFormat,
        uint32                        InExtent,
        uint32                        InArraySlices,
        uint32                        InNumMipLevels,
        uint32                        InNumSamples,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::TextureCubeArray, InFormat, IntVector3(InExtent, InExtent, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue, InTrackingMode);
    }

    NODISCARD static FRHITextureDesc CreateTexture3D(
        EFormat                       InFormat,
        uint32                        InWidth,
        uint32                        InHeight,
        uint32                        InDepth,
        uint32                        InNumMipLevels,
        uint32                        InNumSamples,
        ETextureUsageFlags            InUsageFlags,
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
    {
        return FRHITextureDesc(ETextureDimension::Texture3D, InFormat, IntVector3(InWidth, InHeight, InDepth), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue, InTrackingMode);
    }

    FRHITextureDesc() noexcept = default;

    FRHITextureDesc(
        ETextureDimension             InDimension, 
        EFormat                       InFormat, 
        IntVector3                    InExtent, 
        uint32                        InNumArraySlices, 
        uint32                        InNumMipLevels,
        uint32                        InNumSamples, 
        ETextureUsageFlags            InUsageFlags, 
        const FClearValue&            InClearValue   = FClearValue(),
        ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual) noexcept
        : Dimension(InDimension)
        , Format(InFormat)
        , UsageFlags(InUsageFlags)
        , NumArraySlices(InNumArraySlices)
        , NumMipLevels(InNumMipLevels)
        , NumSamples(InNumSamples)
        , Extent(InExtent)
        , ClearValue(InClearValue)
        , TrackingMode(InTrackingMode)
    {
    }

    NODISCARD constexpr bool IsTexture1D()        const { return (Dimension == ETextureDimension::Texture1D); }
    NODISCARD constexpr bool IsTexture1DArray()   const { return (Dimension == ETextureDimension::Texture1DArray); }
    NODISCARD constexpr bool IsTexture2D()        const { return (Dimension == ETextureDimension::Texture2D); }
    NODISCARD constexpr bool IsTexture2DArray()   const { return (Dimension == ETextureDimension::Texture2DArray); }
    NODISCARD constexpr bool IsTextureCube()      const { return (Dimension == ETextureDimension::TextureCube); }
    NODISCARD constexpr bool IsTextureCubeArray() const { return (Dimension == ETextureDimension::TextureCubeArray); }
    NODISCARD constexpr bool IsTexture3D()        const { return (Dimension == ETextureDimension::Texture3D); }

    NODISCARD constexpr bool IsShaderResourceTexture()     const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShaderResourceTexture); }
    NODISCARD constexpr bool IsUnorderedAccessTexture()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::UnorderedAccessTexture); }
    NODISCARD constexpr bool IsRenderTarget()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::RenderTarget); }
    NODISCARD constexpr bool IsDepthStencil()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::DepthStencil); }
    NODISCARD constexpr bool IsPresentable()               const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::Presentable); }
    NODISCARD constexpr bool IsShadingRateTexture()        const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShadingRateTexture); }
    NODISCARD constexpr bool IsNoDefaultSRV()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultSRV); }
    NODISCARD constexpr bool IsNoDefaultUAV()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultUAV); }
    NODISCARD constexpr bool IsNoDefaultRTV()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultRTV); }
    NODISCARD constexpr bool IsNoDefaultDSV()              const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultDSV); }
    NODISCARD constexpr bool IsCopySource()                const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::CopySource); }
    NODISCARD constexpr bool IsCopyDest()                  const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::CopyDest); }
    NODISCARD constexpr bool IsSamplePositionsCompatible() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::SamplePositionsCompatible); }
    NODISCARD constexpr bool IsSamplerFeedbackTexture()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::SamplerFeedback); }
    NODISCARD constexpr bool IsMultisampled()              const { return (NumSamples > 1); }

    bool operator==(const FRHITextureDesc& Other) const noexcept = default;

    ETextureDimension             Dimension                = ETextureDimension::None;
    EFormat                       Format                   = EFormat::Unknown;
    ETextureUsageFlags            UsageFlags               = ETextureUsageFlags::None;
    uint32                        NumArraySlices           = 0;
    uint32                        NumMipLevels             = 0;
    uint32                        NumSamples               = 0;
    IntVector3                    Extent                   = { };
    FClearValue                   ClearValue               = { };
    ERHIResourceStateTrackingMode TrackingMode             = ERHIResourceStateTrackingMode::Manual;
    IntVector3                    SamplerFeedbackMipRegion = { };
};

class FRHITexture : public FRHIResource
{
protected:
    explicit FRHITexture(const FRHITextureDesc& InTextureDesc)
        : FRHIResource(ERHIResourceType::Texture)
        , Desc(InTextureDesc)
    {
    }

public:
    
    /** @return D3D12: ID3D12Resource*. Vulkan: VkImage. Metal: id<MTLTexture>. Null: nullptr. */
    virtual void* GetRHINativeResource() const = 0;

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const = 0;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const = 0;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const = 0;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const = 0;

    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const = 0;
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const = 0;

    virtual void SetDebugName(const String& InName) = 0;
    virtual void GetDebugName(String& OutDebugName) const = 0;

    NODISCARD const FRHITextureDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHITextureDesc Desc;
};
